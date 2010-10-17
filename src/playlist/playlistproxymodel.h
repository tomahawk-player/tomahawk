#ifndef PLAYLISTPROXYMODEL_H
#define PLAYLISTPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "tomahawk/playlistmodelinterface.h"
#include "playlistmodel.h"

class PlaylistProxyModel : public QSortFilterProxyModel, public PlaylistModelInterface
{
Q_OBJECT

public:
    explicit PlaylistProxyModel( QObject* parent = 0 );

    virtual void setSourceModel( PlaylistModel* sourceModel );

    virtual void addSource( const Tomahawk::source_ptr& source ) { m_model->addSource( source ); }
    virtual void removeSource( const Tomahawk::source_ptr& source ) { m_model->removeSource( source ); }

    virtual void setCurrentItem( const QModelIndex& index );
    virtual PlaylistItem* previousItem();
    virtual PlaylistItem* nextItem();
    virtual PlaylistItem* siblingItem( int itemsAway );

    virtual unsigned int sourceCount() { return m_model->sourceCount(); }
    virtual unsigned int collectionCount() { return m_model->collectionCount(); }
    virtual unsigned int trackCount()  { return rowCount(); }
    virtual unsigned int artistCount() { return 0; } // FIXME

    void setFilterRegExp( const QString& pattern );

    QPersistentModelIndex currentItem() const { return mapFromSource( m_model->currentItem() ); }

signals:
    void repeatModeChanged( PlaylistModelInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void numSourcesChanged( unsigned int i );
    void numTracksChanged( unsigned int i );
    void numArtistsChanged( unsigned int i );
    void numShownChanged( unsigned int i );

public slots:
    virtual void setRepeatMode( RepeatMode mode ) { m_repeatMode = mode; emit repeatModeChanged( mode ); }
    virtual void setShuffled( bool enabled ) { m_shuffled = enabled; emit shuffleModeChanged( enabled ); }

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;
    bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;

private:
    PlaylistModel* m_model;
    RepeatMode m_repeatMode;
    bool m_shuffled;
};

#endif // PLAYLISTPROXYMODEL_H
