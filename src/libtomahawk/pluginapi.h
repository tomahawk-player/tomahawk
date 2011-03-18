/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef PLUGINAPI_H
#define PLUGINAPI_H

#include <QObject>
#include <QSharedPointer>

#include "collection.h"
#include "source.h"

#include "dllmacro.h"

/*
    This is the only API plugins have access to.
    This class must proxy calls to internal functions, because plugins can't
    get a pointer to any old object and start calling methods on it.
*/

namespace Tomahawk
{
class Resolver;
class Pipeline;

class DLLEXPORT PluginAPI : public QObject
{
Q_OBJECT

public:
    explicit PluginAPI( Pipeline * p );

    /// call every time new results are available for a running query
//    void reportResults( const QString& qid, const QList<QVariantMap>& results );

    /// add/remove sources (which have collections)
    void addSource( source_ptr s );
    void removeSource( source_ptr s );

    /// register object capable of searching
    void addResolver( Resolver * r );

    Pipeline * pipeline() const { return m_pipeline; }

private:
    Pipeline * m_pipeline;
};


}; //ns

#endif // PLUGINAPI_H
