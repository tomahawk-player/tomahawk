#ifndef DATABASECOMMAND_LOADFILE_H
#define DATABASECOMMAND_LOADFILE_H

#include <QObject>
#include <QVariantMap>
#include <QMap>

#include "databasecommand.h"

class DatabaseCommand_LoadFile : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_LoadFile( const QString& id, QObject* parent = 0 );
    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadfile"; }

signals:
    void result( const Tomahawk::result_ptr result );

private:
    QString m_id;
};

#endif // DATABASECOMMAND_LOADFILE_H
