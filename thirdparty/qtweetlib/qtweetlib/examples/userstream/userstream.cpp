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

#include "userstream.h"
#include "ui_userstream.h"
#include <QNetworkAccessManager>
#include <QTextStream>
#include <QFile>
#include "oauthtwitter.h"
#include "qtweetuserstream.h"
#include "qtweetstatus.h"
#include "qtweetuser.h"

UserStream::UserStream(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserStream)
{
    ui->setupUi(this);

    m_oauthTwitter = new OAuthTwitter(new QNetworkAccessManager, this);
    connect(m_oauthTwitter, SIGNAL(authorizeXAuthFinished()), SLOT(onAuthorizeFinished()));
    connect(m_oauthTwitter, SIGNAL(authorizeXAuthError()), SLOT(onAuthorizeError()));

    m_userStream = new QTweetUserStream(this);
    m_userStream->setOAuthTwitter(m_oauthTwitter);
    connect(m_userStream, SIGNAL(stream(QByteArray)), SLOT(stream(QByteArray)));
    connect(m_userStream, SIGNAL(statusesStream(QTweetStatus)), SLOT(statusStream(QTweetStatus)));

    connect(ui->connectButton, SIGNAL(clicked()), SLOT(onConnectButtonClicked()));

    //for internal purposes
    m_file.setFileName("logstream.txt");
    m_file.open(QIODevice::WriteOnly | QIODevice::Text);

    m_streamlogger.setDevice(&m_file);

}

UserStream::~UserStream()
{
    delete ui;
}

void UserStream::changeEvent(QEvent *e)
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

void UserStream::onConnectButtonClicked()
{
    m_oauthTwitter->authorizeXAuth(ui->usernameLineEdit->text(), ui->passwordLineEdit->text());
    ui->infoTextBrowser->append("XAuth authorization started");
    ui->connectButton->setEnabled(false);
}

void UserStream::onAuthorizeFinished()
{
    ui->infoTextBrowser->append("XAuth authorization success.");
    ui->infoTextBrowser->append("oauth token: " + m_oauthTwitter->oauthToken());
    ui->infoTextBrowser->append("oauth token secret: " + m_oauthTwitter->oauthTokenSecret());
    ui->infoTextBrowser->append("Starting user stream fetching");

    m_userStream->startFetching();
}

void UserStream::onAuthorizeError()
{
    ui->infoTextBrowser->append("XAuth authorization error");
    ui->connectButton->setEnabled(true);
}

void UserStream::stream(const QByteArray &stream)
{
    //for internal purposes
    m_streamlogger << stream << "\n";
    m_streamlogger << "################################################################" << "\n";
    m_streamlogger.flush();
}

void UserStream::statusStream(const QTweetStatus &tweet)
{
    ui->infoTextBrowser->append("New tweet");
    ui->infoTextBrowser->append("id: " + QString::number(tweet.id()));
    ui->infoTextBrowser->append("text: " + tweet.text());
    ui->infoTextBrowser->append("name: " + tweet.user().name());
}
