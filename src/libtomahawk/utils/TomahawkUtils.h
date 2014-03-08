/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#pragma once
#ifndef TOMAHAWKUTILS_H
#define TOMAHAWKUTILS_H

#include "DllMacro.h"

#include <QtCore/QThread>
#include <QtNetwork/QNetworkProxy>
#include <QtCore/QStringList>
#include <Typedefs.h>

#define RESPATH ":/data/"

class QDir;
class QNetworkAccessManager;

namespace TomahawkUtils
{
    enum MediaType
    {
        MediaTypeArtist,
        MediaTypeAlbum,
        MediaTypeTrack
    };

    enum ImageType
    {
        DefaultAlbumCover,
        DefaultArtistImage,
        DefaultTrackImage,
        DefaultSourceAvatar,
        DefaultCollection,
        DefaultResolver,
        NowPlayingSpeaker,
        NowPlayingSpeakerDark,
        InfoIcon,
        PlayButton,
        PlayButtonPressed,
        PauseButton,
        PauseButtonPressed,
        PrevButton,
        PrevButtonPressed,
        NextButton,
        NextButtonPressed,
        ShuffleOff,
        ShuffleOffPressed,
        ShuffleOn,
        ShuffleOnPressed,
        RepeatOne,
        RepeatOnePressed,
        RepeatAll,
        RepeatAllPressed,
        RepeatOff,
        RepeatOffPressed,
        VolumeMuted,
        VolumeFull,
        Share,
        NotLoved,
        Loved,
        Configure,
        GreenDot,
        AddContact,
        SubscribeOn,
        SubscribeOff,
        JumpLink,
        ProcessStop,
        HeadphonesOn,
        HeadphonesOff,
        PadlockClosed,
        PadlockOpen,
        Downloading,
        Uploading,
        ViewRefresh,
        SuperCollection,
        LovedPlaylist,
        NewReleases,
        NewAdditions,
        RecentlyPlayed,
        AutomaticPlaylist,
        Charts,
        Station,
        Playlist,
        Search,
        ListRemove,
        ListAdd,
        AdvancedSettings,
        AccountSettings,
        MusicSettings,
        Add,
        DropSong,
        DropAlbum,
        DropAllSongs,
        DropLocalSongs,
        DropTopSongs,
        LastfmIcon,
        SpotifyIcon,
        SoundcloudIcon,
        AccountNone,
        Starred,
        Unstarred,
        StarHovered,
        SipPluginOnline,
        SipPluginOffline,
        ResolverBundle,
        Inbox,
        Invalid,
        InboxNewItem,
        Outbox,
        NetworkActivity
    };

    enum ImageMode
    {
        Original,
        CoverInCase,
        Grid,
        DropShadow,
        RoundedCorners
    };


    DLLEXPORT bool headless();
    DLLEXPORT void setHeadless( bool headless );

    DLLEXPORT QString appFriendlyVersion();

    DLLEXPORT QDir appConfigDir();
    DLLEXPORT QDir appDataDir();
    DLLEXPORT QDir appLogDir();

    DLLEXPORT void installTranslator( QObject* parent );

    DLLEXPORT QString timeToString( int seconds );
    DLLEXPORT QString ageToString( const QDateTime& time, bool appendAgoString = false );
    DLLEXPORT QString filesizeToString( unsigned int size );
    DLLEXPORT QString extensionToMimetype( const QString& extension );
    DLLEXPORT QByteArray percentEncode( const QUrl& url );

    DLLEXPORT void msleep( unsigned int ms );
    DLLEXPORT bool newerVersion( const QString& oldVersion, const QString& newVersion );
    DLLEXPORT int levenshtein( const QString& source, const QString& target );

    DLLEXPORT quint64 infosystemRequestId();

    DLLEXPORT QString md5( const QByteArray& data );
    DLLEXPORT bool removeDirectory( const QString& dir );

    /**
      * Check if this URL refers to a http-Result.
      *
      * Attention: This only checks for a http result, not a httpS result.
      */
    DLLEXPORT bool isHttpResult( const QString& url );
    DLLEXPORT bool isLocalResult( const QString& url );
    DLLEXPORT bool isRtmpResult( const QString& url );

    DLLEXPORT bool verifyFile( const QString& filePath, const QString& signature );
    DLLEXPORT QString extractScriptPayload( const QString& filename, const QString& resolverId, const QString& dirName = "atticaresolvers" );
    DLLEXPORT bool unzipFileInFolder( const QString& zipFileName, const QDir& folder );

    // Extracting may be asynchronous, pass in a receiver object with the following slots:
    //  extractSucceeded( const QString& path ) and extractFailed() to be notified/
    DLLEXPORT void extractBinaryResolver( const QString& zipFilename, QObject* receiver );

    // Used by the above, not exported
    void copyWithAuthentication( const QString& srcFile, const QDir dest, QObject* receiver );

    DLLEXPORT bool whitelistedHttpResultHint( const QUrl& url );

    DLLEXPORT int compareVersionStrings( const QString& first, const QString& second );

    /**
     * This helper is designed to help "update" an existing playlist with a newer revision of itself.
     * To avoid re-loading the whole playlist and re-resolving tracks that are the same in the old playlist,
     * it goes through the new playlist and adds only new tracks.
     *
     * The new list of tracks is returned
     *
     * \return true if some changes were made, false if the new tracks are the same as the current tracks in \param orig
     */
    DLLEXPORT QList< Tomahawk::query_ptr > mergePlaylistChanges( const QList< Tomahawk::query_ptr >& orig, const QList< Tomahawk::query_ptr >& newTracks, bool& changed );

    DLLEXPORT void crash();


    /**
     * Qt4 / Qt5 compatibility layer
     */

    /* QUrl */
    DLLEXPORT void urlAddQueryItem( QUrl& url, const QString& key, const QString& value );
    DLLEXPORT QString urlQueryItemValue( const QUrl& url, const QString& key );
    DLLEXPORT bool urlHasQueryItem( const QUrl& url, const QString& key );
    DLLEXPORT QList<QPair<QString, QString> > urlQueryItems( const QUrl& url );
    DLLEXPORT void urlSetQuery( QUrl& url, const QString& query );

}

#endif // TOMAHAWKUTILS_H
