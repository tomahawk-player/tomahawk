/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "JSResolverHelper.h"

#include "resolvers/ScriptEngine.h"
#include "network/Servent.h"
#include "utils/Closure.h"
#include "utils/Cloudstream.h"
#include "utils/Json.h"
#include "utils/NetworkAccessManager.h"
#include "utils/NetworkReply.h"
#include "utils/Logger.h"

#include "config.h"
#include "JSResolver_p.h"
#include "Pipeline.h"
#include "Result.h"
#include "SourceList.h"
#include "UrlHandler.h"
#include "JSAccount.h"
#include "../Album.h"
#include "../Artist.h"
#include "../Result.h"
#include "../Track.h"

#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QWebFrame>
#include <QLocale>
#include <QNetworkReply>

#include <taglib/asffile.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2framefactory.h>
#include <taglib/mp4file.h>
#include <taglib/mpegfile.h>
#include <taglib/oggfile.h>
#include <taglib/vorbisfile.h>

#if defined(TAGLIB_MAJOR_VERSION) && defined(TAGLIB_MINOR_VERSION)
#if TAGLIB_MAJOR_VERSION >= 1 && TAGLIB_MINOR_VERSION >= 9
    #include <taglib/opusfile.h>
#endif
#endif

#ifdef Q_OS_WIN
    // GetUserGeoID for currentCountry
    #include <winnls.h>
#endif

using namespace Tomahawk;

JSResolverHelper::JSResolverHelper( const QString& scriptPath, JSResolver* parent )
    : QObject( parent )
    , m_resolver( parent )
    , m_scriptPath( scriptPath )
    , m_stopped( false )
{
}


void
JSResolverHelper::start()
{
    m_stopped = false;
}


void
JSResolverHelper::stop()
{
    m_stopped = true;
}


QByteArray
JSResolverHelper::readRaw( const QString& fileName )
{
    QString path = QFileInfo( m_scriptPath ).absolutePath();
    // remove directories
    QString cleanedFileName = QFileInfo( fileName ).fileName();
    QString absoluteFilePath = path.append( "/" ).append( cleanedFileName );

    QFile file( absoluteFilePath );
    if ( !file.exists() )
    {
        Q_ASSERT(false);
        return QByteArray();
    }

    file.open( QIODevice::ReadOnly );
    return file.readAll();
}


QString
JSResolverHelper::compress( const QString& data )
{
    QByteArray comp = qCompress( data.toLatin1(), 9 );
    return comp.toBase64();
}


QString
JSResolverHelper::readCompressed( const QString& fileName )
{
    return compress( readRaw( fileName ) );
}


QString
JSResolverHelper::readBase64( const QString& fileName )
{
    return readRaw( fileName ).toBase64();
}


QVariantMap
JSResolverHelper::resolverData()
{
    QVariantMap resolver;
    resolver["config"] = m_resolverConfig;
    resolver["scriptPath"] = m_scriptPath;
    return resolver;
}


void
JSResolverHelper::log( const QString& message )
{
    tLog() << "JAVASCRIPT:" << m_scriptPath << ":" << message;
}


QString
JSResolverHelper::uuid() const
{
    return ::uuid();
}


