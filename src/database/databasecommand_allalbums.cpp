#include "databasecommand_allalbums.h"

#include <QSqlQuery>

#include "databaseimpl.h"

void
DatabaseCommand_AllAlbums::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( !m_collection->source().isNull() );

    TomahawkSqlQuery query = dbi->newquery();
    QList<Tomahawk::album_ptr> al;
    QString m_orderToken;

    switch ( m_sortOrder )
    {
        case 0:
            break;

        case ModificationTime:
            m_orderToken = "file.mtime";
    }

    QString sql = QString(
            "SELECT DISTINCT album.id, album.name, album.artist, artist.name "
            "FROM album, file, file_join "
            "LEFT OUTER JOIN artist "
            "ON album.artist = artist.id "
            "WHERE file.id = file_join.file "
            "AND file_join.album = album.id "
            "AND file.source %1 "
            "%2 %3 %4"
            ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) )
             .arg( m_sortOrder > 0 ? QString( "ORDER BY %1" ).arg( m_orderToken ) : QString() )
             .arg( m_sortDescending ? "DESC" : QString() )
             .arg( m_amount > 0 ? QString( "LIMIT 0, %1" ).arg( m_amount ) : QString() );

    query.prepare( sql );
    query.exec();

    while( query.next() )
    {
        Tomahawk::artist_ptr artist = Tomahawk::artist_ptr( new Tomahawk::Artist( query.value( 3 ).toString() ) );
        Tomahawk::album_ptr album = Tomahawk::Album::get( query.value( 0 ).toUInt(), query.value( 1 ).toString(), artist, m_collection );

        al << album;
    }

    if ( al.count() )
        emit albums( al, m_collection );
    emit done( m_collection );
}
