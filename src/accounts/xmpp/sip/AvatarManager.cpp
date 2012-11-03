/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AvatarManager.h"

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

#include <jreen/vcard.h>
#include <jreen/vcardupdate.h>
#include <jreen/presence.h>
#include <jreen/iqreply.h>

#include <QDir>
#include <QCryptographicHash>
#include <QPixmap>


AvatarManager::AvatarManager( Jreen::Client* client )
    : m_cacheDir( TomahawkUtils::appDataDir().absolutePath().append( "/jreen/" ) )
{
    m_client = client;
    m_cachedAvatars = m_cacheDir.entryList();

    connect( m_client, SIGNAL( serverFeaturesReceived( QSet<QString> ) ), SLOT( onNewConnection() ) );
    connect( m_client, SIGNAL( presenceReceived( Jreen::Presence ) ), SLOT( onNewPresence( Jreen::Presence ) ) );
    connect( m_client, SIGNAL( iqReceived( Jreen::IQ ) ), SLOT( onNewIq( Jreen::IQ ) ) );

    connect( this, SIGNAL( newAvatar( QString ) ), SLOT( onNewAvatar( QString ) ) );
}


AvatarManager::~AvatarManager()
{
}


void
AvatarManager::onNewConnection()
{
    fetchVCard( m_client->jid().bare() );
}


void
AvatarManager::fetchVCard( const QString& jid )
{
    Jreen::IQ iq( Jreen::IQ::Get, jid );
    iq.addExtension( new Jreen::VCard() );
    Jreen::IQReply *reply = m_client->send( iq );

    connect( reply, SIGNAL( received( Jreen::IQ ) ), SLOT( onNewIq( Jreen::IQ ) ) );
}


void
AvatarManager::onNewPresence( const Jreen::Presence& presence )
{
    if ( presence.error() )
    {
        return;
    }

    Jreen::VCardUpdate::Ptr update = presence.payload<Jreen::VCardUpdate>();
    if ( update )
    {
//        qDebug() << "vcard: found update for" << presence.from().full();
        if ( !isCached( update->photoHash() ) )
        {
//            qDebug() << presence.from().full() << "vcard: photo not cached, starting request..." << update->photoHash();
            fetchVCard( presence.from().bare() );
        }
        else
        {
//            qDebug() << presence.from().full() << "vcard: photo already cached no request necessary " << update->photoHash();
            m_JidsAvatarHashes.insert( update->photoHash(), presence.from().bare() );

            if ( !this->avatar( presence.from().bare() ).isNull() )
                emit newAvatar( presence.from().bare() );
        }
    }
    else
    {
        //TODO: do we want this? might fetch avatars for broken clients
        fetchVCard( presence.from().bare() );
    }
}


void
AvatarManager::onNewIq( const Jreen::IQ& iq )
{
    Jreen::VCard::Ptr vcard = iq.payload<Jreen::VCard>();
    if ( vcard )
    {
        iq.accept();
//        qDebug() << Q_FUNC_INFO << "Got vcard from " << iq.from().full();

        QString id = iq.from().full();
        QString avatarHash;

        const Jreen::VCard::Photo &photo = vcard->photo();
        if ( !photo.data().isEmpty() )
        {
//            qDebug() << "vcard: got photo data" << id;

            avatarHash = QCryptographicHash::hash( photo.data(), QCryptographicHash::Sha1 ).toHex();

            if ( !m_cacheDir.exists() )
                m_cacheDir.mkpath( avatarDir( avatarHash ).absolutePath() );

            QFile file( avatarPath( avatarHash ) );
            if ( file.open( QIODevice::WriteOnly ) )
            {
                file.write( photo.data() );
                file.close();
            }

            m_cachedAvatars.append( avatarHash );
            m_JidsAvatarHashes.insert( avatarHash, iq.from().bare() );

            Q_ASSERT( !this->avatar( iq.from().bare() ).isNull() );
            emit newAvatar( iq.from().bare() );
        }

        // got own presence
        if ( m_client->jid().bare() == id )
        {
            Jreen::Presence presence = m_client->presence();
            Jreen::VCardUpdate::Ptr update = presence.payload<Jreen::VCardUpdate>();
            if ( update->photoHash() != avatarHash )
            {
                update->setPhotoHash( avatarHash );
                m_client->send( presence );
            }
        }
    }
}


QPixmap
AvatarManager::avatar( const QString& jid ) const
{
    if ( isCached( avatarHash( jid ) ) )
    {
        return QPixmap( avatarPath( avatarHash( jid ) ) );
    }
    else
    {
        return QPixmap();
    }
}


QString
AvatarManager::avatarHash( const QString& jid ) const
{
    //qDebug() << Q_FUNC_INFO << jid << m_JidsAvatarHashes.key( jid );
    return m_JidsAvatarHashes.key( jid );
}


QDir
AvatarManager::avatarDir( const QString& /* avatarHash */ ) const
{
    return m_cacheDir;
}


QString
AvatarManager::avatarPath( const QString& avatarHash ) const
{
    Q_ASSERT( !avatarHash.contains( "@" ) );
    return avatarDir( avatarHash ).absoluteFilePath( avatarHash );
}


bool
AvatarManager::isCached( const QString& avatarHash ) const
{
    return m_cachedAvatars.contains( avatarHash );
}


void
AvatarManager::onNewAvatar( const QString& /* jid */ )
{
//    qDebug() << Q_FUNC_INFO <<  "Found new Avatar..." << jid;
}