int
JSResolverHelper::currentCountry() const
{
#if defined Q_OS_WIN
    // c.f. https://msdn.microsoft.com/en-us/library/windows/desktop/dd374073(v=vs.85).aspx
    static QHash< GEOID, QLocale::Country > geoIdCountryMapping = {
        { 2, QLocale::AntiguaAndBarbuda },
         { 3, QLocale::Afghanistan },
         { 4, QLocale::Algeria },
         { 5, QLocale::Azerbaijan },
         { 6, QLocale::Albania },
         { 7, QLocale::Armenia },
         { 8, QLocale::Andorra },
         { 9, QLocale::Angola },
         { 10, QLocale::AmericanSamoa },
         { 11, QLocale::Argentina },
         { 12, QLocale::Australia },
         { 14, QLocale::Austria },
         { 17, QLocale::Bahrain },
         { 18, QLocale::Barbados },
         { 19, QLocale::Botswana },
         { 20, QLocale::Bermuda },
         { 21, QLocale::Belgium },
         { 22, QLocale::Bahamas },
         { 23, QLocale::Bangladesh },
         { 24, QLocale::Belize },
         { 25, QLocale::BosniaAndHerzegowina },
         { 26, QLocale::Bolivia },
         { 27, QLocale::Myanmar },
         { 28, QLocale::Benin },
         { 29, QLocale::Belarus },
         { 30, QLocale::SolomonIslands },
         { 32, QLocale::Brazil },
         { 34, QLocale::Bhutan },
         { 35, QLocale::Bulgaria },
         { 37, QLocale::Brunei },
         { 38, QLocale::Burundi },
         { 39, QLocale::Canada },
         { 40, QLocale::Cambodia },
         { 41, QLocale::Chad },
         { 42, QLocale::SriLanka },
         { 43, QLocale::PeoplesRepublicOfCongo }, // Congo
         { 44, QLocale::DemocraticRepublicOfCongo }, // Congo (DRC)
         { 45, QLocale::China },
         { 46, QLocale::Chile },
         { 49, QLocale::Cameroon },
         { 50, QLocale::Comoros },
         { 51, QLocale::Colombia },
         { 54, QLocale::CostaRica },
         { 55, QLocale::CentralAfricanRepublic },
         { 56, QLocale::Cuba },
         { 57, QLocale::CapeVerde },
         { 59, QLocale::Cyprus },
         { 61, QLocale::Denmark },
         { 62, QLocale::Djibouti },
         { 63, QLocale::Dominica },
         { 65, QLocale::DominicanRepublic },
         { 66, QLocale::Ecuador },
         { 67, QLocale::Egypt },
         { 68, QLocale::Ireland },
         { 69, QLocale::EquatorialGuinea },
         { 70, QLocale::Estonia },
         { 71, QLocale::Eritrea },
         { 72, QLocale::ElSalvador },
         { 73, QLocale::Ethiopia },
         { 75, QLocale::CzechRepublic },
         { 77, QLocale::Finland },
         { 78, QLocale::Fiji },
         { 80, QLocale::Micronesia },
         { 81, QLocale::FaroeIslands },
         { 84, QLocale::France },
         { 86, QLocale::Gambia },
         { 87, QLocale::Gabon },
         { 88, QLocale::Georgia },
         { 89, QLocale::Ghana },
         { 90, QLocale::Gibraltar },
         { 91, QLocale::Grenada },
         { 93, QLocale::Greenland },
         { 94, QLocale::Germany },
         { 98, QLocale::Greece },
         { 99, QLocale::Guatemala },
         { 100, QLocale::Guinea },
         { 101, QLocale::Guyana },
         { 103, QLocale::Haiti },
         { 104, QLocale::HongKong },
         { 106, QLocale::Honduras },
         { 108, QLocale::Croatia },
         { 109, QLocale::Hungary },
         { 110, QLocale::Iceland },
         { 111, QLocale::Indonesia },
         { 113, QLocale::India },
         { 114, QLocale::BritishIndianOceanTerritory },
         { 116, QLocale::Iran },
         { 117, QLocale::Israel },
         { 118, QLocale::Italy },
         { 119, QLocale::IvoryCoast },
         { 121, QLocale::Iraq },
         { 122, QLocale::Japan },
         { 124, QLocale::Jamaica },
         { 125, QLocale::SvalbardAndJanMayenIslands	 }, // Jan Mayen
         { 126, QLocale::Jordan },
         { 127, QLocale::AnyCountry }, // Johnston Atoll
         { 129, QLocale::Kenya },
         { 130, QLocale::Kyrgyzstan },
         { 131, QLocale::DemocraticRepublicOfKorea }, // North Korea
         { 133, QLocale::Kiribati },
         { 134, QLocale::RepublicOfKorea }, // Korea
         { 136, QLocale::Kuwait },
         { 137, QLocale::Kazakhstan },
         { 138, QLocale::Laos },
         { 139, QLocale::Lebanon },
         { 140, QLocale::Latvia },
         { 141, QLocale::Lithuania },
         { 142, QLocale::Liberia },
         { 143, QLocale::Slovakia },
         { 145, QLocale::Liechtenstein },
         { 146, QLocale::Lesotho },
         { 147, QLocale::Luxembourg },
         { 148, QLocale::Libya },
         { 149, QLocale::Madagascar },
         { 151, QLocale::Macau },
         { 152, QLocale::Moldova },
         { 154, QLocale::Mongolia },
         { 156, QLocale::Malawi },
         { 157, QLocale::Mali },
         { 158, QLocale::Monaco },
         { 159, QLocale::Morocco },
         { 160, QLocale::Mauritius },
         { 162, QLocale::Mauritania },
         { 163, QLocale::Malta },
         { 164, QLocale::Oman },
         { 165, QLocale::Maldives },
         { 166, QLocale::Mexico },
         { 167, QLocale::Malaysia },
         { 168, QLocale::Mozambique },
         { 173, QLocale::Niger },
         { 174, QLocale::Vanuatu },
         { 175, QLocale::Nigeria },
         { 176, QLocale::Netherlands },
         { 177, QLocale::Norway },
         { 178, QLocale::Nepal },
         { 180, QLocale::NauruCountry }, // Nauru
         { 181, QLocale::Suriname },
         { 182, QLocale::Nicaragua },
         { 183, QLocale::NewZealand },
         { 184, QLocale::PalestinianTerritories }, // Palestinian Authority
         { 185, QLocale::Paraguay },
         { 187, QLocale::Peru },
         { 190, QLocale::Pakistan },
         { 191, QLocale::Poland },
         { 192, QLocale::Panama },
         { 193, QLocale::Portugal },
         { 194, QLocale::PapuaNewGuinea },
         { 195, QLocale::Palau },
         { 196, QLocale::GuineaBissau },
         { 197, QLocale::Qatar },
         { 198, QLocale::Reunion },
         { 199, QLocale::MarshallIslands },
         { 200, QLocale::Romania },
         { 201, QLocale::Philippines },
         { 202, QLocale::PuertoRico },
         { 203, QLocale::Russia },
         { 204, QLocale::Rwanda },
         { 205, QLocale::SaudiArabia },
         { 206, QLocale::SaintPierreAndMiquelon },
         { 207, QLocale::SaintKittsAndNevis },
         { 208, QLocale::Seychelles },
         { 209, QLocale::SouthAfrica },
         { 210, QLocale::Senegal },
         { 212, QLocale::Slovenia },
         { 213, QLocale::SierraLeone },
         { 214, QLocale::SanMarino },
         { 215, QLocale::Singapore },
         { 216, QLocale::Somalia },
         { 217, QLocale::Spain },
         { 218, QLocale::SaintLucia },
         { 219, QLocale::Sudan },
         { 220, QLocale::SvalbardAndJanMayenIslands	 }, // Svalbard
         { 221, QLocale::Sweden },
         { 222, QLocale::Syria },
         { 223, QLocale::Switzerland },
         { 224, QLocale::UnitedArabEmirates },
         { 225, QLocale::TrinidadAndTobago },
         { 227, QLocale::Thailand },
         { 228, QLocale::Tajikistan },
         { 231, QLocale::Tonga },
         { 232, QLocale::Togo },
         { 233, QLocale::SaoTomeAndPrincipe },
         { 234, QLocale::Tunisia },
         { 235, QLocale::Turkey },
         { 236, QLocale::Tuvalu },
         { 237, QLocale::Taiwan },
         { 238, QLocale::Turkmenistan },
         { 239, QLocale::Tanzania },
         { 240, QLocale::Uganda },
         { 241, QLocale::Ukraine },
         { 242, QLocale::UnitedKingdom },
         { 244, QLocale::UnitedStates },
         { 245, QLocale::BurkinaFaso },
         { 246, QLocale::Uruguay },
         { 247, QLocale::Uzbekistan },
         { 248, QLocale::SaintVincentAndTheGrenadines },
         { 249, QLocale::Venezuela },
         { 251, QLocale::Vietnam },
         { 252, QLocale::UnitedStatesVirginIslands }, // VirginIslands (British VI are handled down below)
         { 253, QLocale::VaticanCityState },
         { 254, QLocale::Namibia },
         { 257, QLocale::WesternSahara }, // Western Sahara (disputed)
         { 258, QLocale::UnitedStates }, // Wake Island
         { 259, QLocale::Samoa },
         { 260, QLocale::Swaziland },
         { 261, QLocale::Yemen },
         { 263, QLocale::Zambia },
         { 264, QLocale::Zimbabwe },
         { 269, QLocale::Serbia }, // Serbia and Montenegro(Former)
         { 270, QLocale::Montenegro },
         { 271, QLocale::Serbia },
         { 273, QLocale::CuraSao },
         { 276, QLocale::SouthSudan },
         { 300, QLocale::Anguilla },
         { 301, QLocale::Antarctica },
         { 302, QLocale::Aruba },
         { 303, QLocale::AscensionIsland },
         { 304, QLocale::Australia }, // Ashmore and Cartier Islands
         { 305, QLocale::UnitedStates }, // Baker Island
         { 306, QLocale::BouvetIsland },
         { 307, QLocale::CaymanIslands },
         { 309, QLocale::ChristmasIsland },
         { 310, QLocale::ClippertonIsland },
         { 311, QLocale::CocosIslands }, // Cocos(Keeling)Islands
         { 312, QLocale::CookIslands },
         { 313, QLocale::Australia }, // Coral Sea Islands
         { 314, QLocale::DiegoGarcia },
         { 315, QLocale::FalklandIslands }, // Falkland Islands (IslasMalvinas)
         { 317, QLocale::FrenchGuiana },
         { 318, QLocale::FrenchPolynesia },
         { 319, QLocale::Antarctica }, // French Southern and Antarctic Lands
         { 321, QLocale::Guadeloupe },
         { 322, QLocale::Guam },
         { 323, QLocale::UnitedStates }, // Guantanamo Bay
         { 324, QLocale::Guernsey },
         { 325, QLocale::Australia }, // Heard Island and Mc Donald Islands
         { 326, QLocale::UnitedStates }, // Howland Island
         { 327, QLocale::UnitedStates }, // Jarvis Island
         { 328, QLocale::Jersey },
         { 329, QLocale::UnitedStates }, // Kingman Reef
         { 330, QLocale::Martinique },
         { 331, QLocale::Mayotte },
         { 332, QLocale::Montserrat },
         { 334, QLocale::NewCaledonia },
         { 335, QLocale::Niue },
         { 336, QLocale::NorfolkIsland },
         { 337, QLocale::NorthernMarianaIslands },
         { 338, QLocale::UnitedStates }, // PalmyraAtoll
         { 339, QLocale::Pitcairn }, // Pitcairn Islands
         { 340, QLocale::UnitedStates }, // Rota Island
         { 341, QLocale::UnitedStates }, // Saipan
         { 342, QLocale::SouthGeorgiaAndTheSouthSandwichIslands },
         { 343, QLocale::SaintHelena },
         { 346, QLocale::UnitedStates }, // Tinian Island
         { 347, QLocale::Tokelau },
         { 348, QLocale::TristanDaCunha },
         { 349, QLocale::TurksAndCaicosIslands },
         { 351, QLocale::BritishVirginIslands },
         { 352, QLocale::WallisAndFutunaIslands },
         { 15126, QLocale::IsleOfMan },
         { 19618, QLocale::Macedonia }, // Macedonia, Former Yugoslav Republic Of
         { 21242, QLocale::UnitedStates }, // Midway Islands
         { 30967, QLocale::SaintMartin }, // SintMaarten (DutchPart)
         { 31706, QLocale::SaintMartin }, // (FrenchPart)
         { 7299303, QLocale::EastTimor }, // Democratic Republic Of Timor-Leste
         { 10028789, QLocale::AlandIslands },
         { 161832015, QLocale::SaintBarthelemy },
         { 161832256, QLocale::UnitedStates }, // U.S. Minor Outlying Islands
         { 161832258, QLocale::Bonaire }, // Bonaire, Saint Eustatius and Saba
    };

    GEOID nationId = GetUserGeoID(GEOCLASS_NATION);

    return geoIdCountryMapping.value(nationId);

#else
    return static_cast<int>(QLocale::system().country());
#endif
}


