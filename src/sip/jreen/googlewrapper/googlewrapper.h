/*
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef GOOGLEWRAPPER_H
#define GOOGLEWRAPPER_H

#include "sip/jreen/jabber.h"

class SIPDLLEXPORT GoogleWrapperFactory : public SipPluginFactory
{
    Q_OBJECT
    Q_INTERFACES( SipPluginFactory )

public:
    GoogleWrapperFactory() {}
    virtual ~GoogleWrapperFactory() {}

    virtual QString prettyName() const { return "Google"; }
    virtual QString factoryId() const { return "sipgoogle"; }
    virtual QIcon icon() const;
    virtual SipPlugin* createPlugin( const QString& pluginId );
};

class SIPDLLEXPORT GoogleWrapper : public JabberPlugin
{
    Q_OBJECT
public:
  GoogleWrapper( const QString& pluginID );
  virtual ~GoogleWrapper() {}

  virtual const QString name() const { return QString( "Google" ); }
  virtual const QString friendlyName() const { return "Google"; }
  virtual QIcon icon() const;

};

#endif // GOOGLEWRAPPER_H
