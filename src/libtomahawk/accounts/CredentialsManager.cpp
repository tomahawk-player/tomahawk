/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CredentialsManager.h"

#include "utils/Logger.h"

#include <qtkeychain/keychain.h>

#include <QStringList>


namespace Tomahawk
{

namespace Accounts
{

CredentialsStorageKey::CredentialsStorageKey( const QString& service, const QString& key )
    : m_service( service )
    , m_key( key )
{}

bool
CredentialsStorageKey::operator ==( const CredentialsStorageKey& other ) const
{
    return ( m_key == other.m_key ) && ( m_service == other.m_service );
}


bool
CredentialsStorageKey::operator !=( const CredentialsStorageKey& other ) const
{
    return !operator ==( other );
}

uint
qHash( const Tomahawk::Accounts::CredentialsStorageKey& key )
{
    return qHash( key.service() + key.key() );
}


// CredentialsManager

CredentialsManager::CredentialsManager( QObject* parent )
    : QObject( parent )
{
    tDebug() << Q_FUNC_INFO;
}


void
CredentialsManager::loadCredentials( QList< Service > keysByService )
{
    foreach ( const Service svc, keysByService )
    {
        tDebug() << Q_FUNC_INFO << "keys for service" << svc.name << ":" << svc.keys;
        foreach ( QString key, svc.keys )
        {
            QKeychain::ReadPasswordJob* j = new QKeychain::ReadPasswordJob( svc.name, this );
            j->setKey( key );
            j->setAutoDelete( false );
#if defined( Q_OS_UNIX ) && !defined( Q_OS_MAC )
            j->setInsecureFallback( true );
#endif
            connect( j, SIGNAL( finished( QKeychain::Job* ) ),
                     SLOT( keychainJobFinished( QKeychain::Job* ) ) );
            m_readJobs << j;
            j->start();
            tDebug()  << "Launching QtKeychain readJob for" << key;
        }
    }
}


QList< CredentialsStorageKey >
CredentialsManager::keys() const
{
    QList< CredentialsStorageKey > keys = m_credentials.keys();
    return keys;
}


QVariantHash
CredentialsManager::credentials( const CredentialsStorageKey& key ) const
{
    return m_credentials.value( key );
}


QVariantHash
CredentialsManager::credentials( const QString& serviceName, const QString& key ) const
{
    return credentials( CredentialsStorageKey( serviceName, key ) );
}


void
CredentialsManager::setCredentials( const CredentialsStorageKey& csKey, const QVariantHash& value )
{
    QMutexLocker locker( &m_mutex );

    QKeychain::Job* j;
    if ( value.isEmpty() )
    {
        if ( !m_credentials.contains( csKey ) ) //if we don't have any credentials for this key, we bail
            return;

        m_credentials.remove( csKey );

        QKeychain::DeletePasswordJob* dj = new QKeychain::DeletePasswordJob( csKey.service(), this );
        dj->setKey( csKey.key() );
        j = dj;
    }
    else
    {
        if ( value == m_credentials.value( csKey ) ) //if the credentials haven't actually changed, we bail
            return;

        m_credentials.insert( csKey, value );

        QByteArray data;
        {
            QDataStream ds( &data, QIODevice::WriteOnly );
            ds << value;
        }

        QKeychain::WritePasswordJob* wj = new QKeychain::WritePasswordJob( csKey.service(), this );
        wj->setKey( csKey.key() );
        wj->setBinaryData( data );
        j = wj;
    }

    j->setAutoDelete( false );
#if defined( Q_OS_UNIX ) && !defined( Q_OS_MAC )
    j->setInsecureFallback( true );
#endif
    connect( j, SIGNAL( finished( QKeychain::Job* ) ),
             SLOT( keychainJobFinished( QKeychain::Job* ) ) );
    j->start();
}


void
CredentialsManager::setCredentials( const QString& serviceName, const QString& key, const QVariantHash& value )
{
    setCredentials( CredentialsStorageKey( serviceName, key ), value );
}


void
CredentialsManager::keychainJobFinished( QKeychain::Job* j )
{
    tDebug() << Q_FUNC_INFO;
    if ( QKeychain::ReadPasswordJob* readJob = qobject_cast< QKeychain::ReadPasswordJob* >( j ) )
    {
        if ( readJob->error() == QKeychain::NoError )
        {
            tDebug() << "QtKeychain readJob for" << readJob->key() << "finished without errors";

            QVariantHash creds;
            QDataStream dataStream( readJob->binaryData() );
            dataStream >> creds;

            m_credentials.insert( CredentialsStorageKey( readJob->service(), readJob->key() ), creds );
        }
        else
        {
            tDebug() << "QtKeychain readJob for" << readJob->key() << "finished with error:" << j->error() << j->errorString();
        }

        m_readJobs.removeOne( readJob );

        if ( m_readJobs.isEmpty() )
        {
            emit ready();
        }
    }
    else if ( QKeychain::WritePasswordJob* writeJob = qobject_cast< QKeychain::WritePasswordJob* >( j ) )
    {
        tLog() << Q_FUNC_INFO << "QtKeychain writeJob for" << writeJob->key() << "finished"
               << ( ( j->error() == QKeychain::NoError ) ? "without error" : j->errorString() );
    }
    else if ( QKeychain::DeletePasswordJob* deleteJob = qobject_cast< QKeychain::DeletePasswordJob* >( j ) )
    {
        tLog() << Q_FUNC_INFO << "QtKeychain deleteJob for" << deleteJob->key() << "finished"
               << ( ( j->error() == QKeychain::NoError ) ? "without error" : j->errorString() );
    }
    j->deleteLater();
}

} //namespace Accounts

} //namespace Tomahawk
