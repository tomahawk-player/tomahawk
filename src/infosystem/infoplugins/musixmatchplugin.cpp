#include "tomahawk/infosystem.h"
#include "tomahawk/tomahawkapp.h"
#include "musixmatchplugin.h"

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

void MusixMatchPlugin::getInfo(const QString &caller, const InfoType type, const QVariant& data, Tomahawk::InfoSystem::InfoCustomDataHash customData)
{
    qDebug() << Q_FUNC_INFO;
    if( !isValidTrackData(caller, data, customData) || !data.canConvert<Tomahawk::InfoSystem::MusixMatchHash>())
        return;
    Tomahawk::InfoSystem::MusixMatchHash hash = data.value<Tomahawk::InfoSystem::MusixMatchHash>();
    QString artist = hash["artistName"];
    QString track = hash["trackName"];
    if( artist.isEmpty() || track.isEmpty() )
    {
        emit info(caller, Tomahawk::InfoSystem::InfoTrackLyrics, data, QVariant(), customData);
        emit finished(caller, Tomahawk::InfoSystem::InfoTrackLyrics);
        return;
    }
    qDebug() << "artist is " << artist << ", track is " << track;
    QString requestString("http://api.musixmatch.com/ws/1.1/track.search?format=xml&page_size=1&f_has_lyrics=1");
    QUrl url(requestString);
    url.addQueryItem("apikey", m_apiKey);
    url.addQueryItem("q_artist", artist);
    url.addQueryItem("q_track", track);
    QNetworkReply* reply = TomahawkUtils::nam()->get(QNetworkRequest(url));
    reply->setProperty("customData", QVariant::fromValue<Tomahawk::InfoSystem::InfoCustomDataHash>(customData));
    reply->setProperty("origData", data);
    reply->setProperty("caller", caller);
    
    connect(reply, SIGNAL(finished()), SLOT(trackSearchSlot()));
}

bool MusixMatchPlugin::isValidTrackData(const QString &caller, const QVariant& data, Tomahawk::InfoSystem::InfoCustomDataHash &customData)
{
    qDebug() << Q_FUNC_INFO;
    if (data.isNull() || !data.isValid() || !data.canConvert<Tomahawk::InfoSystem::MusixMatchHash>())
    {
        emit info(caller, Tomahawk::InfoSystem::InfoTrackLyrics, data, QVariant(), customData);
        emit finished(caller, Tomahawk::InfoSystem::InfoTrackLyrics);
        qDebug() << "MusixMatchPlugin::isValidTrackData: Data null, invalid, or can't convert";
        return false;
    }
    MusixMatchHash hash = data.value<Tomahawk::InfoSystem::MusixMatchHash>();
    if (hash["trackName"].isEmpty() )
    {
        emit info(caller, Tomahawk::InfoSystem::InfoTrackLyrics, data, QVariant(), customData);
        emit finished(caller, Tomahawk::InfoSystem::InfoTrackLyrics);
        qDebug() << "MusixMatchPlugin::isValidTrackData: Track name is empty";
        return false;
    }
    if (hash["artistName"].isEmpty() )
    {
        emit info(caller, Tomahawk::InfoSystem::InfoTrackLyrics, data, QVariant(), customData);
        emit finished(caller, Tomahawk::InfoSystem::InfoTrackLyrics);
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
        emit info(QString(), Tomahawk::InfoSystem::InfoTrackLyrics, QVariant(), QVariant(), Tomahawk::InfoSystem::InfoCustomDataHash());
        return;
    }
    QDomDocument doc;
    doc.setContent(oldReply->readAll());
    qDebug() << doc.toString();
    QDomNodeList domNodeList = doc.elementsByTagName("track_id");
    if (domNodeList.isEmpty())
    {
        emit info(oldReply->property("caller").toString(), Tomahawk::InfoSystem::InfoTrackLyrics, oldReply->property("origData"), QVariant(), oldReply->property("customData").value<Tomahawk::InfoSystem::InfoCustomDataHash>());
        emit finished(oldReply->property("caller").toString(), Tomahawk::InfoSystem::InfoTrackLyrics);
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
        emit info(QString(), Tomahawk::InfoSystem::InfoTrackLyrics, QVariant(), QVariant(), Tomahawk::InfoSystem::InfoCustomDataHash());
        return;
    }
    QDomDocument doc;
    doc.setContent(reply->readAll());
    QDomNodeList domNodeList = doc.elementsByTagName("lyrics_body");
    if (domNodeList.isEmpty())
    {
        emit info(reply->property("caller").toString(), Tomahawk::InfoSystem::InfoTrackLyrics, reply->property("origData"), QVariant(), reply->property("customData").value<Tomahawk::InfoSystem::InfoCustomDataHash>());
        emit finished(reply->property("caller").toString(), Tomahawk::InfoSystem::InfoTrackLyrics);
        return;
    }
    QString lyrics = domNodeList.at(0).toElement().text();
    qDebug() << "Emitting lyrics: " << lyrics;
    emit info(reply->property("caller").toString(), Tomahawk::InfoSystem::InfoTrackLyrics, reply->property("origData"), QVariant(lyrics), reply->property("customData").value<Tomahawk::InfoSystem::InfoCustomDataHash>());
    emit finished(reply->property("caller").toString(), Tomahawk::InfoSystem::InfoTrackLyrics);
}
