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

#ifndef PEERINFO_H
#define PEERINFO_H



#include "DllMacro.h"

#include "SipInfo.h"
#include "accounts/Account.h"
#include "utils/TomahawkUtils.h"

#include <QString>
#include <QPixmap>


#define peerInfoDebug(peerInfo) tDebug() << "PEERINFO:" << ( !peerInfo.isNull() ? peerInfo->debugName() : "Invalid PeerInfo" ).toLatin1().constData()

class SipPlugin;
class ControlConnection;

namespace Tomahawk
{

class DLLEXPORT PeerInfo : public QObject
{
Q_OBJECT

public:
    enum Status
    {
        Online,
        Offline
    };

    enum GetOptions
    {
        None,
        AutoCreate
    };

    // this is a uberstupid hack, identify characteristics of the type
    enum Type
    {
        External, // this is the default
        Local
    };

    static Tomahawk::peerinfo_ptr getSelf( SipPlugin* parent, GetOptions options = None );
    static QList< Tomahawk::peerinfo_ptr > getAllSelf();

    static Tomahawk::peerinfo_ptr get( SipPlugin* parent, const QString& id, GetOptions options = None );
    static QList< Tomahawk::peerinfo_ptr > getAll();

    virtual ~PeerInfo();

    const QString id() const;
    SipPlugin* sipPlugin() const;
    const QString debugName() const;
    void sendLocalSipInfo( const SipInfo& sipInfo );

    QWeakPointer< Tomahawk::PeerInfo > weakRef();
    void setWeakRef( QWeakPointer< Tomahawk::PeerInfo > weakRef );

    void setControlConnection( ControlConnection* controlConnection );
    ControlConnection* controlConnection() const;
    bool hasControlConnection();

    void setType( Tomahawk::PeerInfo::Type type );
    PeerInfo::Type type() const;

    /* actual data */

    // while peerId references a certain peer, contact id references the contact
    // e.g. a peerId might be a full jid with resource while contact id is the bare jid
    void setContactId( const QString& contactId );
    const QString contactId() const;

    void setStatus( Status status );
    Status status() const;

    void setSipInfo( const SipInfo& sipInfo );
    const SipInfo sipInfo() const;

    void setFriendlyName( const QString& friendlyName );
    const QString friendlyName() const;

    void setAvatar( const QPixmap& avatar );
    const QPixmap avatar( TomahawkUtils::ImageMode style = TomahawkUtils::Original, const QSize& size = QSize() ) const;

    void setVersionString( const QString& versionString );
    const QString versionString() const;

    // you can store arbitrary internal data for your plugin here
    void setData( const QVariant& data );
    const QVariant data() const;

signals:
    void sipInfoChanged();

private:
    PeerInfo( SipPlugin* parent, const QString& id );
    void announce();

    static QHash< QString, peerinfo_wptr > s_peersByCacheKey;
    static QHash< SipPlugin*, peerinfo_ptr > s_selfPeersBySipPlugin;

    QWeakPointer< Tomahawk::PeerInfo > m_ownRef;
    QPointer< ControlConnection > m_controlConnection;

    SipPlugin* m_parent;
    PeerInfo::Type m_type;

    QString m_id;
    QString m_contactId;
    Status  m_status;
    SipInfo m_sipInfo;
    QString m_friendlyName;
    QString m_versionString;
    QVariant m_data;

    mutable QPixmap* m_avatar;
    mutable QPixmap* m_fancyAvatar;

    mutable QByteArray m_avatarBuffer;
    mutable QByteArray m_avatarHash;
    mutable QHash< TomahawkUtils::ImageMode, QHash< int, QPixmap > > m_coverCache;
};


} // ns


#endif // PEERINFO_H
