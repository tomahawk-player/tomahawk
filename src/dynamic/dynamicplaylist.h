#ifndef DYNAMIC_PLAYLIST_H
#define DYNAMIC_PLAYLIST_H

#include <QObject>
#include <QList>
#include <QSharedPointer>

#include "tomahawk/playlist.h"
#include "dynamic/generatorinterface.h"

namespace Tomahawk {
    
/**
 * Subclass of playlist that adds the information needed to store a dynamic playlist.
 *  It uses normal PlaylistEntries but also has a mode, a generator, and a list of controls
*/

class DynamicPlaylist : public Playlist
{
    Q_OBJECT
    
    Q_PROPERTY( GeneratorMode mode  WRITE setMode   READ mode )
    Q_PROPERTY( QString type                  WRITE setType   READ type )
        
public:
    enum DynamicPlaylistMode {
        OnDemand = 0,
        StaticPlaylist
    };
    
    /// Generate an empty dynamic playlist with default generator
    static Tomahawk::dynplaylist_ptr create( const source_ptr& author,
                                          const QString& guid,
                                          const QString& title,
                                          const QString& info,
                                          const QString& creator,
                                          bool shared
                                          );
    static bool remove( const dynplaylist_ptr& playlist );
    
    virtual void loadRevision( const QString& rev = "" );
    
    GeneratorMode mode() const { return m_generator->mode(); }
    QString type() const { return m_generator->type(); }
    generatorinterface_ptr generator() const { return m_generator; }
    
    // <IGNORE hack="true">
    // these need to exist and be public for the json serialization stuff
    // you SHOULD NOT call them.  They are used for an alternate CTOR method from json.
    // maybe friend QObjectHelper and make them private?
    explicit DynamicPlaylist( const source_ptr& author )
    : Playlist( author )
    {
        qDebug() << Q_FUNC_INFO << "JSON";
    }
    void setMode( GeneratorMode mode ) { m_generator->setMode( mode ); }
    void setType( const QString& type )           { /** TODO */; }
    void setGenerator( const generatorinterface_ptr& gen_ptr )            { m_generator = gen_ptr; }
    // </IGNORE>
    
signals:
    /// emitted when the playlist revision changes (whenever the playlist changes)
    void revisionLoaded( Tomahawk::DynamicPlaylistRevision );
    
public slots:
    // want to update the playlist from the model?
    // generate a newrev using uuid() and call this:
    // if this is a static playlist, pass it a new list of entries. implicitly sets mode to static
    void createNewRevision( const QString& newrev, const QString& oldrev, const QString& type, const QList< dyncontrol_ptr>& controls, const QList< plentry_ptr >& entries );
    // if it is ondemand, no entries are needed implicitly sets mode to ondemand
    void createNewRevision( const QString& newrev, const QString& oldrev, const QString& type, const QList< dyncontrol_ptr>& controls );
    
    void reportCreated( const Tomahawk::dynplaylist_ptr& self );
    void reportDeleted( const Tomahawk::dynplaylist_ptr& self );
    
    // called from setdynamicplaylistrevision db cmd
    // static version
    void setRevision( const QString& rev,
                      const QList<QString>& neworderedguids,
                      const QList<QString>& oldorderedguids,
                      const QString& type,
                      const QList< dyncontrol_ptr>& controls,
                      bool is_newest_rev,
                      const QMap< QString, Tomahawk::plentry_ptr >& addedmap,
                      bool applied );
   
    // ondemand version
    void setRevision( const QString& rev,
                      bool is_newest_rev,
                      const QString& type,
                      const QList< dyncontrol_ptr>& controls,
                      bool applied );
private:
    // called from loadAllPlaylists DB cmd:
    explicit DynamicPlaylist( const source_ptr& src,
                       const QString& currentrevision,
                       const QString& title,
                       const QString& info,
                       const QString& creator,
                       const QString& type,
                       bool shared,
                       int lastmod,
                       const QString& guid = "" ); // populate db
    
    // called when creating new playlist
    explicit DynamicPlaylist( const source_ptr& author,
                       const QString& guid,
                       const QString& title,
                       const QString& info,
                       const QString& creator,
                       const QString& type,
                       bool shared );
    
private:
    generatorinterface_ptr m_generator;
};

}; // namespace

#endif