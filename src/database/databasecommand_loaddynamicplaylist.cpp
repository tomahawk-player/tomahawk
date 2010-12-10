#include "databasecommand_loaddynamicplaylist.h"

#include <QSqlQuery>
#include <QString>

#include "databaseimpl.h"
#include "tomahawksqlquery.h"
#include "dynamic/dynamiccontrol.h"
#include "dynamic/generatorinterface.h"

using namespace Tomahawk;


void
DatabaseCommand_LoadDynamicPlaylist::exec( DatabaseImpl* dbi )
{
    qDebug() << "Loading dynamic playlist revision" << guid();
    // load the entries first    
    generateEntries( dbi );
    
    // now load the controls etc
    
    TomahawkSqlQuery controlsQuery = dbi->newquery();
    controlsQuery.prepare("SELECT controls, plmode, pltype "
                          "FROM dynamic_playlist_revision "
                          "WHERE guid = ?");
    controlsQuery.addBindValue( guid() );
    controlsQuery.exec();
    
    QList< dyncontrol_ptr > controls;
    if( controlsQuery.next() ) 
    {
        QStringList controlIds = controlsQuery.value( 0 ).toStringList();
        foreach( const QString& controlId, controlIds )
        {
            TomahawkSqlQuery controlQuery = dbi->newquery();
            controlQuery.prepare( "SELECT selectedType, match, input"
                                  "FROM dynamic_playlist_controls"
                                  "WHERE id = :id" );
            controlQuery.bindValue( ":id", controlId );
            controlQuery.exec();
            if( controlQuery.next() )
            {
                dyncontrol_ptr c = dyncontrol_ptr( new DynamicControl );
                c->setId( controlId );
                c->setSelectedType( controlQuery.value( 0 ).toString() );
                c->setMatch( controlQuery.value( 1 ).toString() );
                c->setInput( controlQuery.value( 2 ).toString() );
                controls << c;
            }
        }
    }
    
    
    QString type = controlsQuery.value( 2 ).toString();
    GeneratorMode mode = static_cast<GeneratorMode>( controlsQuery.value( 1 ).toInt() );
    if( mode == OnDemand ) { 
        Q_ASSERT( m_entrymap.isEmpty() ); // ondemand should have no entry
        
        emit done( guid(), m_islatest, type, controls, true );
    } else {
        emit done( guid(), m_guids, m_oldentries, type, controls, m_islatest, m_entrymap, true );
    }
}
