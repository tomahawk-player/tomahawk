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

namespace {

// safeobj stuff, from qca

void releaseAndDeleteLater(QObject *owner, QObject *obj)
{
	obj->disconnect(owner);
	obj->setParent(0);
	obj->deleteLater();
}

class SafeTimer : public QObject
{
	Q_OBJECT
public:
	SafeTimer(QObject *parent = 0) :
		QObject(parent)
	{
		t = new QTimer(this);
		connect(t, SIGNAL(timeout()), SIGNAL(timeout()));
	}

	~SafeTimer()
	{
		releaseAndDeleteLater(this, t);
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

}

Q_GLOBAL_STATIC(QMutex, jdnsshared_mutex)
Q_GLOBAL_STATIC(SystemInfoCache, jdnsshared_infocache)

static QJDns::SystemInfo get_sys_info()
{
	QMutexLocker locker(jdnsshared_mutex());
	SystemInfoCache *c = jdnsshared_infocache();

	// cache info for 1/2 second, enough to prevent re-reading of sys
	//   info 20 times because of all the different resolves
	if(c->time.isNull() || c->time.elapsed() >= 500)
	{
		c->info = QJDns::systemInfo();
		c->time.start();
	}

	return c->info;
}

static bool domainCompare(const QByteArray &a, const QByteArray &b)
{
	return (qstricmp(a.data(), b.data()) == 0) ? true: false;
}

// adapted from jdns_mdnsd.c, _a_match()
static bool matchRecordExceptTtl(const QJDns::Record &a, const QJDns::Record &b)
{
	if(a.type != b.type || !domainCompare(a.owner, b.owner))
		return false;

	if(a.type == QJDns::Srv)
	{
		if(domainCompare(a.name, b.name)
			&& a.port == b.port
			&& a.priority == b.priority
			&& a.weight == b.weight)
		{
			return true;
		}
	}
	else if(a.type == QJDns::Ptr || a.type == QJDns::Ns || a.type == QJDns::Cname)
	{
		if(domainCompare(a.name, b.name))
			return true;
	}
	else if(a.rdata == b.rdata)
		return true;

	return false;
}

static void getHex(unsigned char in, char *hi, char *lo)
{
	QString str;
	str.sprintf("%02x", in);
	*hi = str[0].toLatin1();
	*lo = str[1].toLatin1();
}

static QByteArray getDec(int in)
{
	return QString::number(in).toLatin1();
}

static QByteArray makeReverseName(const QHostAddress &addr)
{
	QByteArray out;

	if(addr.protocol() == QAbstractSocket::IPv6Protocol)
	{
		Q_IPV6ADDR raw = addr.toIPv6Address();
		for(int n = 0; n < 32; ++n)
		{
			char hi, lo;
			getHex(raw.c[31 - n], &hi, &lo);
			out += lo;
			out += '.';
			out += hi;
			out += '.';
		}
		out += "ip6.arpa.";
	}
	else
	{
		quint32 rawi = addr.toIPv4Address();
		int raw[4];
		raw[0] = (rawi >> 24) & 0xff;
		raw[1] = (rawi >> 16) & 0xff;
		raw[2] = (rawi >>  8) & 0xff;
		raw[3] = rawi & 0xff;
		for(int n = 0; n < 4; ++n)
		{
			out += getDec(raw[3 - n]);
			out += '.';
		}
		out += "in-addr.arpa.";
	}

	return out;
}

//----------------------------------------------------------------------------
// Handle
//----------------------------------------------------------------------------

namespace {

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

// adapted from qHash<QPair>
static inline uint qHash(const Handle &key)
{
	uint h1 = ::qHash(key.jdns);
	uint h2 = ::qHash(key.id);
	return ((h1 << 16) | (h1 >> 16)) ^ h2;
}

}

//----------------------------------------------------------------------------
// JDnsShutdown
//----------------------------------------------------------------------------
namespace {

class JDnsShutdownAgent : public QObject
{
	Q_OBJECT
public:
	void start()
	{
		QMetaObject::invokeMethod(this, "started", Qt::QueuedConnection);
	}

signals:
	void started();
};

class JDnsShutdownWorker : public QObject
{
	Q_OBJECT
public:
	QList<JDnsShared*> list;

