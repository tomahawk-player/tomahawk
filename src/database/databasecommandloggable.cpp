#include "databasecommandloggable.h"

#include <QDebug>

#include "database/databasecommand_addfiles.h"
#include "database/databasecommand_setplaylistrevision.h"


DatabaseCommandLoggable*
DatabaseCommandLoggable::factory( const QVariantMap& c )
{
    const QString name = c.value( "command" ).toString();
    //TODO dynamic class loading, factory blah

    if( name == "addfiles" )
    {
        DatabaseCommand_AddFiles* cmd = new DatabaseCommand_AddFiles;
        QJson::QObjectHelper::qvariant2qobject( c, cmd );
        return cmd;
    }
    else if( name == "setplaylistrevision" )
    {
        DatabaseCommand_SetPlaylistRevision* cmd = new DatabaseCommand_SetPlaylistRevision;
        QJson::QObjectHelper::qvariant2qobject( c, cmd );
        return cmd;
    }
    else
    {
        qDebug() << "Unhandled command name";
        Q_ASSERT( false );
        return 0;
    }
}
