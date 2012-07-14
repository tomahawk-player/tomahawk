#ifndef ECHONESTSTATION_H
#define ECHONESTSTATION_H

#include "PlayableProxyModel.h"
#include "dynamic/GeneratorInterface.h"

namespace Tomahawk
{
class EchonestStation: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured NOTIFY configuredChanged)
    Q_PROPERTY(Tomahawk::DynamicControl* mainControl READ mainControl)

public:
    EchonestStation( PlayableProxyModel *model, geninterface_ptr generator, QObject *parent = 0);

    Tomahawk::DynamicControl* mainControl();
    bool configured();

public slots:
    void playItem( int row );

    void setMainControl(const QString &type);

signals:
    void configuredChanged();

private:
    PlayableProxyModel *m_model;
    geninterface_ptr m_generator;
};
}

#endif
