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

#ifndef JOB_STATUS_ITEM
#define JOB_STATUS_ITEM

#include <QObject>
#include <QMetaType>

class QStyledItemDelegate;
class QPixmap;

/**
 * Implement your own JobStatusItem if you want to add some sort of job status entry in the JobStatusView.
 *
 * Status items have 3 columns:
 * ________________________________
 * | icon | main col    | right col|
 * _________________________________
 *
 * The right column may be empty.
 *
 */
class JobStatusItem : public QObject
{
    Q_OBJECT
public:
    explicit JobStatusItem();
    virtual ~JobStatusItem();

    virtual QString type() const = 0;
    virtual int weight() const { return 1; }

    /// Please cache this.
    virtual QPixmap icon() const = 0;
    virtual QString mainText() const = 0;
    virtual QString rightColumnText() const { return QString(); };

    /**
     * If collapse item is true, sending multiple items of the same type will "collapse" them into one
     * instead of showing each individually. In this case, the right column from the item will be ignored
     * and a count will be shown instead.
     */
    virtual bool collapseItem() const;
    virtual bool allowMultiLine() const;

    virtual int concurrentJobLimit() const;

    virtual bool hasCustomDelegate() const;
    virtual void createDelegate( QObject* parent );
    virtual QStyledItemDelegate* customDelegate() const;

    qint64 age() const { return m_createdOn; }
signals:
    /// Ask for an update
    void statusChanged();

    /// Job is finished, will be deleted by the model
    void finished();

private:
    qint64 m_createdOn;
};

Q_DECLARE_METATYPE( JobStatusItem* );

#endif
