/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Dominik Schmidt <dev@dominik-schmidt.de>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "PeerInfo_p.h"

#include "accounts/Account.h"
#include "network/ControlConnection.h"
#include "network/Servent.h"
#include "utils/Logger.h"
#include "utils/TomahawkCache.h"
#include "utils/TomahawkUtilsGui.h"

#include "SipInfo.h"
#include "SipPlugin.h"

#include <QCryptographicHash>
#include <QBuffer>

namespace Tomahawk
{

Tomahawk::Utils::WeakObjectHash< PeerInfo > PeerInfoPrivate::s_peersByCacheKey = Tomahawk::Utils::WeakObjectHash< PeerInfo >();
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
    if ( ! ( options & AutoCreate ) )
    {
        return peerinfo_ptr();
    }

    peerinfo_ptr selfPeer( new PeerInfo( parent, "local peerinfo don't use this id for anything" ), &QObject::deleteLater );
    selfPeer->setWeakRef( selfPeer.toWeakRef() );
    selfPeer->setContactId( "localpeer" );

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
    if ( PeerInfoPrivate::s_peersByCacheKey.hash().contains( key ) && !PeerInfoPrivate::s_peersByCacheKey.hash().value( key ).isNull() )
    {
        return PeerInfoPrivate::s_peersByCacheKey.hash().value( key ).toStrongRef();
    }

    // if AutoCreate isn't enabled nothing to do here
    if ( ! ( options & AutoCreate ) )
    {
        return peerinfo_ptr();
    }

    peerinfo_ptr peerInfo( new PeerInfo( parent, id ), &QObject::deleteLater );
    peerInfo->setWeakRef( peerInfo.toWeakRef() );
    PeerInfoPrivate::s_peersByCacheKey.insert( key, peerInfo );

    return peerInfo;
}


QList< Tomahawk::peerinfo_ptr >
PeerInfo::getAll()
{
    QList< Tomahawk::peerinfo_ptr > strongRefs;
    foreach ( Tomahawk::peerinfo_wptr wptr, PeerInfoPrivate::s_peersByCacheKey.hash().values() )
    {
        if ( !wptr.isNull() )
            strongRefs << wptr.toStrongRef();
    }
    return strongRefs;
}


PeerInfo::PeerInfo( SipPlugin* parent, const QString& id )
    : QObject()
    , d_ptr( new Tomahawk::PeerInfoPrivate ( this, parent, id ) )
{
}


PeerInfo::~PeerInfo()
{
    Q_D( PeerInfo );

    tLog( LOGVERBOSE ) << Q_FUNC_INFO;

    delete d->avatar;
    delete d->fancyAvatar;
    delete d_ptr;
}


void
PeerInfo::announce()
{
    Q_ASSERT( !contactId().isEmpty() );

    Servent::instance()->registerPeer( weakRef().toStrongRef() );
}


QWeakPointer< PeerInfo >
PeerInfo::weakRef()
{
    return d_func()->ownRef;
}


void
PeerInfo::setWeakRef( QWeakPointer< PeerInfo > weakRef )
{
    d_func()->ownRef = weakRef;
}


void
PeerInfo::setControlConnection( ControlConnection* controlConnection )
{
    d_func()->controlConnection = controlConnection;
}


ControlConnection*
PeerInfo::controlConnection() const
{
    return d_func()->controlConnection;
}


bool PeerInfo::hasControlConnection()
{
    return !d_func()->controlConnection.isNull();
}


void
PeerInfo::setType( Tomahawk::PeerInfo::Type type )
{
    d_func()->type = type;
}


PeerInfo::Type
PeerInfo::type() const
{
    return d_func()->type;
}


const
QString PeerInfo::id() const
{
    return d_func()->id;
}


SipPlugin*
PeerInfo::sipPlugin() const
{
    return d_func()->parent;
}


void
PeerInfo::sendLocalSipInfos( const QList<SipInfo>& sipInfos )
{
    sipPlugin()->sendSipInfos( weakRef().toStrongRef(), sipInfos );
}


const QString
PeerInfo::debugName() const
{
    return QString("%1 : %2").arg( sipPlugin()->account()->accountFriendlyName() ).arg( id() );
}


void
PeerInfo::setContactId ( const QString& contactId )
{
    d_func()->contactId = contactId;
}


const QString
PeerInfo::contactId() const
{
    return d_func()->contactId;
}

