#ifndef IPCSERVERWORKER_H
#define IPCSERVERWORKER_H

#include <QDataStream>
#include <QFile>
#include <QLocalSocket>
#include <QObject>
#include <QReadWriteLock>

class IpcServerWorker : public QObject
{
    Q_OBJECT
    quintptr socketDescriptor;
    QReadWriteLock& lock;
    QString fileName;
    QFile* file;
    QLocalSocket socket;
    QDataStream dsIn;
    quint32 blockSize;

public:
    explicit IpcServerWorker(quintptr socketDescriptor,
                             const QString& fileName,
                             QReadWriteLock& lock);

private:
    bool handleRequest();
    void storeData(uint size);
    void getData();
    void sendAck();
    void waitForAck();
    bool openFile(QIODevice::OpenMode openMode);
    void closeFile();

public slots:
    void process();

signals:
    void finished();
    void error(QString err);

};

#endif // IPCSERVERWORKER_H
