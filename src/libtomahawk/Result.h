/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef RESULT_H
#define RESULT_H

#include "utils/TomahawkUtils.h"
// #include "Track.h"
#include "Typedefs.h" // RID
#include "query_wptr.h"
#include "collection_ptr.h"
#include "track_ptr.h"
#include "result_ptr.h"
//
// #include <QObject>
// #include <QPixmap>
#include <QPointer>
#include <QSize>
// #include <QVariant>
// #include <QMutex>

#include "DllMacro.h"

class MetadataEditor;
class DatabaseCommand_Resolve;
class DatabaseCommand_AllTracks;
class DatabaseCommand_AddFiles;
class DatabaseCommand_LoadFile;

namespace Tomahawk
{

class Resolver;

class DLLEXPORT Result : public QObject
{
Q_OBJECT

friend class ::MetadataEditor;
friend class ::DatabaseCommand_Resolve;
friend class ::DatabaseCommand_AllTracks;
friend class ::DatabaseCommand_AddFiles;
friend class ::DatabaseCommand_LoadFile;

public:
    static Tomahawk::result_ptr get( const QString& url );
    static bool isCached( const QString& url );
    virtual ~Result();

    bool isValid() const;

    QVariant toVariant() const;
    QString toString() const;
    Tomahawk::query_ptr toQuery();

    Tomahawk::Resolver* resolvedBy() const;
    void setResolvedBy( Tomahawk::Resolver* resolver );

    float score() const;
    RID id() const;
    bool isOnline() const;
    bool playable() const;

    collection_ptr collection() const;
    QString url() const { return m_url; }
    /**
     * Has the given url been checked that it is accessible/valid.
     *
     * Results marked as true will bypass the ResultUrlChecker.
     */
    bool checked() const { return m_checked; }
    QString mimetype() const { return m_mimetype; }
    QString friendlySource() const;
    QString purchaseUrl() const { return m_purchaseUrl; }
    QString linkUrl() const { return m_linkUrl; }

    QPixmap sourceIcon( TomahawkUtils::ImageMode style, const QSize& desiredSize = QSize() ) const;

    unsigned int bitrate() const { return m_bitrate; }
    unsigned int size() const { return m_size; }
    unsigned int modificationTime() const { return m_modtime; }

    void setScore( float score ) { m_score = score; }
    void setFileId( unsigned int id ) { m_fileId = id; }
    void setRID( RID id ) { m_rid = id; }
    void setCollection( const Tomahawk::collection_ptr& collection );
    void setFriendlySource( const QString& s ) { m_friendlySource = s; }
    void setPurchaseUrl( const QString& u ) { m_purchaseUrl = u; }
    void setLinkUrl( const QString& u ) { m_linkUrl = u; }
    void setChecked( bool checked ) { m_checked = checked; }
    void setMimetype( const QString& mimetype ) { m_mimetype = mimetype; }
    void setBitrate( unsigned int bitrate ) { m_bitrate = bitrate; }
    void setSize( unsigned int size ) { m_size = size; }
    void setModificationTime( unsigned int modtime ) { m_modtime = modtime; }

    void setTrack( const track_ptr& track ) { m_track = track; }

    unsigned int fileId() const { return m_fileId; }

    track_ptr track() const;

public slots:
    void deleteLater();

signals:
    // emitted when the collection this result comes from is going offline/online:
    void statusChanged();
    void updated();

private slots:
    void onOffline();
    void onOnline();

    void onResolverRemoved( Tomahawk::Resolver* resolver );
    void doneEditing();

private:
    // private constructor
    explicit Result( const QString& url );
    explicit Result();

    mutable RID m_rid;
    collection_ptr m_collection;
    QPointer< Tomahawk::Resolver > m_resolvedBy;

    QString m_url;
    QString m_purchaseUrl;
    QString m_linkUrl;
    QString m_mimetype;
    QString m_friendlySource;

    bool m_checked;
    unsigned int m_bitrate;
    unsigned int m_size;
    unsigned int m_modtime;
    float m_score;

    unsigned int m_fileId;

    track_ptr m_track;
    query_wptr m_query;
};

} //ns

#endif // RESULT_H
