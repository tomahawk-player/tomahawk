/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TOMAHAWK_APIV2_ACLJOBSTATUSITEM_H
#define TOMAHAWK_APIV2_ACLJOBSTATUSITEM_H

#include "jobview/JobStatusItem.h"
#include "web/Api2User.h"

#include <QPixmap>

namespace Tomahawk {
namespace APIv2 {

class ACLJobStatusItem : public JobStatusItem
{
    Q_OBJECT
public:
    explicit ACLJobStatusItem( const QSharedPointer<Api2User>& user );
    virtual ~ACLJobStatusItem();

    virtual int weight() const { return 99; }

    virtual QString rightColumnText() const { return QString(); }
    virtual QString mainText() const { return QString(); }
    virtual QPixmap icon() const { return QPixmap(); }
    virtual QString type() const { return "apiv2-acljob"; }

    virtual int concurrentJobLimit() const { return 3; }

    virtual bool hasCustomDelegate() const { return true; }
    virtual void createDelegate( QObject* parent = 0 );
    virtual QStyledItemDelegate* customDelegate() const { return m_delegate; }

    virtual QSharedPointer<Api2User> user() const { return m_user; }
    virtual QString name() const { return m_user->name(); }

signals:
//    void userDecision( QSharedPointer<Api2User> user );

public slots:
//    void aclResult( Api2User::ACLDecision );

private:
    QStyledItemDelegate* m_delegate;
    QSharedPointer<Api2User> m_user;
};

} // namespace APIv2
} // namespace Tomahawk

#endif // TOMAHAWK_APIV2_ACLJOBSTATUSITEM_H
