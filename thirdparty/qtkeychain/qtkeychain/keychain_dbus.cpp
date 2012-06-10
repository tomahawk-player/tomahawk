/******************************************************************************
 *   Copyright (C) 2011 Frank Osterfeld <frank.osterfeld@gmail.com>           *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#include "keychain_p.h"

#include <QSettings>

#include <auto_ptr.h>

using namespace QKeychain;

void ReadPasswordJob::Private::doStart() {
    iface = new org::kde::KWallet( QLatin1String("org.kde.kwalletd"), QLatin1String("/modules/kwalletd"), QDBusConnection::sessionBus(), this );
    const QDBusPendingReply<int> reply = iface->open( QLatin1String("kdewallet"), 0, q->service() );
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher( reply, this );
    connect( watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(kwalletOpenFinished(QDBusPendingCallWatcher*)) );
}

void ReadPasswordJob::Private::kwalletOpenFinished( QDBusPendingCallWatcher* watcher ) {
    watcher->deleteLater();
    const QDBusPendingReply<int> reply = *watcher;
    if ( reply.isError() ) {
        const QDBusError err = reply.error();

        if ( q->insecureFallback() ) {
            std::auto_ptr<QSettings> local( !q->settings() ? new QSettings( q->service() ) : 0 );
            QSettings* actual = q->settings() ? q->settings() : local.get();

            data = actual->value( key ).toByteArray();

            q->emitFinished();
        } else {
            if ( err.type() == QDBusError::ServiceUnknown ) //KWalletd not running
                q->emitFinishedWithError( NoBackendAvailable, tr("No keychain service available") );
            else
                q->emitFinishedWithError( OtherError, tr("Could not open wallet: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );

            return;
        }
    }

    walletHandle = reply.value();

    if ( walletHandle < 0 ) {
        q->emitFinishedWithError( AccessDenied, tr("Access to keychain denied") );
        return;
    }

    const QDBusPendingReply<int> nextReply = iface->entryType( walletHandle, q->service(), key, q->service() );
    QDBusPendingCallWatcher* nextWatcher = new QDBusPendingCallWatcher( nextReply, this );
    connect( nextWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(kwalletEntryTypeFinished(QDBusPendingCallWatcher*)) );
}

void ReadPasswordJob::Private::kwalletEntryTypeFinished( QDBusPendingCallWatcher* watcher ) {
    watcher->deleteLater();
    if ( watcher->isError() ) {
        const QDBusError err = watcher->error();
        q->emitFinishedWithError( OtherError, tr("Could not determine data type: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );
        return;
    }

    const QDBusPendingReply<int> reply = *watcher;

    dataType = reply.value() == 1/*Password*/ ? Text : Binary;

    const QDBusPendingCall nextReply = dataType == Text
        ? QDBusPendingCall( iface->readPassword( walletHandle, q->service(), key, q->service() ) )
        : QDBusPendingCall( iface->readEntry( walletHandle, q->service(), key, q->service() ) );
    QDBusPendingCallWatcher* nextWatcher = new QDBusPendingCallWatcher( nextReply, this );
    connect( nextWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(kwalletReadFinished(QDBusPendingCallWatcher*)) );
}

void ReadPasswordJob::Private::kwalletReadFinished( QDBusPendingCallWatcher* watcher ) {
    watcher->deleteLater();
    if ( watcher->isError() ) {
        const QDBusError err = watcher->error();
        q->emitFinishedWithError( OtherError, tr("Could not read password: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );
        return;
    }

    if ( dataType == Binary ) {
        QDBusPendingReply<QByteArray> reply = *watcher;
        data = reply.value();
    } else {
        QDBusPendingReply<QString> reply = *watcher;
        data = reply.value().toUtf8();
    }
    q->emitFinished();
}

void WritePasswordJob::Private::doStart() {
    iface = new org::kde::KWallet( QLatin1String("org.kde.kwalletd"), QLatin1String("/modules/kwalletd"), QDBusConnection::sessionBus(), this );
    const QDBusPendingReply<int> reply = iface->open( QLatin1String("kdewallet"), 0, q->service() );
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher( reply, this );
    connect( watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(kwalletOpenFinished(QDBusPendingCallWatcher*)) );
}

void WritePasswordJob::Private::kwalletOpenFinished( QDBusPendingCallWatcher* watcher ) {
    watcher->deleteLater();
    QDBusPendingReply<int> reply = *watcher;
    if ( reply.isError() ) {
        if ( q->insecureFallback() ) {
            std::auto_ptr<QSettings> local( !q->settings() ? new QSettings( q->service() ) : 0 );
            QSettings* actual = q->settings() ? q->settings() : local.get();

            if ( mode == Delete ) {
                actual->remove( key );
                actual->sync();

                q->emitFinished();
                return;
            }
            const QByteArray data = mode == Binary ? binaryData : textData.toUtf8();
            actual->setValue( key, data );
            actual->sync();

            q->emitFinished();
        } else {
            const QDBusError err = reply.error();
            q->emitFinishedWithError( OtherError, tr("Could not open wallet: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );
        }
        return;
    }

    const int handle = reply.value();

    if ( handle < 0 ) {
        q->emitFinishedWithError( AccessDenied, tr("Access to keychain denied") );
        return;
    }

    QDBusPendingReply<int> nextReply;

    if ( !textData.isEmpty() )
        nextReply = iface->writePassword( handle, q->service(), key, textData, q->service() );
    else if ( !binaryData.isEmpty() )
        nextReply = iface->writeEntry( handle, q->service(), key, binaryData, q->service() );
    else
        nextReply = iface->removeEntry( handle, q->service(), key, q->service() );

    QDBusPendingCallWatcher* nextWatcher = new QDBusPendingCallWatcher( nextReply, this );
    connect( nextWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(kwalletWriteFinished(QDBusPendingCallWatcher*)) );
}

void WritePasswordJob::Private::kwalletWriteFinished( QDBusPendingCallWatcher* watcher ) {
    watcher->deleteLater();
    QDBusPendingReply<int> reply = *watcher;
    if ( reply.isError() ) {
        const QDBusError err = reply.error();
        q->emitFinishedWithError( OtherError, tr("Could not open wallet: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );
        return;
    }

    q->emitFinished();
}
