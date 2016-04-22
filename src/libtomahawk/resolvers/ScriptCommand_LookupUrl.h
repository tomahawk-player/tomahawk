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

#ifndef SCRIPTCOMMAND_LOOKUPURL_H
#define SCRIPTCOMMAND_LOOKUPURL_H

#include "ScriptCommand.h"

#include "DllMacro.h"
#include "Typedefs.h"

#include <QVariant>

namespace Tomahawk
{

class ScriptCommand_LookupUrlPrivate;
class ExternalResolver;

class DLLEXPORT ScriptCommand_LookupUrl : public ScriptCommand
{
    Q_OBJECT
public:
    explicit ScriptCommand_LookupUrl( Tomahawk::ExternalResolver* resolver,
                                      const QString& url,
                                      QObject* parent = nullptr );
    virtual ~ScriptCommand_LookupUrl();

    void enqueue();

signals:
    void information( const QString& url, const QSharedPointer<QObject>& variant );
    void done() override;

protected:
    void exec() override;
    void reportFailure() override;

private slots:
    void onResolverDone( const QString& url, const QSharedPointer<QObject>& information );

private:
    Q_DECLARE_PRIVATE( ScriptCommand_LookupUrl )
    ScriptCommand_LookupUrlPrivate* d_ptr;
};

} // ns: Tomahawk

#endif // SCRIPTCOMMAND_LOOKUPURL_H