	JDnsShutdownWorker(const QList<JDnsShared*> &_list) : QObject(0), list(_list)
	{
		foreach(JDnsShared *i, list)
		{
			connect(i, SIGNAL(shutdownFinished()), SLOT(jdns_shutdownFinished()));
			i->shutdown(); // MUST support DOR-DS, and it does
		}
	}

signals:
	void finished();

private slots:
	void jdns_shutdownFinished()
	{
		JDnsShared *i = (JDnsShared *)sender();
		list.removeAll(i);
		delete i;
		if(list.isEmpty())
			emit finished();
	}
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

	void waitForShutdown(const QList<JDnsShared*> &_list)
	{
		list = _list;
		phase = 0;

		m.lock();
		start();
		w.wait(&m);

		foreach(JDnsShared *i, list)
		{
			i->setParent(0);
			i->moveToThread(this);
		}

		phase = 1;
		agent->start();
		wait();
	}

protected:
	virtual void run()
	{
		m.lock();
		agent = new JDnsShutdownAgent;
		connect(agent, SIGNAL(started()), SLOT(agent_started()), Qt::DirectConnection);
		agent->start();
		exec();
		delete agent;
	}

private slots:
	void agent_started()
	{
		if(phase == 0)
		{
			w.wakeOne();
			m.unlock();
		}
		else
		{
			worker = new JDnsShutdownWorker(list);
			connect(worker, SIGNAL(finished()), SLOT(worker_finished()), Qt::DirectConnection);
		}
	}

	void worker_finished()
	{
		delete worker;
		worker = 0;

		quit();
	}
};

}

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

	JDnsSharedDebugPrivate(JDnsSharedDebug *_q) : QObject(_q), q(_q)
	{
		dirty = false;
	}

	void addDebug(const QString &name, const QStringList &_lines)
	{
		if(!_lines.isEmpty())
		{
			QMutexLocker locker(&m);
			for(int n = 0; n < _lines.count(); ++n)
				lines += name + ": " + _lines[n];
			if(!dirty)
			{
				dirty = true;
				QMetaObject::invokeMethod(this, "doUpdate", Qt::QueuedConnection);
			}
		}
	}

private slots:
	void doUpdate()
	{
		{
			QMutexLocker locker(&m);
			if(!dirty)
				return;
		}
		emit q->readyRead();
	}
};

JDnsSharedDebug::JDnsSharedDebug(QObject *parent)
:QObject(parent)
{
	d = new JDnsSharedDebugPrivate(this);
}

JDnsSharedDebug::~JDnsSharedDebug()
{
	delete d;
}

QStringList JDnsSharedDebug::readDebugLines()
{
	QMutexLocker locker(&d->m);
	QStringList tmplines = d->lines;
	d->lines.clear();
	d->dirty = false;
	return tmplines;
}

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

	JDnsSharedPrivate(JDnsShared *_q) : QObject(_q), q(_q)
	{
	}

	JDnsSharedRequest *findRequest(QJDns *jdns, int id) const
	{
		Handle h(jdns, id);
		return requestForHandle.value(h);
	}

	void jdns_link(QJDns *jdns)
	{
		connect(jdns, SIGNAL(resultsReady(int, const QJDns::Response &)), SLOT(jdns_resultsReady(int, const QJDns::Response &)));
		connect(jdns, SIGNAL(published(int)), SLOT(jdns_published(int)));
		connect(jdns, SIGNAL(error(int, QJDns::Error)), SLOT(jdns_error(int, QJDns::Error)));
		connect(jdns, SIGNAL(shutdownFinished()), SLOT(jdns_shutdownFinished()));
		connect(jdns, SIGNAL(debugLinesReady()), SLOT(jdns_debugLinesReady()));
	}

	int getNewIndex() const
	{
		// find lowest unused value
		for(int n = 0;; ++n)
		{
			bool found = false;
			foreach(Instance *i, instances)
			{
				if(i->index == n)
				{
					found = true;
					break;
				}
			}
			if(!found)
				return n;
		}
	}

