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

#include <QDebug>
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
    explicit XSPFLoader( bool autoCreate = true, QObject* parent = 0 )
        : QObject( parent )
        , m_autoCreate( autoCreate )
        , m_NS("http://xspf.org/ns/0/")
    {}

    virtual ~XSPFLoader()
    {
        qDebug() << Q_FUNC_INFO;
    }

    QList< Tomahawk::plentry_ptr > entries() const { return m_entries; }

    void setOverrideTitle( const QString& newTitle );
signals:
    void failed();
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

    bool m_autoCreate;
    QString m_NS,m_overrideTitle;
    QList< Tomahawk::plentry_ptr > m_entries;
    QString m_title, m_info, m_creator;

    QByteArray m_body;
    Tomahawk::playlist_ptr m_playlist;
};

#endif // XSPFLOADER_H
