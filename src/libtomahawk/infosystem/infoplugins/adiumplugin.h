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

#ifndef ADIUMPLUGIN_H
#define ADIUMPLUGIN_H

#include "infosystem/infosystem.h"

#include <QObject>
#include <QVariant>

class QTimer;

namespace Tomahawk {

namespace InfoSystem {

class AdiumPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    AdiumPlugin();
    virtual ~AdiumPlugin();

protected slots:
    void getInfo( const QString caller, const InfoType type, const QVariant data, InfoCustomData customData );
    void pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input );

public slots:
    void namChangedSlot( QNetworkAccessManager* /*nam*/ ) {} // unused
    void notInCacheSlot( const Tomahawk::InfoSystem::InfoCriteriaHash /*criteria*/, const QString /*caller*/, const Tomahawk::InfoSystem::InfoType /*type*/, const QVariant /*input*/, const Tomahawk::InfoSystem::InfoCustomData /*customData*/ ) {} // unused

private slots:
    void clearStatus();

private:
    void settingsChanged();

    void audioStarted( const QVariant &input );
    void audioFinished( const QVariant &input );
    void audioStopped();
    void audioPaused();
    void audioResumed( const QVariant &input );

    QUrl openLinkFromHash( const InfoCriteriaHash& hash ) const;

    bool m_active;
    QString m_beforeStatus;
    QString m_afterStatus;

    QTimer* m_pauseTimer;

};


}

}

#endif // ADIUMPLUGIN_H