const QString
PeerInfo::nodeId() const
{
    Q_D( const PeerInfo );

    if ( d->sipInfos.isEmpty() )
    {
        // Return an empty nodeId if we have not yet received the acutal nodeId.
        return QString();
    }

    // All sip infos share the same nodeId
    return d->sipInfos.first().nodeId();
}

const QString
PeerInfo::key() const
{
    Q_ASSERT( !d_func()->sipInfos.isEmpty() );
    // All sip infos share the same key
    return d_func()->sipInfos.first().key();
}



void
PeerInfo::setStatus( PeerInfo::Status status )
{
    d_func()->status = status;

    if ( status == Online )
    {
        announce();
    }
    else if ( status == Offline && controlConnection() )
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
    return d_func()->status;
}


void
PeerInfo::setSipInfos( const QList<SipInfo>& sipInfos )
{
    d_func()->sipInfos = sipInfos;

    tLog() << "id:" << id() << "info changed" << sipInfos;
    emit sipInfoChanged();
}


const QList<SipInfo>
PeerInfo::sipInfos() const
{
    return d_func()->sipInfos;
}


void
PeerInfo::setFriendlyName( const QString& friendlyName )
{
    d_func()->friendlyName = friendlyName;
}


const QString
PeerInfo::friendlyName() const
{
    return d_func()->friendlyName;
}


void
PeerInfo::setAvatar( const QPixmap& avatar )
{
    Q_D( PeerInfo );

    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );
    avatar.save( &buffer, "PNG" );

    // Check if the avatar is different by comparing a hash of the first 4096 bytes
    const QByteArray hash = QCryptographicHash::hash( ba.left( 4096 ), QCryptographicHash::Sha1 );
    if ( d->avatarHash == hash )
        return;

    d->avatarHash = hash;
    d->avatarBuffer = ba;

    delete d->avatar;
    delete d->fancyAvatar;
    d->avatar = 0;
    d->fancyAvatar = 0;

    Q_ASSERT( !contactId().isEmpty() );
    TomahawkUtils::Cache::instance()->putData( "Sources", 7776000000 /* 90 days */, contactId(), ba );
}


const QPixmap
PeerInfo::avatar( TomahawkUtils::ImageMode style, const QSize& size ) const
{
    Q_D( const PeerInfo );

    if ( !d->avatar )
    {
        tDebug() << "Avatar for:" << id();
        Q_ASSERT( !contactId().isEmpty() );
        if ( d->avatarBuffer.isEmpty() && !contactId().isEmpty() )
            d->avatarBuffer = TomahawkUtils::Cache::instance()->getData( "Sources", contactId() ).toByteArray();

        d->avatar = new QPixmap();
        if ( !d->avatarBuffer.isEmpty() )
            d->avatar->loadFromData( d->avatarBuffer );

        d->avatarBuffer.clear();
    }

    if ( style == TomahawkUtils::RoundedCorners && d->avatar && !d->avatar->isNull() && !d->fancyAvatar )
        d->fancyAvatar = new QPixmap( TomahawkUtils::createRoundedImage( QPixmap( *d->avatar ), QSize( 0, 0 ) ) );

    QPixmap pixmap;
    if ( style == TomahawkUtils::RoundedCorners && d->fancyAvatar )
    {
        pixmap = *d->fancyAvatar;
    }
    else if ( d->avatar && !d->avatar->isNull() )
    {
        pixmap = *d->avatar;
    }

    if ( !pixmap.isNull() && !size.isEmpty() )
    {
        if ( d->coverCache[ style ].contains( size.width() ) )
        {
            return d->coverCache[ style ].value( size.width() );
        }

        QPixmap scaledCover;
        scaledCover = pixmap.scaled( size, Qt::KeepAspectRatio, Qt::SmoothTransformation );

        QHash< int, QPixmap > innerCache = d->coverCache[ style ];
        innerCache.insert( size.width(), scaledCover );
        d->coverCache[ style ] = innerCache;

        return scaledCover;
    }

    return pixmap;
}


void
PeerInfo::setVersionString( const QString& versionString )
{
    d_func()->versionString = versionString;
}


const QString
PeerInfo::versionString() const
{
    return d_func()->versionString;
}


void
PeerInfo::setData( const QVariant& data )
{
    d_func()->data = data;
}

const QVariant
PeerInfo::data() const
{
    return d_func()->data;
}


} // ns
