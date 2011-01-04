#include "databasecommand_importplaylist.h"

#include <QSqlQuery>

#include "tomahawk/query.h"
#include "tomahawk/playlist.h"
#include "databaseimpl.h"

void DatabaseCommand_ImportPlaylist::exec(DatabaseImpl * dbi)
{
    /*
    qDebug() << "Importing playlist of" << m_playlist->length() << "tracks";
    TomahawkSqlQuery query = dbi->newquery();
    query.prepare("INSERT INTO playlist(title, info, creator, lastmodified) "
                  "VALUES(?,?,?,?)");
    query.addBindValue(m_playlist->title());
    query.addBindValue(m_playlist->info());
    query.addBindValue(m_playlist->creator());
    query.addBindValue(m_playlist->lastmodified());
    query.exec();
    int pid = query.lastInsertId().toInt();
    int pos = 0;
    query.prepare("INSERT INTO playlist_tracks( "
                  "playlist, position, trackname, albumname, artistname) "
                  "VALUES (?,?,?,?,?)");

    for(int k = 0; k < m_playlist->length(); k++)
    {
        pos++;
        query.addBindValue(pid);
        query.addBindValue(pos);
        query.addBindValue(m_playlist->at(k)->artist());
        query.addBindValue(m_playlist->at(k)->album());
        query.addBindValue(m_playlist->at(k)->track());
        query.exec();
    }
    emit done(pid);
    */
}

