/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QModelIndex>

class QListWidgetItem;
class Ui_StackedSettingsDialog;
class SipPluginFactory;
class SipPlugin;
class SipModel;
class ResolversModel;
class QNetworkReply;

namespace Ui
{
    class SettingsDialog;
    class ProxyDialog;
}

class ProxyDialog : public QDialog
{
Q_OBJECT

public:
    explicit ProxyDialog( QWidget* parent = 0 );
    ~ProxyDialog() {};

    void saveSettings();

private slots:
    void proxyTypeChangedSlot( int index );
    
private:
    Ui::ProxyDialog* ui;
    QHash<int,int> m_forwardMap;
    QHash<int,int> m_backwardMap;
};

class SettingsDialog : public QDialog
{
Q_OBJECT

public:
    explicit SettingsDialog( QWidget* parent = 0 );
    ~SettingsDialog();

signals:
    void settingsChanged();

protected:
    void changeEvent( QEvent* e );

private slots:
    void onRejected();

    void showPathSelector();

    void toggleUpnp( bool preferStaticEnabled );
    void showProxySettings();

    void testLastFmLogin();
    void onLastFmFinished();

    void addScriptResolver();
    void scriptSelectionChanged();
    void removeScriptResolver();
    void openResolverConfig( const QString& );
    void sipItemClicked ( const QModelIndex& );
    void openSipConfig( SipPlugin* );
    void factoryActionTriggered ( bool );
    void sipFactoryClicked( SipPluginFactory* );
    void sipContextMenuRequest( const QPoint& );
    void sipPluginDeleted( bool );
    void sipPluginRowDeleted( bool );

    void updateScanOptionsView();
    
    // dialog slots
    void resolverConfigClosed( int value );
    void sipConfigClosed( int value );

    void changePage( QListWidgetItem*, QListWidgetItem* );

private:
    void createIcons();
    void setupSipButtons();

    Ui_StackedSettingsDialog* ui;

    ProxyDialog m_proxySettings;
    bool m_rejected;
    SipModel* m_sipModel;
    ResolversModel* m_resolversModel;
};

#endif // SETTINGSDIALOG_H
