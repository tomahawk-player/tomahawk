/*
    MsgProcessor is a FIFO queue of msg_ptr, you .add() a msg_ptr, and
    it emits done(msg_ptr) for each msg, preserving the order.

    It can be configured to auto-compress, or de-compress msgs for sending
    or receiving.

    It uses QtConcurrent, but preserves msg order.

    NOT threadsafe.
*/
#ifndef MSGPROCESSOR_H
#define MSGPROCESSOR_H

#include <QObject>
#include "msg.h"
#include <QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

class MsgProcessor : public QObject
{
Q_OBJECT
public:
    enum Mode
    {
        NOTHING = 0,
        COMPRESS_IF_LARGE = 1,
        UNCOMPRESS_ALL = 2,
        PARSE_JSON = 4
    };

    explicit MsgProcessor( quint32 mode = NOTHING, quint32 t = 512 );

    void setMode( quint32 m ) { m_mode = m ; }

    static msg_ptr process( msg_ptr msg, quint32 mode, quint32 threshold );

    int length() const { return m_msgs.length(); }

signals:
    void ready( msg_ptr );
    void empty();

public slots:
    void append( msg_ptr msg );
    void processed();

private:
    void handleProcessedMsg( msg_ptr msg );

    quint32 m_mode;
    quint32 m_threshold;
    QList<msg_ptr> m_msgs;
    QMap< Msg*, bool> m_msg_ready;
    unsigned int m_totmsgsize;
};

#endif // MSGPROCESSOR_H
