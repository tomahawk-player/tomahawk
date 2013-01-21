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

#ifndef TWITTERINFOPLUGIN_H
#define TWITTERINFOPLUGIN_H

#include "infosystem/InfoSystem.h"
#include "accounts/twitter/TomahawkOAuthTwitter.h"

#include <QTweetLib/qtweetuser.h>
#include <QTweetLib/qtweetstatus.h>
#include <QTweetLib/qtweetnetbase.h>

namespace Tomahawk {

    namespace Accounts {
        class TwitterAccount;
    }
    
    namespace InfoSystem {

        class TwitterInfoPlugin  : public InfoPlugin
        {
            Q_OBJECT

        public:
            TwitterInfoPlugin( Tomahawk::Accounts::TwitterAccount* account );
            virtual ~TwitterInfoPlugin();
            
        public slots:
            void notInCacheSlot( const Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
            {
                Q_UNUSED( criteria );
                Q_UNUSED( requestData );
            }

        protected slots:
            void init();
            void pushInfo( Tomahawk::InfoSystem::InfoPushData pushData );
            void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
            {
                Q_UNUSED( requestData );
            }

        private slots:
            void connectAuthVerifyReply( const QTweetUser &user );
            void postLovedStatusUpdateReply( const QTweetStatus& status );
            void postLovedStatusUpdateError( QTweetNetBase::ErrorCode code, const QString& errorMsg );
            
        private:
            bool refreshTwitterAuth();
            bool isValid() const;
            
            Tomahawk::Accounts::TwitterAccount* m_account;
            QPointer< TomahawkOAuthTwitter > m_twitterAuth;
        };

    }

}

#endif // TWITTERINFOPLUGIN_H

struct A;
