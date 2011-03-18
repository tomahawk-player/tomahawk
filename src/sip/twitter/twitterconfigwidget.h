/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TWITTERCONFIGWIDGET_H
#define TWITTERCONFIGWIDGET_H

#include "sip/SipPlugin.h"

#include <qtweetstatus.h>
#include <qtweetuser.h>
#include <qtweetnetbase.h>

#include <QWidget>


namespace Ui {
    class TwitterConfigWidget;
}

class TwitterConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TwitterConfigWidget(SipPlugin* plugin = 0, QWidget *parent = 0);
    ~TwitterConfigWidget();

private slots:
    void authenticateTwitter();
    void startPostGotTomahawkStatus();
    void postGotTomahawkStatusAuthVerifyReply( const QTweetUser &user );
    void postGotTomahawkStatusUpdateReply( const QTweetStatus &status );
    void postGotTomahawkStatusUpdateError( QTweetNetBase::ErrorCode, const QString &errorMsg );

private:
    Ui::TwitterConfigWidget *ui;
    SipPlugin *m_plugin;
};

#endif // TWITTERCONFIGWIDGET_H
