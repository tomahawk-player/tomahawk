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

#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QFrame>
#include <QTime>

#include "DllMacro.h"

class DLLEXPORT ElidedLabel : public QFrame
{
    Q_OBJECT
    Q_PROPERTY( QString text READ text WRITE setText NOTIFY textChanged )
    Q_PROPERTY( Qt::Alignment alignment READ alignment WRITE setAlignment )
    Q_PROPERTY( Qt::TextElideMode elideMode READ elideMode WRITE setElideMode )

public:
    explicit ElidedLabel( QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    explicit ElidedLabel( const QString& text, QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    virtual ~ElidedLabel();

    QString text() const;

    Qt::Alignment alignment() const;
    void setAlignment( Qt::Alignment alignment );

    Qt::TextElideMode elideMode() const;
    void setElideMode( Qt::TextElideMode mode );

    void setFont( const QFont& font );

    void setMargin( int margin );
    int margin() const;

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void init( const QString& txt = QString() );
    void updateLabel();

public slots:
    void setText( const QString& text );
    void setWordWrap( bool b ) { m_multiLine = b; }

signals:
    void clicked();
    void textChanged( const QString& text );

protected:
    virtual void changeEvent( QEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void paintEvent( QPaintEvent* event );

private:
    QTime m_time;
    QString m_text;
    Qt::Alignment m_align;
    Qt::TextElideMode m_mode;
    int m_margin;
    bool m_multiLine;
};

#endif // ELIDEDLABEL_H
