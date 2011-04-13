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

#include "musixmatchplugin.h"

#include "utils/tomahawkutils.h"

#include <QNetworkReply>
#include <QDomDocument>

using namespace Tomahawk::InfoSystem;

// for internal neatness

MusixMatchPlugin::MusixMatchPlugin(QObject *parent)
    : InfoPlugin(parent)
    , m_apiKey("61be4ea5aea7dd942d52b2f1311dd9fe")
{
    qDebug() << Q_FUNC_INFO;
    QSet< InfoType > supportedTypes;
    supportedTypes << Tomahawk::InfoSystem::InfoTrackLyrics;
    qobject_cast< InfoSystem* >(parent)->registerInfoTypes(this, supportedTypes);
}

MusixMatchPlugin::~MusixMatchPlugin()
{
    qDebug() << Q_FUNC_INFO;
}

void MusixMatchPlugin::getInfo(const QString &caller, const InfoType type, const QVariant& data, Tomahawk::InfoSystem::InfoCustomData customData)
{
    qDebug() << Q_FUNC_INFO;
    if( !isValidTrackData(caller, data, customData) || !data.canConvert<Tomahawk::InfoSystem::InfoCustomData>())
        return;
    Tomahawk::InfoSystem::InfoCustomData hash = data.value<Tomahawk::InfoSystem::InfoCustomData>();
    QString artist = hash["artistName"].toString();
    QString track = hash["trackName"].toString();
    if( artist.isEmpty() || track.isEmpty() )
    {
        emit info(caller, Tomahawk::InfoSystem::InfoTrackLyrics, data, QVariant(), customData);
        return;
    }
    qDebug() << "artist is " << artist << ", track is " << track;
    QString requestString("http://api.musixmatch.com/ws/1.1/track.search?format=xml&page_size=1&f_has_lyrics=1");
    QUrl url(requestString);
    url.addQueryItem("apikey", m_apiKey);
    url.addQueryItem("q_artist", artist);
    url.addQueryItem("q_track", track);
    QNetworkReply* reply = TomahawkUtils::nam()->get(QNetworkRequest(url));
    reply->setProperty("customData", QVariant::fromValue<Tomahawk::InfoSystem::InfoCustomData>(customData));
    reply->setProperty("origData", data);
    reply->setProperty("caller", caller);

    connect(reply, SIGNAL(finished()), SLOT(trackSearchSlot()));
}

bool MusixMatchPlugin::isValidTrackData(const QString &caller, const QVariant& data, Tomahawk::InfoSystem::InfoCustomData &customData)
{
    qDebug() << Q_FUNC_INFO;
    if (data.isNull() || !data.isValid() || !data.canConvert<Tomahawk::InfoSystem::InfoCustomData>())
    {
        emit info(caller, Tomahawk::InfoSystem::InfoTrackLyrics, data, QVariant(), customData);
        qDebug() << "MusixMatchPlugin::isValidTrackData: Data null, invalid, or can't convert";
        return false;
    }
    InfoCustomData hash = data.value<Tomahawk::InfoSystem::InfoCustomData>();
    if (hash["trackName"].toString().isEmpty() )
    {
        emit info(caller, Tomahawk::InfoSystem::InfoTrackLyrics, data, QVariant(), customData);
        qDebug() << "MusixMatchPlugin::isValidTrackData: Track name is empty";
        return false;
    }
    if (hash["artistName"].toString().isEmpty() )
    {
        emit info(caller, Tomahawk::InfoSystem::InfoTrackLyrics, data, QVariant(), customData);
        qDebug() << "MusixMatchPlugin::isValidTrackData: No artist name found";
        return false;
    }
    return true;
}

void MusixMatchPlugin::trackSearchSlot()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* oldReply = qobject_cast<QNetworkReply*>( sender() );
    if (!oldReply)
    {
        emit info(QString(), Tomahawk::InfoSystem::InfoTrackLyrics, QVariant(), QVariant(), Tomahawk::InfoSystem::InfoCustomData());
        return;
    }
    QDomDocument doc;
    doc.setContent(oldReply->readAll());
    qDebug() << doc.toString();
    QDomNodeList domNodeList = doc.elementsByTagName("track_id");
    if (domNodeList.isEmpty())
    {
        emit info(oldReply->property("caller").toString(), Tomahawk::InfoSystem::InfoTrackLyrics, oldReply->property("origData"), QVariant(), oldReply->property("customData").value<Tomahawk::InfoSystem::InfoCustomData>());
        return;
    }
    QString track_id = domNodeList.at(0).toElement().text();
    QString requestString("http://api.musixmatch.com/ws/1.1/track.lyrics.get?track_id=%1&format=xml&apikey=%2");
    QUrl url(requestString);
    url.addQueryItem("apikey", m_apiKey);
    url.addQueryItem("track_id", track_id);
    QNetworkReply* newReply = TomahawkUtils::nam()->get(QNetworkRequest(url));
    newReply->setProperty("origData", oldReply->property("origData"));
    newReply->setProperty("customData", oldReply->property("customData"));
    newReply->setProperty("caller", oldReply->property("caller"));
    connect(newReply, SIGNAL(finished()), SLOT(trackLyricsSlot()));
}

void MusixMatchPlugin::trackLyricsSlot()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    if (!reply)
    {
        emit info(QString(), Tomahawk::InfoSystem::InfoTrackLyrics, QVariant(), QVariant(), Tomahawk::InfoSystem::InfoCustomData());
        return;
    }
    QDomDocument doc;
    doc.setContent(reply->readAll());
    QDomNodeList domNodeList = doc.elementsByTagName("lyrics_body");
    if (domNodeList.isEmpty())
    {
        emit info(reply->property("caller").toString(), Tomahawk::InfoSystem::InfoTrackLyrics, reply->property("origData"), QVariant(), reply->property("customData").value<Tomahawk::InfoSystem::InfoCustomData>());
        return;
    }
    QString lyrics = domNodeList.at(0).toElement().text();
    qDebug() << "Emitting lyrics: " << lyrics;
    emit info(reply->property("caller").toString(), Tomahawk::InfoSystem::InfoTrackLyrics, reply->property("origData"), QVariant(lyrics), reply->property("customData").value<Tomahawk::InfoSystem::InfoCustomData>());
}
