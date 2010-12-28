#ifndef DATABASECOMMAND_SETPLAYLISTREVISION_H
#define DATABASECOMMAND_SETPLAYLISTREVISION_H

#include "databasecommandloggable.h"
#include "databaseimpl.h"
#include "collection.h"
#include "playlist.h"

#include "dllmacro.h"

using namespace Tomahawk;

class DLLEXPORT DatabaseCommand_SetPlaylistRevision : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QString playlistguid      READ playlistguid  WRITE setPlaylistguid )
Q_PROPERTY( QString newrev            READ newrev        WRITE setNewrev )
Q_PROPERTY( QString oldrev            READ oldrev        WRITE setOldrev )
Q_PROPERTY( QVariantList orderedguids READ orderedguids  WRITE setOrderedguids )
Q_PROPERTY( QVariantList addedentries READ addedentriesV WRITE setAddedentriesV )

public:
    explicit DatabaseCommand_SetPlaylistRevision( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
        , m_applied( false )
    {}

    explicit DatabaseCommand_SetPlaylistRevision( const source_ptr& s,
                                                  const QString& playlistguid,
                                                  const QString& newrev,
                                                  const QString& oldrev,
                                                  const QStringList& orderedguids,
                                                  const QList<Tomahawk::plentry_ptr>& addedentries );

    QString commandname() const { return "setplaylistrevision"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }

    void setAddedentriesV( const QVariantList& vlist )
    {
        m_addedentries.clear();
        foreach( const QVariant& v, vlist )
        {
            PlaylistEntry * pep = new PlaylistEntry;
            QJson::QObjectHelper::qvariant2qobject( v.toMap(), pep );
            m_addedentries << plentry_ptr(pep);
        }
    }

    QVariantList addedentriesV() const
    {
        QVariantList vlist;
        foreach( const plentry_ptr& pe, m_addedentries )
        {
            QVariant v = QJson::QObjectHelper::qobject2qvariant( pe.data() );
            vlist << v;
        }
        return vlist;
    }

    void setPlaylistguid( const QString& s ) { m_playlistguid = s; }

    void setNewrev( const QString& s ) { m_newrev = s; }
    void setOldrev( const QString& s ) { m_oldrev = s; }
    QString newrev() const { return m_newrev; }
    QString oldrev() const { return m_oldrev; }
    QString playlistguid() const { return m_playlistguid; }

    void setOrderedguids( const QVariantList& l ) { m_orderedguids = l; }
    QVariantList orderedguids() const { return m_orderedguids; }

private:
    QString m_playlistguid;
    QString m_newrev, m_oldrev;
    QVariantList m_orderedguids;
    QStringList m_previous_rev_orderedguids;
    QList<Tomahawk::plentry_ptr> m_addedentries;
    bool m_applied;
    QMap<QString, Tomahawk::plentry_ptr> m_addedmap;
};

#endif // DATABASECOMMAND_SETPLAYLISTREVISION_H
