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

#ifndef ACCOUNTWIDGET_H
#define ACCOUNTWIDGET_H

#include <QWidget>
#include <QPersistentModelIndex>

class AnimatedSpinner;
class ElidedLabel;
class SlideSwitchButton;
class UnstyledFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

class AccountWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AccountWidget( QWidget* parent = 0 );

    virtual ~AccountWidget();

    void update( const QPersistentModelIndex& idx, int accountIdx );
    void setupConnections( const QPersistentModelIndex& idx, int accountIdx );

    void setConnectionState( bool state );
    bool connectionState() const;

private slots:
    void changeAccountConnectionState( bool connected );
    void sendInvite();
    void onInviteSentSuccess( const QString& inviteId );
    void onInviteSentFailure( const QString& inviteId );
    void clearInviteWidgets();
    void setInviteWidgetsEnabled( bool enabled );

private:
    QLabel*            m_imageLabel;
    ElidedLabel*       m_idLabel;
    QWidget*           m_spinnerWidget;
    AnimatedSpinner*   m_spinner;
    SlideSwitchButton* m_statusToggle;
    QLineEdit*         m_inviteEdit;
    QPushButton*       m_inviteButton;
    UnstyledFrame*     m_inviteContainer;
    QLabel*            m_addAccountIcon;

    QPersistentModelIndex m_myFactoryIdx;
    int                   m_myAccountIdx;
};

#endif // ACCOUNTWIDGET_H
