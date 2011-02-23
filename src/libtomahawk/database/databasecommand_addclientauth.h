/****************************************************************************************
 * Copyright (c) 2011 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef DATABASECOMMAND_ADDCLIENTAUTH_H
#define DATABASECOMMAND_ADDCLIENTAUTH_H

#include "databaseimpl.h"
#include "databasecommand.h"
#include "dllmacro.h"

#include <QObject>

class DLLEXPORT DatabaseCommand_AddClientAuth : public DatabaseCommand
{
    Q_OBJECT
public:
    explicit DatabaseCommand_AddClientAuth( QObject* parent = 0 )
            : DatabaseCommand( parent )
    {}

    explicit DatabaseCommand_AddClientAuth( const QString& clientToken, const QString& website, const QString& name, const QString& userAgent, QObject* parent = 0 );

    QString commandname() const { return "addclientauth"; }

    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return true; }
    
private:
    QString m_clientToken, m_website, m_name, m_userAgent;
};

#endif // DATABASECOMMAND_ADDCLIENTAUTH_H
