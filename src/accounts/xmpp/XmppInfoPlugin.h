/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Dominik Schmidt <domme@tomahawk-player.org>
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef XMPPINFOPLUGIN_H
#define XMPPINFOPLUGIN_H

#include "infosystem/infosystem.h"

#include <QTimer>

namespace Jreen {
    namespace PubSub {
        class Manager;
    }
}

class XmppSipPlugin;

namespace Tomahawk {

    namespace InfoSystem {

        class XmppInfoPlugin  : public InfoPlugin
        {
            Q_OBJECT

        public:
            XmppInfoPlugin(XmppSipPlugin* parent);
            virtual ~XmppInfoPlugin();

        public slots:
            void notInCacheSlot( const Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );

        protected slots:
            void pushInfo( Tomahawk::InfoSystem::InfoPushData pushData );
            void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData );

        private slots:
            void audioStarted( const Tomahawk::InfoSystem::PushInfoPair& pushInfoPair );
            void audioStopped();
            void audioPaused();

        private:
            QWeakPointer< XmppSipPlugin > m_sipPlugin;
            Jreen::PubSub::Manager* m_pubSubManager;
            QTimer m_pauseTimer;
        };

    }

}

#endif // XMPPINFOPLUGIN_H
