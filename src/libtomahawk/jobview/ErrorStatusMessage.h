/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
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

#ifndef ERRORSTATUSMESSAGE_H
#define ERRORSTATUSMESSAGE_H

#include "JobStatusItem.h"
#include "DllMacro.h"

#include <QPixmap>

class QTimer;

class DLLEXPORT ErrorStatusMessage : public JobStatusItem
{
    Q_OBJECT
public:
    explicit ErrorStatusMessage( const QString& errorMessage, int defaultTimeoutSecs = 8 );

    QString type() const { return "errormessage"; }
    QString rightColumnText() const { return QString(); }

    QPixmap icon() const;
    QString mainText() const;

    bool allowMultiLine() const { return true; }
private:
    QString m_message;
    QTimer* m_timer;

    static QPixmap* s_pixmap;
};

#endif // ERRORSTATUSMESSAGE_H
