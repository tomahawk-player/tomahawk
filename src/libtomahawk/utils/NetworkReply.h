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

#ifndef NETWORKREPLY_H
#define NETWORKREPLY_H

#include <QObject>
#include <QNetworkReply>
#include <QStringList>

#include "Typedefs.h"

#include "DllMacro.h"

class DLLEXPORT NetworkReply : public QObject
{
Q_OBJECT

public:
    explicit NetworkReply( QNetworkReply* parent = 0 );
    virtual ~NetworkReply();

    void blacklistHostFromRedirection( const QString& host );
    QNetworkReply* reply() const { return m_reply; }

    static const int maxRedirects = 100;
    static const int maxSameRedirects = 5;

signals:
    void redirected();

    void finalUrlReached();
    void finalUrlReached( const QUrl& finalUrlReached );

    void finished();
    void finished( const QUrl& finalUrl );
    void error( QNetworkReply::NetworkError error );

private slots:
    void deletedByParent();
    void metaDataChanged();
    void networkLoadFinished();

private:
    void connectReplySignals();
    void disconnectReplySignals();
    void emitFinished( const QUrl& url );
    void load( const QUrl& url );

    QStringList m_blacklistedHosts;
    QStringList m_formerUrls;
    QNetworkReply* m_reply;
    QUrl m_url;
};

#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    Q_DECLARE_METATYPE( NetworkReply* )
#endif

#endif // NETWORKREPLY_H
