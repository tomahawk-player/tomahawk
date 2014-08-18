/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,      Uwe L. Korn  <uwelk@xhochy.com>
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

#ifndef TOMAHAWK_TR1_FUNCTIONAL
#define TOMAHAWK_TR1_FUNCTIONAL

#if defined(_WEBSOCKETPP_CPP11_STL_) || defined(CXX_STD_FUNCTIONAL)
#include <functional>
using std::function;
using std::bind;
#else
#include <tr1/functional>
using std::tr1::function;
using std::tr1::bind;
#endif

#endif
