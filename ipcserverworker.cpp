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

    if (!socket.waitForReadyRead()) { // waiting for Client's request
        qDebug() << "socket error: " << socket.error();
        emit error(socket.errorString());
    }

    if (!handleRequest()) { // requested received so let's handle it
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
            // received "send request" so let's store the data sent by Client
            case IpcProtReqId::kIpcProtReqIdStore:
        {
            qDebug() << "Request [kIpcProtReqIdStore]";
            uint size;
            // reading 32-bit representing the number of bytes to receive
            // from client
            dsIn >> size;

            storeData(size); // receives data from Client and store it into file
            sendAck();
        }
            break;

            // received "get request" so let's read data from file
            // to send to Client
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

    lock.lockForWrite(); // locking... no one can read from file

    while (totalReadBytes < size)
    {
        qDebug() << "socket.bytesAvailable() " << socket.bytesAvailable();

        char data[kIpcMaxPayloadLength];

        // read from socket in chunks of kIpcMaxPayloadLength bytes
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

        // write readBytes bytes into file
        file->write(data, readBytes);

        if (totalReadBytes < size && dsIn.atEnd()) {
            // there no enough data to be read so let's wait
            // the client to send more
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
    closeFile();
    lock.unlock(); // unlocking... file is now free to be read from or written to
    if (!errMsg.isEmpty()) {
        qDebug() << errMsg;
        emit error(errMsg);
    }
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

    lock.lockForRead(); // locking... we can now read safely from the file

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
            // reads from file chunks of kIpcMaxPayloadLength bytes
            auto readBytes = file->read(buffer, kIpcMaxPayloadLength);

            // then write those data to socket to send to Client
            socket.write(buffer, readBytes);
            totalReadBytes += readBytes;

            // continue reading then writing until reaching readBytes == 0
            if (readBytes == 0) break;
        }
        socket.flush();
    }

    exit:
    closeFile();
    lock.unlock(); // unlocking... we can now write safely to the file
    if (!errMsg.isEmpty()) {
        qDebug() << errMsg;
        emit error(errMsg);
    }
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
