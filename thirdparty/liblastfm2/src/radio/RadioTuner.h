/*
   Copyright 2009 Last.fm Ltd. 
      - Primarily authored by Max Howell, Jono Cole and Doug Mansell

   This file is part of liblastfm.

   liblastfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   liblastfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with liblastfm.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef LASTFM_TUNER_H
#define LASTFM_TUNER_H

#include <lastfm/RadioStation>
#include <lastfm/Track>
#include <lastfm/Xspf>
#include <lastfm/ws.h>
#include <QList>

namespace lastfm
{
    /** With regard to error handling. We handle Ws::TryAgain up to 5 times,
      * don't try again after that! Just tell the user to try again later. 
      */
    class LASTFM_DLLEXPORT RadioTuner : public QObject
    {
        Q_OBJECT
    
    public:
        /** You need to have assigned Ws::* for this to work, creating the tuner
          * automatically fetches the first 5 tracks for the station */
        explicit RadioTuner( const RadioStation& );

        Track takeNextTrack();

        void retune( const RadioStation& );

    signals:
        void title( const QString& );
        void supportsDisco( bool supportsDisco );
        void trackAvailable();
        void error( lastfm::ws::Error, const QString& message );

    private slots:
        void onTuneReturn();
        void onGetPlaylistReturn();
        void onXspfExpired();

        void onTwoSecondTimeout();

    private:
        /** Tries again up to 5 times 
          * @returns true if we tried again, otherwise you should emit error */
        bool tryAgain();
        /** Will emit 5 tracks from tracks(), they have to played within an hour
          * or the streamer will refuse to stream them. Also the previous five are
          * invalidated apart from the one that is currently playing, so sorry, you
          * can't build up big lists of tracks.
          *
          * I feel I must point out that asking the user which one they want to play
          * is also not allowed according to our terms and conditions, which you
          * already agreed to in order to get your API key. Sorry about that dude. 
          */
        void fetchFiveMoreTracks();

    private:
        QList<Xspf*> m_playlistQueue;
        uint m_retry_counter;
        bool m_fetchingPlaylist;
        bool m_requestedPlaylist;
        class QTimer* m_twoSecondTimer;
        RadioStation m_retuneStation;
    };
}

#endif
