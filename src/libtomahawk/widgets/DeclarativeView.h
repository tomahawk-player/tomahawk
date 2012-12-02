/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Michael Zanetti <mzanetti@kde.org>
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

#ifndef DECLARATIVEVIEW_H
#define DECLARATIVEVIEW_H

#include <QDeclarativeView>

class QAbstractItemModel;

/**
  * @class This is the main class for Tomahawk's declarative views
  *
  * DeclarativeView inherits from QDeclarativeView and registers some
  * common types, properties and functions used by all of Tomhawk's
  * declarative views:
  *
  * Registered Types:
  * - PlayableItem
  *
  * Set context properties:
  * - mainView: This view, so you can invoke this view's slots from QML
  * - defaultFontSize: system default font point size
  * - defaultFontHeight: system default font pixel height
  *
  * It also registers an albumart image provider. You can access album art
  * in QML with the source url "image://albumart/<coverid>".
  * The cover id can be obtained by the CoverIdRole in PlayableModels
  *
  * After subclassing this, all you have to do is call setSource() to
  * load the QML file and optionally setModel().
  */

namespace Tomahawk
{

class DeclarativeView: public QDeclarativeView
{
    Q_OBJECT
public:
    DeclarativeView(QWidget *parent = 0);
    ~DeclarativeView();
};

}
#endif