void
JSResolverHelper::nativeReportCapabilities( const QVariant& v )
{
    if( m_stopped )
        return;


    bool ok;
    int intCap = v.toInt( &ok );
    Tomahawk::ExternalResolver::Capabilities capabilities;
    if ( !ok )
        capabilities = Tomahawk::ExternalResolver::NullCapability;
    else
        capabilities = static_cast< Tomahawk::ExternalResolver::Capabilities >( intCap );

    m_resolver->onCapabilitiesChanged( capabilities );
}


void
JSResolverHelper::reportScriptJobResults( const QVariantMap& result )
{
    if( m_stopped )
        return;

    m_resolver->d_func()->scriptAccount->reportScriptJobResult( result );
}


void
JSResolverHelper::registerScriptPlugin( const QString& type, const QString& objectId )
{
    if( m_stopped )
        return;

    m_resolver->d_func()->scriptAccount->registerScriptPlugin( type, objectId );
}


void
JSResolverHelper::unregisterScriptPlugin( const QString& type, const QString& objectId )
{
    if( m_stopped )
        return;

    m_resolver->d_func()->scriptAccount->unregisterScriptPlugin( type, objectId );
}


void
JSResolverHelper::setResolverConfig( const QVariantMap& config )
{
    m_resolverConfig = config;
}


