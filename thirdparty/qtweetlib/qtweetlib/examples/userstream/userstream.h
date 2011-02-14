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

#ifndef USERSTREAM_H
#define USERSTREAM_H

#include <QWidget>
#include <QTextStream>
#include <QFile>

namespace Ui {
    class UserStream;
}

class OAuthTwitter;
class QTweetUserStream;
class QTweetStatus;

class UserStream : public QWidget
{
    Q_OBJECT

public:
    explicit UserStream(QWidget *parent = 0);
    ~UserStream();

protected:
    void changeEvent(QEvent *e);

private slots:
    void onConnectButtonClicked();
    void onAuthorizeFinished();
    void onAuthorizeError();
    void stream(const QByteArray& stream);
    void statusStream(const QTweetStatus& tweet);

private:
    Ui::UserStream *ui;
    OAuthTwitter *m_oauthTwitter;
    QTweetUserStream *m_userStream;
    QTextStream m_streamlogger;
    QFile m_file;
};

#endif // USERSTREAM_H
