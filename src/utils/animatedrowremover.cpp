#include "animatedrowremover.h"

#include <QDebug>
#include <QModelIndex>
#include <QSortFilterProxyModel>

AnimatedRowRemover::AnimatedRowRemover( PlaylistView* view, PlaylistModel* model, const QList<PlaylistItem*>& items, QObject* parent )
    : QTimeLine( 1000 )
    , m_view( view )
    , m_model( model )
    , m_items( items )
{
    setFrameRange( 100, 0 );
}


AnimatedRowRemover::~AnimatedRowRemover()
{
    m_view->delegate()->setRemovalProgress( 100 );
}


void AnimatedRowRemover::start( bool animate )
{
    if ( m_items.length() == 0 )
    {
        qDebug() << "Not animating: no rows to remove.";
        emit finished();
        deleteLater();
        return;
    }

    connect( this, SIGNAL( frameChanged( int ) ), SLOT( trigger( int ) ) );
    connect( this, SIGNAL( finished() ), SLOT( onFinished() ) );

    if ( animate )
        QTimeLine::start();
    else
        onFinished();
}


void
AnimatedRowRemover::trigger( int step )
{
    m_view->setUpdatesEnabled( false );
    m_view->delegate()->setRemovalProgress( step );
    m_view->setUpdatesEnabled( true );
}


void
AnimatedRowRemover::onFinished()
{
    qDebug() << "Starting removal";

//    m_view->setProgressStarted( m_items.length() );
//    QSortFilterProxyModel* p = ((QSortFilterProxyModel*)( m_view->model() ));

/*    foreach ( PlaylistItem* item, m_items )
    {
        m_model->removeRow( item->columns().at( 0 )->row() );
    }*/

    int s = m_items.at( 0 )->columns().at( 0 )->row();
    int e = m_items.at( 0 )->columns().at( 0 )->row();
    int j = 0;
    foreach ( PlaylistItem* item, m_items )
    {
        j++;
        int currentRow = item->columns().at( 0 )->row();
        if ( currentRow == s - 1 )
            s--;

        if ( s >= 0 && ( s != currentRow || ( e - s ) > 1000 ) )
        {
//            m_view->setProgressCompletion( j );

            qDebug() << "Removing:" << s << e << currentRow;
            m_model->removeRows( s, e - s + 1 );
            qApp->processEvents();
            s = currentRow;
            e = currentRow - 1;
        }
    }

    qDebug() << "Removing:" << s << e;
    m_model->removeRows( s, e - s + 1 );
    qDebug() << "Removing done";

//    m_view->setProgressEnded();

    deleteLater();
}
