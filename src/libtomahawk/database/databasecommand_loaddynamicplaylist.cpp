#include "databasecommand_loaddynamicplaylist.h"

#include <QSqlQuery>
#include <QString>

#include "databaseimpl.h"
#include "tomahawksqlquery.h"
#include "dynamic/DynamicControl.h"
#include "dynamic/GeneratorInterface.h"
#include <dynamic/GeneratorFactory.h>

using namespace Tomahawk;


void
DatabaseCommand_LoadDynamicPlaylist::exec( DatabaseImpl* dbi )
{
    qDebug() << "Loading dynamic playlist revision" << guid();
    // load the entries first    
    generateEntries( dbi );
    
    // now load the controls etc
    
    TomahawkSqlQuery controlsQuery = dbi->newquery();
    controlsQuery.prepare("SELECT playlist_revision.playlist, controls, plmode, pltype "
                          "FROM dynamic_playlist_revision, playlist_revision "
                          "WHERE dynamic_playlist_revision.guid = ? AND playlist_revision.guid = dynamic_playlist_revision.guid");
    controlsQuery.addBindValue( revisionGuid() );
    controlsQuery.exec();
    
    QString type;
    GeneratorMode mode;
    
    QList< QVariantMap > controls;
    QString playlist_guid;
    qDebug() << "Loading controls..." << revisionGuid();
    qDebug() << "SELECT playlist_revision.playlist, controls, plmode, pltype "
    "FROM dynamic_playlist_revision, playlist_revision "
    "WHERE dynamic_playlist_revision.guid = "<< revisionGuid() << " AND playlist_revision.guid = dynamic_playlist_revision.guid";
    if( controlsQuery.first() ) 
    {
        playlist_guid = controlsQuery.value( 0 ).toString();
        QJson::Parser parser;
        bool ok;
        QVariant v = parser.parse( controlsQuery.value(1).toByteArray(), &ok );
        Q_ASSERT( ok && v.type() == QVariant::List ); //TODO
        
        
        type = controlsQuery.value( 3 ).toString();
        GeneratorMode mode = static_cast<GeneratorMode>( controlsQuery.value( 2 ).toInt() );
        
        QStringList controlIds = v.toStringList();
        qDebug() << "Got controls in dynamic playlist, loading:" << controlIds << controlsQuery.value(1);
        foreach( const QString& controlId, controlIds )
        {
            TomahawkSqlQuery controlQuery = dbi->newquery();
            controlQuery.prepare( "SELECT selectedType, match, input "
                                  "FROM dynamic_playlist_controls "
                                  "WHERE id = :id" );
            controlQuery.bindValue( ":id", controlId );
            controlQuery.exec();
            if( controlQuery.next() )
            {
                QVariantMap c;
                c[ "type" ] = type;
                c[ "id" ] = controlId;
                c[ "selectedType" ] = controlQuery.value( 0 ).toString();
                c[ "match" ] = controlQuery.value( 1 ).toString();
                c[ "input" ] = controlQuery.value( 2 ).toString();
                controls << c;
            }
        }
    } else {
        // No controls, lets load the info we need directly from the playlist table
        TomahawkSqlQuery info = dbi->newquery();
        info.prepare( QString( "SELECT dynamic_playlist.pltype, dynamic_playlist.plmode FROM playlist, dynamic_playlist WHERE playlist.guid = \"%1\" AND playlist.guid = dynamic_playlist.guid" ).arg( playlist_guid ) );
        if( !info.exec()  ) {
            qWarning() << "Failed to load dynplaylist info..";
            return;
        } else if( !info.first() ) {
            qWarning() << "Noo results for queryL:" << info.lastQuery();
            return;
        }
        type = info.value( 0 ).toString();
        mode = static_cast<GeneratorMode>( info.value( 1 ).toInt() );
    }
   
    if( mode == OnDemand ) { 
        Q_ASSERT( m_entrymap.isEmpty() ); // ondemand should have no entry
        
        emit done( revisionGuid(), m_islatest, type, controls, true );
    } else {
        emit done( revisionGuid(), m_guids, m_oldentries, type, controls, m_islatest, m_entrymap, true );
    }
}
