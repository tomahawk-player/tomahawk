#ifndef FILETRANSFERCONNECTION_H
#define FILETRANSFERCONNECTION_H

#include <QObject>
#include <QSharedPointer>
#include <QIODevice>

#include "connection.h"

class ControlConnection;
class BufferIODevice;

class FileTransferConnection : public Connection
{
Q_OBJECT

public:
    enum Type
    {
        SENDING = 0,
        RECEIVING = 1
    };

    // RX:
    explicit FileTransferConnection( Servent* s, ControlConnection* parent, QString fid, unsigned int size );
    // TX:
    explicit FileTransferConnection( Servent* s, QString fid );

    virtual ~FileTransferConnection();

    QString id() const;
    void setup();
    Connection* clone();

    const QSharedPointer<QIODevice>& iodevice()
    {
        return m_iodev;
    }

    Type type() const { return m_type; }
    QString fid() const { return m_fid; }

protected slots:
    virtual void handleMsg( msg_ptr msg );

private slots:
    void startSending( QVariantMap );
    void sendSome();
    void showStats(qint64 tx, qint64 rx);

private:
    QSharedPointer<QIODevice> m_iodev;
    ControlConnection* m_cc;
    QString m_fid;
    Type m_type;
    QSharedPointer<QIODevice> m_readdev;

    int m_badded, m_bsent;
    bool m_allok; // got last msg ok, transfer complete?
};

#endif // FILETRANSFERCONNECTION_H
