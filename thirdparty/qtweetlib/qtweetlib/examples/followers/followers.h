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

#ifndef FOLLOWERS_H
#define FOLLOWERS_H

#include <QWidget>

class OAuthTwitter;
class FollowersListModel;

namespace Ui {
    class Followers;
}

class Followers : public QWidget
{
    Q_OBJECT

public:
    explicit Followers(QWidget *parent = 0);
    ~Followers();

protected:
    void changeEvent(QEvent *e);

private slots:
    void onFetchFollowersPushButtonClicked();

private:
    Ui::Followers *ui;
    OAuthTwitter *m_oauthTwitter;
    FollowersListModel *m_followersListModel;
};

#endif // FOLLOWERS_H
