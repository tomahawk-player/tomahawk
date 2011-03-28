/*
 * Copyright (C) 2006-2008  Justin Karneges
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 *
 */

// Note: JDnsShared supports multiple interfaces for multicast, but only one
//   for IPv4 and one for IPv6.  Sharing multiple interfaces of the same IP
//   version for multicast is unfortunately not possible without reworking
//   the jdns subsystem.
//
//   The reason for this limitation is that in order to do multi-interface
//   multicast, you have to do a single bind to Any, and then use special
//   functions to determine which interface a packet came from and to
//   specify which interface a packet should go out on.  Again this is just
//   not possible with the current system and the assumptions made by jdns.

// Note: When quering against multiple interfaces with multicast, it is
//   possible that different answers for a unique record may be reported
//   on each interface.  We don't do anything about this.

#include "jdnsshared.h"
#include "jdns_export.h"

#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QStringList>
// safeobj stuff, from qca

class JDNS_EXPORT JDnsSharedSafeTimer : public QObject
{
	Q_OBJECT
public:

	void JDnsSharedReleaseAndDeleteLater(QObject *owner, QObject *obj)
	{
		obj->disconnect(owner);
		obj->setParent(0);
		obj->deleteLater();
	}

	JDnsSharedSafeTimer(QObject *parent = 0) :
		QObject(parent)
	{
		t = new QTimer(this);
		connect(t, SIGNAL(timeout()), SIGNAL(timeout()));
	}

	~JDnsSharedSafeTimer()
	{
		JDnsSharedReleaseAndDeleteLater(this, t);
	}

	int interval() const                { return t->interval(); }
	bool isActive() const               { return t->isActive(); }
	bool isSingleShot() const           { return t->isSingleShot(); }
	void setInterval(int msec)          { t->setInterval(msec); }
	void setSingleShot(bool singleShot) { t->setSingleShot(singleShot); }
	int timerId() const                 { return t->timerId(); }

public slots:
	void start(int msec)                { t->start(msec); }
	void start()                        { t->start(); }
	void stop()                         { t->stop(); }

signals:
	void timeout();

private:
	QTimer *t;
};

// for caching system info

class SystemInfoCache
{
public:
	QJDns::SystemInfo info;
	QTime time;
};


//----------------------------------------------------------------------------
// Handle
//----------------------------------------------------------------------------

// QJDns uses integer handle ids, but they are only unique within
//   the relevant QJDns instance.  Since we want our handles to be
//   unique across all instances, we'll make an instance/id pair.
class Handle
{
public:
	QJDns *jdns;
	int id;

	Handle() : jdns(0), id(-1)
	{
	}

	Handle(QJDns *_jdns, int _id) : jdns(_jdns), id(_id)
	{
	}

	~Handle() {}

	bool operator==(const Handle &a) const
	{
		if(a.jdns == jdns && a.id == id)
			return true;
		return false;
	}

	bool operator!=(const Handle &a) const
	{
		return !(operator==(a));
	}
};

//----------------------------------------------------------------------------
// JDnsShutdown
//----------------------------------------------------------------------------
class JDnsShutdownAgent : public QObject
{
	Q_OBJECT
public:
	JDnsShutdownAgent();
	~JDnsShutdownAgent() {}
	void start();

signals:
	void started();
};


class JDnsShutdownWorker : public QObject
{
	Q_OBJECT
public:
	QList<JDnsShared*> list;

	JDnsShutdownWorker(const QList<JDnsShared*> &_list);
	virtual ~JDnsShutdownWorker() {}

signals:
	void finished();

private slots:
	void jdns_shutdownFinished();
};

class JDnsShutdown : public QThread
{
	Q_OBJECT
public:

	QMutex m;
	QWaitCondition w;
	QList<JDnsShared*> list;
	JDnsShutdownAgent *agent;
	JDnsShutdownWorker *worker;
	int phase;
	
