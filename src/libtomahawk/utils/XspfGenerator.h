/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef XSPFGENERATOR_H
#define XSPFGENERATOR_H

#include "Typedefs.h"

#include <QtCore/qobject.h>


class XSPFGenerator : public QObject
{
    Q_OBJECT
public:
    explicit XSPFGenerator( const Tomahawk::playlist_ptr& pl, QObject* parent = 0 );
    virtual ~XSPFGenerator();

signals:
    void generated( const QByteArray& xspf );

private slots:
    void generate();

private:
    Tomahawk::playlist_ptr m_playlist;
};

#endif // XSPFGENERATOR_H
