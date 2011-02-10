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

#include <time.h>
#include "qjdns_sock.h"
#include "qjdns_helpers.h"
#include "jdns.h"

// for fprintf
#include <stdio.h>


//----------------------------------------------------------------------------
// QJDns::NameServer
//----------------------------------------------------------------------------
QJDns::NameServer::NameServer()
{
	port = JDNS_UNICAST_PORT;
}

//----------------------------------------------------------------------------
// QJDns::Record
//----------------------------------------------------------------------------
QJDns::Record::Record()
{
	ttl = 0;
	type = -1;
	haveKnown = false;
}

bool QJDns::Record::verify() const
{
	jdns_rr_t *rr = export_record(*this);
	int ok = jdns_rr_verify(rr);
	jdns_rr_delete(rr);
	return (ok ? true : false);
}

//----------------------------------------------------------------------------
// QJDns
//----------------------------------------------------------------------------
QJDns::Private::Private(QJDns *_q) :
	QObject(_q),
	q(_q),
	stepTrigger(this),
	debugTrigger(this),
	stepTimeout(this),
	pErrors(0),
	pPublished(0),
	pResponses(0)
{
	sess = 0;
	shutting_down = false;
	new_debug_strings = false;
	pending = 0;

	connect(&stepTrigger, SIGNAL(timeout()), SLOT(doNextStepSlot()));
	stepTrigger.setSingleShot(true);

	connect(&debugTrigger, SIGNAL(timeout()), SLOT(doDebug()));
	debugTrigger.setSingleShot(true);

	connect(&stepTimeout, SIGNAL(timeout()), SLOT(st_timeout()));
	stepTimeout.setSingleShot(true);

	my_srand();

	clock.start();
}

QJDns::Private::~Private()
{
	cleanup();
}

void QJDns::Private::cleanup()
{
	if(sess)
	{
		jdns_session_delete(sess);
		sess = 0;
	}

	shutting_down = false;
	pending = 0;

	// it is safe to delete the QUdpSocket objects here without
	//   deleteLater, since this code path never occurs when
	//   a signal from those objects is on the stack
	qDeleteAll(socketForHandle);
	socketForHandle.clear();
	handleForSocket.clear();

	stepTrigger.stop();
	stepTimeout.stop();
	need_handle = 0;
}

bool QJDns::Private::init(QJDns::Mode _mode, const QHostAddress &address)
{
	mode = _mode;

	jdns_callbacks_t callbacks;
	callbacks.app = this;
	callbacks.time_now = cb_time_now;
	callbacks.rand_int = cb_rand_int;
	callbacks.debug_line = cb_debug_line;
	callbacks.udp_bind = cb_udp_bind;
	callbacks.udp_unbind = cb_udp_unbind;
	callbacks.udp_read = cb_udp_read;
	callbacks.udp_write = cb_udp_write;
	sess = jdns_session_new(&callbacks);
	jdns_set_hold_ids_enabled(sess, 1);
	next_handle = 1;
	need_handle = false;

	int ret;

	jdns_address_t *baddr = qt2addr(address);
	if(mode == Unicast)
	{
		ret = jdns_init_unicast(sess, baddr, 0);
	}
	else
	{
		jdns_address_t *maddr;
		if(address.protocol() == QAbstractSocket::IPv6Protocol)
			maddr = jdns_address_multicast6_new();
		else
			maddr = jdns_address_multicast4_new();
		ret = jdns_init_multicast(sess, baddr, JDNS_MULTICAST_PORT, maddr);
		jdns_address_delete(maddr);
	}
	jdns_address_delete(baddr);

	if(!ret)
	{
		jdns_session_delete(sess);
		sess = 0;
		return false;
	}
	return true;
}

void QJDns::Private::setNameServers(const QList<NameServer> &nslist)
{
	jdns_nameserverlist_t *addrs = jdns_nameserverlist_new();
	for(int n = 0; n < nslist.count(); ++n)
	{
		jdns_address_t *addr = qt2addr(nslist[n].address);
		jdns_nameserverlist_append(addrs, addr, nslist[n].port);
		jdns_address_delete(addr);
	}
	jdns_set_nameservers(sess, addrs);
	jdns_nameserverlist_delete(addrs);
}

void QJDns::Private::process()
{
	if(!stepTrigger.isActive())
	{
		stepTimeout.stop();
		stepTrigger.start();
	}
}

void QJDns::Private::processDebug()
{
	new_debug_strings = true;
	if(!debugTrigger.isActive())
		debugTrigger.start();
}

