/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef JSRESOLVER_P_H
#define JSRESOLVER_P_H

#include "JSResolver.h"

#include "JSResolverHelper.h"
#include "JSInfoSystemHelper.h"
#include "database/fuzzyindex/FuzzyIndex.h"

namespace Tomahawk
{

class JSResolverPrivate
{
    friend class JSResolverHelper;
public:
    JSResolverPrivate( JSResolver* q, const QString& pAccountId, const QString& scriptPath, const QStringList& additionalScriptPaths )
        : q_ptr ( q )
        , accountId( pAccountId )
        , ready( false )
        , stopped( true )
        , error( Tomahawk::ExternalResolver::NoError )
        , resolverHelper( new JSResolverHelper( scriptPath, q ) )
        // TODO: be smarter about this, only instantiate this if the resolver supports infoplugins
        , infoSystemHelper( nullptr )
        , requiredScriptPaths( additionalScriptPaths )
    {
    }
    JSResolver* q_ptr;
    Q_DECLARE_PUBLIC ( JSResolver )

private:
    QString accountId;
    QString name;
    QPixmap icon;
    unsigned int weight, timeout;
    Tomahawk::ExternalResolverGui::Capabilities capabilities;

    bool ready;
    bool stopped;
    Tomahawk::ExternalResolver::ErrorState error;

    JSResolverHelper* resolverHelper;
    JSInfoSystemHelper* infoSystemHelper;
    QScopedPointer<FuzzyIndex> fuzzyIndex;
    QPointer< AccountConfigWidget > configWidget;
    QList< QVariant > dataWidgets;
    QStringList requiredScriptPaths;

    JSPlugin* scriptPlugin;
};

} // ns: Tomahawk

#endif // JSRESOLVER_P_H
