#ifndef DATABASECOMMAND_PLAYBACKHISTORY_H
#define DATABASECOMMAND_PLAYBACKHISTORY_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "source.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_PlaybackHistory : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_PlaybackHistory( const Tomahawk::source_ptr& source, QObject* parent = 0 )
        : DatabaseCommand( parent )
    {
        setSource( source );
    }

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "playbackhistory"; }

signals:
    void tracks( const QList<Tomahawk::query_ptr>& queries );

private:
};

#endif // DATABASECOMMAND_PLAYBACKHISTORY_H
