/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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
#ifndef RESOLVER_CONFIG_WRAPPER
#define RESOLVER_CONFIG_WRAPPER

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDebug>

class AccountConfigWidget;

class DelegateConfigWrapper : public QDialog
{
    Q_OBJECT
public:
    DelegateConfigWrapper( AccountConfigWidget* conf, QWidget* aboutWidget, const QString& title, QWidget* parent, Qt::WindowFlags flags = 0 );

    ~DelegateConfigWrapper() {}

    void setShowDelete( bool del );

    bool deleted() const { return m_deleted; }

public slots:
    void toggleOkButton( bool dataError );
    void closed( QAbstractButton* b );

    // we get a rejected() signal emitted if the user presses escape (and no clicked() signal )
    void rejected();

    void updateSizeHint();

signals:
    void closedWithDelete();

private slots:
    void aboutClicked( bool );

private:
    QDialogButtonBox* m_buttons;
    AccountConfigWidget* m_widget;
    QWidget* m_aboutW;
    QPushButton *m_okButton, *m_deleteButton;
    bool m_deleted;
};

#endif
