#ifndef DATABASECOMMAND_ADDSOURCE_H
#define DATABASECOMMAND_ADDSOURCE_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_addSource : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_addSource( const QString& username, const QString& fname, QObject* parent = 0 );
    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return true; }
    virtual QString commandname() const { return "addsource"; }
signals:
    void done( unsigned int, const QString& );

private:
    QString m_username, m_fname;
};

#endif // DATABASECOMMAND_ADDSOURCE_H
