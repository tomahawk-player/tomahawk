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

#include <QObject>

class QNetworkReply;
namespace Echonest {
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
    EchoNestPlugin(QObject *parent);
    virtual ~EchoNestPlugin();

    void getInfo( const QString &caller, const InfoType type, const QVariant &data, InfoCustomData customData );

private:
    void getSongProfile( const QString &caller, const QVariant &data, InfoCustomData &customData, const QString &item = QString() );
    void getArtistBiography ( const QString &caller, const QVariant &data, InfoCustomData &customData );
    void getArtistFamiliarity( const QString &caller, const QVariant &data, InfoCustomData &customData );
    void getArtistHotttnesss( const QString &caller, const QVariant &data, InfoCustomData &customData );
    void getArtistTerms( const QString &caller, const QVariant &data, InfoCustomData &customData );
    void getMiscTopTerms( const QString &caller, const QVariant &data, InfoCustomData &customData );

    bool isValidArtistData( const QString &caller, const QVariant& data, InfoCustomData& customData );
    bool isValidTrackData( const QString &caller, const QVariant& data, InfoCustomData& customData );
    Echonest::Artist artistFromReply( QNetworkReply* );

private slots:
    void getArtistBiographySlot();
    void getArtistFamiliaritySlot();
    void getArtistHotttnesssSlot();
    void getArtistTermsSlot();
    void getMiscTopSlot();

private:
    QHash< QNetworkReply*, InfoCustomData > m_replyMap;
    QHash< QNetworkReply*, QString > m_callerMap;
};

}

}

#endif // ECHONESTPLUGIN_H
