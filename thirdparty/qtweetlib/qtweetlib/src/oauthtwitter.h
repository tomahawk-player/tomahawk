/* Copyright (c) 2010, Antonie Jovanoski
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#ifndef OAUTHTWITTER_H
#define OAUTHTWITTER_H

#include "oauth.h"

class QNetworkAccessManager;

/**
 *   OAuth Twitter authorization class
 */
class QTWEETLIBSHARED_EXPORT OAuthTwitter : public OAuth
{
	Q_OBJECT
    Q_PROPERTY(QNetworkAccessManager* networkAccessManager
               READ networkAccessManager
               WRITE setNetworkAccessManager)
public:
	OAuthTwitter(QObject *parent = 0);
    OAuthTwitter(QNetworkAccessManager* netManager, QObject *parent = 0);
	void setNetworkAccessManager(QNetworkAccessManager* netManager);
	QNetworkAccessManager* networkAccessManager() const;
    void authorizeXAuth(const QString& username, const QString& password);
    void authorizePin();

signals:
    /** Emited when XAuth authorization is finished */
    void authorizeXAuthFinished();
    /** Emited when there is error in XAuth authorization */
    // ### TODO Error detection
    // Sigh, bad documentation on errors in twitter api
    void authorizeXAuthError();

protected:
    virtual int authorizationWidget();
    virtual void requestAuthorization();

private slots:
    void finishedAuthorization();
    void requestAccessToken(int pin);

private:
	QNetworkAccessManager *m_netManager;
};	

#endif //OAUTHTWITTER_H