QString
JSResolverHelper::accountId()
{
    return m_resolver->d_func()->accountId;
}


void JSResolverHelper::nativeAssert( bool assertion, const QString& message )
{
    if ( !assertion )
    {
        tLog() << "Assertion failed" << message;
        Q_ASSERT( assertion );
    }
}


void
JSResolverHelper::nativeRetrieveMetadata( int metadataId, const QString& url,
                                          const QString& mime_type, int sizehint,
                                          const QVariantMap& options )
{
    if ( sizehint <= 0 )
    {
        QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Supplied size is not (yet) supported');" )
                .arg( metadataId );
        m_resolver->d_func()->scriptAccount->evaluateJavaScript( javascript );
        return;
    }

    if ( TomahawkUtils::isHttpResult( url ) || TomahawkUtils::isHttpsResult( url ) )
    {
        QMap<QString, QString> headers;
        if ( options.contains( "headers" ) && options["headers"].canConvert( QVariant::Map ) )
        {
            const QVariantMap variantHeaders = options["headers"].toMap();
            foreach ( const QString& key, variantHeaders.keys() )
            {
                headers.insert( key, variantHeaders[key].toString() );
            }
        }

        // TODO: Add heuristic if size is not defined
        CloudStream stream( url, sizehint, headers, Tomahawk::Utils::nam() );
        stream.Precache();
        QScopedPointer<TagLib::File> tag;
        if ( mime_type == "audio/mpeg" )
        {
            tag.reset( new TagLib::MPEG::File( &stream,
                TagLib::ID3v2::FrameFactory::instance(),
                TagLib::AudioProperties::Accurate
            ));
        }
        else if ( mime_type == "audio/mp4" )
        {
            tag.reset( new TagLib::MP4::File( &stream,
                true, TagLib::AudioProperties::Accurate
            ));
        }
#if defined(TAGLIB_MAJOR_VERSION) && defined(TAGLIB_MINOR_VERSION)
#if TAGLIB_MAJOR_VERSION >= 1 && TAGLIB_MINOR_VERSION >= 9
        else if ( mime_type == "application/opus" || mime_type == "audio/opus" )
        {
            tag.reset( new TagLib::Ogg::Opus::File( &stream, true,
                TagLib::AudioProperties::Accurate
            ));
        }
#endif
#endif
        else if ( mime_type == "application/ogg" || mime_type == "audio/ogg" )
        {
            tag.reset( new TagLib::Ogg::Vorbis::File( &stream, true,
                TagLib::AudioProperties::Accurate
            ));
        }
        else if ( mime_type == "application/x-flac" || mime_type == "audio/flac" ||
                   mime_type == "audio/x-flac" )
        {
            tag.reset( new TagLib::FLAC::File( &stream,
                TagLib::ID3v2::FrameFactory::instance(),
                true, TagLib::AudioProperties::Accurate
            ));
        }
        else if ( mime_type == "audio/x-ms-wma" )
        {
            tag.reset( new TagLib::ASF::File( &stream, true,
                TagLib::AudioProperties::Accurate
            ));
        }
        else
        {
            QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Unknown mime type for tagging: %2');" )
                    .arg( metadataId ).arg( mime_type );
            m_resolver->d_func()->scriptAccount->evaluateJavaScript( javascript );
            return;
        }

        if ( stream.num_requests() > 2)
        {
            // Warn if pre-caching failed.
            tLog() << "Total requests for file:" << url
                   << stream.num_requests() << stream.cached_bytes();
        }

        if ( !tag->tag() || tag->tag()->isEmpty() )
        {
            QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Could not read tag information.');" )
                    .arg( metadataId );
            m_resolver->d_func()->scriptAccount->evaluateJavaScript( javascript );
            return;
        }

        QVariantMap m;
        m["url"] = url;
        m["track"] = QString( tag->tag()->title().toCString() ).trimmed();
        m["album"] = QString( tag->tag()->album().toCString() ).trimmed();
        m["artist"] = QString( tag->tag()->artist().toCString() ).trimmed();

        if ( m["track"].toString().isEmpty() )
        {
            QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Empty track returnd');" )
                    .arg( metadataId );
            m_resolver->d_func()->scriptAccount->evaluateJavaScript( javascript );
            return;
        }

        if ( m["artist"].toString().isEmpty() )
        {
            QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Empty artist returnd');" )
                    .arg( metadataId );
            m_resolver->d_func()->scriptAccount->evaluateJavaScript( javascript );
            return;
        }

        if ( tag->audioProperties() )
        {
            m["bitrate"] = tag->audioProperties()->bitrate();
            m["channels"] = tag->audioProperties()->channels();
            m["duration"] = tag->audioProperties()->length();
            m["samplerate"] = tag->audioProperties()->sampleRate();
        }

        QString javascript = QString( "Tomahawk.retrievedMetadata( %1, %2 );" )
                .arg( metadataId )
                .arg( QString::fromLatin1( TomahawkUtils::toJson( m ) ) );
        m_resolver->d_func()->scriptAccount->evaluateJavaScript( javascript );
    }
    else
    {
        QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Protocol not supported');" )
                .arg( metadataId );
        m_resolver->d_func()->scriptAccount->evaluateJavaScript( javascript );
    }
}