	void addDebug(int index, const QString &line)
	{
		if(db)
			db->d->addDebug(dbname + QString::number(index), QStringList() << line);
	}

	void doDebug(QJDns *jdns, int index)
	{
		QStringList lines = jdns->debugLines();
		if(db)
			db->d->addDebug(dbname + QString::number(index), lines);
	}

	PreprocessMode determinePpMode(const QJDns::Record &in)
	{
		// Note: since our implementation only allows 1 ipv4 and 1 ipv6
		//   interface to exist, it is safe to publish both kinds of
		//   records on both interfaces, with the same values.  For
		//   example, an A record can be published on both interfaces,
		//   with the value set to the ipv4 interface.  If we supported
		//   multiple ipv4 interfaces, then this wouldn't work, because
		//   we wouldn't know which value to use for the A record when
		//   publishing on the ipv6 interface.

		// publishing our own IP address?  null address means the user
		//   wants us to fill in the blank with our address.
		if((in.type == QJDns::Aaaa || in.type == QJDns::A) && in.address.isNull())
		{
			return FillInAddress;
		}
		// publishing our own reverse lookup?  partial owner means
		//   user wants us to fill in the rest.
		else if(in.type == QJDns::Ptr && in.owner == ".ip6.arpa.")
		{
			return FillInPtrOwner6;
		}
		else if(in.type == QJDns::Ptr && in.owner == ".in-addr.arpa.")
		{
			return FillInPtrOwner4;
		}

		return None;
	}

	QJDns::Record manipulateRecord(const QJDns::Record &in, PreprocessMode ppmode, bool *modified = 0)
	{
		if(ppmode == FillInAddress)
		{
			QJDns::Record out = in;

			if(in.type == QJDns::Aaaa)
			{
				// are we operating on ipv6?
				foreach(Instance *i, instances)
				{
					if(i->addr.protocol() == QAbstractSocket::IPv6Protocol)
					{
						if(modified && !(out.address == i->addr))
							*modified = true;
						out.address = i->addr;
						break;
					}
				}
			}
			else // A
			{
				// are we operating on ipv4?
				foreach(Instance *i, instances)
				{
					if(i->addr.protocol() == QAbstractSocket::IPv4Protocol)
					{
						if(modified && !(out.address == i->addr))
							*modified = true;
						out.address = i->addr;
						break;
					}
				}
			}

			return out;
		}
		else if(ppmode == FillInPtrOwner6)
		{
			QJDns::Record out = in;

			// are we operating on ipv6?
			foreach(Instance *i, instances)
			{
				if(i->addr.protocol() == QAbstractSocket::IPv6Protocol)
				{
					QByteArray newOwner = makeReverseName(i->addr);
					if(modified && !(out.owner == newOwner))
						*modified = true;
					out.owner = newOwner;
					break;
				}
			}

			return out;
		}
		else if(ppmode == FillInPtrOwner4)
		{
			QJDns::Record out = in;

			// are we operating on ipv4?
			foreach(Instance *i, instances)
			{
				if(i->addr.protocol() == QAbstractSocket::IPv4Protocol)
				{
					QByteArray newOwner = makeReverseName(i->addr);
					if(modified && !(out.owner == newOwner))
						*modified = true;
					out.owner = newOwner;
					break;
				}
			}

			return out;
		}

		if(modified)
			*modified = false;
		return in;
	}

	bool addInterface(const QHostAddress &addr);
	void removeInterface(const QHostAddress &addr);

	void queryStart(JDnsSharedRequest *obj, const QByteArray &name, int qType);
	void queryCancel(JDnsSharedRequest *obj);
	void publishStart(JDnsSharedRequest *obj, QJDns::PublishMode m, const QJDns::Record &record);
	void publishUpdate(JDnsSharedRequest *obj, const QJDns::Record &record);
	void publishCancel(JDnsSharedRequest *obj);

public slots:
	void late_shutdown()
	{
		shutting_down = false;
		emit q->shutdownFinished();
	}

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
	SafeTimer lateTimer;

