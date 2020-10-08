#include "ipcsocketthread.h"

#include <QDataStream>
#include <QDebug>
#include <QString>

IpcSocketThread::IpcSocketThread(quintptr socketDescriptor, QObject* parent)
        : QThread(parent), socketDescriptor(socketDescriptor)
{
    in.setDevice(&socket_);
    in.setVersion(QDataStream::Qt_5_10);
}

void IpcSocketThread::run() {
    if (socket_.setSocketDescriptor(socketDescriptor)) {
        qDebug() << "Socket error: " << socket_.error();
        emit error(socket_.error());
        return;
    }

    qDebug() << "New connection [" << socketDescriptor << "] has arrived. ";

    if (!socket_.waitForReadyRead()) {
        qDebug() << "socket_ error: " << socket_.error();
        emit error(socket_.error());
    }

    if (!ParseRequest()) return;

    {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_10);

        QString message =  "Server: Jefferson Fujioka | Berlin";
        out << quint32(message.size());
        out << message;

        qDebug() << "Server sending: [" << message << "] " << block.size() << " bytes";

        auto bytes_written = socket_.write(block);

        socket_.flush();

        qDebug() << "written " << bytes_written << " bytes... disconnecting now";
        socket_.disconnectFromServer();
        socket_.waitForDisconnected();
        qDebug() << "Disconnected!!!";
    }
}

bool IpcSocketThread::ParseRequest() {
    auto available = socket_.bytesAvailable();
    if (available > 0)
    {
        qDebug() << "Thre are " << available << " to be read.";

        in >> blockSize;

        if (socket_.bytesAvailable() < blockSize || in.atEnd())
        {
            qDebug() << "There is no enough data";
            return false;
        }

        QString msg;
        in >> msg;
        qDebug() << "blockSize: " << blockSize;
        qDebug() << "Msg: " << msg;

        qDebug() << "socket_.bytesAvailable() " << socket_.bytesAvailable();

        char* data = nullptr;
        in.readBytes(data, blockSize);

        qDebug() << "read " << blockSize << " bytes";

        if (data == nullptr) {
            qDebug() << "not possible to read";
            return false;
        }
        QString debug;
        int counter = 0;
        for (int i = 0; i < 512; ++i) {
            ++counter;
            debug += QString::number(static_cast<quint8>(data[i]));
            debug += " ";
        }
        qDebug() << "//////////////////////\nReceived data is:" << debug;

        qDebug() << "data successfully received";
    }
    return true;
}
