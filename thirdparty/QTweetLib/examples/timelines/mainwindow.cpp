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

#include <QNetworkAccessManager>
#include <QTimer>
#include <QDateTime>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "oauthtwitter.h"
#include "qtweethometimeline.h"
#include "qtweetmentions.h"
#include "qtweetusertimeline.h"
#include "qtweetdirectmessages.h"
#include "qtweetstatus.h"
#include "qtweetdmstatus.h"
#include "qtweetuser.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_sinceidHomeTimeline = 0;
    m_sinceidMentions = 0;
    m_sinceidUserTimeline = 0;
    m_sinceidDirectMessages = 0;

    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));
    connect(m_oauthTwitter, SIGNAL(authorizeXAuthFinished()), this, SLOT(xauthFinished()));
    connect(m_oauthTwitter, SIGNAL(authorizeXAuthError()), this, SLOT(xauthError()));

    m_timer = new QTimer(this);
    m_timer->setInterval(60000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerTimeOut()));

    connect(ui->authorizePushButton, SIGNAL(clicked()), this, SLOT(authorizeButtonClicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::authorizeButtonClicked()
{
    m_oauthTwitter->authorizeXAuth(ui->usernameLineEdit->text(), ui->passwordLineEdit->text());

}

void MainWindow::xauthFinished()
{
    ui->statusBar->showMessage("xauth succesfull");
    m_timer->start();
    timerTimeOut();
}

void MainWindow::xauthError()
{
    ui->statusBar->showMessage("xauth failed");
}

void MainWindow::timerTimeOut()
{
    QTweetHomeTimeline *homeTimeline = new QTweetHomeTimeline(m_oauthTwitter, this);
    homeTimeline->fetch(m_sinceidHomeTimeline);
    connect(homeTimeline, SIGNAL(parsedStatuses(QList<QTweetStatus>)),
            this, SLOT(homeTimelineStatuses(QList<QTweetStatus>)));

    QTweetMentions *mentions = new QTweetMentions(m_oauthTwitter, this);
    mentions->fetch(m_sinceidMentions);
    connect(mentions, SIGNAL(parsedStatuses(QList<QTweetStatus>)),
            this, SLOT(mentionsStatuses(QList<QTweetStatus>)));

    QTweetUserTimeline *userTimeline = new QTweetUserTimeline(m_oauthTwitter, this);
    userTimeline->fetch(0, QString(), m_sinceidUserTimeline);
    connect(userTimeline, SIGNAL(parsedStatuses(QList<QTweetStatus>)),
            this, SLOT(userTimelineStatuses(QList<QTweetStatus>)));

    QTweetDirectMessages *dmTimeline = new QTweetDirectMessages(m_oauthTwitter, this);
    dmTimeline->fetch(m_sinceidDirectMessages);
    connect(dmTimeline, SIGNAL(parsedDirectMessages(QList<QTweetDMStatus>)),
            this, SLOT(directMessages(QList<QTweetDMStatus>)));
}

void MainWindow::homeTimelineStatuses(const QList<QTweetStatus> &statuses)
{
    QTweetHomeTimeline *homeTimeline = qobject_cast<QTweetHomeTimeline*>(sender());

    if (homeTimeline) {
        if (statuses.count()) {
            //order is messed up, but this is just example
            foreach (const QTweetStatus& status, statuses) {
                ui->homeTimelineTextEdit->append("id: " + QString::number(status.id()));
                ui->homeTimelineTextEdit->append("text: " + status.text());
                ui->homeTimelineTextEdit->append("created: " + status.createdAt().toString());

                QTweetUser userinfo = status.user();

                ui->homeTimelineTextEdit->append("screen name: " + userinfo.screenName());
                ui->homeTimelineTextEdit->append("user id: " + QString::number(userinfo.id()));

                //is it retweet?
                QTweetStatus rtStatus = status.retweetedStatus();

                if (rtStatus.id()) {
                    ui->homeTimelineTextEdit->append("retweet text: " + rtStatus.text());
                }

                ui->homeTimelineTextEdit->append("----------------------------------------");

            }

            m_sinceidHomeTimeline = statuses.at(0).id();
        }

        homeTimeline->deleteLater();
    }
}

void MainWindow::mentionsStatuses(const QList<QTweetStatus> &statuses)
{
    QTweetMentions *mentions = qobject_cast<QTweetMentions*>(sender());

    if (mentions) {
        if (statuses.count()) {
            foreach (const QTweetStatus& status, statuses) {
                ui->mentionsTextEdit->append("id: " + QString::number(status.id()));
                ui->mentionsTextEdit->append("text: " + status.text());
                ui->mentionsTextEdit->append("created: " + status.createdAt().toString());

                QTweetUser userinfo = status.user();

                ui->mentionsTextEdit->append("screen name: " + userinfo.screenName());
                ui->mentionsTextEdit->append("user id: " + QString::number(userinfo.id()));

                ui->mentionsTextEdit->append("----------------------------------------");
            }

            m_sinceidMentions = statuses.at(0).id();
        }
        mentions->deleteLater();
    }
}

void MainWindow::userTimelineStatuses(const QList<QTweetStatus> &statuses)
{
    QTweetUserTimeline *userTimeline = qobject_cast<QTweetUserTimeline*>(sender());

    if (userTimeline) {
        if (statuses.count()) {
            //order is messed up, but this is just example
            foreach (const QTweetStatus& status, statuses) {
                ui->userTimelineTextEdit->append("id: " + QString::number(status.id()));
                ui->userTimelineTextEdit->append("text: " + status.text());
                ui->userTimelineTextEdit->append("created: " + status.createdAt().toString());

                QTweetUser userinfo = status.user();

                ui->userTimelineTextEdit->append("screen name: " + userinfo.screenName());
                ui->userTimelineTextEdit->append("user id: " + QString::number(userinfo.id()));

                ui->userTimelineTextEdit->append("----------------------------------------");
            }

            m_sinceidUserTimeline = statuses.at(0).id();
        }

        userTimeline->deleteLater();
    }
}

void MainWindow::directMessages(const QList<QTweetDMStatus> &directMessages)
{
    QTweetDirectMessages *dmTimeline = qobject_cast<QTweetDirectMessages*>(sender());

    if (dmTimeline) {
        if (directMessages.count()) {
            foreach (const QTweetDMStatus& message, directMessages) {
                ui->directMessagesTextEdit->append("id: " + QString::number(message.id()));
                ui->directMessagesTextEdit->append("text: " + message.text());
                ui->directMessagesTextEdit->append("created: " + message.createdAt().toString());
                ui->directMessagesTextEdit->append("sender: " + message.senderScreenName());
                ui->directMessagesTextEdit->append("sender id: " + QString::number(message.senderId()));

                ui->directMessagesTextEdit->append("----------------------------------------");
            }

            m_sinceidDirectMessages = directMessages.at(0).id();
        }
    }

    dmTimeline->deleteLater();
}
