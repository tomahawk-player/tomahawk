#include "EchonestStation.h"

#include "PlayableItem.h"
#include "audio/AudioEngine.h"

namespace Tomahawk
{

EchonestStation::EchonestStation( PlayableProxyModel *model, geninterface_ptr generator, QObject *parent )
    : QObject(parent)
    , m_model(model)
    , m_generator(generator)
{

}

Tomahawk::DynamicControl* EchonestStation::mainControl() {
    foreach(dyncontrol_ptr control, m_generator->controls()) {
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
    PlayableItem* item = m_model->itemFromIndex( index );
    if ( item && !item->query().isNull() )
    {
        m_model->setCurrentIndex( index );
        AudioEngine::instance()->playItem( m_model->playlistInterface(), item->query() );
    }
}

void EchonestStation::setMainControl(const QString &type)
{
    dyncontrol_ptr control = m_generator->createControl("echonest");
    control->setSelectedType("Style");
    control->setMatch("1");
    control->setInput(type);
    qDebug() << "created control" << control->type() << control->selectedType() << control->match();
    m_generator->generate(20);

    emit configuredChanged();
}

}

