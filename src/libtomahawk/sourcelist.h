#ifndef SOURCELIST_H
#define SOURCELIST_H

#include <QObject>
#include <QMutex>
#include <QMap>

#include "source.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT SourceList : public QObject
{
Q_OBJECT

public:
    static SourceList* instance();

    explicit SourceList( QObject* parent = 0 );

    const Tomahawk::source_ptr& getLocal();
    void add( const Tomahawk::source_ptr& s );
    void remove( const Tomahawk::source_ptr& s );
    void remove( Tomahawk::Source* s );
    void removeAllRemote();

    QList<Tomahawk::source_ptr> sources() const;
    Tomahawk::source_ptr get( const QString& username ) const;
    Tomahawk::source_ptr get( unsigned int id ) const;
    unsigned int count() const;

signals:
    void sourceAdded( const Tomahawk::source_ptr& );
    void sourceRemoved( const Tomahawk::source_ptr& );

private:
    QMap<QString, Tomahawk::source_ptr > m_sources;
    QMap<unsigned int, QString> m_sources_id2name;

    Tomahawk::source_ptr m_local;
    mutable QMutex m_mut; // mutable so const methods can use a lock
    
    static SourceList* s_instance;
};

#endif // SOURCELIST_H
