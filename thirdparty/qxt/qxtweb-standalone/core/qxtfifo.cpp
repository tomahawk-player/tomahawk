
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

/*!
\class QxtFifo

\inmodule QxtCore

\brief The QxtFifo class provides a simple loopback QIODevice.

read and write to the same object
emits a readyRead Signal.
useful for loopback tests where QBuffer does not work.

\code
QxtFifo fifo;
 QTextStream (&fifo)<<QString("foo");
 QString a;
 QTextStream(&fifo)>>a;
 qDebug()<<a;
\endcode

*/



#include "qxtfifo.h"
#include <string.h>
#include <limits.h>
#include <QDebug>
#include <QQueue>

#include <qatomic.h>

#if QT_VERSION >= 0x50000
# define QXT_EXCHANGE(x) fetchAndStoreOrdered(x)
# define QXT_EXCHANGE_(x) fetchAndStoreOrdered(x.load())
# define QXT_ADD fetchAndAddOrdered
#elif QT_VERSION >= 0x040400
# include <qbasicatomic.h>
# define QXT_EXCHANGE fetchAndStoreOrdered
# define QXT_EXCHANGE_ QXT_EXCHANGE
# define QXT_ADD fetchAndAddOrdered
#else
  typedef QBasicAtomic QBasicAtomicInt;
# define QXT_EXCHANGE exchange
# define QXT_EXCHANGE_ QXT_EXCHANGE
# define QXT_ADD fetchAndAdd
#endif

struct QxtFifoNode {
    QxtFifoNode(const char* data, int size) : content(data, size) {
#if QT_VERSION >= 0x50000
        next.store(0);
#else
        next = NULL;
#endif
    }
    QxtFifoNode(const QByteArray &data) : content(data) {
#if QT_VERSION >=  0x50000
        next.store(0);
#else
        next = NULL;
#endif
    }
   
    QByteArray content;
    QBasicAtomicPointer<QxtFifoNode> next;
};

class QxtFifoPrivate : public QxtPrivate<QxtFifo> {
public:
    QXT_DECLARE_PUBLIC(QxtFifo)
    QxtFifoPrivate() {
        QxtFifoNode *n = new QxtFifoNode(NULL, 0);
#if QT_VERSION >=  0x50000
        head.store(n);
        tail.store(n);
#else
        head = n;
        tail = n;
#endif

#if QT_VERSION >=  0x50000
        available.store(0);
#else
        available = 0;
#endif
    }

    QBasicAtomicPointer<QxtFifoNode> head, tail;
    QBasicAtomicInt available;
};

/*!
Constructs a new QxtFifo with \a parent.
*/
QxtFifo::QxtFifo(QObject *parent) : QIODevice(parent)
{
    QXT_INIT_PRIVATE(QxtFifo);
    setOpenMode(QIODevice::ReadWrite);
}

/*!
Constructs a new QxtFifo with \a parent and initial content from \a prime.
*/
QxtFifo::QxtFifo(const QByteArray &prime, QObject *parent) : QIODevice(parent)
{
    QXT_INIT_PRIVATE(QxtFifo);
    setOpenMode(QIODevice::ReadWrite);
    // Since we're being constructed, access to the internals is safe
    QxtFifoNode* node;
#if QT_VERSION >=  0x50000
    node = qxt_d().head.load();
#else
    node = qxt_d().head;
#endif
    node->content = prime;
    qxt_d().available.QXT_ADD( prime.size() );
}

/*!
\reimp
*/
qint64 QxtFifo::readData ( char * data, qint64 maxSize )
{
    int bytes, step;
#if QT_VERSION >=  0x50000
    bytes = qxt_d().available.load();
#else
    bytes =  qxt_d().available;
#endif


    if(!bytes) return 0;
    if(bytes > maxSize) bytes = maxSize;
    int written = bytes;
    char* writePos = data;
    QxtFifoNode* node;
    while(bytes > 0) {

#if QT_VERSION >=  0x50000
        node = qxt_d().head.load();
#else
        node = qxt_d().head;
#endif

        step = node->content.size();
        if(step >= bytes) {
            int rem = step - bytes;
            memcpy(writePos, node->content.constData(), bytes);
            step = bytes;
            node->content = node->content.right(rem);
        } else {
            memcpy(writePos, node->content.constData(), step);
            qxt_d().head.QXT_EXCHANGE_(node->next);
            delete node;
#if QT_VERSION >=  0x50000
            node = qxt_d().head.load();
#else
            node = qxt_d().head;
#endif
        }
        writePos += step;
        bytes -= step;
    }
    qxt_d().available.QXT_ADD(-written);
    return written;
}

/*!
\reimp
*/
qint64 QxtFifo::writeData ( const char * data, qint64 maxSize )
{
    if(maxSize > 0) {
        if(maxSize > INT_MAX) maxSize = INT_MAX; // qint64 could easily exceed QAtomicInt, so let's play it safe
        QxtFifoNode* newData = new QxtFifoNode(data, maxSize);
#if QT_VERSION >=  0x50000
        qxt_d().tail.load()->next.QXT_EXCHANGE(newData);
#else
        qxt_d().tail->next.QXT_EXCHANGE(newData);
#endif
        qxt_d().tail.QXT_EXCHANGE(newData);
        qxt_d().available.QXT_ADD(maxSize);
        QMetaObject::invokeMethod(this, "bytesWritten", Qt::QueuedConnection, Q_ARG(qint64, maxSize));
        QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    }
    return maxSize;
}

/*!
\reimp
*/
bool QxtFifo::isSequential () const
{
    return true;
}

/*!
\reimp
*/
qint64 QxtFifo::bytesAvailable () const
{
#if QT_VERSION >=  0x50000
    return qxt_d().available.load() + QIODevice::bytesAvailable();
#else
    return qxt_d().available + QIODevice::bytesAvailable();

#endif
}

/*!
*/
void QxtFifo::clear()
{
    qxt_d().available.QXT_EXCHANGE(0);
    qxt_d().tail.QXT_EXCHANGE_(qxt_d().head);

#if QT_VERSION >=  0x50000
    QxtFifoNode* node = qxt_d().head.load()->next.QXT_EXCHANGE(NULL);
#else
    QxtFifoNode* node = qxt_d().head->next.QXT_EXCHANGE(NULL);
#endif

#if QT_VERSION >=  0x50000
    while (node && node->next.load())
#else
    while (node && node->next)
#endif
    {
        QxtFifoNode* next = node->next.QXT_EXCHANGE(NULL);
        delete node;
        node = next;
    }
#if QT_VERSION >=  0x50000
    qxt_d().head.load()->content = QByteArray();
#else
    qxt_d().head->content = QByteArray();
#endif
}
