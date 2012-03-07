/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef EXTERNALRESOLVER_H
#define EXTERNALRESOLVER_H


#include "query.h"
#include "dllmacro.h"
#include "resolver.h"

#include <boost/function.hpp>

#include <QObject>

class QWidget;

namespace Tomahawk
{

/**
 * Generic resolver object, used to manage a resolver that Tomahawk knows about
 *
 * You *must* start() a resolver after creating an ExternalResolver in order to use it,
 * otherwise it will not do anything.
 */
class DLLEXPORT ExternalResolver : public Resolver
{
Q_OBJECT

public:
    enum ErrorState {
        NoError,
        FileNotFound,
        FailedToLoad
    };

    ExternalResolver( const QString& filePath ) { m_filePath = filePath; }

    virtual QString filePath() const { return m_filePath; }

    virtual void saveConfig() = 0;

    virtual void reload() {} // Reloads from file (especially useful to check if file now exists)
    virtual ErrorState error() const;
    virtual bool running() const = 0;

public slots:
    virtual void start() = 0;
    virtual void stop() = 0;

signals:
    void changed(); // if config widget was added/removed, name changed, etc

protected:
    void setFilePath( const QString& path ) { m_filePath = path; }

private:
    QString m_filePath;
};

}; //ns

#endif // EXTERNALESOLVER_H
