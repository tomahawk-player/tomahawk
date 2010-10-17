#ifndef DATABASECOMMAND_DIRMTIMES_H
#define DATABASECOMMAND_DIRMTIMES_H

#include <QObject>
#include <QVariantMap>
#include <QMap>

#include "databasecommand.h"

// Not loggable, mtimes only used to speed up our local scanner.

class DatabaseCommand_DirMtimes : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_DirMtimes( const QString& prefix = "", QObject* parent = 0 )
        : DatabaseCommand( parent ), m_prefix( prefix ), m_update( false )
    {}

    explicit DatabaseCommand_DirMtimes( QMap<QString, unsigned int> tosave, QObject* parent = 0 )
        : DatabaseCommand( parent ), m_update( true ), m_tosave( tosave )
    {}

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return m_update; }
    virtual QString commandname() const { return "dirmtimes"; }

signals:
    void done( const QMap<QString, unsigned int>& );

public slots:

private:
    void execSelect( DatabaseImpl* dbi );
    void execUpdate( DatabaseImpl* dbi );
    QString m_prefix;
    bool m_update;
    QMap<QString, unsigned int> m_tosave;
};

#endif // DATABASECOMMAND_DIRMTIMES_H
