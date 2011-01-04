#include "databasecommand_modifyplaylist.h"

using namespace Tomahawk;


DatabaseCommand_ModifyPlaylist::DatabaseCommand_ModifyPlaylist( Playlist* playlist, const QList< plentry_ptr >& entries, Mode mode )
    : DatabaseCommand()
    , m_playlist( playlist )
    , m_entries( entries )
    , m_mode( mode )
{
}


void DatabaseCommand_ModifyPlaylist::exec( DatabaseImpl* lib )
{
}
