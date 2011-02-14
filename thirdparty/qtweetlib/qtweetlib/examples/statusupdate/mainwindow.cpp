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
#include <QDoubleValidator>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "oauthtwitter.h"
#include "qtweetstatusupdate.h"
#include "qtweetstatus.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //m_authorized = false;

    QDoubleValidator *latValidator = new QDoubleValidator(ui->latLineEdit);
    latValidator->setNotation(QDoubleValidator::StandardNotation);
    ui->latLineEdit->setValidator(latValidator);

    QDoubleValidator *longValidator = new QDoubleValidator(ui->longLineEdit);
    longValidator->setNotation(QDoubleValidator::StandardNotation);
    ui->longLineEdit->setValidator(longValidator);

    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));
    connect(m_oauthTwitter, SIGNAL(authorizeXAuthFinished()), SLOT(xauthFinished()));
    connect(m_oauthTwitter, SIGNAL(authorizeXAuthError()), SLOT(xauthError()));

    connect(ui->authPushButton, SIGNAL(clicked()), SLOT(authorizeButtonClicked()));
    connect(ui->udpatePushButton, SIGNAL(clicked()), SLOT(updateButtonClicked()));

    m_authorized = true;
    m_oauthTwitter->setOAuthToken("16290455-CPyk9D9hJoCghpw7zAE73IZ0g0XtbVHU7xbI5RJE2");
    m_oauthTwitter->setOAuthTokenSecret("NMCzZHio4YAB1ZrTsNP35HLHeN4Ze1GI3qT4zvMCctQ");
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
    m_oauthTwitter->authorizeXAuth(ui->userNameLineEdit->text(), ui->passwordLineEdit->text());
}

void MainWindow::xauthFinished()
{
    ui->statusbar->showMessage("XAuth succesfull!");
    m_authorized = true;
}

void MainWindow::xauthError()
{
    ui->statusbar->showMessage("XAuth failed");
    m_authorized = false;
}

void MainWindow::updateButtonClicked()
{
    if (m_authorized) {
        QTweetStatusUpdate *statusUpdate = new QTweetStatusUpdate(m_oauthTwitter, this);
        statusUpdate->post(ui->statusTextEdit->toPlainText(),
                           0,
                           QTweetGeoCoord(ui->latLineEdit->text().toDouble(), ui->longLineEdit->text().toDouble()),
                           QString(),
                           true);
        connect(statusUpdate, SIGNAL(postedStatus(QTweetStatus)), SLOT(postStatusFinished(QTweetStatus)));

    } else {
        ui->statusbar->showMessage("You cannot post, needs autorization!");
    }
}

void MainWindow::postStatusFinished(const QTweetStatus &status)
{
    QTweetStatusUpdate *statusUpdate = qobject_cast<QTweetStatusUpdate*>(sender());

    if (statusUpdate) {
        ui->statusbar->showMessage("Posted status with id " + QString::number(status.id()));

        statusUpdate->deleteLater();
    }
}
