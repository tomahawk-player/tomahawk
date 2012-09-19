/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef XSPFUPDATER_H
#define XSPFUPDATER_H

#include "PlaylistUpdaterInterface.h"
#include "DllMacro.h"

class QTimer;
class QCheckBox;

namespace Tomahawk
{


class DLLEXPORT XspfUpdater : public PlaylistUpdaterInterface
{
    Q_OBJECT
public:
    XspfUpdater( const playlist_ptr& pl, int interval /* ms */, bool autoUpdate, const QString& xspfUrl );

    virtual ~XspfUpdater();

    virtual QString type() const { return "xspf"; }

#ifndef ENABLE_HEADLESS
    virtual QWidget* configurationWidget() const;
#endif

    bool autoUpdate() const { return m_autoUpdate; }

    void setInterval( int intervalMsecs ) ;
    int intervalMsecs() const { return m_timer->interval(); }

    bool canSubscribe() const { return true; }
    bool subscribed() const { return m_autoUpdate; }
    void setSubscribed( bool subscribed );

public slots:
    void updateNow();
    void setAutoUpdate( bool autoUpdate );

private slots:
    void playlistLoaded( const QList<Tomahawk::query_ptr> & );

private:
    QTimer* m_timer;
    bool m_autoUpdate;
    QString m_url;

#ifndef ENABLE_HEADLESS
    QCheckBox* m_toggleCheckbox;
#endif
};

class DLLEXPORT XspfUpdaterFactory : public PlaylistUpdaterFactory
{
public:
    XspfUpdaterFactory() {}

    virtual QString type() const { return "xspf"; }
    virtual PlaylistUpdaterInterface* create( const playlist_ptr& pl, const QVariantHash& settings );
};

}

#endif // XSPFUPDATER_H
