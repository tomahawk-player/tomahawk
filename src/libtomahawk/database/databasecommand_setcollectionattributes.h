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

#ifndef DATABASECOMMAND_SAVEECHONESTCATALOG_H
#define DATABASECOMMAND_SAVEECHONESTCATALOG_H

#include "typedefs.h"
#include "databasecommandloggable.h"
#include <QByteArray>

class DatabaseCommand_SetCollectionAttributes : public DatabaseCommandLoggable
{
public:
    enum AttributeType {
        EchonestSongCatalog = 0,
        EchonestArtistCatalog = 1
    };

    DatabaseCommand_SetCollectionAttributes( const Tomahawk::source_ptr& source, AttributeType type, const QByteArray& id );
    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return true; }

    virtual QString commandname() const { return "saveechonestcatalog"; }

private:
    AttributeType m_type;
    QByteArray m_id;
};

#endif // DATABASECOMMAND_SAVEECHONESTCATALOG_H
