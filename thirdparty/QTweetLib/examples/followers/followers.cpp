/* Copyright (c) 2010, Antonie Jovanoski
 *
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#include "followers.h"
#include "ui_followers.h"
#include <QDeclarativeContext>
#include <QNetworkAccessManager>
#include "followerslistmodel.h"
#include "oauthtwitter.h"

Followers::Followers(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Followers)
{
    ui->setupUi(this);

    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));
    m_oauthTwitter->setOAuthToken("");
    m_oauthTwitter->setOAuthTokenSecret("");

    m_followersListModel = new FollowersListModel(m_oauthTwitter, this);

    ui->declarativeView->rootContext()->setContextProperty("followersListModel", m_followersListModel);
    ui->declarativeView->setSource(QUrl("qrc:/FollowersList.qml"));

    connect(ui->fetchFollowersPushButton, SIGNAL(clicked()), SLOT(onFetchFollowersPushButtonClicked()));
}

Followers::~Followers()
{
    delete ui;
}

void Followers::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void Followers::onFetchFollowersPushButtonClicked()
{
    m_followersListModel->fetchFollowers();
}
