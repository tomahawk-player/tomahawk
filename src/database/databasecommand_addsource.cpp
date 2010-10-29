#include <QSqlQuery>
#include "databasecommand_addsource.h"
#include "databaseimpl.h"


DatabaseCommand_addSource::DatabaseCommand_addSource( const QString& username, const QString& fname, QObject* parent )
    : DatabaseCommand( parent )
    , m_username( username )
    , m_fname( fname )
{
}


void
DatabaseCommand_addSource::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( "SELECT id, friendlyname FROM source WHERE name = ?" );
    query.addBindValue( m_username );
    query.exec();

    if ( query.next() )
    {
        unsigned int id = query.value( 0 ).toInt();
        QString fname = query.value( 1 ).toString();
        query.prepare( "UPDATE source SET isonline = 'true', friendlyname = ? WHERE id = ?" );
        query.addBindValue( m_fname );
        query.addBindValue( id );
        query.exec();
        emit done( id, fname );
        return;
    }

    query.prepare( "INSERT INTO source(name, friendlyname, isonline) VALUES(?,?,?)" );
    query.addBindValue( m_username );
    query.addBindValue( m_fname );
    query.addBindValue( true );
    query.exec();

    unsigned int id = query.lastInsertId().toUInt();
    qDebug() << "Inserted new source to DB, id:" << id << " friendlyname" << m_username;

    emit done( id, m_fname );
}
