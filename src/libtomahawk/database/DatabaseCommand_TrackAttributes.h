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

#ifndef DATABASECOMMAND_TRACKATTRIBUTES_H
#define DATABASECOMMAND_TRACKATTRIBUTES_H

#include "Typedefs.h"
#include "DatabaseCommand.h"
#include "DatabaseCommand_CollectionAttributes.h"
#include "DatabaseCommand_SetTrackAttributes.h"
#include <QByteArray>

class DatabaseCommand_TrackAttributes : public DatabaseCommand
{
    Q_OBJECT
public:

    // Get all tracks with this attribute
    DatabaseCommand_TrackAttributes( DatabaseCommand_SetTrackAttributes::AttributeType type );
    // Get the specific tracks with this attribute
    DatabaseCommand_TrackAttributes( DatabaseCommand_SetTrackAttributes::AttributeType type, const QList< Tomahawk::QID > ids );

    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return false; }

    virtual QString commandname() const { return "trackattributes"; }

signals:
    void trackAttributes( PairList );

private:
    DatabaseCommand_SetTrackAttributes::AttributeType m_type;
    QList< Tomahawk::QID > m_ids;
};

#endif
