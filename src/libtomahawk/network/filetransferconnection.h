
#ifndef FILETRANSFERCONNECTION_H
#define FILETRANSFERCONNECTION_H

#include <QObject>
#include <QSharedPointer>
#include <QIODevice>

#include "network/connection.h"
#include "result.h"

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
    explicit FileTransferConnection( Servent* s, ControlConnection* cc, QString fid, const Tomahawk::result_ptr& result );
    // TX:
    explicit FileTransferConnection( Servent* s, ControlConnection* cc, QString fid );

    virtual ~FileTransferConnection();

    QString id() const;
    void setup();
    Connection* clone();

    const QSharedPointer<QIODevice>& iodevice() { return m_iodev; }
    ControlConnection* controlConnection() const { return m_cc; }

	Tomahawk::source_ptr source() const;
    Tomahawk::result_ptr track() const { return m_result; }
    qint64 transferRate() const { return m_transferRate; }

    Type type() const { return m_type; }
    QString fid() const { return m_fid; }

signals:
    void updated();

protected slots:
    virtual void handleMsg( msg_ptr msg );

private slots:
    void startSending( const Tomahawk::result_ptr& );
    void sendSome();
    void showStats( qint64 tx, qint64 rx );

    void onBlockRequest( int pos );

private:
    QSharedPointer<QIODevice> m_iodev;
    ControlConnection* m_cc;
    QString m_fid;
    Type m_type;
    QSharedPointer<QIODevice> m_readdev;

    int m_curBlock;

    int m_badded, m_bsent;
    bool m_allok; // got last msg ok, transfer complete?

    Tomahawk::source_ptr m_source;
    Tomahawk::result_ptr m_result;
    qint64 m_transferRate;
};

#endif // FILETRANSFERCONNECTION_H
