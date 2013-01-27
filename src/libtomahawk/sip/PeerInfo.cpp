/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Dominik Schmidt <dev@dominik-schmidt.de>
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

#include "PeerInfo.h"
#include "SipPlugin.h"
#include "utils/TomahawkCache.h"
#include "utils/TomahawkUtilsGui.h"
#include "network/ControlConnection.h"
#include "network/Servent.h"
#include "utils/Logger.h"

#include <QCryptographicHash>
#include <QBuffer>

namespace Tomahawk
{

QHash< QString, peerinfo_ptr > PeerInfo::s_peersByCacheKey = QHash< QString, peerinfo_ptr >();
QHash< SipPlugin*, peerinfo_ptr > PeerInfo::s_selfPeersBySipPlugin = QHash< SipPlugin*, peerinfo_ptr >();

inline QString
peerCacheKey( SipPlugin* plugin, const QString& peerId )
{
    return QString( "%1\t\t%2" ).arg( (quintptr) plugin ).arg( peerId );
}

Tomahawk::peerinfo_ptr
PeerInfo::getSelf( SipPlugin* parent, PeerInfo::GetOptions options )
{
    if ( s_selfPeersBySipPlugin.keys().contains( parent ) )
    {
        return s_selfPeersBySipPlugin.value( parent );
    }

    // if AutoCreate isn't enabled nothing to do here
    if( ! ( options & AutoCreate ) )
    {
        return peerinfo_ptr();
    }

    peerinfo_ptr selfPeer( new PeerInfo( parent, "local peerinfo don't use this id for anything" ) );
    selfPeer->setWeakRef( selfPeer.toWeakRef() );

//     parent->setSelfPeer( selfPeer );
    s_selfPeersBySipPlugin.insert( parent, selfPeer );

    return selfPeer;
}


QList< Tomahawk::peerinfo_ptr >
PeerInfo::getAllSelf()
{
    return s_selfPeersBySipPlugin.values();
}


Tomahawk::peerinfo_ptr
PeerInfo::get( SipPlugin* parent, const QString& id, GetOptions options )
{
    const QString key = peerCacheKey( parent, id );
    if ( s_peersByCacheKey.contains( key ) )
    {
        return s_peersByCacheKey.value( key );
    }

    // if AutoCreate isn't enabled nothing to do here
    if( ! ( options & AutoCreate ) )
    {
        return peerinfo_ptr();
    }

    peerinfo_ptr peerInfo( new PeerInfo( parent, id ) );
    peerInfo->setWeakRef( peerInfo.toWeakRef() );
    s_peersByCacheKey.insert( key, peerInfo);

    return peerInfo;
}


QList< Tomahawk::peerinfo_ptr >
PeerInfo::getAll()
{
    return s_peersByCacheKey.values();
}

PeerInfo::PeerInfo( SipPlugin* parent, const QString& id )
    : QObject( parent )
    , m_type( External )
    , m_id( id )
    , m_status( Offline )
    , m_avatar( 0 )
    , m_fancyAvatar( 0 )
    , m_avatarUpdated( true )
{
}



PeerInfo::~PeerInfo()
{
    tDebug() << Q_FUNC_INFO;
    delete m_avatar;
    delete m_fancyAvatar;
}


void
PeerInfo::announce()
{
    Servent::instance()->registerPeer( weakRef().toStrongRef() );
}


QWeakPointer< PeerInfo >
PeerInfo::weakRef()
{
    return m_ownRef;
}


void
PeerInfo::setWeakRef( QWeakPointer< PeerInfo > weakRef )
{
    m_ownRef = weakRef;
}


void
PeerInfo::setControlConnection( ControlConnection* controlConnection )
{
    m_controlConnection = controlConnection;
}

ControlConnection*
PeerInfo::controlConnection() const
{
    return m_controlConnection;
}

bool PeerInfo::hasControlConnection()
{
    return !m_controlConnection.isNull();
}



void
PeerInfo::setType( Tomahawk::PeerInfo::Type type )
{
    m_type = type;
}

PeerInfo::Type
PeerInfo::type() const
{
    return m_type;
}

const
QString PeerInfo::id() const
{
    return m_id;
}



SipPlugin*
PeerInfo::sipPlugin() const
{
    return qobject_cast< SipPlugin* >( parent() );
}


void
PeerInfo::sendLocalSipInfo( const SipInfo& sipInfo )
{
    sipPlugin()->sendSipInfo( weakRef().toStrongRef(), sipInfo );
}


const QString
PeerInfo::debugName() const
{
    return QString("%1 : %2").arg( sipPlugin()->account()->accountFriendlyName() ).arg( id() );
}



void
PeerInfo::setStatus( PeerInfo::Status status )
{
    m_status = status;

    if( status == Online )
    {
        announce();
    }
    else if( status == Offline && controlConnection() )
    {
        controlConnection()->removePeerInfo( weakRef().toStrongRef() );
    }

    // we need this to update the DiagnosticsDialog on new peers
    // if we ever happen to have a central PeerInfo manager object
    // we better add it there, but so far this would be the only
    // usage
    emit sipPlugin()->peerStatusChanged( weakRef().toStrongRef() );
}


PeerInfo::Status
PeerInfo::status() const
{
    return m_status;
}


void
PeerInfo::setSipInfo( const SipInfo& sipInfo )
{
    if(sipInfo == m_sipInfo)
        return;

    m_sipInfo = sipInfo;

    tLog() << "id: " << id() << " info changed" << sipInfo;
    emit sipInfoChanged();
}


const SipInfo
PeerInfo::sipInfo() const
{
    return m_sipInfo;
}


void
PeerInfo::setFriendlyName( const QString& friendlyName )
{
    m_friendlyName = friendlyName;
}


const QString
PeerInfo::friendlyName() const
{
    return m_friendlyName;
}


void
PeerInfo::setAvatar( const QPixmap& avatar )
{
    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );
    avatar.save( &buffer, "PNG" );

