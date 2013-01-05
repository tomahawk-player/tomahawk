/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef DYNAMIC_PLAYLIST_CONTROL
#define DYNAMIC_PLAYLIST_CONTROL

#include "Typedefs.h"


#include <QObject>
#include <QSharedPointer>
#include <QStringList>
#include <QWidget>

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
    Q_PROPERTY( QString type READ type WRITE setType ) // the generator type associated with this control
    Q_PROPERTY( QString id READ id WRITE setId )
    Q_PROPERTY( QString selectedType READ selectedType WRITE setSelectedType )
    Q_PROPERTY( QString match READ match WRITE setMatch )
    Q_PROPERTY( QString input READ input WRITE setInput )
    Q_PROPERTY( QString summary READ summary ) // a summary of the control in phrase form

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

    /// The user-readable match value, for showing in read-only playlists
    virtual QString matchString() const { Q_ASSERT( false ); return QString(); }

    /// the serializable value of the match
    virtual QString match() const  { Q_ASSERT( false ); return QString(); }
    /// the serializable value of the input
    virtual QString input() const { Q_ASSERT( false ); return QString(); }
    /// the user-readable summary phrase
    virtual QString summary() const { Q_ASSERT( false ); return QString(); }

    // used by JSON serialization
    virtual void setMatch( const QString& /*match*/ ) { Q_ASSERT( false ); }
    virtual void setInput( const QString& /*input*/ ) { Q_ASSERT( false ); }
    /// All the potential type selectors for this control
    QStringList typeSelectors() const { return m_typeSelectors; }

    QString id() {
        if( m_id.isEmpty() )
            m_id = uuid();
        return m_id;
    };
    void setId( const QString& id ) { m_id = id; }

    void setType( const QString& type ) { m_type = type; }
    QString type() const { return m_type; }

signals:
    void changed();

public slots:
    /**
     * Sets the type to the newly specified one. Note that this will update the matchSelector
     *  and inputField widgets, so you should fetch the new widgets for use immediately.
     */
    virtual void setSelectedType( const QString& selectedType ) { m_selectedType = selectedType; }

protected:
    // Private constructor, you can't make one. Get it from your Generator.
    explicit DynamicControl( const QString& selectedType, const QStringList& typeSelectors, QObject* parent = 0 );

private:
    QString m_type;
    QString m_selectedType;
    QStringList m_typeSelectors;
    QString m_id;
};

};

#endif
