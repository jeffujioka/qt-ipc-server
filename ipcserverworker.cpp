#include "ipcserverworker.h"

#include <QThread>

#include "ipccommon.h"

IpcServerWorker::IpcServerWorker(quintptr socketDescriptor,
                                 const QString& fileName,
                                 QReadWriteLock& lock)
    : socketDescriptor(socketDescriptor),
      lock(lock), fileName(fileName), file(nullptr) {
    dsIn.setDevice(&socket);
    dsIn.setVersion(QDataStream::Qt_5_10);
}

void IpcServerWorker::process() {
    if (!socket.setSocketDescriptor(socketDescriptor)) {
        qDebug() << "Socket error: " << socket.error();
        emit error(socket.errorString());
        return;
    }

    qDebug() << "New connection [" << socketDescriptor << "] has arrived. ";

    if (!socket.waitForReadyRead()) {
        qDebug() << "socket error: " << socket.error();
        emit error(socket.errorString());
    }

    if (!handleRequest()) {
        emit error("Unexpected error!");
        return;
    }

    socket.disconnectFromServer();

    emit finished();
}

bool IpcServerWorker::handleRequest() {
    auto available = socket.bytesAvailable();
    if (available > 0)
    {
        qDebug() << "Thre are " << available << " to be read.";

        IpcProtReqId id;

        dsIn >> id;

        switch (id) {
            case IpcProtReqId::kIpcProtReqIdStore:
        {
            qDebug() << "Request [kIpcProtReqIdStore]";
            uint size;
            dsIn >> size;

            storeData(size);
            sendAck();
        }
            break;

        case IpcProtReqId::kIpcProtReqIdGet:
        {
            qDebug() << "Request [kIpcProtReqIdGet]";

            getData();
            waitForAck();
        }
            break;

        default:
        {
            qDebug() << "Invalid request [" << static_cast<quint8>(id) << "]";
        }
            break;
        }
    }
    return true;
}

void IpcServerWorker::storeData(uint size) {
    QString errMsg;
    uint totalReadBytes = 0;

    if (!openFile(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        errMsg.append("Error opening file ");
        if (file) errMsg.append(file->fileName());
        qDebug() <<  errMsg;
        goto exit;
    }

    lock.lockForWrite();

    while (totalReadBytes < size)
    {
        qDebug() << "socket.bytesAvailable() " << socket.bytesAvailable();

        char data[kIpcMaxPayloadLength];

        auto readBytes = socket.read(data, kIpcMaxPayloadLength);

        if (readBytes < 0) {
            qDebug() << "needs more data";
            if (!socket.waitForReadyRead(3000)) {
                errMsg.append("error waiting more data: ")
                      .append(socket.errorString());
                goto exit;
            }
        }

        totalReadBytes += readBytes;

        file->write(data, readBytes);

        if (totalReadBytes < size && dsIn.atEnd()) {
            qDebug() << "waiting for more data";
            if (!socket.waitForReadyRead()) {
                errMsg.append("error waiting more data: ")
                      .append(socket.errorString());
                goto exit;
            }
        }
    }
    qDebug() << "//////////////////////"
             << "totalReadBytes: " << totalReadBytes;

    exit:
    lock.unlock();
    if (!errMsg.isEmpty()) {
        qDebug() << errMsg;
        emit error(errMsg);
    }
    closeFile();
}

void IpcServerWorker::getData() {
    QString errMsg;
    uint totalReadBytes = 0;

    if (!openFile(QIODevice::ReadOnly | QIODevice::Text)) {
        errMsg.append("Error opening file ")
              .append(file->fileName());
        qDebug() <<  errMsg;
        goto exit;
    }

    lock.lockForRead();

    {
        char buffer[kIpcMaxPayloadLength];
        // actually, it would be better not to use QDataStream for this case
        // I started using it but it can be removed now
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_10);
        out << file->size();
        socket.write(block); // send file size to the peer

        forever {
            auto readBytes = file->read(buffer, kIpcMaxPayloadLength);

            socket.write(buffer, readBytes);
            totalReadBytes += readBytes;

            if (readBytes == 0) break;
        }
        socket.flush();
    }

    exit:
    lock.unlock();
    if (!errMsg.isEmpty()) {
        qDebug() << errMsg;
        emit error(errMsg);
    }
    closeFile();
}

void IpcServerWorker::sendAck() {
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);

    out << kIpcProtAck;

    socket.write(block);
    socket.flush();
}

void IpcServerWorker::waitForAck() {
    if (!socket.waitForReadyRead()) {
        qDebug() << "Error waiting for ReadyRead!";
        emit error(socket.errorString());
    }

    quint32 ack;
    QDataStream in;
    in.setDevice(&socket);
    in.setVersion(QDataStream::Qt_5_10);
    in >> ack;

    if (ack != kIpcProtAck) {
        QString err = "Invalid [";
                err += ack;
                err += "] ACK. Something went wrong.";
        qDebug() << err;
        emit error(err);
    }
    socket.disconnectFromServer();
}

bool IpcServerWorker::openFile(QIODevice::OpenMode openMode) {
    if (!file) file = new QFile(fileName);

    if (!file->open(openMode)) {
        QString errMsg;
        errMsg.append("Error opening file ")
              .append(file->fileName());
        qDebug() <<  errMsg;
        return false;
    }

    return true;
}

void IpcServerWorker::closeFile() {
    if (file && file->isOpen()) file->close();
    file = nullptr;
}
