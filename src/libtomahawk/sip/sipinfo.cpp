/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Dominik Schmidt <dev@dominik-schmidt.de>
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

#include "sipinfo.h"

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QVariant>

class SipInfoPrivate : public QSharedData {
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
    QHostAddress host;
    int port;
    QString uniqname;
    QString key;
};

SipInfo::SipInfo()
{
    d = new SipInfoPrivate;
}

SipInfo::SipInfo(const SipInfo& other): d ( other.d )
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
    d->host = QHostAddress();
    d->port = -1;
    d->uniqname = QString();
    d->key = QString();
}

bool
SipInfo::isValid() const
{
    if( !d->visible.isNull() )
        if(
            // visible and all data available
            (  d->visible.toBool() && !d->host.isNull() && ( d->port > 0 ) && !d->uniqname.isNull() && !d->key.isNull() )
            // invisible and no data available
         || ( !d->visible.toBool() &&  d->host.isNull() && ( d->port < 0 ) && d->uniqname.isNull() &&   d->key.isNull() )
        )
            return true;
        else
            return false;

}

void
SipInfo::setVisible(bool visible)
{
    d->visible.setValue(visible);
}

bool
SipInfo::isVisible() const
{
    Q_ASSERT( isValid() );

    d->visible.toBool();
}

void
SipInfo::setHost( const QHostAddress& host )
{
    d->host = host;
}

const QHostAddress
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
    Q_ASSERT( isValid() );

    // build variant map
    QVariantMap m;
    m["visible"] = isVisible();
    if( isVisible() )
    {
        m["ip"] = host().toString();
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
        qDebug() << Q_FUNC_INFO << "Invalid JSON";
        return info;
    }
    QVariantMap m = v.toMap();

    info.setVisible( m["visible"].toBool() );
    if( m["visible"].toBool() )
    {
        info.setHost( QHostAddress( m["host"].toString() ) );
        info.setPort( m["port"].toInt() );
        info.setUniqname( m["uniqname"].toString() );
        info.setKey( m["key"].toString() );
    }

    return info;
}


QDebug operator<< ( QDebug dbg, const SipInfo& info )
{
    if( !isValid() )
        dbg.nospace() << "info is invalid";
    else
        dbg.nospace() << info.toJson();

    return dbg.maybeSpace();
}
