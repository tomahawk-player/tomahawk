/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Dominik Schmidt <domme@tomahawk-player.org>
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

#ifndef ACCOUNTCONFIGWIDGET_H
#define ACCOUNTCONFIGWIDGET_H

#include <QWidget>
#include <QVariantMap>

#include "DllMacro.h"

class DLLEXPORT AccountConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AccountConfigWidget( QWidget* parent = nullptr );

    virtual void checkForErrors();
    virtual QStringList errors() const;
    virtual void resetErrors();

    virtual bool settingsValid() const;

    void setDataWidgets( const QVariantList& dataWidgets );
    QVariantMap readData();
    static QVariant widgetData( QWidget* widget, const QString& property );
    static void setWidgetData( const QVariant& value, QWidget* widget, const QString& property );
    void fillDataInWidgets( const QVariantMap& data );


protected:
    QStringList m_errors;
    QVariantList m_dataWidgets;
};

#endif // ACCOUNTCONFIGWIDGET_H
