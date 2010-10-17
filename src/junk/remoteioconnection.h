#ifndef REMOTEIOCONNECTION_H
#define REMOTEIOCONNECTION_H

#include <QIODevice>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>
#include <QSharedPointer>

#include "controlconnection.h"
#include "filetransfersession.h"

class RemoteIOConnection : public Connection
{
    Q_OBJECT
public:
    RemoteIOConnection(Servent * s, FileTransferSession * fts);
    ~RemoteIOConnection();
    QString id() const;


    void shutdown(bool wait = false);
    void setup();
    void handleMsg(QByteArray msg);
    Connection * clone();

signals:

private slots:
    void sendSome();

private:

    FileTransferSession * m_fts;
    QSharedPointer<QIODevice> m_readdev;
};

#endif // REMOTEIOCONNECTION_H