	JDnsSharedRequestPrivate(JDnsSharedRequest *_q) : QObject(_q), q(_q), lateTimer(this)
	{
		connect(&lateTimer, SIGNAL(timeout()), SLOT(lateTimer_timeout()));
	}

	void resetSession()
	{
		name = QByteArray();
		pubrecord = QJDns::Record();
		handles.clear();
		published.clear();
		queryCache.clear();
	}

private slots:
	void lateTimer_timeout()
	{
		emit q->resultsReady();
	}
};

JDnsSharedRequest::JDnsSharedRequest(JDnsShared *jdnsShared, QObject *parent)
:QObject(parent)
{
	d = new JDnsSharedRequestPrivate(this);
	d->jsp = jdnsShared->d;
}

JDnsSharedRequest::~JDnsSharedRequest()
{
	cancel();
	delete d;
}

JDnsSharedRequest::Type JDnsSharedRequest::type()
{
	return d->type;
}

void JDnsSharedRequest::query(const QByteArray &name, int type)
{
	cancel();
	d->jsp->queryStart(this, name, type);
}

void JDnsSharedRequest::publish(QJDns::PublishMode m, const QJDns::Record &record)
{
	cancel();
	d->jsp->publishStart(this, m, record);
}

void JDnsSharedRequest::publishUpdate(const QJDns::Record &record)
{
	// only allowed to update if we have an active publish
	if(!d->handles.isEmpty() && d->type == Publish)
		d->jsp->publishUpdate(this, record);
}

void JDnsSharedRequest::cancel()
{
	d->lateTimer.stop();
	if(!d->handles.isEmpty())
	{
		if(d->type == Query)
			d->jsp->queryCancel(this);
		else
			d->jsp->publishCancel(this);
	}
	d->resetSession();
}

bool JDnsSharedRequest::success() const
{
	return d->success;
}

JDnsSharedRequest::Error JDnsSharedRequest::error() const
{
	return d->error;
}

QList<QJDns::Record> JDnsSharedRequest::results() const
{
	return d->results;
}

//----------------------------------------------------------------------------
// JDnsShared
//----------------------------------------------------------------------------
JDnsShared::JDnsShared(Mode mode, QObject *parent)
:QObject(parent)
{
	d = new JDnsSharedPrivate(this);
	d->mode = mode;
	d->shutting_down = false;
	d->db = 0;
}

JDnsShared::~JDnsShared()
{
	foreach(JDnsSharedPrivate::Instance *i, d->instances)
	{
		delete i->jdns;
		delete i;
	}
	delete d;
}

void JDnsShared::setDebug(JDnsSharedDebug *db, const QString &name)
{
	d->db = db;
	d->dbname = name;
}

bool JDnsShared::addInterface(const QHostAddress &addr)
{
	return d->addInterface(addr);
}

void JDnsShared::removeInterface(const QHostAddress &addr)
{
	d->removeInterface(addr);
}

void JDnsShared::shutdown()
{
	d->shutting_down = true;
	if(!d->instances.isEmpty())
	{
		foreach(JDnsSharedPrivate::Instance *i, d->instances)
			i->jdns->shutdown();
	}
	else
		QMetaObject::invokeMethod(d, "late_shutdown", Qt::QueuedConnection);
}

QList<QByteArray> JDnsShared::domains()
{
	return get_sys_info().domains;
}

void JDnsShared::waitForShutdown(const QList<JDnsShared*> &instances)
{
	JDnsShutdown s;
	s.waitForShutdown(instances);
}

