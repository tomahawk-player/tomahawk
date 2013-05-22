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


CredentialsManager::CredentialsManager( QObject* parent )
    : QObject( parent )
{

}


void
CredentialsManager::loadCredentials( QStringList keys )
{
    foreach ( QString key, keys )
    {
        QKeychain::ReadPasswordJob* j = new QKeychain::ReadPasswordJob( QLatin1String( "tomahawkaccounts" ), this );
        j->setKey( key );
        j->setAutoDelete( false );
#if defined( Q_OS_UNIX ) && !defined( Q_OS_MAC )
        j->setInsecureFallback( true );
#endif
        connect( j, SIGNAL( finished( QKeychain::Job* ) ),
                 SLOT( keychainJobFinished( QKeychain::Job* ) ) );
        m_readJobs << j;
        j->start();
    }

}


QStringList
CredentialsManager::keys() const
{
    QStringList keys = m_credentials.keys();
    return keys;
}


QVariantHash
CredentialsManager::credentials( const QString& key ) const
{
    return m_credentials.value( key );
}


void
CredentialsManager::setCredentials( const QString& key, const QVariantHash& value )
{
    if ( value.isEmpty() )
    {
        m_credentials.remove( key );

        //TODO: delete job
    }
    else
    {
        m_credentials.insert( key, value );

        //TODO: write job
    }
}


void
CredentialsManager::keychainJobFinished( QKeychain::Job* j )
{
    if ( j->error() == QKeychain::NoError )
    {
        if ( QKeychain::ReadPasswordJob* readJob = qobject_cast< QKeychain::ReadPasswordJob* >( j ) )
        {
            tDebug() << "QtKeychain readJob for" << readJob->key() << "finished without errors";

            QVariantHash creds;
            QDataStream dataStream( readJob->binaryData() );
            dataStream >> creds;

            m_credentials.insert( readJob->key(), creds );

            m_readJobs.removeAll( readJob );

            if ( m_readJobs.isEmpty() )
            {
                emit ready();
            }
        }
    }
    else if ( QKeychain::WritePasswordJob* writeJob = qobject_cast< QKeychain::WritePasswordJob* >( j ) )
    {
        tLog() << Q_FUNC_INFO << "QtKeychain writeJob finished";
    }
    else if ( QKeychain::DeletePasswordJob* deleteJob = qobject_cast< QKeychain::DeletePasswordJob* >( j ) )
    {
        tLog() << Q_FUNC_INFO << "QtKeychain deleteJob finished";
    }
    else
    {
        tDebug() << "QtKeychain job finished with error:" << j->error() << j->errorString();
    }
    j->deleteLater();
}


} //namespace Accounts

} //namespace Tomahawk
