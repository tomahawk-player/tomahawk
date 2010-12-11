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
#include <tomahawk/typedefs.h>

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
    Q_PROPERTY( QString id READ id WRITE setId )
    Q_PROPERTY( QString selectedType READ selectedType WRITE setSelectedType )
    Q_PROPERTY( QString match READ match WRITE setMatch )
    Q_PROPERTY( QString input READ input WRITE setInput )
    
public:
    DynamicControl( const QStringList& typeSelectors = QStringList() );
    virtual ~DynamicControl();
    
    
    /// The current type of this control
    QString  selectedType() const { return m_selectedType; }
    /**
     * The match selector widget based on this control's type
     * 
     * The control manages the lifetime of the widget.
     */
    virtual QWidget* matchSelector()  { Q_ASSERT( false ); return 0; }
    /**
     * The input field widget that is associated with this type
     * 
     * The control manages the lifetime of the widget.
     */
    virtual QWidget* inputField()  { Q_ASSERT( false ); return 0;  }
    
    /// the serializable value of the match
    QString match() const  { Q_ASSERT( false ); return QString(); }
    /// the serializable value of the input
    QString input() const { Q_ASSERT( false ); return QString(); }
    
    // used by JSON serialization
    void setMatch( const QString& match ) { m_match = match; }
    void setInput( const QString& input ) { m_input = input; }
    
    /// All the potential type selectors for this control
    QStringList typeSelectors() const { return m_typeSelectors; }
    
    QString id() {
        if( m_id.isEmpty() )
            m_id = uuid();
        return m_id;
    };
    void setId( const QString& id ) { m_id = id; }
    
public slots:
    /**
     * Sets the type to the newly specified one. Note that this will update the matchSelector
     *  and inputField widgets, so you should fetch the new widgets for use immediately.
     */
    virtual void setSelectedType( const QString& selectedType ) { m_selectedType = selectedType; }
    
protected:
    // Private constructor, you can't make one. Get it from your Generator.
    explicit DynamicControl( const QString& selectedType, const QStringList& typeSelectors, QObject* parent = 0 );
    
    QString m_match;
    QString m_input;
    
private:
    QString m_selectedType;
    QStringList m_typeSelectors;
    QString m_id;
};

};

#endif
