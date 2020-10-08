#ifndef IPCSERVER_H
#define IPCSERVER_H

#include <QFile>
#include <QLocalServer>
#include <QReadWriteLock>
#include <QString>

class IpcServer : public QLocalServer
{
    Q_OBJECT
    QReadWriteLock lock;
    QString fileName;

public:
    explicit IpcServer(QObject *parent = nullptr);

public slots:
    void setFileName(const QString& name) { fileName = name; }
    void startListening(const QString& name);
    void errorString(QString err);
    void finished();

protected:
    void incomingConnection(quintptr socketDescriptor) override;

};

#endif // IPCSERVER_H
