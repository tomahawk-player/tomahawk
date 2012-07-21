#include "EchonestStation.h"
#include "dynamic/DynamicPlaylist.h"

#include "PlayableItem.h"
#include "audio/AudioEngine.h"

namespace Tomahawk
{

EchonestStation::EchonestStation( PlayableProxyModel *model, dynplaylist_ptr playlist, QObject *parent )
    : QObject( parent )
    , m_name( model->sourceModel()->title() )
    , m_model( model )
    , m_playlist( playlist )
{

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

Tomahawk::DynamicControl* EchonestStation::mainControl() {
    foreach(dyncontrol_ptr control, m_playlist->generator()->controls()) {
        qDebug() << "got control" << control->selectedType();
        if(control->selectedType() == "Artist" || control->selectedType() == "Style") {
            return control.data();
        }
    }
    return 0;
}

bool EchonestStation::configured()
{
    return mainControl() != 0;
}

void EchonestStation::playItem(int row)
{
    QModelIndex index( m_model->index( row, 0) );
    if( index.isValid() ) {
        PlayableItem* item = m_model->itemFromIndex( index );
        if ( item && !item->query().isNull() )
        {
            m_model->setCurrentIndex( index );
            AudioEngine::instance()->playItem( m_model->playlistInterface(), item->query() );
        }
    }
}

void EchonestStation::setMainControl(const QString &type)
{
    dyncontrol_ptr control = m_playlist->generator()->createControl("echonest");
    control->setSelectedType("Style");
    control->setMatch("1");
    control->setInput(type);
    qDebug() << "created control" << control->type() << control->selectedType() << control->match();
    m_playlist->generator()->generate(20);

    emit configuredChanged();
}

}

