/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef DATABASECOMMAND_SHARETRACK_H
#define DATABASECOMMAND_SHARETRACK_H

#include <QObject>
#include <QVariantMap>

#include "database/DatabaseCommand_SocialAction.h"
#include "SourceList.h"
#include "Typedefs.h"

#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT DatabaseCommand_ShareTrack : public DatabaseCommand_SocialAction
{
    Q_OBJECT
    Q_PROPERTY( QString recipient   READ recipient  WRITE setRecipient )

public:
    explicit DatabaseCommand_ShareTrack( QObject* parent = 0 );

    explicit DatabaseCommand_ShareTrack( const Tomahawk::trackdata_ptr& track,
                                         const QString& recipientDbid,
                                         QObject* parent = 0 );

    virtual QString commandname() const;

    virtual void exec( DatabaseImpl* );
    virtual void postCommitHook();

    virtual bool doesMutates() const;
    virtual bool singletonCmd() const;
    virtual bool localOnly() const;
    virtual bool groupable() const;

    virtual QString recipient() const;
    virtual void setRecipient( const QString& s );

private:
    QString m_recipient;
};

}

#endif // DATABASECOMMAND_SHARETRACK_H
