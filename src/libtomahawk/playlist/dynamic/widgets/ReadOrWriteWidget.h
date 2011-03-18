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

#ifndef READ_OR_WRITE_WIDGET_H
#define READ_OR_WRITE_WIDGET_H

#include <QWidget>

class QStackedLayout;
class QLabel;

/**
 * Utility class for encapsulating either a QLabel (read-only) or an editable widget (combobox, lineedit, etc)
 */

class ReadOrWriteWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ReadOrWriteWidget( QWidget* writableWidget, bool writable, QWidget* parent = 0 );
    
    void setWritable( bool write );
    bool writable() const;
    
    void setWritableWidget( QWidget* w );
    QWidget* writableWidget() const;
    
    void setLabel( const QString& label );
    QString label() const;
    
    virtual QSize sizeHint() const;
    
private:
    QWidget* m_writableWidget;
    QLabel* m_label;
    QStackedLayout* m_layout;
    
    bool m_writable;
};

#endif
