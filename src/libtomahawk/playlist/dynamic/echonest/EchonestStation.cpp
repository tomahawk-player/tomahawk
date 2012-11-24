#include "EchonestStation.h"
#include "playlist/dynamic/DynamicPlaylist.h"

#include "playlist/PlayableItem.h"
#include "audio/AudioEngine.h"

#include <echonest/Playlist.h>

namespace Tomahawk
{

EchonestStation::EchonestStation( PlayableProxyModel *model, dynplaylist_ptr playlist, QObject *parent )
    : QObject( parent )
    , m_name( model->sourceModel()->title() )
    , m_model( model )
    , m_playlist( playlist )
{
    connect(m_playlist->generator().data(), SIGNAL(controlAdded(const dyncontrol_ptr&)), SLOT(controlsChanged()));
}

QString EchonestStation::name() const
{
    return m_playlist->title();
}

void EchonestStation::setName(const QString &name)
{
    m_playlist->setTitle( name );
    emit nameChanged();
}

bool EchonestStation::configured()
{
    return true;
}

int EchonestStation::minTempo() const
{
}

int EchonestStation::maxTempo() const
{
}

void EchonestStation::playItem(int row)
{
    QModelIndex index = m_model->index( row, 0);
    if( index.isValid() ) {
        PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource(index) );
        if ( item && !item->query().isNull() )
        {
            m_model->setCurrentIndex( index );
            AudioEngine::instance()->playItem( m_model->playlistInterface(), item->query() );
        }
    }
}


void EchonestStation::setTempo(int min, int max)
{

}

void EchonestStation::controlsChanged()
{
    Q_ASSERT(false);
}

}

