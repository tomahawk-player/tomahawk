
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

#include "qxtsslserver.h"
#include <QQueue>
#include <QFile>

/*!
 * \class QxtSslServer
 * \inmodule QxtNetwork
 * \brief The QxtSslServer class provides a simple SSL server.
 *
 * QxtSslServer is a simple SSL server. As a QTcpServer subclass it shares all of the same behaviors
 * as its parent class, except that new connections are created as QSslSocket objects instead of
 * QTcpSocket objects.
 *
 * Before QxtSslServer can accept any encrypted connections, the local certificate (see setLocalCertificate)
 * and private key (see setPrivateKey) must be set. Failure to set these properties before accepting
 * connections will cause all incoming connections to generate sslError signals when negotiating
 * encryption. If autoEncrypt is disabled, the local certificate and private key can be set on the
 * individual socket objects before starting encryption; this behavior is generally not necessary unless
 * you wish to serve up a different certificate based on some property of the connection or some data
 * negotiated before beginning encryption.
 *
 * Unlike QTcpServer, overriding QxtSslServer::incomingConnection() is not recommended.
 *
 * QxtSslServer is only available if Qt was compiled with OpenSSL support.
 */

#ifndef QT_NO_OPENSSL
#include <QSslKey>

class QxtSslServerPrivate : public QxtPrivate<QxtSslServer>
{
public:
    QXT_DECLARE_PUBLIC(QxtSslServer)
    QSslCertificate localCertificate;
    QSslKey privateKey;
    bool autoEncrypt;
    QQueue<QSslSocket*> pendingConnections;
};

/*!
 * Constructs a new QxtSslServer object with the specified \a parent.
 */
QxtSslServer::QxtSslServer(QObject* parent) : QTcpServer(parent)
{
    QXT_INIT_PRIVATE(QxtSslServer);
    qxt_d().autoEncrypt = true;
}

/*!
 * \reimp
 */
bool QxtSslServer::hasPendingConnections() const
{
    return !qxt_d().pendingConnections.isEmpty();
}

/*!
 * \reimp
 */
QTcpSocket* QxtSslServer::nextPendingConnection()
{
    if(!hasPendingConnections()) return 0;
    return qxt_d().pendingConnections.dequeue();
}

/*!
 * Sets the certificate to be presented to clients during the SSL handshake.
 *
 * Setting the certificate only affects new connections established after the certificate
 * has been set. 
 *
 * A certificate is the means of identification used in the SSL process. The local
 * certificate is used by the remote end to verify the local user's identity against its
 * list of Certification Authorities.
 *
 * \sa localCertificate
 * \sa setPrivateKey
 */
void QxtSslServer::setLocalCertificate(const QSslCertificate& cert)
{
    qxt_d().localCertificate = cert;
}

/*!
 * This is an overloaded function, provided for convenience.
 *
 * Sets the certificate to be presented to clients during the SSL handshake to
 * the first certificate contained in the file specified by \a path.
 *
 * \sa localCertificate
 */
void QxtSslServer::setLocalCertificate(const QString& path, QSsl::EncodingFormat format)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return;
    setLocalCertificate(QSslCertificate(&file, format));
}

/*!
 * Returns the local certificate used to identify the server, or an empty certificate if none has been set.
 * \sa setLocalCertificate
 */
QSslCertificate QxtSslServer::localCertificate() const
{
    return qxt_d().localCertificate;
}

/*!
 * Sets the private key used for encryption.
 *
 * Setting the private key only affects new connections established after the key has been set. 
 *
 * \sa privateKey
 */
void QxtSslServer::setPrivateKey(const QSslKey& key)
{
    qxt_d().privateKey = key;
}

/*!
 * This is an overloaded function, provided for convenience.
 *
 * Sets the private key used for encryption to the key contained in the file specified by
 * \a path. The specified algorithm, format, and pass phrase are used to decrypt the key.
 *
 * \sa privateKey
 */
void QxtSslServer::setPrivateKey(const QString& path, QSsl::KeyAlgorithm algo, QSsl::EncodingFormat format, const QByteArray& passPhrase)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return;
    setPrivateKey(QSslKey(&file, algo, format, QSsl::PrivateKey, passPhrase));
}

/*!
 * Returns the private key used to encrypt communication, or an empty key if none has been set.
 * \sa setPrivateKey
 */
QSslKey QxtSslServer::privateKey() const
{
    return qxt_d().privateKey;
}

/*!
 * Enables or disables automatically starting encryption on new connections.
 *
 * Set this property to false if you wish to enable encryption on incoming connections
 * at a later time. The default value is true.
 *
 * \sa autoEncrypt
 */
void QxtSslServer::setAutoEncrypt(bool on)
{
    qxt_d().autoEncrypt = on;
}

/*!
 * Returns true if incoming connections will automatically be encrypted, or false otherwise.
 * \sa setAutoEncrypt
 */
bool QxtSslServer::autoEncrypt() const
{
    return qxt_d().autoEncrypt;
}

/*!
 * \reimp
 */
void QxtSslServer::incomingConnection(int socketDescriptor)
{
    QSslSocket* socket = new QSslSocket(this);
    if(socket->setSocketDescriptor(socketDescriptor)) {
        socket->setLocalCertificate(qxt_d().localCertificate);
        socket->setPrivateKey(qxt_d().privateKey);
        if(parent()){
            connect(socket, SIGNAL(sslErrors(const QList<QSslError>&)),
                    parent(), SLOT(sslErrors(const QList<QSslError>&)));
            connect(socket, SIGNAL(peerVerifyError(const QSslError&)),
                    parent(), SLOT(peerVerifyError(const QSslError&)));
        }
        qxt_d().pendingConnections.enqueue(socket);
        // emit newConnection(); // removed: QTcpServerPrivate emits this for us
        if(qxt_d().autoEncrypt) socket->startServerEncryption();
    } else {
        delete socket;
    }
}

#endif /* QT_NO_OPENSSL */