bool JDnsSharedPrivate::addInterface(const QHostAddress &addr)
{
	if(shutting_down)
		return false;

	// make sure we don't have this one already
	foreach(Instance *i, instances)
	{
		if(i->addr == addr)
			return false;
	}

	int index = getNewIndex();
	addDebug(index, QString("attempting to use interface %1").arg(addr.toString()));

	QJDns *jdns;

	if(mode == JDnsShared::UnicastInternet || mode == JDnsShared::UnicastLocal)
	{
		jdns = new QJDns(this);
		jdns_link(jdns);
		if(!jdns->init(QJDns::Unicast, addr))
		{
			doDebug(jdns, index);
			delete jdns;
			return false;
		}

		if(mode == JDnsShared::UnicastLocal)
		{
			QJDns::NameServer host;
			if(addr.protocol() == QAbstractSocket::IPv6Protocol)
				host.address = QHostAddress("FF02::FB");
			else
				host.address = QHostAddress("224.0.0.251");
			host.port = 5353;
			jdns->setNameServers(QList<QJDns::NameServer>() << host);
		}
	}
	else // Multicast
	{
		// only one multicast interface allowed per IP protocol version.
		// this is because we bind to INADDR_ANY.

		bool have_v6 = false;
		bool have_v4 = false;
		foreach(Instance *i, instances)
		{
			if(i->addr.protocol() == QAbstractSocket::IPv6Protocol)
				have_v6 = true;
			else
				have_v4 = true;
		}

		bool is_v6 = (addr.protocol() == QAbstractSocket::IPv6Protocol) ? true : false;

		if(is_v6 && have_v6)
		{
			addDebug(index, "already have an ipv6 interface");
			return false;
		}

		if(!is_v6 && have_v4)
		{
			addDebug(index, "already have an ipv4 interface");
			return false;
		}

		QHostAddress actualBindAddress;
		if(is_v6)
			actualBindAddress = QHostAddress::AnyIPv6;
		else
			actualBindAddress = QHostAddress::Any;

		jdns = new QJDns(this);
		jdns_link(jdns);
		if(!jdns->init(QJDns::Multicast, actualBindAddress))
		{
			doDebug(jdns, index);
			delete jdns;
			return false;
		}
	}

	Instance *i = new Instance;
	i->jdns = jdns;
	i->addr = addr;
	i->index = index;
	instances += i;
	instanceForQJDns.insert(i->jdns, i);

	addDebug(index, "interface ready");

	if(mode == JDnsShared::Multicast)
	{
		// extend active requests to this interface
		foreach(JDnsSharedRequest *obj, requests)
		{
			if(obj->d->type == JDnsSharedRequest::Query)
			{
				Handle h(i->jdns, i->jdns->queryStart(obj->d->name, obj->d->qType));
				obj->d->handles += h;
				requestForHandle.insert(h, obj);
			}
			else // Publish
			{
				bool modified;
				obj->d->pubrecord = manipulateRecord(obj->d->pubrecord, obj->d->ppmode, &modified);
				// if the record changed, update on the other (existing) interfaces
				if(modified)
				{
					foreach(Handle h, obj->d->handles)
						h.jdns->publishUpdate(h.id, obj->d->pubrecord);
				}

				// publish the record on the new interface
				Handle h(i->jdns, i->jdns->publishStart(obj->d->pubmode, obj->d->pubrecord));
				obj->d->handles += h;
				requestForHandle.insert(h, obj);
			}
		}
	}

	return true;
}

void JDnsSharedPrivate::removeInterface(const QHostAddress &addr)
{
	Instance *i = 0;
	for(int n = 0; n < instances.count(); ++n)
	{
		if(instances[n]->addr == addr)
		{
			i = instances[n];
			break;
		}
	}
	if(!i)
		return;

	int index = i->index;

	// we don't cancel operations or shutdown jdns, we simply
	//   delete our references.  this is because if the interface
	//   is gone, then we have nothing to send on anyway.

	foreach(JDnsSharedRequest *obj, requests)
	{
		for(int n = 0; n < obj->d->handles.count(); ++n)
		{
			Handle h = obj->d->handles[n];
			if(h.jdns == i->jdns)
			{
				// see above, no need to cancel the operation
				obj->d->handles.removeAt(n);
				requestForHandle.remove(h);
				break;
			}
		}

		// remove published reference
		if(obj->d->type == JDnsSharedRequest::Publish)
		{
			for(int n = 0; n < obj->d->published.count(); ++n)
			{
				Handle h = obj->d->published[n];
				if(h.jdns == i->jdns)
				{
					obj->d->published.removeAt(n);
					break;
				}
			}
		}
	}

	// see above, no need to shutdown jdns
	instanceForQJDns.remove(i->jdns);
	instances.removeAll(i);
	delete i->jdns;
	delete i;

	// if that was the last interface to be removed, then there should
	//   be no more handles left.  let's take action with these
	//   handleless requests.
	foreach(JDnsSharedRequest *obj, requests)
	{
		if(obj->d->handles.isEmpty())
		{
			if(mode == JDnsShared::UnicastInternet || mode == JDnsShared::UnicastLocal)
			{
				// for unicast, we'll invalidate with ErrorNoNet
				obj->d->success = false;
				obj->d->error = JDnsSharedRequest::ErrorNoNet;
				obj->d->lateTimer.start();
			}
			else // Multicast
			{
				// for multicast, we'll keep all requests alive.
				//   activity will resume when an interface is
				//   added.
			}
		}
	}

	addDebug(index, QString("removing from %1").arg(addr.toString()));
}

