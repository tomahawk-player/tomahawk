#include "xspfloader.h"

#include <QApplication>
#include <QDomDocument>
#include <QMessageBox>

#include "utils/tomahawkutils.h"

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
    bool namespaceProcessing = true;
    xmldoc.setContent( m_body, namespaceProcessing );
    QDomElement docElement( xmldoc.documentElement() );

    QString origTitle;
    QDomNodeList tracklist;
    QDomElement n = docElement.firstChildElement();
    for ( ; !n.isNull(); n = n.nextSiblingElement() ) {
        if (n.namespaceURI() == m_NS && n.localName() == "title") {
            origTitle = n.text();
        } else if (n.namespaceURI() == m_NS && n.localName() == "creator") {
            m_creator = n.text();
        } else if (n.namespaceURI() == m_NS && n.localName() == "info") {
            m_info = n.text();
        } else if (n.namespaceURI() == m_NS && n.localName() == "trackList") {
            tracklist = n.childNodes();
        }
    }

    m_title = origTitle;
    if ( m_title.isEmpty() )
        m_title = tr( "New Playlist" );

    for ( unsigned int i = 0; i < tracklist.length(); i++ )
    {
        QDomNode e = tracklist.at( i );

        QString artist, album, track, duration, annotation;
        QDomElement n = e.firstChildElement();
        for ( ; !n.isNull(); n = n.nextSiblingElement() ) {
            if (n.namespaceURI() == m_NS && n.localName() == "duration") {
                duration = n.text();
            } else if (n.namespaceURI() == m_NS && n.localName() == "annotation") {
                annotation = n.text();
            } else if (n.namespaceURI() == m_NS && n.localName() == "creator") {
                artist = n.text();
            } else if (n.namespaceURI() == m_NS && n.localName() == "album") {
                album = n.text();
            } else if (n.namespaceURI() == m_NS && n.localName() == "title") {
                track = n.text();
            }
        }

        plentry_ptr p( new PlaylistEntry );
        p->setGuid( uuid() );
        p->setDuration( duration.toInt() / 1000 );
        p->setLastmodified( 0 );
        p->setAnnotation( annotation );

        p->setQuery( Tomahawk::Query::get( artist, track, album, uuid() ) );
        p->query()->setDuration( duration.toInt() / 1000 );
        m_entries << p;
    }

    if ( origTitle.isEmpty() && m_entries.isEmpty() )
    {
        if ( m_autoCreate )
        {
            QMessageBox::critical( 0, tr( "XSPF Error" ), tr( "This is not a valid XSPF playlist." ) );
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
