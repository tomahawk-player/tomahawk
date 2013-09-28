/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef GOOGLEWRAPPER_H
#define GOOGLEWRAPPER_H

#include "../xmpp/XmppAccount.h"

namespace Tomahawk
{

namespace Accounts
{

class ACCOUNTDLLEXPORT GoogleWrapperFactory : public XmppAccountFactory
{
    Q_OBJECT
    Q_INTERFACES( Tomahawk::Accounts::AccountFactory )
    Q_PLUGIN_METADATA( IID "org.tomahawk-player.Player.AccountFactory" )

public:
    GoogleWrapperFactory() {}
    virtual ~GoogleWrapperFactory() {}

    virtual QString prettyName() const { return "Google"; }
    virtual QString factoryId() const { return "googleaccount"; }
    QString description() const { return tr( "Connect to Google Talk to find your friends" ); }
    virtual QPixmap icon() const;
    virtual Account* createAccount( const QString& pluginId );
};

class ACCOUNTDLLEXPORT GoogleWrapperSip : public XmppSipPlugin
{
    Q_OBJECT
public:
    GoogleWrapperSip( Tomahawk::Accounts::Account* account );
    virtual ~GoogleWrapperSip();

    virtual QString inviteString() const;

public slots:
    void showAddFriendDialog();

protected:
    QString defaultSuffix() const;
};

class ACCOUNTDLLEXPORT GoogleWrapper : public XmppAccount
{
    Q_OBJECT
public:
    GoogleWrapper( const QString& pluginID );
    virtual ~GoogleWrapper();

    virtual const QString name() const { return QString( "Google" ); }
    virtual const QString friendlyName() const { return "Google"; }

    virtual SipPlugin* sipPlugin( bool create = true );

private:
    QPointer< GoogleWrapperSip > m_sipPlugin;
};

}

}

#endif // GOOGLEWRAPPER_H
