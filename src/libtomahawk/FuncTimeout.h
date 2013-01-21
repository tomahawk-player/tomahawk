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

#ifndef FUNCTIMEOUT_H
#define FUNCTIMEOUT_H

#include <QObject>
#include <QPointer>

#include "boost/function.hpp"
#include "boost/bind.hpp"

#include "DllMacro.h"

/*
    I want to do:
        QTimer::singleShot(1000, this, SLOT(doSomething(x)));
    instead, I'm doing:
        new FuncTimeout(1000, boost::bind(&MyClass::doSomething, this, x));

 */
namespace Tomahawk
{

class DLLEXPORT FuncTimeout : public QObject
{
Q_OBJECT

public:
    FuncTimeout( int ms, boost::function<void()> func, QObject* besafe );

    ~FuncTimeout();

public slots:
    void exec();

private:
    boost::function<void()> m_func;
    QPointer< QObject > m_watch;
};

}; // ns

#endif // FUNCTIMEOUT_H
