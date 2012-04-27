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

#ifndef DATABASECOMMAND_SETCOLLECTIONATTRIBUTES
#define DATABASECOMMAND_SETCOLLECTIONATTRIBUTES

#include "Typedefs.h"
#include "DatabaseCommandLoggable.h"
#include <QByteArray>

class DatabaseCommand_SetCollectionAttributes : public DatabaseCommandLoggable
{
    Q_OBJECT
    Q_PROPERTY( QByteArray     id                    READ id          WRITE setId )
    Q_PROPERTY( int           type                   READ type        WRITE setType )
    Q_PROPERTY( bool          del                    READ del         WRITE setDel )

public:
    enum AttributeType {
        EchonestSongCatalog = 0,
        EchonestArtistCatalog = 1
    };

    DatabaseCommand_SetCollectionAttributes( AttributeType type, const QByteArray& id );
    // Delete all attributes for the source+type
    DatabaseCommand_SetCollectionAttributes( AttributeType type, bool toDelete );
    DatabaseCommand_SetCollectionAttributes() : m_delete( false ) {} // JSON
    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return true; }
    virtual void postCommitHook();

    virtual QString commandname() const { return "setcollectionattributes"; }

    void setId( const QByteArray& id ) { m_id = id; }
    QByteArray id() const { return m_id; }

    void setType( int type ) { m_type = (AttributeType)type; }
    int type() const { return (int)m_type; }

    void setDel( bool del ) { m_delete = del; }
    bool del() const { return m_delete; }
private:
    bool m_delete;
    AttributeType m_type;
    QByteArray m_id;
};

#endif // DATABASECOMMAND_SETCOLLECTIONATTRIBUTES
