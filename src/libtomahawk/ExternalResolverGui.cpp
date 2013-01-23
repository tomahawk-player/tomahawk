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

#include "ExternalResolverGui.h"


#include "Source.h"
#include "utils/Logger.h"
#include "accounts/AccountConfigWidget.h"

#include <QMetaProperty>
#include <QBuffer>
#include <QDir>
#include <QIcon>
#include <QWidget>
#include <QUiLoader>
#include <QBoxLayout>

Tomahawk::ExternalResolverGui::ExternalResolverGui(const QString& filePath)
    : Tomahawk::ExternalResolver(filePath)
{
}


QVariant
Tomahawk::ExternalResolverGui::configMsgFromWidget( QWidget* w )
{
    if( !w )
        return QVariant();

    // generate a qvariantmap of all the widgets in the hierarchy, and for each one include the list of properties and values
    QVariantMap widgetMap;
    addChildProperties( w, widgetMap );
//     qDebug() << "Generated widget variant:" << widgetMap;
    return widgetMap;
}


void
Tomahawk::ExternalResolverGui::addChildProperties( QObject* widget, QVariantMap& m )
{
    // recursively add all properties of this widget to the map, then repeat on all children.
    // bare QWidgets are boring---so skip them! They have no input that the user can set.
    if( !widget || !widget->isWidgetType() )
        return;

    if( qstrcmp( widget->metaObject()->className(), "QWidget" ) != 0 )
    {
//         qDebug() << "Adding properties for this:" << widget->metaObject()->className();
        // add this widget's properties
        QVariantMap props;
        for( int i = 0; i < widget->metaObject()->propertyCount(); i++ )
        {
            QString prop = widget->metaObject()->property( i ).name();
            QVariant val = widget->property( prop.toLatin1() );
            // clean up for QJson....
            if( val.canConvert< QPixmap >() || val.canConvert< QImage >() || val.canConvert< QIcon >() )
                continue;
            props[ prop ] = val.toString();
//             qDebug() << QString( "%1: %2" ).arg( prop ).arg( props[ prop ].toString() );
        }
        m[ widget->objectName() ] = props;
    }
    // and recurse
    foreach( QObject* child, widget->children() )
        addChildProperties( child, m );
}


AccountConfigWidget*
Tomahawk::ExternalResolverGui::widgetFromData( QByteArray& data, QWidget* parent )
{
    if( data.isEmpty() )
        return 0;

    AccountConfigWidget* configWidget = new AccountConfigWidget( parent );

    QUiLoader l;
    QBuffer b( &data );
    QWidget* w = l.load( &b, configWidget );

    // HACK: proper way would be to create a designer plugin for this widget type
    configWidget->setLayout( new QBoxLayout( QBoxLayout::TopToBottom ) );
    configWidget->layout()->addWidget( w );

#ifdef Q_OS_MAC
    w->setContentsMargins( 12, 12, 12, 12 );
#else
    w->setContentsMargins( 6, 6, 6, 6 );
#endif

    return configWidget;
}


QByteArray
Tomahawk::ExternalResolverGui::fixDataImagePaths( const QByteArray& data, bool compressed, const QVariantMap& images )
{
    // with a list of images and image data, write each to a temp file, replace the path in the .ui file with the temp file path
    QString uiFile = QString::fromUtf8( data );
    foreach( const QString& filename, images.keys() )
    {
        if( !uiFile.contains( filename ) ) // make sure the image is used
            continue;

        QString fullPath = QDir::tempPath() + "/" + filename;
        QFile imgF( fullPath );
        if( !imgF.open( QIODevice::WriteOnly ) )
        {
            qWarning() << "Failed to write to temporary image in UI file:" << filename << fullPath;
            continue;
        }
        QByteArray data = images[ filename ].toByteArray();

//        qDebug() << "expanding data:" << data << compressed;
        data = compressed ? qUncompress( QByteArray::fromBase64( data ) ) : QByteArray::fromBase64( data );
        imgF.write( data );
        imgF.close();

        // replace the path to the image with the real path
        uiFile.replace( filename, fullPath );
    }
    return uiFile.toUtf8();
}
