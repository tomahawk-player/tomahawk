#ifndef DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H
#define DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H

#include "databaseimpl.h"
#include "databasecommand_createplaylist.h"
#include "dynamic/DynamicPlaylist.h"
#include "typedefs.h"

class DatabaseCommand_CreateDynamicPlaylist : public DatabaseCommand_CreatePlaylist
{
    Q_OBJECT
    Q_PROPERTY( QVariant playlist READ playlistV WRITE setPlaylistV )
    
public:
    explicit DatabaseCommand_CreateDynamicPlaylist( QObject* parent = 0 );
    explicit DatabaseCommand_CreateDynamicPlaylist( const Tomahawk::source_ptr& author, const Tomahawk::dynplaylist_ptr& playlist );
    
    QString commandname() const { return "createdynamicplaylist"; }
    
    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }
    
    QVariant playlistV() const
    {
        if( m_v.isNull() )
            return QJson::QObjectHelper::qobject2qvariant( (QObject*)m_playlist.data() );
        else
            return m_v;
    }
    
    void setPlaylistV( const QVariant& v )
    {
        m_v = v;
    }
    
private:
    Tomahawk::dynplaylist_ptr m_playlist;
};

#endif // DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H
