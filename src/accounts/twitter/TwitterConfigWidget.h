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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TWITTERACCOUNTCONFIGWIDGET_H
#define TWITTERACCOUNTCONFIGWIDGET_H

#include "accounts/AccountDllMacro.h"

#include <QTweetLib/qtweetstatus.h>
#include <QTweetLib/qtweetdmstatus.h>
#include <QTweetLib/qtweetuser.h>
#include <QTweetLib/qtweetnetbase.h>

#include "accounts/AccountConfigWidget.h"

namespace Ui
{
    class TwitterConfigWidget;
}

namespace Tomahawk
{

namespace Accounts
{

class TwitterAccount;


class ACCOUNTDLLEXPORT TwitterConfigWidget : public AccountConfigWidget
{
    Q_OBJECT

public:
    explicit TwitterConfigWidget( TwitterAccount* account = 0, QWidget *parent = 0 );
    virtual ~TwitterConfigWidget();

signals:
    void twitterAuthed( bool authed );

    void sizeHintChanged();

private slots:
    void authDeauthTwitter();
    void startPostGotTomahawkStatus();
    void authenticateVerifyReply( const QTweetUser &user );
    void authenticateVerifyError( QTweetNetBase::ErrorCode code, const QString &errorMsg );
    void postGotTomahawkStatusAuthVerifyReply( const QTweetUser &user );
    void postGotTomahawkStatusUpdateReply( const QTweetStatus &status );
    void postGotTomahawkDirectMessageReply( const QTweetDMStatus &status );
    void postGotTomahawkStatusUpdateError( QTweetNetBase::ErrorCode, const QString &errorMsg );
    void tweetComboBoxIndexChanged( int index );

private:
    void authenticateTwitter();
    void deauthenticateTwitter();

    Ui::TwitterConfigWidget *m_ui;
    TwitterAccount *m_account;
    QString m_postGTtype;
};

}

}

#endif // TWITTERCONFIGWIDGET_H
