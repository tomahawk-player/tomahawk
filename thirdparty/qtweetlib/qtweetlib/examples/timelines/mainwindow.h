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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
    class MainWindow;
}

class OAuthTwitter;
class QTimer;
class QTweetStatus;
class QTweetDMStatus;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private slots:
    void authorizeButtonClicked();
    void xauthFinished();
    void xauthError();
    void timerTimeOut();
    void homeTimelineStatuses(const QList<QTweetStatus>& statuses);
    void mentionsStatuses(const QList<QTweetStatus>& statuses);
    void userTimelineStatuses(const QList<QTweetStatus>& statuses);
    void directMessages(const QList<QTweetDMStatus>& directMessages);

private:
    Ui::MainWindow *ui;
    OAuthTwitter *m_oauthTwitter;
    QTimer *m_timer;
    qint64 m_sinceidHomeTimeline;
    qint64 m_sinceidMentions;
    qint64 m_sinceidUserTimeline;
    qint64 m_sinceidDirectMessages;

};

#endif // MAINWINDOW_H
