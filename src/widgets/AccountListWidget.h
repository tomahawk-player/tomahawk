/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Teo Mrnjavac <teo@kde.org>
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

#ifndef ACCOUNTLISTWIDGET_H
#define ACCOUNTLISTWIDGET_H

#include "AccountModelFactoryProxy.h"

#include <QLabel>
#include <QVBoxLayout>

class AccountWidget;
class QPushButton;

class AccountListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AccountListWidget( AccountModelFactoryProxy *model, QWidget* parent = 0 );

private slots:
    void updateEntries( const QModelIndex& topLeft, const QModelIndex& bottomRight );
    void updateEntry( const QPersistentModelIndex& idx );
    void loadAllEntries();
    void insertEntries( const QModelIndex& parent, int start, int end );
    void removeEntries( const QModelIndex& parent, int start, int end );
    void toggleOnlineStateForAll();
    void updateToggleOnlineStateButton();

private:
    QHash< QPersistentModelIndex, QList< AccountWidget* > > m_entries;
    AccountModelFactoryProxy* m_model;
    QVBoxLayout* m_layout;
    QPushButton* m_toggleOnlineButton;
    bool         m_toggleOnlineButtonState;
};

#endif // ACCOUNTLISTWIDGET_H
