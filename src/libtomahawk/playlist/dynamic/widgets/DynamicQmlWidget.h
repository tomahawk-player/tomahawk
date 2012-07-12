/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Michael Zanetti <mzanetti@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DYNAMIC_QML_WIDGET_H
#define DYNAMIC_QML_WIDGET_H

#include "ViewPage.h"
#include "Typedefs.h"

#include <QDeclarativeView>
#include <QDeclarativeImageProvider>

class PlayableProxyModel;

namespace Tomahawk
{

class DynamicModel;

class DynamicQmlWidget : public QDeclarativeView, public Tomahawk::ViewPage, public QDeclarativeImageProvider
{
Q_OBJECT
public:
    explicit DynamicQmlWidget( const dynplaylist_ptr& playlist, QWidget* parent = 0 );
    virtual ~DynamicQmlWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const;
    virtual QString description() const;
    virtual QPixmap pixmap() const;

    virtual bool showModes() const { return true; }
    virtual bool showFilter() const { return true; }

    virtual bool jumpToCurrentTrack();

    QPixmap requestPixmap( const QString &id, QSize *size, const QSize &requestedSize );


private slots:
    void currentItemChanged( const QPersistentModelIndex &currentIndex );
    void tracksGenerated( const QList< Tomahawk::query_ptr>& queries );
    void onRevisionLoaded( Tomahawk::DynamicPlaylistRevision );
private:
    DynamicModel* m_model;
    PlayableProxyModel* m_proxyModel;

    dynplaylist_ptr m_playlist;
};

}

#include "dynamic/GeneratorInterface.h"

namespace Tomahawk
{
class EchonestStation: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured NOTIFY configuredChanged)
    Q_PROPERTY(Tomahawk::DynamicControl* mainControl READ mainControl)

public:
    EchonestStation(geninterface_ptr generator, QObject *parent = 0) : QObject(parent), m_generator(generator) {}

    Tomahawk::DynamicControl* mainControl() {
        foreach(dyncontrol_ptr control, m_generator->controls()) {
            qDebug() << "got control" << control->selectedType();
            if(control->selectedType() == "Artist" || control->selectedType() == "Style") {
                return control.data();
            }
        }
        return 0;
    }

    bool configured() { return mainControl() != 0; }

    Q_INVOKABLE void setMainControl(const QString &type) {
        dyncontrol_ptr control = m_generator->createControl("echonest");
        control->setSelectedType("Style");
        control->setMatch("1");
        control->setInput(type);
        qDebug() << "created control" << control->type() << control->selectedType() << control->match();
        m_generator->generate(20);

        emit configuredChanged();
    }

signals:
    void configuredChanged();

private:
    geninterface_ptr m_generator;
};
}

namespace Tomahawk
{

class ControlModel: public QAbstractListModel
{
    Q_OBJECT
public:
    ControlModel(geninterface_ptr generator, QObject *parent = 0): QAbstractListModel(parent), m_generator(generator) {
        connect(generator.data(), SIGNAL(controlAdded(const dyncontrol_ptr&)), SLOT(controlAdded()));
    }

    int rowCount(const QModelIndex &parent) const { return m_generator->controls().size(); }
    QVariant data(const QModelIndex &index, int role) const {
        return "blabla";
    }
    Q_INVOKABLE Tomahawk::DynamicControl *controlAt( int index ) { qDebug() << "returning" << m_generator->controls().at(index).data(); return m_generator->controls().at(index).data(); }

private slots:
    void controlAdded() {
        qDebug() << "control added";
        beginInsertRows(QModelIndex(), m_generator->controls().size() - 1, m_generator->controls().size() - 1);
        endInsertRows();
    }
private:
    geninterface_ptr m_generator;

};

}
#endif // DYNAMIC_QML_WIDGET_H
