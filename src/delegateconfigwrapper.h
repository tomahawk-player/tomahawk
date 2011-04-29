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
#ifndef RESOLVER_CONFIG_WRAPPER
#define RESOLVER_CONFIG_WRAPPER

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>

class DelegateConfigWrapper : public QDialog
{
    Q_OBJECT
public:
    DelegateConfigWrapper( QWidget* conf, const QString& title, QWidget* parent ) : QDialog( parent ), m_widget( conf )
    {
        m_widget->setVisible( true );

        setWindowTitle( title );
        QVBoxLayout* v = new QVBoxLayout( this );
        v->addWidget( m_widget );

        QDialogButtonBox* buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this );
        connect( buttons, SIGNAL( clicked( QAbstractButton*)  ), this, SLOT( closed( QAbstractButton* ) ) );
        connect( this, SIGNAL( rejected() ), this, SLOT( rejected() ) );
        v->addWidget( buttons );

        setLayout( v );
    }
public slots:
    void closed( QAbstractButton* b )
    {
        // let the config widget live to see another day
        layout()->removeWidget( m_widget );
        m_widget->setParent( 0 );
        m_widget->setVisible( false );

        QDialogButtonBox* buttons = qobject_cast< QDialogButtonBox* >( sender() );
        if( buttons->standardButton( b ) == QDialogButtonBox::Ok )
            done( QDialog::Accepted );
        else
            done( QDialog::Rejected );
    }

    // we get a rejected() signal emitted if the user presses escape (and no clicked() signal )
    void rejected()
    {
        layout()->removeWidget( m_widget );
        m_widget->setParent( 0 );
        m_widget->setVisible( false );
    }

private:
    QWidget* m_widget;
};

#endif