	JDnsShutdown();
	~JDnsShutdown() {}
	void waitForShutdown(const QList<JDnsShared*> &_list);

protected:
	virtual void run();

private slots:
	void agent_started();

	void worker_finished();
};
 
//----------------------------------------------------------------------------
// JDnsSharedDebug
//----------------------------------------------------------------------------
class JDnsSharedDebugPrivate : public QObject
{
	Q_OBJECT
public:
	JDnsSharedDebug *q;
	QMutex m;
	QStringList lines;
	bool dirty;

	JDnsSharedDebugPrivate(JDnsSharedDebug *_q);
	~JDnsSharedDebugPrivate() {}

	void addDebug(const QString &name, const QStringList &_lines);

private slots:
	void doUpdate();
};

//----------------------------------------------------------------------------
// JDnsSharedRequest
//----------------------------------------------------------------------------
class JDnsSharedPrivate : public QObject
{
	Q_OBJECT
public:
	class Instance
	{
	public:
		QJDns *jdns;
		QHostAddress addr;
		int index;

		Instance() : jdns(0)
		{
		}
	};

	enum PreprocessMode
	{
		None,            // don't muck with anything
		FillInAddress,   // for A/AAAA
		FillInPtrOwner6, // for PTR, IPv6
		FillInPtrOwner4, // for PTR, IPv4
	};

	JDnsShared *q;
	JDnsShared::Mode mode;
	bool shutting_down;
	JDnsSharedDebug *db;
	QString dbname;

	QList<Instance*> instances;
	QHash<QJDns*,Instance*> instanceForQJDns;

	QSet<JDnsSharedRequest*> requests;
	QHash<Handle,JDnsSharedRequest*> requestForHandle;

	JDnsSharedPrivate(JDnsShared *_q);
	
	~JDnsSharedPrivate() {}

	JDnsSharedRequest *findRequest(QJDns *jdns, int id) const;

	void jdns_link(QJDns *jdns);

	int getNewIndex() const;

	void addDebug(int index, const QString &line);

	void doDebug(QJDns *jdns, int index);

	PreprocessMode determinePpMode(const QJDns::Record &in);

	QJDns::Record manipulateRecord(const QJDns::Record &in, PreprocessMode ppmode, bool *modified = 0);

	bool addInterface(const QHostAddress &addr);
	void removeInterface(const QHostAddress &addr);

	void queryStart(JDnsSharedRequest *obj, const QByteArray &name, int qType);
	void queryCancel(JDnsSharedRequest *obj);
	void publishStart(JDnsSharedRequest *obj, QJDns::PublishMode m, const QJDns::Record &record);
	void publishUpdate(JDnsSharedRequest *obj, const QJDns::Record &record);
	void publishCancel(JDnsSharedRequest *obj);

public slots:
	void late_shutdown();

private slots:
	void jdns_resultsReady(int id, const QJDns::Response &results);
	void jdns_published(int id);
	void jdns_error(int id, QJDns::Error e);
	void jdns_shutdownFinished();
	void jdns_debugLinesReady();
};

class JDnsSharedRequestPrivate : public QObject
{
	Q_OBJECT
public:
	JDnsSharedRequest *q;
	JDnsSharedPrivate *jsp;

	// current action
	JDnsSharedRequest::Type type;
	QByteArray name;
	int qType;
	QJDns::PublishMode pubmode;
	JDnsSharedPrivate::PreprocessMode ppmode;
	QJDns::Record pubrecord;

	// a single request might have to perform multiple QJDns operations
	QList<Handle> handles;

	// keep a list of handles that successfully publish
	QList<Handle> published;

	// use to weed out dups for multicast
	QList<QJDns::Record> queryCache;

	bool success;
	JDnsSharedRequest::Error error;
	QList<QJDns::Record> results;
	JDnsSharedSafeTimer lateTimer;

	JDnsSharedRequestPrivate(JDnsSharedRequest *_q);
	~JDnsSharedRequestPrivate() {};

	void resetSession();

private slots:
	void lateTimer_timeout();
};

