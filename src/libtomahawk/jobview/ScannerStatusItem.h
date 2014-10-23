/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef SCANNERSTATUSITEM_H
#define SCANNERSTATUSITEM_H

#include "jobview/JobStatusItem.h"
#include "Query.h"

#include <QPixmap>
#include <QPointer>

class ScannerStatusItem : public JobStatusItem
{
    Q_OBJECT
public:
    explicit ScannerStatusItem();
    virtual ~ScannerStatusItem();

    virtual QString rightColumnText() const;
    virtual QString mainText() const;
    virtual QPixmap icon() const;

    virtual QString type() const { return "scanner"; }

    virtual bool collapseItem() const { return false; } // We can't collapse, since we use this meta-item instead of one per resolve

private slots:
    void onProgress( unsigned int files );

private:
    unsigned int m_scannedFiles;
};

class ScannerStatusManager : public QObject
{
    Q_OBJECT
public:
    explicit ScannerStatusManager( QObject* parent = 0 );
    virtual ~ScannerStatusManager() {}

private slots:
    void onProgress( unsigned int files );

private:
    QPointer<ScannerStatusItem> m_curItem;
};


#endif // PIPELINESTATUSITEM_H
