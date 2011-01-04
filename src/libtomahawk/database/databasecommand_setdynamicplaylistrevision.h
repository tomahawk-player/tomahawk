#ifndef DATABASECOMMAND_SETDYNAMICPLAYLISTREVISION_H
#define DATABASECOMMAND_SETDYNAMICPLAYLISTREVISION_H

#include "databasecommand_setplaylistrevision.h"
#include "databaseimpl.h"
#include "collection.h"
#include "playlist.h"
#include "dynamic/GeneratorInterface.h"
#include "network/servent.h"

using namespace Tomahawk;

class DatabaseCommand_SetDynamicPlaylistRevision : public DatabaseCommand_SetPlaylistRevision
{
    Q_OBJECT
    Q_PROPERTY( QString type                     READ type          WRITE setType )
    Q_PROPERTY( int           mode               READ mode          WRITE setMode )
    Q_PROPERTY( QVariantList controls            READ controlsV     WRITE setControlsV )
    
public:
    explicit DatabaseCommand_SetDynamicPlaylistRevision( QObject* parent = 0 )
    : DatabaseCommand_SetPlaylistRevision( parent )
    {}
    
    explicit DatabaseCommand_SetDynamicPlaylistRevision( const source_ptr& s,
                                                  const QString& playlistguid,
                                                  const QString& newrev,
                                                  const QString& oldrev,
                                                  const QStringList& orderedguids,
                                                  const QList<Tomahawk::plentry_ptr>& addedentries,
                                                  const QString& type,
                                                  GeneratorMode mode,
                                                  const QList< dyncontrol_ptr >& controls );
    
    explicit DatabaseCommand_SetDynamicPlaylistRevision( const source_ptr& s,
                                                  const QString& playlistguid,
                                                  const QString& newrev,
                                                  const QString& oldrev,
                                                  const QString& type,
                                                  GeneratorMode mode,
                                                  const QList< dyncontrol_ptr >& controls );
    
    QString commandname() const { return "setdynamicplaylistrevision"; }
    
    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }
    
    void setControlsV( const QVariantList& vlist )
    {
        m_controlsV = vlist;
    }
    
    QVariantList controlsV();
    
    QString type() const { return m_type; }
//     GeneratorMode mode() const { return m_mode; }
    int mode() const { return (int)m_mode; }
    
    void setType( const QString& type ) { m_type = type; }
//     void setMode( GeneratorMode mode ) { m_mode = mode; }
    void setMode( int mode ) { m_mode = (GeneratorMode)mode; }
    
private:
    QString m_type;
    GeneratorMode m_mode;
    QList< dyncontrol_ptr > m_controls;
    QList< QVariant > m_controlsV;
};

#endif // DATABASECOMMAND_SETDYNAMICPLAYLISTREVISION_H
