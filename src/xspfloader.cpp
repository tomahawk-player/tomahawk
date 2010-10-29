#include <QDomDocument>

#include "xspfloader.h"

#include "tomahawk/tomahawkapp.h"
#include "tomahawk/playlist.h"

using namespace Tomahawk;


void
XSPFLoader::load( const QUrl& url )
{
    QNetworkRequest request( url );
    QNetworkReply* reply = APP->nam()->get( request );

    // isn't there a race condition here? something could happen before we connect()
    connect( reply, SIGNAL( finished() ),
                      SLOT( networkLoadFinished() ) );

    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
                      SLOT( networkError( QNetworkReply::NetworkError ) ) );
}


void
XSPFLoader::load( QFile& file )
{
    if( file.open( QFile::ReadOnly ) )
    {
        m_body = file.readAll();
        gotBody();
    }
    else
    {
        qDebug() << "Failed to open xspf file";
        reportError();
    }
}


void
XSPFLoader::reportError()
{
    qDebug() << Q_FUNC_INFO;
    emit failed();
    deleteLater();
}


void
XSPFLoader::networkLoadFinished()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    m_body = reply->readAll();
    gotBody();
}


void
XSPFLoader::networkError( QNetworkReply::NetworkError e )
{
    qDebug() << Q_FUNC_INFO << e;
    reportError();
}


void
XSPFLoader::gotBody()
{
    qDebug() << Q_FUNC_INFO;

    QDomDocument xmldoc;
    xmldoc.setContent( m_body );
    QDomElement docElement( xmldoc.documentElement() );

    QString title, info, creator;
    title   = docElement.firstChildElement( "title" ).text();
    info    = docElement.firstChildElement( "creator" ).text();
    creator = docElement.firstChildElement( "info" ).text();

    m_playlist = Playlist::create( APP->sourcelist().getLocal(),
                                   uuid(),
                                   title,
                                   info,
                                   creator,
                                   false );

    QList< plentry_ptr > entries;

    QDomNodeList tracklist = docElement.elementsByTagName( "track" );
    for ( unsigned int i = 0; i < tracklist.length(); i++ )
    {
        QDomNode e = tracklist.at( i );

        plentry_ptr p( new PlaylistEntry );
        p->setGuid( uuid() );
        p->setDuration( e.firstChildElement( "duration" ).text().toInt() / 1000 );
        p->setLastmodified( 0 );
        p->setAnnotation( e.firstChildElement( "annotation" ).text() );

        QVariantMap v;
        v.insert( "duration", e.firstChildElement( "duration" ).text().toInt() / 1000 );
        v.insert( "artist", e.firstChildElement( "creator" ).text() );
        v.insert( "album", e.firstChildElement( "album" ).text() );
        v.insert( "track", e.firstChildElement( "title" ).text() );

        p->setQuery( Tomahawk::query_ptr(new Tomahawk::Query(v)) );
        entries << p;
    }

    m_playlist->createNewRevision( uuid(), m_playlist->currentrevision(), entries );
    emit ok( m_playlist );

    deleteLater();
}