void JDnsSharedPrivate::queryStart(JDnsSharedRequest *obj, const QByteArray &name, int qType)
{
	obj->d->type = JDnsSharedRequest::Query;
	obj->d->success = false;
	obj->d->results.clear();
	obj->d->name = name;
	obj->d->qType = qType;

	// is the input an IP address and the qType is an address record?
	if(qType == QJDns::Aaaa || qType == QJDns::A)
	{
		QHostAddress addr;
		if(addr.setAddress(QString::fromLocal8Bit(name)))
		{
			if(qType == QJDns::Aaaa && addr.protocol() == QAbstractSocket::IPv6Protocol)
			{
				QJDns::Record rec;
				rec.owner = name;
				rec.type = QJDns::Aaaa;
				rec.ttl = 120;
				rec.haveKnown = true;
				rec.address = addr;
				obj->d->success = true;
				obj->d->results = QList<QJDns::Record>() << rec;
				obj->d->lateTimer.start();
				return;
			}
			else if(qType == QJDns::A && addr.protocol() == QAbstractSocket::IPv4Protocol)
			{
				QJDns::Record rec;
				rec.owner = name;
				rec.type = QJDns::A;
				rec.ttl = 120;
				rec.haveKnown = true;
				rec.address = addr;
				obj->d->success = true;
				obj->d->results = QList<QJDns::Record>() << rec;
				obj->d->lateTimer.start();
				return;
			}
		}
	}

	QJDns::SystemInfo sysInfo = get_sys_info();

	// is the input name a known host and the qType is an address record?
	if(qType == QJDns::Aaaa || qType == QJDns::A)
	{
		QByteArray lname = name.toLower();
		QList<QJDns::DnsHost> known = sysInfo.hosts;
		foreach(QJDns::DnsHost host, known)
		{
			if(((qType == QJDns::Aaaa && host.address.protocol() == QAbstractSocket::IPv6Protocol)
				|| (qType == QJDns::A && host.address.protocol() == QAbstractSocket::IPv4Protocol))
				&& host.name.toLower() == lname)
			{
				QJDns::Record rec;
				rec.owner = name;
				rec.type = qType;
				rec.ttl = 120;
				rec.haveKnown = true;
				rec.address = host.address;
				obj->d->success = true;
				obj->d->results = QList<QJDns::Record>() << rec;
				obj->d->lateTimer.start();
				return;
			}
		}
	}

	// if we have no QJDns instances to operate on, then error
	if(instances.isEmpty())
	{
		obj->d->error = JDnsSharedRequest::ErrorNoNet;
		obj->d->lateTimer.start();
		return;
	}

	if(mode == JDnsShared::UnicastInternet)
	{
		// get latest nameservers, split into ipv6/v4, apply to jdns instances
		QList<QJDns::NameServer> ns_v6;
		QList<QJDns::NameServer> ns_v4;
		{
			QList<QJDns::NameServer> nameServers = sysInfo.nameServers;
			foreach(QJDns::NameServer ns, nameServers)
			{
				if(ns.address.protocol() == QAbstractSocket::IPv6Protocol)
					ns_v6 += ns;
				else
					ns_v4 += ns;
			}
		}
		foreach(Instance *i, instances)
		{
			if(i->addr.protocol() == QAbstractSocket::IPv6Protocol)
				i->jdns->setNameServers(ns_v6);
			else
				i->jdns->setNameServers(ns_v4);
		}
	}

	// keep track of this request
	requests += obj;

	// query on all jdns instances
	foreach(Instance *i, instances)
	{
		Handle h(i->jdns, i->jdns->queryStart(name, qType));
		obj->d->handles += h;

		// keep track of this handle for this request
		requestForHandle.insert(h, obj);
	}
}