    // Check if the avatar is different by comparing a hash of the first 4096 bytes
    const QByteArray hash = QCryptographicHash::hash( ba.left( 4096 ), QCryptographicHash::Sha1 );
    if ( m_avatarHash == hash )
        return;
    else
        m_avatarHash = hash;

    delete m_avatar;
    m_avatar = new QPixmap( avatar );
    m_fancyAvatar = 0;

    TomahawkUtils::Cache::instance()->putData( "Sources", 7776000000 /* 90 days */, id(), ba );
    m_avatarUpdated = true;
}


const QPixmap
PeerInfo::avatar( TomahawkUtils::ImageMode style, const QSize& size ) const
{
//     tLog() << "*****************************************" << Q_FUNC_INFO << id();

    if ( !m_avatar && m_avatarUpdated )
    {
        m_avatar = new QPixmap();
        QByteArray ba = TomahawkUtils::Cache::instance()->getData( "Sources", id() ).toByteArray();

        if ( ba.count() )
            m_avatar->loadFromData( ba );

        if ( m_avatar->isNull() )
        {
            delete m_avatar;
            m_avatar = 0;
        }
        m_avatarUpdated = false;
    }

    if ( style == TomahawkUtils::RoundedCorners && m_avatar && !m_avatar->isNull() && !m_fancyAvatar )
        m_fancyAvatar = new QPixmap( TomahawkUtils::createRoundedImage( QPixmap( *m_avatar ), QSize( 0, 0 ) ) );

    QPixmap pixmap;
    if ( style == TomahawkUtils::RoundedCorners && m_fancyAvatar )
    {
        pixmap = *m_fancyAvatar;
    }
    else if ( m_avatar )
    {
        pixmap = *m_avatar;
    }

    if ( !pixmap.isNull() && !size.isEmpty() )
    {
        if ( m_coverCache[ style ].contains( size.width() ) )
        {
            return m_coverCache[ style ].value( size.width() );
        }

        QPixmap scaledCover;
        scaledCover = pixmap.scaled( size, Qt::KeepAspectRatio, Qt::SmoothTransformation );

        QHash< int, QPixmap > innerCache = m_coverCache[ style ];
        innerCache.insert( size.width(), scaledCover );
        m_coverCache[ style ] = innerCache;

        return scaledCover;
    }

    return pixmap;
}

void
PeerInfo::setVersionString(const QString& versionString)
{
    m_versionString = versionString;
}


const QString
PeerInfo::versionString() const
{
    return m_versionString;
}


void
PeerInfo::setData(const QVariant& data)
{
    m_data = data;
}


const
QVariant PeerInfo::data() const
{
    return m_data;
}


} // ns
