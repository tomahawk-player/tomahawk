/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef ECHONEST_GENERATOR_H
#define ECHONEST_GENERATOR_H

#include <stdexcept>
#include <echonest/Playlist.h>

#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/dynamic/GeneratorFactory.h"
#include "playlist/dynamic/DynamicControl.h"

namespace Tomahawk 
{

class EchonestFactory : public GeneratorFactoryInterface
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
    
private slots:
    void staticFinished();
    void dynamicStarted();
    void dynamicFetched();
    
private:
    Echonest::DynamicPlaylist::PlaylistParams getParams() const throw( std::runtime_error );
    query_ptr queryFromSong( const Echonest::Song& song );
    void appendRadioType( Echonest::DynamicPlaylist::PlaylistParams& params ) const throw( std::runtime_error );
    bool onlyThisArtistType( Echonest::DynamicPlaylist::ArtistTypeEnum type ) const throw( std::runtime_error );
    
    Echonest::DynamicPlaylist* m_dynPlaylist;
    QPixmap m_logo;
};

};

#endif