void QJDns::Private::doNextStep()
{
	if(shutting_down && complete_shutdown)
	{
		cleanup();
		emit q->shutdownFinished();
		return;
	}

	QPointer<QObject> self = this;

	int ret = jdns_step(sess);

	QList<LateError> errors;
	QList<int> published;
	QList<LateResponse> responses;
	bool finish_shutdown = false;

	pErrors = &errors;
	pPublished = &published;
	pResponses = &responses;

	while(1)
	{
		jdns_event_t *e = jdns_next_event(sess);
		if(!e)
			break;

		if(e->type == JDNS_EVENT_SHUTDOWN)
		{
			finish_shutdown = true;
		}
		else if(e->type == JDNS_EVENT_PUBLISH)
		{
			if(e->status != JDNS_STATUS_SUCCESS)
			{
				QJDns::Error error;
				if(e->status == JDNS_STATUS_CONFLICT)
					error = QJDns::ErrorConflict;
				else
					error = QJDns::ErrorGeneric;
				LateError le;
				le.source_type = 1;
				le.id = e->id;
				le.error = error;
				errors += le;
			}
			else
			{
				published += e->id;
			}
		}
		else if(e->type == JDNS_EVENT_RESPONSE)
		{
			if(e->status != JDNS_STATUS_SUCCESS)
			{
				QJDns::Error error;
				if(e->status == JDNS_STATUS_NXDOMAIN)
					error = QJDns::ErrorNXDomain;
				else if(e->status == JDNS_STATUS_TIMEOUT)
					error = QJDns::ErrorTimeout;
				else
					error = QJDns::ErrorGeneric;
				LateError le;
				le.source_type = 0;
				le.id = e->id;
				le.error = error;
				errors += le;
			}
			else
			{
				QJDns::Response out_response;
				for(int n = 0; n < e->response->answerCount; ++n)
					out_response.answerRecords += import_record(e->response->answerRecords[n]);
				LateResponse lr;
				lr.id = e->id;
				lr.response = out_response;
				if(mode == Unicast)
					lr.do_cancel = true;
				else
					lr.do_cancel = false;
				responses += lr;
			}
		}

		jdns_event_delete(e);
	}

	if(ret & JDNS_STEP_TIMER)
		stepTimeout.start(jdns_next_timer(sess));
	else
		stepTimeout.stop();

	need_handle = (ret & JDNS_STEP_HANDLE);

	// read the lists safely enough so that items can be deleted
	//   behind our back

	while(!errors.isEmpty())
	{
		LateError i = errors.takeFirst();
		if(i.source_type == 0)
			jdns_cancel_query(sess, i.id);
		else
			jdns_cancel_publish(sess, i.id);
		emit q->error(i.id, i.error);
		if(!self)
			return;
	}

	while(!published.isEmpty())
	{
		int i = published.takeFirst();
		emit q->published(i);
		if(!self)
			return;
	}

	while(!responses.isEmpty())
	{
		LateResponse i = responses.takeFirst();
		if(i.do_cancel)
			jdns_cancel_query(sess, i.id);
		emit q->resultsReady(i.id, i.response);
		if(!self)
			return;
	}

	if(finish_shutdown)
	{
		// if we have pending udp packets to write, stick around
		if(pending > 0)
		{
			pending_wait = true;
		}
		else
		{
			complete_shutdown = true;
			process();
		}
	}

	pErrors = 0;
	pPublished = 0;
	pResponses = 0;
}

void QJDns::Private::removeCancelled(int id)
{
	if(pErrors)
	{
		for(int n = 0; n < pErrors->count(); ++n)
		{
			if(pErrors->at(n).id == id)
			{
				pErrors->removeAt(n);
				--n; // adjust position
			}
		}
	}

	if(pPublished)
	{
		for(int n = 0; n < pPublished->count(); ++n)
		{
			if(pPublished->at(n) == id)
			{
				pPublished->removeAt(n);
				--n; // adjust position
			}
		}
	}

	if(pResponses)
	{
		for(int n = 0; n < pResponses->count(); ++n)
		{
			if(pResponses->at(n).id == id)
			{
				pResponses->removeAt(n);
				--n; // adjust position
			}
		}
	}
}

void QJDns::Private::udp_readyRead()
{
	QUdpSocket *sock = (QUdpSocket *)sender();
	int handle = handleForSocket.value(sock);

	if(need_handle)
	{
		jdns_set_handle_readable(sess, handle);
		process();
	}
	else
	{
		// eat packet
		QByteArray buf(4096, 0);
		QHostAddress from_addr;
		quint16 from_port;
		sock->readDatagram(buf.data(), buf.size(), &from_addr, &from_port);
	}
}

void QJDns::Private::udp_bytesWritten(qint64)
{
	if(pending > 0)
	{
		--pending;
		if(shutting_down && pending_wait && pending == 0)
		{
			pending_wait = false;
			complete_shutdown = true;
			process();
		}
	}
}

void QJDns::Private::st_timeout()
{
	doNextStep();
}

void QJDns::Private::doNextStepSlot()
{
	doNextStep();
}

void QJDns::Private::doDebug()
{
	if(new_debug_strings)
	{
		new_debug_strings = false;
		if(!debug_strings.isEmpty())
			emit q->debugLinesReady();
	}
}

QJDns::QJDns(QObject *parent)
:QObject(parent)
{
	d = new Private(this);
}

QJDns::~QJDns()
{
	delete d;
}

bool QJDns::init(Mode mode, const QHostAddress &address)
{
	return d->init(mode, address);
}