void JDnsSharedPrivate::queryCancel(JDnsSharedRequest *obj)
{
	if(!requests.contains(obj))
		return;

	foreach(Handle h, obj->d->handles)
	{
		h.jdns->queryCancel(h.id);
		requestForHandle.remove(h);
	}

	obj->d->handles.clear();
	requests.remove(obj);
}

void JDnsSharedPrivate::publishStart(JDnsSharedRequest *obj, QJDns::PublishMode m, const QJDns::Record &record)
{
	obj->d->type = JDnsSharedRequest::Publish;
	obj->d->success = false;
	obj->d->results.clear();
	obj->d->pubmode = m;
	obj->d->ppmode = determinePpMode(record);
	obj->d->pubrecord = manipulateRecord(record, obj->d->ppmode);

	// if we have no QJDns instances to operate on, then error
	if(instances.isEmpty())
	{
		obj->d->error = JDnsSharedRequest::ErrorNoNet;
		obj->d->lateTimer.start();
		return;
	}

	// keep track of this request
	requests += obj;

	// attempt to publish on all jdns instances
	foreach(JDnsSharedPrivate::Instance *i, instances)
	{
		Handle h(i->jdns, i->jdns->publishStart(m, obj->d->pubrecord));
		obj->d->handles += h;

		// keep track of this handle for this request
		requestForHandle.insert(h, obj);
	}
}

void JDnsSharedPrivate::publishUpdate(JDnsSharedRequest *obj, const QJDns::Record &record)
{
	if(!requests.contains(obj))
		return;

	obj->d->ppmode = determinePpMode(record);
	obj->d->pubrecord = manipulateRecord(record, obj->d->ppmode);

	// publish update on all handles for this request
	foreach(Handle h, obj->d->handles)
		h.jdns->publishUpdate(h.id, obj->d->pubrecord);
}

void JDnsSharedPrivate::publishCancel(JDnsSharedRequest *obj)
{
	if(!requests.contains(obj))
		return;

	foreach(Handle h, obj->d->handles)
	{
		h.jdns->publishCancel(h.id);
		requestForHandle.remove(h);
	}

	obj->d->handles.clear();
	obj->d->published.clear();
	requests.remove(obj);
}

void JDnsSharedPrivate::jdns_resultsReady(int id, const QJDns::Response &results)
{
	QJDns *jdns = (QJDns *)sender();
	JDnsSharedRequest *obj = findRequest(jdns, id);
	Q_ASSERT(obj);

	obj->d->success = true;
	obj->d->results = results.answerRecords;

	if(mode == JDnsShared::UnicastInternet || mode == JDnsShared::UnicastLocal)
	{
		// only one response, so "cancel" it
		for(int n = 0; n < obj->d->handles.count(); ++n)
		{
			Handle h = obj->d->handles[n];
			if(h.jdns == jdns && h.id == id)
			{
				obj->d->handles.removeAt(n);
				requestForHandle.remove(h);
				break;
			}
		}

		// cancel related handles
		foreach(Handle h, obj->d->handles)
		{
			h.jdns->queryCancel(h.id);
			requestForHandle.remove(h);
		}

		obj->d->handles.clear();
		requests.remove(obj);
	}
	else // Multicast
	{
		// check our cache to see how we should report these results
		for(int n = 0; n < obj->d->results.count(); ++n)
		{
			QJDns::Record &r = obj->d->results[n];

			// do we have this answer already in our cache?
			QJDns::Record *c = 0;
			int c_at = -1;
			for(int k = 0; k < obj->d->queryCache.count(); ++k)
			{
				QJDns::Record &tmp = obj->d->queryCache[k];
				if(matchRecordExceptTtl(r, tmp))
				{
					c = &tmp;
					c_at = k;
					break;
				}
			}

			// don't report duplicates or unknown removals
			if((c && r.ttl != 0) || (!c && r.ttl == 0))
			{
				obj->d->results.removeAt(n);
				--n; // adjust position
				continue;
			}

			// if we have it, and it is removed, remove from cache
			if(c && r.ttl == 0)
			{
				obj->d->queryCache.removeAt(c_at);
			}
			// otherwise, if we don't have it, add it to the cache
			else if(!c)
			{
				obj->d->queryCache += r;
			}
		}

		if(obj->d->results.isEmpty())
			return;
	}

	emit obj->resultsReady();
}

