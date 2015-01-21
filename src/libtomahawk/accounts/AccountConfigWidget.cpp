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

#include "AccountConfigWidget.h"


#include "../utils/Logger.h"

#include <QMetaProperty>


AccountConfigWidget::AccountConfigWidget( QWidget* parent )
    : QWidget( parent )
{
}


void
AccountConfigWidget::checkForErrors()
{
}


QStringList
AccountConfigWidget::errors() const
{
    return m_errors;
}


void
AccountConfigWidget::resetErrors()
{
    m_errors.clear();
}


bool
AccountConfigWidget::settingsValid() const
{
    return m_errors.empty();
}


void
AccountConfigWidget::setDataWidgets( const QVariantList& dataWidgets )
{
    m_dataWidgets = dataWidgets;
}


QVariantMap
AccountConfigWidget::readData()
{

    QVariantMap saveData;
    foreach( const QVariant& dataWidget, m_dataWidgets )
    {
        QVariantMap data = dataWidget.toMap();

        QString widgetName = data["widget"].toString();
        QWidget* widget= findChild<QWidget*>( widgetName );

        QVariant value = widgetData( widget, data["property"].toString() );

        saveData[ data["name"].toString() ] = value;
    }

    return saveData;
}


QVariant
AccountConfigWidget::widgetData( QWidget* widget, const QString& property )
{
    for ( int i = 0; i < widget->metaObject()->propertyCount(); i++ )
    {
        if ( widget->metaObject()->property( i ).name() == property )
        {
            return widget->property( property.toLatin1() );
        }
    }

    return QVariant();
}


void
AccountConfigWidget::fillDataInWidgets( const QVariantMap& data )
{
    foreach(const QVariant& dataWidget, m_dataWidgets)
    {
        const QVariantMap m = dataWidget.toMap();
        QString widgetName = m["widget"].toString();
        QWidget* widget= findChild<QWidget*>( widgetName );
        if ( !widget )
        {
            tLog() << Q_FUNC_INFO << "Widget specified in resolver was not found:" << widgetName;
            Q_ASSERT(false);
            return;
        }

        QString propertyName = m["property"].toString();
        QString name = m["name"].toString();

        setWidgetData( data[ name ], widget, propertyName );
    }
}


void
AccountConfigWidget::setWidgetData( const QVariant& value, QWidget* widget, const QString& property )
{
    const QMetaObject *metaObject = widget->metaObject();
    for ( int i = 0; i < metaObject->propertyCount(); i++ )
    {
        const QMetaProperty &prop = metaObject->property( i );
        if ( prop.name() == property )
        {
            prop.write( widget, value );
            return;
        }
    }
}
