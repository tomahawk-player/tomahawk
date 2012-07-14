/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "SipInfo.h"

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QVariant>


class SipInfoPrivate : public QSharedData
{
public:
    SipInfoPrivate()
        : port( -1 )
    {
    }

    SipInfoPrivate( const SipInfoPrivate& other ) : QSharedData( other ),
        visible(other.visible),
        host(other.host),
        port(other.port),
        uniqname(other.uniqname),
        key(other.key)
    {
    }
    ~SipInfoPrivate() { }

    QVariant visible;
    QString host;
    int port;
    QString uniqname;
    QString key;
};


SipInfo::SipInfo()
{
    d = new SipInfoPrivate;
}


SipInfo::SipInfo(const SipInfo& other)
    : QObject()
    , d( other.d )
{
}


SipInfo::~SipInfo()
{
}


SipInfo&
SipInfo::operator=( const SipInfo& other )
{
    d = other.d;
    return *this;
}


void
SipInfo::clear()
{
    d->visible.clear();
    d->host = QString();
    d->port = -1;
    d->uniqname = QString();
    d->key = QString();
}


bool
SipInfo::isValid() const
{
//    qDebug() << Q_FUNC_INFO << d->visible << d->host.hostName() << d->port << d->uniqname << d->key;
    if( !d->visible.isNull() )
    {
        if(
            // visible and all data available
            (  d->visible.toBool() && !d->host.isEmpty() && ( d->port > 0 ) && !d->uniqname.isNull() && !d->key.isNull() )
            // invisible and no data available
         || ( !d->visible.toBool() &&  d->host.isEmpty() && ( d->port < 0 ) && d->uniqname.isNull() &&   d->key.isNull() )
        )
            return true;
    }

    return false;
}


void
SipInfo::setVisible( bool visible )
{
    d->visible.setValue(visible);
}


bool
SipInfo::isVisible() const
{
    Q_ASSERT( isValid() );

    return d->visible.toBool();
}


void
SipInfo::setHost( const QString& host )
{
    d->host = host;
}


const QString
SipInfo::host() const
{
    Q_ASSERT( isValid() );

    return d->host;
}


void
SipInfo::setPort( int port )
{
    d->port = port;
}


int
SipInfo::port() const
{
    Q_ASSERT( isValid() );

    return d->port;
}


void
SipInfo::setUniqname( const QString& uniqname )
{
    d->uniqname = uniqname;
}


const QString
SipInfo::uniqname() const
{
    Q_ASSERT( isValid() );

    return d->uniqname;
}


void
SipInfo::setKey( const QString& key )
{
    d->key = key;
}


const QString
SipInfo::key() const
{
    Q_ASSERT( isValid() );

    return d->key;
}


const QString
SipInfo::toJson() const
{
    // build variant map
    QVariantMap m;
    m["visible"] = isVisible();
    if( isVisible() )
    {
        m["ip"] = host();
        m["port"] = port();
        m["key"] = key();
        m["uniqname"] = uniqname();
    }

    // serialize
    QJson::Serializer serializer;
    QByteArray ba = serializer.serialize( m );

    return QString::fromAscii( ba );
}


const SipInfo
SipInfo::fromJson( QString json )
{
    SipInfo info;

    QJson::Parser parser;
    bool ok;
    QVariant v = parser.parse( json.toAscii(), &ok );
    if ( !ok  || v.type() != QVariant::Map )
    {
        qDebug() << Q_FUNC_INFO << "Invalid JSON: " << json;
        return info;
    }
    QVariantMap m = v.toMap();

    info.setVisible( m["visible"].toBool() );
    if( m["visible"].toBool() )
    {
        info.setHost( m["host"].toString() );
        info.setPort( m["port"].toInt() );
        info.setUniqname( m["uniqname"].toString() );
        info.setKey( m["key"].toString() );
    }

    return info;
}


QDebug operator<< ( QDebug dbg, const SipInfo& info )
{
    if( !info.isValid() )
        dbg.nospace() << "info is invalid";
    else
        dbg.nospace() << info.toJson();

    return dbg.maybeSpace();
}
