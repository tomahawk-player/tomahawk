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

#ifndef PEERINFO_P_H
#define PEERINFO_P_H

#include "PeerInfo.h"

#include "utils/WeakObjectHash.h"

namespace Tomahawk
{

class PeerInfoPrivate
{
public:
    PeerInfoPrivate( PeerInfo* q, SipPlugin* parent, const QString& id )
        : q_ptr ( q )
        , parent( parent )
        , type( PeerInfo::External )
        , id( id )
        , status( PeerInfo::Offline )
        , avatar( 0 )
        , fancyAvatar( 0 )
    {
    }
    PeerInfo* q_ptr;
    Q_DECLARE_PUBLIC ( PeerInfo )

    static Tomahawk::Utils::WeakObjectHash<PeerInfo> s_peersByCacheKey;

private:
    QWeakPointer< Tomahawk::PeerInfo > ownRef;
    QPointer< ControlConnection > controlConnection;

    SipPlugin* parent;
    PeerInfo::Type type;

    QString id;
    QString contactId;
    PeerInfo::Status  status;
    QList<SipInfo> sipInfos;
    QString friendlyName;
    QString versionString;
    QVariant data;

    mutable QPixmap* avatar;
    mutable QPixmap* fancyAvatar;

    mutable QByteArray avatarBuffer;
    mutable QByteArray avatarHash;
    mutable QHash< TomahawkUtils::ImageMode, QHash< int, QPixmap > > coverCache;
};

}

#endif // PEERINFO_P_H
