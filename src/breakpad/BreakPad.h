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

#include <QString>

#ifdef __APPLE__
#   include "client/mac/handler/exception_handler.h"
#elif defined WIN32
#   include "client/windows/handler/exception_handler.h"
#elif defined __linux__
#   include "client/linux/handler/exception_handler.h"
#endif

class BreakPad : public google_breakpad::ExceptionHandler
{
    const char* m_productName; // yes! It MUST be const char[]
    const char* m_crashReporter; // again, const char[]

public:
    BreakPad( const QString& dump_write_dirpath, bool active );

    ~BreakPad()
    {}

    static void setActive( bool enabled );
    static bool isActive();

    void setProductName( const char* s ) { m_productName = s; };
    const char* productName() const { return m_productName; }

    void setCrashReporter( const char* s ) { m_crashReporter = s; };
    const char* crashReporter() const { return m_crashReporter; }
};

#undef char
