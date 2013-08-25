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

#ifndef ACLREQUEST_P_H
#define ACLREQUEST_P_H

#include "AclRequest.h"

namespace Tomahawk {
namespace Network {
namespace ACL {

class AclRequestPrivate
{
public:
    AclRequestPrivate( AclRequest* q, const QString& _nodeid, const QString& _username, Tomahawk::ACLStatus::Type _status )
        : q_ptr( q )
        , nodeid( _nodeid )
        , username( _username )
        , status( _status )
    {
    }

    virtual ~AclRequestPrivate()
    {
    }

    AclRequest* q_ptr;
    Q_DECLARE_PUBLIC( AclRequest )
private:
    QString nodeid;
    QString username;
    Tomahawk::ACLStatus::Type status;
};

}
}
}

#endif // ACLREQUEST_P_H
