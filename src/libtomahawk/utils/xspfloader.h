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

/*
    Fetches and parses an XSPF document from a QFile or QUrl.
 */

#ifndef XSPFLOADER_H
#define XSPFLOADER_H

#include <QObject>
#include <QUrl>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "playlist.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT XSPFLoader : public QObject
{
Q_OBJECT

public:
    enum XSPFErrorCode { ParseError, InvalidTrackError, FetchError };
    explicit XSPFLoader( bool autoCreate = true, bool autoUpdate = false, QObject* parent = 0 );

    virtual ~XSPFLoader();
    QList< Tomahawk::query_ptr > entries() const;

    void setOverrideTitle( const QString& newTitle );
    void setAutoResolveTracks( bool autoResolve ) { m_autoResolve = autoResolve; }

signals:
    void error( XSPFLoader::XSPFErrorCode error );
    void ok( const Tomahawk::playlist_ptr& );

public slots:
    void load( const QUrl& url );
    void load( QFile& file );

private slots:
    void networkLoadFinished();
    void networkError( QNetworkReply::NetworkError e );

private:
    void reportError();
    void gotBody();

    bool m_autoCreate, m_autoUpdate, m_autoResolve;
    QString m_NS,m_overrideTitle;
    QList< Tomahawk::query_ptr > m_entries;
    QString m_title, m_info, m_creator;

    QUrl m_url;
    QByteArray m_body;
    Tomahawk::playlist_ptr m_playlist;
};

#endif // XSPFLOADER_H
