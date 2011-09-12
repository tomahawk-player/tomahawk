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

#ifndef ECHONESTPLUGIN_H
#define ECHONESTPLUGIN_H

#include "infosystem/infosystem.h"
#include "infosystem/infosystemworker.h"

#include <QObject>

class QNetworkReply;

namespace Echonest
{
    class Artist;
}

namespace Tomahawk
{

namespace InfoSystem
{

class EchoNestPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    EchoNestPlugin();
    virtual ~EchoNestPlugin();

protected slots:
    virtual void getInfo( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData );

    virtual void pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant data )
    {
        Q_UNUSED( caller );
        Q_UNUSED( type );
        Q_UNUSED( data );
    }

    virtual void notInCacheSlot( uint requestId, Tomahawk::InfoSystem::InfoCriteriaHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
    {
        Q_UNUSED( requestId );
        Q_UNUSED( criteria );
        Q_UNUSED( requestData );
    }

public slots:
    void namChangedSlot( QNetworkAccessManager *nam );

private:
    void getSongProfile( uint requestId, const Tomahawk::InfoSystem::InfoRequestData &requestData, const QString &item = QString() );
    void getArtistBiography( uint requestId, const Tomahawk::InfoSystem::InfoRequestData &requestData );
    void getArtistFamiliarity( uint requestId, const Tomahawk::InfoSystem::InfoRequestData &requestData );
    void getArtistHotttnesss( uint requestId, const Tomahawk::InfoSystem::InfoRequestData &requestData );
    void getArtistTerms( uint requestId, const Tomahawk::InfoSystem::InfoRequestData &requestData );
    void getMiscTopTerms( uint requestId, const Tomahawk::InfoSystem::InfoRequestData &requestData );

    bool isValidArtistData( uint requestId, const Tomahawk::InfoSystem::InfoRequestData &requestData );
    bool isValidTrackData( uint requestId, const Tomahawk::InfoSystem::InfoRequestData &requestData );
    Echonest::Artist artistFromReply( QNetworkReply* );

private slots:
    void getArtistBiographySlot();
    void getArtistFamiliaritySlot();
    void getArtistHotttnesssSlot();
    void getArtistTermsSlot();
    void getMiscTopSlot();
};

}

}

#endif // ECHONESTPLUGIN_H
