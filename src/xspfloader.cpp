#include "xspfloader.h"

#include <QDomDocument>
#include <QMessageBox>

#include "tomahawk/tomahawkapp.h"
#include "sourcelist.h"
#include "playlist.h"

using namespace Tomahawk;


void
XSPFLoader::load( const QUrl& url )
{
    QNetworkRequest request( url );
    QNetworkReply* reply = TomahawkUtils::nam()->get( request );

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

    QString origTitle;
    origTitle = docElement.firstChildElement( "title" ).text();
    m_info    = docElement.firstChildElement( "creator" ).text();
    m_creator = docElement.firstChildElement( "info" ).text();

    m_title = origTitle;
    if ( m_title.isEmpty() )
        m_title = tr( "New Playlist" );

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
        m_entries << p;
    }

    if ( origTitle.isEmpty() && m_entries.isEmpty() )
    {
        if ( m_autoCreate )
        {
            QMessageBox::critical( APP->mainWindow(), tr( "XSPF Error" ), tr( "This is not a valid XSPF playlist." ) );
            deleteLater();
            return;
        }
        else
        {
            emit failed();
            return;
        }
    }

    if ( m_autoCreate )
    {
        m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                       uuid(),
                                       m_title,
                                       m_info,
                                       m_creator,
                                       false );

        m_playlist->createNewRevision( uuid(), m_playlist->currentrevision(), m_entries );
        deleteLater();
    }

    emit ok( m_playlist );
}
