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

#ifndef DATABASE_GENERATOR_H
#define DATABASE_GENERATOR_H

#include <stdexcept>

#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/dynamic/GeneratorFactory.h"
#include "playlist/dynamic/DynamicControl.h"
#include "database/DatabaseCommand_GenericSelect.h"

#include <QPixmap>

#include "DllMacro.h"

namespace Tomahawk
{

    class DLLEXPORT DatabaseFactory : public GeneratorFactoryInterface
    {
    public:
        DatabaseFactory() {}

        virtual GeneratorInterface* create();
        virtual dyncontrol_ptr createControl( const QString& controlType = QString() );

        // TO create a special SQL resolver that consists of a pre-baked SQL query and a description of it
        virtual dyncontrol_ptr createControl( const QString& sql, DatabaseCommand_GenericSelect::QueryType type, const QString& summary );

        virtual QStringList typeSelectors() const;
    };

    /**
     * Generator based on the database. Can filter the database based on some user-controllable options,
     *  or just be the front-facing part of any given SQL query to fake an interesting read-only playlist.
     */
    class DatabaseGenerator : public GeneratorInterface
    {
        Q_OBJECT
    public:
        explicit DatabaseGenerator( QObject* parent = 0 );
        virtual ~DatabaseGenerator();

        virtual dyncontrol_ptr createControl( const QString& type = QString() );
        virtual dyncontrol_ptr createControl( const QString& sql, DatabaseCommand_GenericSelect::QueryType type, const QString& summary );

        virtual QPixmap logo();
        virtual void generate ( int number = -1 );
        virtual void startOnDemand();
        virtual void fetchNext( int rating = -1 );
        virtual QString sentenceSummary();
        virtual bool onDemandSteerable() const { return false; }
        virtual QWidget* steeringWidget() { return 0; }

    private slots:
        void tracksGenerated( const QList< Tomahawk::query_ptr >& tracks );
        void dynamicStarted();
        void dynamicFetched();

    private:
        QPixmap m_logo;
    };

};

#endif
