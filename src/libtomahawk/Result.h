/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2015,      Dominik Schmidt <domme@tomahawk-player.org>
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

#include "DownloadJob.h"
#include "utils/TomahawkUtils.h"
#include "Typedefs.h"

#include "DllMacro.h"

#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QVariant>
#include <QMutex>

class MetadataEditor;

namespace Tomahawk
{

class Resolver;

class DLLEXPORT Result : public QObject
{
Q_OBJECT

friend class ::MetadataEditor;

public:
    /**
     * Get a Result instance for an URL if it is cached, otherwise create a new
     * instance using the supplied Track object.
     */
    static Tomahawk::result_ptr get( const QString& url,
                                     const Tomahawk::track_ptr& track );

    /**
     * Get a Result instance for an URL if it is already cached.
     *
     * This will not create a new Result instance if there is no matching
     * Result in the cache, use Result::get for this.
     *
     * @param url Unique result identifier
     * @return nullptr if the Result is not yet cached
     */
    static Tomahawk::result_ptr getCached( const QString& url );

    virtual ~Result();

    QWeakPointer< Tomahawk::Result > weakRef();
    void setWeakRef( QWeakPointer< Tomahawk::Result > weakRef );

    QVariant toVariant() const;
    QString toString() const;
    Tomahawk::query_ptr toQuery();

    /**
     * Associate the used collection for this result.
     *
     * @param emitOnlineEvents disableing this will not emit statusChanged anymore thus the query will not update (use with care!, only when this is the sole result)
     */
    void setResolvedByCollection( const Tomahawk::collection_ptr& collection, bool emitOnlineEvents = true );
    collection_ptr resolvedByCollection() const;

    QPointer< Tomahawk::Resolver > resolvedByResolver() const;
    void setResolvedByResolver( Tomahawk::Resolver* resolver );

    /**
     *  TODO: Make this a smart pointer
     */
    Resolver* resolvedBy() const;

    RID id() const;
    bool isOnline() const;
    bool playable() const;

    /**
     * @brief whether this result isLocal, i.e. resolved by a local collection
     * @return isLocal
     */
    bool isLocal() const;

    QString url() const;
    /**
     * Has the given url been checked that it is accessible/valid.
     *
     * Results marked as true will bypass the ResultUrlChecker.
     */
    bool checked() const;
    QString mimetype() const;
    QString friendlySource() const;
    bool isPreview() const;
    QString purchaseUrl() const;
    QString linkUrl() const;

    QPixmap sourceIcon( TomahawkUtils::ImageMode style, const QSize& desiredSize = QSize() ) const;

    unsigned int bitrate() const;
    unsigned int size() const;
    unsigned int modificationTime() const;

    void setFileId( unsigned int id );
    void setRID( RID id ) { m_rid = id; }

    void setFriendlySource( const QString& s );
    void setPreview( bool isPreview );
    void setPurchaseUrl( const QString& u );
    void setLinkUrl( const QString& u );
    void setChecked( bool checked );
    void setMimetype( const QString& mimetype );
    void setBitrate( unsigned int bitrate );
    void setSize( unsigned int size );
    void setModificationTime( unsigned int modtime );

    void setTrack( const track_ptr& track );

    unsigned int fileId() const;

    track_ptr track() const;

    QList< DownloadFormat > downloadFormats() const;
    void setDownloadFormats( const QList<DownloadFormat>& formats );

    downloadjob_ptr downloadJob() const { return m_downloadJob; }
    downloadjob_ptr toDownloadJob( const DownloadFormat& format );

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

    void onDownloadJobStateChanged( DownloadJob::TrackState newState, DownloadJob::TrackState oldState );

    void onSettingsChanged();

private:
    // private constructor
    explicit Result( const QString& url, const Tomahawk::track_ptr& track );
    explicit Result();

    mutable QMutex m_mutex;

    mutable RID m_rid;
    collection_wptr m_collection;
    QPointer< Tomahawk::Resolver > m_resolver;

    QString m_url;
    bool m_isPreview;
    QString m_purchaseUrl;
    QString m_linkUrl;
    QString m_mimetype;
    QString m_friendlySource;

    QList<DownloadFormat> m_formats;
    downloadjob_ptr m_downloadJob;

    bool m_checked;
    unsigned int m_bitrate;
    unsigned int m_size;
    unsigned int m_modtime;
    unsigned int m_fileId;

    track_ptr m_track;
    query_wptr m_query;
    QWeakPointer< Tomahawk::Result > m_ownRef;
};

} //ns

Q_DECLARE_METATYPE( Tomahawk::result_ptr )

#endif // RESULT_H