void
JSResolverHelper::invokeNativeScriptJob( int requestId, const QString& methodName, const QVariantMap& params )
{
    if ( methodName == "httpRequest" ) {
        nativeAsyncRequest( requestId, params );
    } else {
        QVariantMap error;
        error["message"] = "NativeScriptJob methodName was not found";
        error["name"] = "method_was_not_found";

        m_resolver->d_func()->scriptAccount->reportNativeScriptJobError( requestId, error );
    }
}


void
JSResolverHelper::nativeAsyncRequest( const int requestId, const QVariantMap& options )
{
    QString url = options[ "url" ].toString();
    QVariantMap headers = options[ "headers" ].toMap();

    QNetworkRequest req( url );
    foreach ( const QString& key, headers.keys() )
    {
        req.setRawHeader( key.toLatin1(), headers[key].toString().toLatin1() );
    }

    if ( options.contains( "username" ) && options.contains( "password" ) )
    {
        // If we have sufficient authentication data, we will send
        // username+password as HTTP Basic Auth
        QString credentials = QString( "Basic %1" )
                .arg( QString( QString("%1:%2")
                        .arg( options["username"].toString() )
                        .arg( options["password"].toString() )
                        .toLatin1().toBase64() )
                );
        req.setRawHeader( "Authorization", credentials.toLatin1() );
    }

    NetworkReply* reply = NULL;
    if ( options.contains( "method") && options["method"].toString().toUpper() == "POST" )
    {
        QByteArray data;
        if ( options.contains( "data" ) )
        {
            data = options["data"].toString().toUtf8();
        }
        reply = new NetworkReply( Tomahawk::Utils::nam()->post( req, data ) );
    }
    else if ( options.contains( "method") && options["method"].toString().toUpper() == "HEAD" )
    {
        reply = new NetworkReply( Tomahawk::Utils::nam()->head( req ) );
    }
    else
    {
        reply = new NetworkReply( Tomahawk::Utils::nam()->get( req ) );
    }

    NewClosure( reply, SIGNAL( finished() ), this, SLOT( nativeAsyncRequestDone( int, NetworkReply* ) ), requestId, reply );
}


