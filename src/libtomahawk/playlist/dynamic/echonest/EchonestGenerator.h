/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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
#include "database/DatabaseCommand_CollectionAttributes.h"

#include "DllMacro.h"

namespace Tomahawk
{

class EchonestSteerer;

class CatalogManager : public QObject
{
    Q_OBJECT
public:
    CatalogManager( QObject* parent );

    QHash< QString, QString > catalogs() const;

signals:
   void catalogsUpdated();

private slots:
    void init();
    void doCatalogUpdate();
    void collectionAttributes( const PairList& );

private:
    QHash< QString, QString > m_catalogs;
};

class DLLEXPORT EchonestFactory : public GeneratorFactoryInterface
{
public:
    EchonestFactory();

    virtual GeneratorInterface* create();
    virtual dyncontrol_ptr createControl( const QString& controlType = QString() );
    virtual QStringList typeSelectors() const;
};

class DLLEXPORT EchonestGenerator : public GeneratorInterface
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
    virtual bool onDemandSteerable() const { return false; }
    virtual QWidget* steeringWidget() { return 0; }

    static QStringList styles();
    static QStringList moods();
    static QStringList genres();
    static QStringList userCatalogs();
    static QByteArray catalogId( const QString& collectionId );

    static void setupCatalogs();
signals:
    void paramsGenerated( const Echonest::DynamicPlaylist::PlaylistParams& );

    void stylesSaved();
    void moodsSaved();
    void genresSaved();

private slots:
    void staticFinished();
    void dynamicStarted();
    void dynamicFetched();

    void doGenerate( const Echonest::DynamicPlaylist::PlaylistParams& params );
    void doStartOnDemand( const Echonest::DynamicPlaylist::PlaylistParams& params );

    void loadStyles();
    void loadMoods();
    void loadGenres();

    void stylesReceived();
    void moodsReceived();
    void genresReceived();
    void knownCatalogsChanged();

    void songLookupFinished();
private:
    // get result from signal paramsGenerated
    void getParams() throw( std::runtime_error );

    query_ptr queryFromSong( const Echonest::Song& song );
    Echonest::DynamicPlaylist::ArtistTypeEnum appendRadioType( Echonest::DynamicPlaylist::PlaylistParams& params ) const throw( std::runtime_error );
    bool onlyThisArtistType( Echonest::DynamicPlaylist::ArtistTypeEnum type ) const throw( std::runtime_error );

    void loadStylesMoodsAndGenres();

    Echonest::DynamicPlaylist* m_dynPlaylist;
    QPixmap m_logo;

    static QStringList s_styles;
    static QStringList s_moods;
    static QStringList s_genres;
    static QNetworkReply* s_stylesJob;
    static QNetworkReply* s_moodsJob;
    static QNetworkReply* s_genresJob;

    static CatalogManager* s_catalogs;

    // used for the intermediary song id lookup
    QSet< QNetworkReply* > m_waiting;
    Echonest::DynamicPlaylist::PlaylistParams m_storedParams;
};

};

#endif

