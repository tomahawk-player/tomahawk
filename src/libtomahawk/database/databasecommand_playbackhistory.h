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
        , m_amount( 0 )
    {
        setSource( source );
    }

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "playbackhistory"; }

    void setLimit( unsigned int amount ) { m_amount = amount; }

signals:
    void tracks( const QList<Tomahawk::query_ptr>& queries );

private:
    unsigned int m_amount;
};

#endif // DATABASECOMMAND_PLAYBACKHISTORY_H
