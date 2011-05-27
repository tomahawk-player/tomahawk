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

#ifndef ECHONEST_GENERATOR_H
#define ECHONEST_GENERATOR_H

#include <stdexcept>
#include <echonest/Playlist.h>

#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/dynamic/GeneratorFactory.h"
#include "playlist/dynamic/DynamicControl.h"

#include "dllmacro.h"

namespace Tomahawk
{

class EchonestSteerer;

class DLLEXPORT EchonestFactory : public GeneratorFactoryInterface
{
public:
    EchonestFactory();

    virtual GeneratorInterface* create();
    virtual dyncontrol_ptr createControl( const QString& controlType = QString() );
    virtual QStringList typeSelectors() const;
};

class EchonestGenerator : public GeneratorInterface
{
    Q_OBJECT
public:
    explicit EchonestGenerator( QObject* parent = 0 );
    virtual ~EchonestGenerator();

    virtual dyncontrol_ptr createControl( const QString& type = QString() );
    virtual QPixmap logo();
    virtual void generate ( int number = -1 );
    virtual void startOnDemand();
    virtual void fetchNext( int rating = -1 );
    virtual QString sentenceSummary();
    virtual bool onDemandSteerable() const { return true; }
    virtual QWidget* steeringWidget();

    static QVector< QString > styles();
    static QVector< QString > moods();

signals:
    void paramsGenerated( const Echonest::DynamicPlaylist::PlaylistParams& );

private slots:
    void staticFinished();
    void dynamicStarted();
    void dynamicFetched();

    // steering controls
    void steerField( const QString& field );
    void steerDescription( const QString& desc );
    void resetSteering();

    void doGenerate( const Echonest::DynamicPlaylist::PlaylistParams& params );
    void doStartOnDemand( const Echonest::DynamicPlaylist::PlaylistParams& params );

    void stylesReceived();
    void moodsReceived();

    void songLookupFinished();
private:
    // get result from signal paramsGenerated
    void getParams() throw( std::runtime_error );

    query_ptr queryFromSong( const Echonest::Song& song );
    Echonest::DynamicPlaylist::ArtistTypeEnum appendRadioType( Echonest::DynamicPlaylist::PlaylistParams& params ) const throw( std::runtime_error );
    bool onlyThisArtistType( Echonest::DynamicPlaylist::ArtistTypeEnum type ) const throw( std::runtime_error );

    Echonest::DynamicPlaylist* m_dynPlaylist;
    QPixmap m_logo;

    static QVector< QString > s_styles;
    static QVector< QString > s_moods;

    // used for the intermediary song id lookup
    QSet< QNetworkReply* > m_waiting;
    Echonest::DynamicPlaylist::PlaylistParams m_storedParams;

    QWeakPointer<EchonestSteerer> m_steerer;
    bool m_steeredSinceLastTrack;
    Echonest::DynamicPlaylist::DynamicControl m_steerData;
};

};

#endif
