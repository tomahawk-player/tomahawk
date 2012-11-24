#ifndef ECHONESTSTATION_H
#define ECHONESTSTATION_H

#include "playlist/PlayableProxyModel.h"
#include "playlist/dynamic/GeneratorInterface.h"

namespace Tomahawk
{
class EchonestStation: public QObject
{
    Q_OBJECT
    Q_ENUMS(StationType)
    Q_PROPERTY( QString name READ name WRITE setName NOTIFY nameChanged )
    Q_PROPERTY( bool configured READ configured NOTIFY configurationChanged )
    Q_PROPERTY( Tomahawk::DynamicControl* mainControl READ mainControl )
    Q_PROPERTY( int minTempo READ minTempo NOTIFY configurationChanged )
    Q_PROPERTY( int maxTempo READ maxTempo NOTIFY configurationChanged )

public:
    enum StationType {
        StationTypeStyle,
        StationTypeArtist
    };

    EchonestStation( PlayableProxyModel *model, dynplaylist_ptr playlist, QObject *parent = 0);

    QString name() const;
    void setName( const QString &name );

    Tomahawk::DynamicControl* mainControl();
    bool configured();

    int minTempo() const;
    int maxTempo() const;

public slots:
    void playItem( int row );

    void setMainControl(StationType type, const QString &value);
    void setTempo( int min, int max );

signals:
    void nameChanged();
    void configurationChanged();

private:
    dyncontrol_ptr findControl( const QString &selectedType, const QString &match ) const;

private slots:
    void controlsChanged();

private:
    QString m_name;
    PlayableProxyModel *m_model;
    dynplaylist_ptr m_playlist;
};
}

#endif