void
JSResolverHelper::nativeAsyncRequestDone( int requestId, NetworkReply* reply )
{
    reply->deleteLater();

    QVariantMap map;
    map["response"] = QString::fromUtf8( reply->reply()->readAll() );
    map["responseText"] = map["response"];
    map["responseType"] = QString(); // Default, indicates a string in map["response"]
    map["readyState"] = 4;
    map["status"] = reply->reply()->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    map["statusText"] = QString("%1 %2").arg( map["status"].toString() )
            .arg( reply->reply()->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString() );


    QVariantMap responseHeaders;
    foreach( const QNetworkReply::RawHeaderPair& pair, reply->reply()->rawHeaderPairs() )
    {
        responseHeaders[ pair.first ] = pair.second;
    }
    map["responseHeaders"] = responseHeaders;

    m_resolver->d_func()->scriptAccount->reportNativeScriptJobResult( requestId, map );
}


bool
JSResolverHelper::hasFuzzyIndex()
{
    return !m_resolver->d_func()->fuzzyIndex.isNull();
}


bool
JSResolverHelper::indexDataFromVariant( const QVariantMap &map, struct Tomahawk::IndexData& indexData )
{
    // We do not use artistId at the moment
    indexData.artistId = 0;

    if ( map.contains( "album" ) )
    {
        indexData.album = map["album"].toString();
    }
    else
    {
        indexData.album = QString();
    }

    // Check that we have the three required attributes
    if ( !map.contains( "id" ) || !map["id"].canConvert( QVariant::Int )
         || !map.contains( "track" ) || !map.contains( "artist" ) )
    {
        return false;
    }

    bool ok;
    indexData.id = map["id"].toInt( &ok );
    if ( !ok )
    {
        return false;
    }

    indexData.artist = map["artist"].toString().trimmed();
    if ( indexData.artist.isEmpty() )
    {
        return false;
    }

    indexData.track = map["track"].toString().trimmed();
    if ( indexData.track.isEmpty() )
    {
        return false;
    }

    return true;
}