void QJDns::shutdown()
{
	d->shutting_down = true;
	d->pending_wait = false;
	d->complete_shutdown = false;
	jdns_shutdown(d->sess);
	d->process();
}

QStringList QJDns::debugLines()
{
	QStringList tmp = d->debug_strings;
	d->debug_strings.clear();
	return tmp;
}

QJDns::SystemInfo QJDns::systemInfo()
{
	SystemInfo out;
	jdns_dnsparams_t *params = jdns_system_dnsparams();
	for(int n = 0; n < params->nameservers->count; ++n)
	{
		NameServer ns;
		ns.address = addr2qt(params->nameservers->item[n]->address);
		out.nameServers += ns;
	}
	for(int n = 0; n < params->domains->count; ++n)
		out.domains += str2qt(params->domains->item[n]);
	for(int n = 0; n < params->hosts->count; ++n)
	{
		DnsHost h;
		h.name = str2qt(params->hosts->item[n]->name);
		h.address = addr2qt(params->hosts->item[n]->address);
		out.hosts += h;
	}
	jdns_dnsparams_delete(params);
	return out;
}

#define PROBE_BASE  20000
#define PROBE_RANGE   100

QHostAddress QJDns::detectPrimaryMulticast(const QHostAddress &address)
{
	my_srand();

	QUdpSocket *sock = new QUdpSocket;
	QUdpSocket::BindMode mode;
	mode |= QUdpSocket::ShareAddress;
	mode |= QUdpSocket::ReuseAddressHint;
	int port = -1;
	for(int n = 0; n < PROBE_RANGE; ++n)
	{
		if(sock->bind(address, PROBE_BASE + n, mode))
		{
			port = PROBE_BASE + n;
			break;
		}
	}
	if(port == -1)
	{
		delete sock;
		return QHostAddress();
	}

	jdns_address_t *a;
	if(address.protocol() == QAbstractSocket::IPv6Protocol)
		a = jdns_address_multicast6_new();
	else
		a = jdns_address_multicast4_new();
	QHostAddress maddr = addr2qt(a);
	jdns_address_delete(a);

	if(address.protocol() == QAbstractSocket::IPv6Protocol)
	{
		int x;
		if(!qjdns_sock_setMulticast6(sock->socketDescriptor(), maddr.toIPv6Address().c, &x))
		{
			delete sock;
			return QHostAddress();
		}
		qjdns_sock_setTTL6(sock->socketDescriptor(), 0);
	}
	else
	{
		int x;
		if(!qjdns_sock_setMulticast4(sock->socketDescriptor(), maddr.toIPv4Address(), &x))
		{
			delete sock;
			return QHostAddress();
		}
		qjdns_sock_setTTL4(sock->socketDescriptor(), 0);
	}

	QHostAddress result;
	QByteArray out(128, 0);
	for(int n = 0; n < out.size(); ++n)
		out[n] = rand();
	if(sock->writeDatagram(out.data(), out.size(), maddr, port) == -1)
	{
		delete sock;
		return QHostAddress();
	}
	while(1)
	{
		if(!sock->waitForReadyRead(1000))
		{
			fprintf(stderr, "QJDns::detectPrimaryMulticast: timeout while checking %s\n", qPrintable(address.toString()));
			delete sock;
			return QHostAddress();
		}
		QByteArray in(128, 0);
		QHostAddress from_addr;
		quint16 from_port;
		int ret = sock->readDatagram(in.data(), in.size(), &from_addr, &from_port);
		if(ret == -1)
		{
			delete sock;
			return QHostAddress();
		}

		if(from_port != port)
			continue;
		in.resize(ret);
		if(in != out)
			continue;

		result = from_addr;
		break;
	}
	delete sock;

	return result;
}

void QJDns::setNameServers(const QList<NameServer> &list)
{
	d->setNameServers(list);
}

int QJDns::queryStart(const QByteArray &name, int type)
{
	int id = jdns_query(d->sess, (const unsigned char *)name.data(), type);
	d->process();
	return id;
}

void QJDns::queryCancel(int id)
{
	jdns_cancel_query(d->sess, id);
	d->removeCancelled(id);
	d->process();
}

int QJDns::publishStart(PublishMode m, const Record &record)
{
	jdns_rr_t *rr = export_record(record);

	int pubmode;
	if(m == QJDns::Unique)
		pubmode = JDNS_PUBLISH_UNIQUE;
	else
		pubmode = JDNS_PUBLISH_SHARED;

	int id = jdns_publish(d->sess, pubmode, rr);
	jdns_rr_delete(rr);
	d->process();
	return id;
}

void QJDns::publishUpdate(int id, const Record &record)
{
	jdns_rr_t *rr = export_record(record);

	jdns_update_publish(d->sess, id, rr);
	jdns_rr_delete(rr);
	d->process();
}

void QJDns::publishCancel(int id)
{
	jdns_cancel_publish(d->sess, id);
	d->removeCancelled(id);
	d->process();
}

//#include "qjdns.moc"
