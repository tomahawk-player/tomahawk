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
    void setLocal( const Tomahawk::source_ptr& localSrc );

    void removeAllRemote();

    QList<Tomahawk::source_ptr> sources( bool onlyOnline = false ) const;
    unsigned int count() const;

    Tomahawk::source_ptr get( const QString& username, const QString& friendlyName = QString() );
    Tomahawk::source_ptr get( unsigned int id ) const;

signals:
    void ready();

    void sourceAdded( const Tomahawk::source_ptr& );
    void sourceRemoved( const Tomahawk::source_ptr& );

private slots:
    void setSources( const QList<Tomahawk::source_ptr>& sources );
    void sourceSynced();
    
private:
    void loadSources();
    void add( const Tomahawk::source_ptr& source );

    QMap< QString, Tomahawk::source_ptr > m_sources;
    QMap< unsigned int, QString > m_sources_id2name;

    Tomahawk::source_ptr m_local;
    mutable QMutex m_mut; // mutable so const methods can use a lock
    
    static SourceList* s_instance;
};

#endif // SOURCELIST_H