void JDnsSharedPrivate::jdns_published(int id)
{
	QJDns *jdns = (QJDns *)sender();
	JDnsSharedRequest *obj = findRequest(jdns, id);
	Q_ASSERT(obj);

	// find handle
	Handle handle;
	for(int n = 0; n < obj->d->handles.count(); ++n)
	{
		Handle h = obj->d->handles[n];
		if(h.jdns == jdns && h.id == id)
		{
			handle = h;
			break;
		}
	}

	obj->d->published += handle;

	// if this publish has already been considered successful, then
	//   a publish has succeeded on a new interface and there's no
	//   need to report success for this request again
	if(obj->d->success)
		return;

	// all handles published?
	if(obj->d->published.count() == obj->d->handles.count())
	{
		obj->d->success = true;
		emit obj->resultsReady();
	}
}

void JDnsSharedPrivate::jdns_error(int id, QJDns::Error e)
{
	QJDns *jdns = (QJDns *)sender();
	JDnsSharedRequest *obj = findRequest(jdns, id);
	Q_ASSERT(obj);

	// "cancel" it
	for(int n = 0; n < obj->d->handles.count(); ++n)
	{
		Handle h = obj->d->handles[n];
		if(h.jdns == jdns && h.id == id)
		{
			obj->d->handles.removeAt(n);
			requestForHandle.remove(h);
			break;
		}
	}

	if(obj->d->type == JDnsSharedRequest::Query)
	{
		// ignore the error if it is not the last error
		if(!obj->d->handles.isEmpty())
			return;

		requests.remove(obj);

		obj->d->success = false;
		JDnsSharedRequest::Error x = JDnsSharedRequest::ErrorGeneric;
		if(e == QJDns::ErrorNXDomain)
			x = JDnsSharedRequest::ErrorNXDomain;
		else if(e == QJDns::ErrorTimeout)
			x = JDnsSharedRequest::ErrorTimeout;
		else // ErrorGeneric
			x = JDnsSharedRequest::ErrorGeneric;
		obj->d->error = x;
		emit obj->resultsReady();
	}
	else // Publish
	{
		// cancel related handles
		foreach(Handle h, obj->d->handles)
		{
			h.jdns->publishCancel(h.id);
			requestForHandle.remove(h);
		}

		obj->d->handles.clear();
		obj->d->published.clear();
		requests.remove(obj);

		obj->d->success = false;
		JDnsSharedRequest::Error x = JDnsSharedRequest::ErrorGeneric;
		if(e == QJDns::ErrorConflict)
			x = JDnsSharedRequest::ErrorConflict;
		else // ErrorGeneric
			x = JDnsSharedRequest::ErrorGeneric;
		obj->d->error = x;
		emit obj->resultsReady();
	}
}

void JDnsSharedPrivate::jdns_shutdownFinished()
{
	QJDns *jdns = (QJDns *)sender();

	addDebug(instanceForQJDns.value(jdns)->index, "jdns_shutdownFinished, removing interface");

	Instance *instance = instanceForQJDns.value(jdns);
	delete instance->jdns;
	delete instance;
	instanceForQJDns.remove(jdns);
	instances.removeAll(instance);

	if(instances.isEmpty())
		late_shutdown();
}

void JDnsSharedPrivate::jdns_debugLinesReady()
{
	QJDns *jdns = (QJDns *)sender();

	doDebug(jdns, instanceForQJDns.value(jdns)->index);
}

#include "jdnsshared.moc"
