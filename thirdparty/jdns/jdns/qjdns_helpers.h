/*
 * Copyright (C) 2005-2008  Justin Karneges
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "qjdns.h"
#include "qjdns_sock.h"
#include "jdns.h"

#include <QTimer>
#include <QUdpSocket>
#include <QPointer>
#include <QStringList>
#include <QMetaEnum>

#include <time.h>

// safeobj stuff, from qca

class QJDnsSafeTimer : public QObject
{
	Q_OBJECT
public:

	void releaseAndDeleteLater(QObject *owner, QObject *obj)
	{
		obj->disconnect(owner);
		obj->setParent(0);
		obj->deleteLater();
	}

	QJDnsSafeTimer(QObject *parent = 0) :
		QObject(parent)
	{
		t = new QTimer(this);
		connect(t, SIGNAL(timeout()), SIGNAL(timeout()));
	}

	~QJDnsSafeTimer()
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

static jdns_string_t *qt2str(const QByteArray &in)
{
	jdns_string_t *out = jdns_string_new();
	jdns_string_set(out, (const unsigned char *)in.data(), in.size());
	return out;
}

static QByteArray str2qt(const jdns_string_t *in)
{
	return QByteArray((const char *)in->data, in->size);
}

static void qt2addr_set(jdns_address_t *addr, const QHostAddress &host)
{
	if(host.protocol() == QAbstractSocket::IPv6Protocol)
		jdns_address_set_ipv6(addr, host.toIPv6Address().c);
	else
		jdns_address_set_ipv4(addr, host.toIPv4Address());
}

static jdns_address_t *qt2addr(const QHostAddress &host)
{
	jdns_address_t *addr = jdns_address_new();
	qt2addr_set(addr, host);
	return addr;
}

static QHostAddress addr2qt(const jdns_address_t *addr)
{
	if(addr->isIpv6)
		return QHostAddress(addr->addr.v6);
	else
		return QHostAddress(addr->addr.v4);
}

static QJDns::Record import_record(const jdns_rr_t *in)
{
	QJDns::Record out;

	out.owner = QByteArray((const char *)in->owner);
	out.ttl = in->ttl;
	out.type = in->type;
	out.rdata = QByteArray((const char *)in->rdata, in->rdlength);

	// known
	if(in->haveKnown)
	{
		int type = in->type;

		if(type == QJDns::A || type == QJDns::Aaaa)
		{
			out.haveKnown = true;
			out.address = addr2qt(in->data.address);
		}
		else if(type == QJDns::Mx)
		{
			out.haveKnown = true;
			out.name = QByteArray((const char *)in->data.server->name);
			out.priority = in->data.server->priority;
		}
		else if(type == QJDns::Srv)
		{
			out.haveKnown = true;
			out.name = QByteArray((const char *)in->data.server->name);
			out.priority = in->data.server->priority;
			out.weight = in->data.server->weight;
			out.port = in->data.server->port;
		}
		else if(type == QJDns::Cname || type == QJDns::Ptr || type == QJDns::Ns)
		{
			out.haveKnown = true;
			out.name = QByteArray((const char *)in->data.name);
		}
		else if(type == QJDns::Txt)
		{
			out.haveKnown = true;
			out.texts.clear();
			for(int n = 0; n < in->data.texts->count; ++n)
				out.texts += str2qt(in->data.texts->item[n]);
		}
		else if(type == QJDns::Hinfo)
		{
			out.haveKnown = true;
			out.cpu = str2qt(in->data.hinfo.cpu);
			out.os = str2qt(in->data.hinfo.os);
		}
	}

	return out;
}

static int my_srand_done = 0;

static void my_srand()
{
	if(my_srand_done)
		return;

	// lame attempt at randomizing without srand
	int count = ::time(NULL) % 128;
	for(int n = 0; n < count; ++n)
		rand();

	my_srand_done = 1;
}

static jdns_rr_t *export_record(const QJDns::Record &in)
{
	jdns_rr_t *out = jdns_rr_new();

	jdns_rr_set_owner(out, (const unsigned char *)in.owner.data());
	out->ttl = in.ttl;

	// if we have known, use that
	if(in.haveKnown)
	{
		int type = in.type;

		if(type == QJDns::A)
		{
			jdns_address_t *addr = qt2addr(in.address);
			jdns_rr_set_A(out, addr);
			jdns_address_delete(addr);
		}
		else if(type == QJDns::Aaaa)
		{
			jdns_address_t *addr = qt2addr(in.address);
			jdns_rr_set_AAAA(out, addr);
			jdns_address_delete(addr);
		}
		else if(type == QJDns::Mx)
		{
			jdns_rr_set_MX(out, (const unsigned char *)in.name.data(), in.priority);
		}
		else if(type == QJDns::Srv)
		{
			jdns_rr_set_SRV(out, (const unsigned char *)in.name.data(), in.port, in.priority, in.weight);
		}
		else if(type == QJDns::Cname)
		{
			jdns_rr_set_CNAME(out, (const unsigned char *)in.name.data());
		}
		else if(type == QJDns::Ptr)
		{
			jdns_rr_set_PTR(out, (const unsigned char *)in.name.data());
		}
		else if(type == QJDns::Txt)
		{
			jdns_stringlist_t *list = jdns_stringlist_new();
			for(int n = 0; n < in.texts.count(); ++n)
			{
				jdns_string_t *str = qt2str(in.texts[n]);
				jdns_stringlist_append(list, str);
				jdns_string_delete(str);
			}
			jdns_rr_set_TXT(out, list);
			jdns_stringlist_delete(list);
		}
		else if(type == QJDns::Hinfo)
		{
			jdns_string_t *cpu = qt2str(in.cpu);
			jdns_string_t *os = qt2str(in.os);
			jdns_rr_set_HINFO(out, cpu, os);
			jdns_string_delete(cpu);
			jdns_string_delete(os);
		}
		else if(type == QJDns::Ns)
		{
			jdns_rr_set_NS(out, (const unsigned char *)in.name.data());
		}
	}
	else
		jdns_rr_set_record(out, in.type, (const unsigned char *)in.rdata.data(), in.rdata.size());

	return out;
}


class QJDns::Private : public QObject
{
	Q_OBJECT
public:
	class LateError
	{
	public:
		int source_type; // 0 for query, 1 for publish
		int id;
		Error error;
	};

	class LateResponse
	{
	public:
		int id;
		QJDns::Response response;
		bool do_cancel;
	};

	QJDns *q;
	QJDns::Mode mode;
	jdns_session_t *sess;
	bool shutting_down;
	QJDnsSafeTimer stepTrigger, debugTrigger;
	QJDnsSafeTimer stepTimeout;
	QTime clock;
	QStringList debug_strings;
	bool new_debug_strings;
	int next_handle;
	bool need_handle;
	QHash<int,QUdpSocket*> socketForHandle;
	QHash<QUdpSocket*,int> handleForSocket;
	int pending;
	bool pending_wait;
	bool complete_shutdown;

	// pointers that will point to things we are currently signalling
	//   about.  when a query or publish is cancelled, we can use these
	//   pointers to extract anything we shouldn't signal.
	QList<LateError> *pErrors;
	QList<int> *pPublished;
	QList<LateResponse> *pResponses;

	Private(QJDns *_q);
	~Private();

	void cleanup();

	bool init(QJDns::Mode _mode, const QHostAddress &address);

	void setNameServers(const QList<NameServer> &nslist);

	void process();

	void processDebug();

	void doNextStep();

	void removeCancelled(int id);

private slots:
	void udp_readyRead();

	void udp_bytesWritten(qint64);

	void st_timeout();

	void doNextStepSlot();

	void doDebug();

private:
	static int cb_time_now(jdns_session_t *, void *app)
	{
		QJDns::Private *self = (QJDns::Private *)app;

		return self->clock.elapsed();
	}

	static int cb_rand_int(jdns_session_t *, void *)
	{
		return rand() % 65536;
	}

	static void cb_debug_line(jdns_session_t *, void *app, const char *str)
	{
		QJDns::Private *self = (QJDns::Private *)app;

		self->debug_strings += QString::fromLatin1(str);
		self->processDebug();
	}

	static int cb_udp_bind(jdns_session_t *, void *app, const jdns_address_t *addr, int port, const jdns_address_t *maddr)
	{
		QJDns::Private *self = (QJDns::Private *)app;

		// we always pass non-null to jdns_init, so this should be a valid address
		QHostAddress host = addr2qt(addr);

		QUdpSocket *sock = new QUdpSocket(self);
		self->connect(sock, SIGNAL(readyRead()), SLOT(udp_readyRead()));

		// use queued for bytesWritten, since qt is evil and emits before writeDatagram returns
		qRegisterMetaType<qint64>("qint64");
		self->connect(sock, SIGNAL(bytesWritten(qint64)), SLOT(udp_bytesWritten(qint64)), Qt::QueuedConnection);

		QUdpSocket::BindMode mode;
		mode |= QUdpSocket::ShareAddress;
		mode |= QUdpSocket::ReuseAddressHint;
		if(!sock->bind(host, port, mode))
		{
			delete sock;
			return 0;
		}

		if(maddr)
		{
			int sd = sock->socketDescriptor();
			bool ok;
			int errorCode;
			if(maddr->isIpv6)
				ok = qjdns_sock_setMulticast6(sd, maddr->addr.v6, &errorCode);
			else
				ok = qjdns_sock_setMulticast4(sd, maddr->addr.v4, &errorCode);

			if(!ok)
			{
				delete sock;

				self->debug_strings += QString("failed to setup multicast on the socket (errorCode=%1)").arg(errorCode);
				self->processDebug();
				return 0;
			}

			if(maddr->isIpv6)
			{
				qjdns_sock_setTTL6(sd, 255);
				qjdns_sock_setIPv6Only(sd);
			}
			else
				qjdns_sock_setTTL4(sd, 255);
		}

		int handle = self->next_handle++;
		self->socketForHandle.insert(handle, sock);
		self->handleForSocket.insert(sock, handle);
		return handle;
	}

	static void cb_udp_unbind(jdns_session_t *, void *app, int handle)
	{
		QJDns::Private *self = (QJDns::Private *)app;

		QUdpSocket *sock = self->socketForHandle.value(handle);
		if(!sock)
			return;

		self->socketForHandle.remove(handle);
		self->handleForSocket.remove(sock);
		delete sock;
	}

	static int cb_udp_read(jdns_session_t *, void *app, int handle, jdns_address_t *addr, int *port, unsigned char *buf, int *bufsize)
	{
		QJDns::Private *self = (QJDns::Private *)app;

		QUdpSocket *sock = self->socketForHandle.value(handle);
		if(!sock)
			return 0;

		// nothing to read?
		if(!sock->hasPendingDatagrams())
			return 0;

		QHostAddress from_addr;
		quint16 from_port;
		int ret = sock->readDatagram((char *)buf, *bufsize, &from_addr, &from_port);
		if(ret == -1)
			return 0;

		qt2addr_set(addr, from_addr);
		*port = (int)from_port;
		*bufsize = ret;
		return 1;
	}

	static int cb_udp_write(jdns_session_t *, void *app, int handle, const jdns_address_t *addr, int port, unsigned char *buf, int bufsize)
	{
		QJDns::Private *self = (QJDns::Private *)app;

		QUdpSocket *sock = self->socketForHandle.value(handle);
		if(!sock)
			return 0;

		QHostAddress host = addr2qt(addr);
		int ret = sock->writeDatagram((const char *)buf, bufsize, host, port);
		if(ret == -1)
		{
			// this can happen if the datagram to send is too big.  i'm not sure what else
			//   may cause this.  if we return 0, then jdns may try to resend the packet,
			//   which might not work if it is too large (causing the same error over and
			//   over).  we'll return success to jdns, so the result is as if the packet
			//   was dropped.
			return 1;
		}

		++self->pending;
		return 1;
	}

};

