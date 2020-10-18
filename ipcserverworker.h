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
    QReadWriteLock& lock; // QReadWriteLock object to protect access to file
    QString fileName;     // The name of the file to store data received from
                          // clients or to read data to send to clients
    QFile* file;
    QLocalSocket socket;
    QDataStream dsIn;

public:
    explicit IpcServerWorker(quintptr socketDescriptor,
                             const QString& fileName,
                             QReadWriteLock& lock);

private:
    /// \details  IPC request received.
    ///           There are currently two available requests:
    ///             32: IpcProtReqId::kIpcProtReqIdStore
    ///                 It receives 32-bit data representing the number of
    ///                 bytes which will be received from Client through socket
    ///                 to write to the file.
    ///                 It will be handled by storeData(uint size) method.
    ///             33: IpcProtReqId::kIpcProtReqIdGet
    ///                 It sends 64-bit data representing the number of bytes
    ///                 to be sent to Client through socket then sends the
    ///                 data bytes (read from the file in chunks of
    ///                 kIpcMaxPayloadLength) indeed.
    ///                 It will be handled by getData() method
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