void
JSResolverHelper::createFuzzyIndex( const QVariantList& list )
{
    if ( hasFuzzyIndex() )
    {
        m_resolver->d_func()->fuzzyIndex->wipeIndex();
    }
    else
    {
        m_resolver->d_func()->fuzzyIndex.reset( new FuzzyIndex( m_resolver, accountId() + ".lucene", true ) );
    }

    addToFuzzyIndex( list );
}


void
JSResolverHelper::addToFuzzyIndex( const QVariantList& list )
{
    if ( !hasFuzzyIndex() )
    {
        tLog() << Q_FUNC_INFO << "Cannot add entries to non-existing index.";
        return;
    }

    m_resolver->d_func()->fuzzyIndex->beginIndexing();

    foreach ( const QVariant& variant, list )
    {
        // Convert each entry to IndexData
        if ( variant.canConvert( QVariant::Map ) )
        {
            QVariantMap map = variant.toMap();

            // Convert each entry and do multiple checks that we have valid data.
            struct IndexData indexData;

            if ( indexDataFromVariant( map, indexData ) )
            {
                m_resolver->d_func()->fuzzyIndex->appendFields( indexData );
            }
        }
    }

    m_resolver->d_func()->fuzzyIndex->endIndexing();
}


bool
cmpTuple( const QVariant& x, const QVariant& y )
{
    return x.toList().at( 1 ).toFloat() < y.toList().at( 1 ).toFloat();
}


QVariantList
JSResolverHelper::searchInFuzzyIndex( const query_ptr& query )
{
    if ( m_resolver->d_func()->fuzzyIndex )
    {
        QMap<int, float> map = m_resolver->d_func()->fuzzyIndex->search( query );

        // Convert map to sorted QVariantList
        QVariantList list;
        foreach ( int id, map.keys() )
        {
            QVariantList innerList;
            innerList.append( QVariant( id ) );
            innerList.append( QVariant( map[id] ) );
            // Wrap into QVariant or the list will be flattend
            list.append( QVariant( innerList  ));
        }
        std::sort( list.begin(), list.end(), cmpTuple );

        return list;
    }
    return QVariantList();
}


QVariantList
JSResolverHelper::searchFuzzyIndex( const QString& query )
{
    return searchInFuzzyIndex( Query::get( query, QString() ) );
}


QVariantList
JSResolverHelper::resolveFromFuzzyIndex( const QString& artist, const QString& album, const QString& track )
{
    // Important: Do not autoresolve!
    query_ptr query = Query::get( artist, track, album, QString(), false );
    if ( query.isNull() )
    {
        tLog() << Q_FUNC_INFO << "Could not create a query for" << artist << "-" << track;
        return QVariantList();
    }
    return searchInFuzzyIndex( query );
}


void
JSResolverHelper::deleteFuzzyIndex()
{
    if ( m_resolver->d_func()->fuzzyIndex )
    {
        m_resolver->d_func()->fuzzyIndex->deleteIndex();
        m_resolver->d_func()->fuzzyIndex->deleteLater();
        m_resolver->d_func()->fuzzyIndex.reset();
    }
}


void
JSResolverHelper::readdResolver()
{
    Pipeline::instance()->removeResolver( m_resolver );
    Pipeline::instance()->addResolver( m_resolver );
}

