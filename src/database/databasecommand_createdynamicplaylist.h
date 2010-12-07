#ifndef DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H
#define DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H

#include "databaseimpl.h"
#include "databasecommand_createplaylist.h"
#include "dynamic/dynamicplaylist.h"
#include "tomahawk/typedefs.h"

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
        return QJson::QObjectHelper::qobject2qvariant( (QObject*)m_playlist.data() );
    }
    
    void setPlaylistV( const QVariant& v )
    {
        qDebug() << "***********" << Q_FUNC_INFO << v;
        using namespace Tomahawk;
        
        DynamicPlaylist* p = new DynamicPlaylist( source() );
        QJson::QObjectHelper::qvariant2qobject( v.toMap(), p );
        m_playlist = dynplaylist_ptr( p );
    }
    
private:
    Tomahawk::dynplaylist_ptr m_playlist;
};

#endif // DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H
