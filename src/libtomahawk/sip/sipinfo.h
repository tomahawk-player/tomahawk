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

#ifndef SIPINFO_H
#define SIPINFO_H

#include <QDebug>
#include <QSharedPointer>
#include <QHostAddress>

class SipInfoPrivate;

class SipInfo : public QObject
{
Q_OBJECT

public:
    // create invalid message
    // becomes valid if either visible == false or visible == true and all attributes are set
    SipInfo();
    SipInfo(const SipInfo &other);
    virtual ~SipInfo();
    SipInfo& operator=(const SipInfo &info);

    void clear();
    bool isValid() const;

    void setVisible( bool visible );
    bool isVisible() const;

    void setHost( const QHostAddress &host );
    const QHostAddress host() const;

    void setPort( int port );
    int port() const;

    void setUniqname( const QString &uniqname );
    const QString uniqname() const;

    void setKey( const QString &key );
    const QString key() const;


    const QString toJson() const;
    static const SipInfo fromJson( QString json );

private:
    QSharedDataPointer<SipInfoPrivate> d;
};

QDebug operator<<( QDebug dbg, const SipInfo &info );



#endif // SIPINFO_H