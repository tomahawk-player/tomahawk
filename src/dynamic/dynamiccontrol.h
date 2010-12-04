/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DYNAMIC_PLAYLIST_CONTROL
#define DYNAMIC_PLAYLIST_CONTROL

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QStringList>
#include <QtGui/QWidget>

namespace Tomahawk 
{

/**
 * A Dynamic Control is a single constraint that limits a dynamic playlist. Each generator creates controls specific to that generator.
 * Each control has 3 pieces:
 *  - Type (string selector for what this control is matching)
 *  - Match selector (how to match the type to the input)
 *  - Input field (the user input field).
 *  
 *  Each control also has a list of TypeSelectors that comes from the generator, and only one is selected at once.
 * 
 */
class DynamicControl : public QObject 
{
    Q_OBJECT
    Q_PROPERTY( QString selectedType READ selectedType WRITE setSelectedType )
    Q_PROPERTY( QStringList typeSelectors READ typeSelectors )
    
public:
    virtual ~DynamicControl();
    
    /// The current type of this control
    QString  selectedType() const { return m_selectedType; }
    /// The match selector widget based on this control's type
    virtual QWidget* matchSelector() { return 0; }
    /// The input field widget that is associated with this type
    virtual QWidget* inputField() { return 0; }
    
    /// All the potential type selectors for this control
    QStringList typeSelectors() const { return m_typeSelectors; }
    
public slots:
    /**
     * Sets the type to the newly specified one. Note that this will update the matchSelector
     *  and inputField widgets, so you should fetch the new widgets for use immediately.
     */
    virtual void setSelectedType( const QString& type ) { m_selectedType = type; }
    
protected:
    // Private constructor, you can't make one. Get it from your Generator.
    explicit DynamicControl( const QString& type, const QStringList& typeSelectors, QObject* parent = 0 ) : QObject( parent ), m_selectedType( type ), m_typeSelectors( typeSelectors ) {}
    
private:
    QString m_selectedType;
    QStringList m_typeSelectors;
};

typedef QSharedPointer<DynamicControl> dyncontrol_ptr;

};

#endif
