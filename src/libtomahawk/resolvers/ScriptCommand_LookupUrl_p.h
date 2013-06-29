/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef SCRIPTCOMMAND_LOOKUPURL_P_H
#define SCRIPTCOMMAND_LOOKUPURL_P_H

#include "ScriptCommand_LookupUrl.h"

#include "ExternalResolver.h"

class ScriptCommand_LookupUrlPrivate
{
public:
    ScriptCommand_LookupUrlPrivate( ScriptCommand_LookupUrl* q, Tomahawk::ExternalResolver* _resolver, const QString& _url )
        : q_ptr ( q )
        , url( _url )
        , resolver( _resolver )
    {
    }
    ScriptCommand_LookupUrl* q_ptr;
    Q_DECLARE_PUBLIC ( ScriptCommand_LookupUrl )

private:
    QString url;
    Tomahawk::ExternalResolver* resolver;
};

#endif // SCRIPTCOMMAND_LOOKUPURL_P_H
