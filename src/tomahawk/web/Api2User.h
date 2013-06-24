/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef API2USER_H
#define API2USER_H

#include <QObject>
#include <QSslKey>

class Api2User : public QObject
{
    Q_OBJECT
public:
    Api2User( const QString& name );

    enum ACLDecision {
        None, Deny, FullAccess
    };

    ACLDecision aclDecision() const;
    void setAclDecision( ACLDecision decision );

    QString clientDescription() const;
    void setClientDescription( const QString& clientDescription );

    QString clientName() const;
    void setClientName( const QString& clientName );

    QString name() const;

    QSslKey pubkey() const;
    void setPubkey( const QSslKey& pubkey );
    
signals:
    
public slots:

private:
    ACLDecision m_aclDecision;
    QString m_clientDescription;
    QString m_clientName;
    QString m_name;
    QSslKey m_pubkey;

};

#endif // API2USER_H
