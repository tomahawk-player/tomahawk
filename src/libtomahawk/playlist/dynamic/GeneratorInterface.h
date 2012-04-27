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

#ifndef GENERATOR_INTERFACE_H
#define GENERATOR_INTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QStringList>

#include "Typedefs.h"
#include "Query.h"
#include "playlist/dynamic/DynamicControl.h"

#include "DllMacro.h"

namespace Tomahawk
{

/**
 * The abstract interface for Dynamic Playlist Generators. Generators have the following features:
 *      - They create new DynamicControls that are appropriate for the generator
 *      - They expose a list of controls that this generator currently is operating on
 *      - They have a mode of OnDemand or Static
 *
 *  And they generate tracks in two ways:
 *      - Statically (ask for X tracks, get X tracks)
 *      - On Demand (as for next track, ask for next track again, etc)
 */
class DLLEXPORT GeneratorInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString type READ type )
    /// oh qjson.
    Q_PROPERTY( int mode READ mode WRITE setMode )

public:
    // can't inline constructors/destructors for forward declared shared pointer types
    explicit GeneratorInterface( QObject* parent = 0 );
    virtual ~GeneratorInterface();

    // Can't make it pure otherwise we can't shove it in QVariants :-/
    // empty QString means use default
    /// The generator will keep track of all the controls it creates. No need to tell it about controls
    ///  you ask it to create
    virtual dyncontrol_ptr createControl( const QString& type = QString() );

    /// A logo to display for this generator, if it has one
    virtual QPixmap logo();

    /**
     * Generate tracks from the controls in this playlist. If this generator is in static
     *  mode, then it will return the desired number of tracks. If the generator is in OnDemand
     *  mode, this will do nothing.
     *
     * Connect to the generated() signal for the results.
     *
     */
    virtual void generate( int number = -1 ) { Q_UNUSED( number ); }

    /**
     * Starts an on demand session for this generator. Listen to the nextTrack() signal to get
     *  the first generated track
     */
    virtual void startOnDemand() {}

    /**
     * Get the next on demand track.
     * \param rating Rating from 1-5, -1 for none
     */
    virtual void fetchNext( int rating = -1 ) { Q_UNUSED( rating ) }

    /**
     * Return a sentence that describes this generator's controls. TODO english only ATM
     */
    virtual QString sentenceSummary() { return QString(); }

    /**
     * If an OnDemand playlist can be steered, this returns true.
     * If so, the generator should also provide a steering widget
     *  in steeringWidget()
     */
    virtual bool onDemandSteerable() const { return false; }

    /**
     * Returns a widget used to steer the OnDemand dynamic playlist.
     *  If this generator doesn't support this (and returns false for
     * \c onDemandSteerable) this will be null. The generator is responsible
     *  for reacting to changes in the widget.
     *
     * Steering widgets may emit a \c steeringChanged() signal, which will cause the model to toss any
     *  upcoming tracks and re-fetch them.
     *
     */
    virtual QWidget* steeringWidget() { return 0; }

    /// The type of this generator
    QString type() const { return m_type; }

    int mode() const { return (int)m_mode; }
    void setMode( int mode ) { m_mode = (GeneratorMode)mode; }

    // control functions
    QList< dyncontrol_ptr > controls();
    void addControl( const dyncontrol_ptr& control );
    void clearControls();
    void setControls( const QList< dyncontrol_ptr>& controls );
    void removeControl( const dyncontrol_ptr& control );

signals:
    void error( const QString& title, const QString& body);
    void generated( const QList< Tomahawk::query_ptr>& queries );
    void nextTrackGenerated( const Tomahawk::query_ptr& track );

protected:
    QString m_type;
    GeneratorMode m_mode;
    QList< dyncontrol_ptr > m_controls;
};

typedef QSharedPointer<GeneratorInterface> geninterface_ptr;

};

#endif
