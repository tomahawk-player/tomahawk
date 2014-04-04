/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef FILTERHEADER_H
#define FILTERHEADER_H

#include "widgets/BasicHeader.h"

#include "DllMacro.h"

#include <QTimer>

class QSearchField;

class DLLEXPORT FilterHeader : public BasicHeader
{
    Q_OBJECT
  public:
    explicit FilterHeader( QWidget* parent = 0 );
    virtual ~FilterHeader();

  public slots:
    void setFilter( const QString& filter );

  signals:
    void filterTextChanged( const QString& filter );

  private slots:
    void onFilterEdited();
    void applyFilter();

  protected:
    QSearchField* m_filterField;

  private:
    QString m_filter;
    QTimer m_filterTimer;
};

#endif // FILTERHEADER_H
