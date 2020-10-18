#include "ipcserver.h"

#include <QThread>

#include "ipcserverworker.h"

IpcServer::IpcServer(QObject *parent)
    : QLocalServer(parent), fileName("log.txt") {
}

void IpcServer::incomingConnection(quintptr socketDescriptor) {
    // starts a new thread for each incoming connection.
    // That's not a good solution because it might lead to
    // thread oversubscription problem
    // but it suffices the purposes of this example.
    QThread* thread = new QThread;
    IpcServerWorker* worker = new IpcServerWorker(socketDescriptor,
                                                  fileName, lock);

    connect(worker, SIGNAL (error(QString)), this, SLOT (errorString(QString)));
    connect(thread, SIGNAL (started()), worker, SLOT (process()));
    connect(worker, SIGNAL (finished()), thread, SLOT (quit()));
    connect(worker, SIGNAL (finished()), worker, SLOT (deleteLater()));
    connect(thread, SIGNAL (finished()), this, SLOT (finished()));
    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));

    thread->start();
}

void IpcServer::startListening(const QString& name) {
    if (!listen(name)) {
        qDebug() << "Unable to start the server [" << name << "]";
        return;
    }
    qDebug() << "Server is listening at [" << name << "]";
}

void IpcServer::finished() {
    qDebug() << "IpcServer finished!!!";
}
