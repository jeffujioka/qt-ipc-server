#ifndef IPCSOCKETTHREAD_H
#define IPCSOCKETTHREAD_H

#include <QThread>
#include <QDataStream>
#include <QLocalSocket>

class IpcSocketThread : public QThread
{
    Q_OBJECT
public:
    IpcSocketThread(quintptr socketDescriptor, QObject* parent);

    void run() override;

signals:
    void error(QLocalSocket::LocalSocketError err);

private:
    bool ParseRequest();

private:
    QLocalSocket socket_;
    quintptr socketDescriptor;
    QDataStream in;
    quint32 blockSize;
};

#endif // IPCSOCKETTHREAD_H
