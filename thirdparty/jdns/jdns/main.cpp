/*
 * Copyright (C) 2005  Justin Karneges
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

#include <QtCore>
#include <QtNetwork>
#include "qjdns.h"

QString dataToString(const QByteArray &buf)
{
	QString out;
	for(int n = 0; n < buf.size(); ++n)
	{
		unsigned char c = (unsigned char)buf[n];
		if(c == '\\')
			out += "\\\\";
		else if(c >= 0x20 && c < 0x7f)
			out += c;
		else
			out += QString().sprintf("\\x%02x", (unsigned int)c);
	}
	return out;
}

void print_record(const QJDns::Record &r)
{
	switch(r.type)
	{
		case QJDns::A:
			printf("  A: [%s] (ttl=%d)\n", qPrintable(r.address.toString()), r.ttl);
			break;
		case QJDns::Aaaa:
			printf("  AAAA: [%s] (ttl=%d)\n", qPrintable(r.address.toString()), r.ttl);
			break;
		case QJDns::Mx:
			printf("  MX: [%s] priority=%d (ttl=%d)\n", r.name.data(), r.priority, r.ttl);
			break;
		case QJDns::Srv:
			printf("  SRV: [%s] port=%d priority=%d weight=%d (ttl=%d)\n", r.name.data(), r.port, r.priority, r.weight, r.ttl);
			break;
		case QJDns::Cname:
			printf("  CNAME: [%s] (ttl=%d)\n", r.name.data(), r.ttl);
			break;
		case QJDns::Ptr:
			printf("  PTR: [%s] (ttl=%d)\n", r.name.data(), r.ttl);
			break;
		case QJDns::Txt:
		{
			printf("  TXT: count=%d (ttl=%d)\n", r.texts.count(), r.ttl);
			for(int n = 0; n < r.texts.count(); ++n)
				printf("    len=%d [%s]\n", r.texts[n].size(), qPrintable(dataToString(r.texts[n])));
			break;
		}
		case QJDns::Hinfo:
			printf("  HINFO: [%s] [%s] (ttl=%d)\n", r.cpu.data(), r.os.data(), r.ttl);
			break;
		case QJDns::Ns:
			printf("  NS: [%s] (ttl=%d)\n", r.name.data(), r.ttl);
			break;
		default:
			printf("  (Unknown): type=%d, size=%d (ttl=%d)\n", r.type, r.rdata.size(), r.ttl);
			break;
	}
}

class App : public QObject
{
	Q_OBJECT
public:
	bool opt_debug, opt_ipv6, opt_quit;
	int quit_time;
	QString mode, type, name, ipaddr;
	QStringList nslist;
	QList<QJDns::Record> pubitems;
	QJDns jdns;
	int req_id;

	App()
	{
		connect(&jdns, SIGNAL(resultsReady(int, const QJDns::Response &)), SLOT(jdns_resultsReady(int, const QJDns::Response &)));
		connect(&jdns, SIGNAL(published(int)), SLOT(jdns_published(int)));
		connect(&jdns, SIGNAL(error(int, QJDns::Error)), SLOT(jdns_error(int, QJDns::Error)));
		connect(&jdns, SIGNAL(shutdownFinished()), SLOT(jdns_shutdownFinished()));
		connect(&jdns, SIGNAL(debugLinesReady()), SLOT(jdns_debugLinesReady()));
	}

	~App()
	{
	}

public slots:
	void start()
	{
		if(mode == "uni")
		{
			if(!jdns.init(QJDns::Unicast, opt_ipv6 ? QHostAddress::AnyIPv6 : QHostAddress::Any))
			{
				jdns_debugLinesReady();
				printf("unable to bind\n");
				emit quit();
				return;
			}

			QList<QJDns::NameServer> addrs;
			for(int n = 0; n < nslist.count(); ++n)
			{
				QJDns::NameServer host;
				QString str = nslist[n];
				if(str == "mul")
				{
					if(opt_ipv6)
						host.address = QHostAddress("FF02::FB");
					else
						host.address = QHostAddress("224.0.0.251");
					host.port = 5353;
				}
				else
				{
					int at = str.indexOf(';');
					if(at != -1)
					{
						host.address = QHostAddress(str.mid(0, at));
						host.port = str.mid(at + 1).toInt();
					}
					else
					{
						host.address = QHostAddress(str);
					}
				}

				if(host.address.isNull() || host.port <= 0)
				{
					printf("bad nameserver: [%s]\n", qPrintable(nslist[n]));
					emit quit();
					return;
				}
				addrs += host;
			}

			if(addrs.isEmpty())
				addrs = QJDns::systemInfo().nameServers;

			if(addrs.isEmpty())
			{
				printf("no nameservers were detected or specified\n");
				emit quit();
				return;
			}

			jdns.setNameServers(addrs);
		}
		else
		{
			if(!jdns.init(QJDns::Multicast, opt_ipv6 ? QHostAddress::AnyIPv6 : QHostAddress::Any))
			{
				jdns_debugLinesReady();
				printf("unable to bind\n");
				emit quit();
				return;
			}
		}

		if(mode == "uni" || mode == "mul")
		{
			int x = QJDns::A;
			if(type == "ptr")
				x = QJDns::Ptr;
			else if(type == "srv")
				x = QJDns::Srv;
			else if(type == "a")
				x = QJDns::A;
			else if(type == "aaaa")
				x = QJDns::Aaaa;
			else if(type == "mx")
				x = QJDns::Mx;
			else if(type == "txt")
				x = QJDns::Txt;
			else if(type == "hinfo")
				x = QJDns::Hinfo;
			else if(type == "cname")
				x = QJDns::Cname;
			else if(type == "any")
				x = QJDns::Any;
			else
			{
				bool ok;
				int y = type.toInt(&ok);
				if(ok)
					x = y;
			}

			req_id = jdns.queryStart(name.toLatin1(), x);
			printf("[%d] Querying for [%s] type=%d ...\n", req_id, qPrintable(name), x);
		}
		else // publish
		{
			for(int n = 0; n < pubitems.count(); ++n)
			{
				const QJDns::Record &rr = pubitems[n];
				QJDns::PublishMode m = QJDns::Unique;
				if(rr.type == QJDns::Ptr)
					m = QJDns::Shared;
				int id = jdns.publishStart(m, rr);
				printf("[%d] Publishing [%s] type=%d ...\n", id, rr.owner.data(), rr.type);
			}
		}

		if(opt_quit)
			QTimer::singleShot(quit_time * 1000, this, SLOT(doShutdown()));
	}

signals:
	void quit();

private slots:
	void jdns_resultsReady(int id, const QJDns::Response &results)
	{
		printf("[%d] Results\n", id);
		for(int n = 0; n < results.answerRecords.count(); ++n)
			print_record(results.answerRecords[n]);

		if(mode == "uni")
			jdns.shutdown();
	}

	void jdns_published(int id)
	{
		printf("[%d] Published\n", id);
	}

	void jdns_error(int id, QJDns::Error e)
	{
		QString str;
		if(e == QJDns::ErrorGeneric)
			str = "Generic";
		else if(e == QJDns::ErrorNXDomain)
			str = "NXDomain";
		else if(e == QJDns::ErrorTimeout)
			str = "Timeout";
		else if(e == QJDns::ErrorConflict)
			str = "Conflict";
		printf("[%d] Error: %s\n", id, qPrintable(str));
		jdns.shutdown();
	}

	void jdns_shutdownFinished()
	{
		emit quit();
	}

	void jdns_debugLinesReady()
	{
		QStringList lines = jdns.debugLines();
		if(opt_debug)
		{
			for(int n = 0; n < lines.count(); ++n)
				printf("jdns: %s\n", qPrintable(lines[n]));
		}
	}

	void doShutdown()
	{
		jdns.shutdown();
	}
};

#include "main.moc"

void usage()
{
	printf("usage: jdns (options) uni [type] [name] (nameserver(;port)|mul ...)\n");
	printf("       jdns (options) mul [type] [name]\n");
	printf("       jdns (options) pub [items ...]\n");
	printf("       jdns sys\n");
	printf("\n");
	printf("options:\n");
	printf("  -d     show debug output\n");
	printf("  -6     use ipv6\n");
	printf("  -q x   quit x seconds after starting\n");
	printf("\n");
	printf("uni/mul types: a aaaa ptr srv mx txt hinfo cname any\n");
	printf("pub items: ptr:name,answer srv:name,answer,port a:name,ipaddr\n");
	printf("           txt:name,str0,...,strn aaaa:name,ipaddr\n");
	printf("\n");
	printf("examples:\n");
	printf("  jdns uni a jabber.org 192.168.0.1\n");
	printf("  jdns uni srv _xmpp-client._tcp.jabber.org 192.168.0.1;53\n");
	printf("  jdns uni 10 user@host._presence._tcp.local mul\n");
	printf("  jdns mul a foobar.local\n");
	printf("  jdns mul ptr _services._dns-sd._udp.local\n");
	printf("  jdns pub a:mybox.local.,192.168.0.55\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	if(argc < 2)
	{
		usage();
		return 1;
	}

	// get args
	QStringList args;
	for(int n = 1; n < argc; ++n)
		args += QString(argv[n]);

	bool opt_debug = false;
	bool opt_ipv6 = false;
	bool opt_quit = false;
	int quit_time = 0;
	QString mode, type, name, ipaddr;
	QStringList nslist;
	QList<QJDns::Record> pubitems;

	// options
	for(int n = 0; n < args.count(); ++n)
	{
		if(args[n].left(1) == "-")
		{
			if(args[n] == "-d")
				opt_debug = true;
			else if(args[n] == "-6")
				opt_ipv6 = true;
			else if(args[n] == "-q")
			{
				if(n + 1 >= args.count())
				{
					printf("need to specify number of seconds\n");
					usage();
					return 1;
				}

				int x = args[n + 1].toInt();
				if(x < 1)
					x = 30;

				opt_quit = true;
				quit_time = x;

				args.removeAt(n + 1);
			}
			else
			{
				printf("bad option\n");
				usage();
				return 1;
			}
			args.removeAt(n);
			--n; // adjust position
		}
	}

	mode = args[0];
	if(mode == "uni" || mode == "mul")
	{
		if(args.count() < 3)
		{
			printf("not enough args\n");
			usage();
			return 1;
		}
		type = args[1];
		name = args[2];
		if(mode == "uni")
		{
			for(int n = 3; n < args.count(); ++n)
				nslist += QString(args[n]);
		}
	}
	else if(mode == "pub")
	{
		if(args.count() < 2)
		{
			printf("not enough args\n");
			usage();
			return 1;
		}
		for(int n = 1; n < args.count(); ++n)
		{
			QString arg = args[n];
			int at = arg.indexOf(':');
			if(at == -1)
			{
				printf("missing colon\n");
				usage();
				return 1;
			}
			QString type = arg.mid(0, at).toLower();
			QString val = arg.mid(at + 1);
			if(type == "a")
			{
				QStringList list = val.split(',');
				if(list.count() != 2)
				{
					printf("bad format for A type\n");
					usage();
					return 1;
				}
				QHostAddress host(list[1]);
				if(host.isNull() || host.protocol() != QAbstractSocket::IPv4Protocol)
				{
					printf("bad format for A type IP address\n");
					usage();
					return 1;
				}

				QJDns::Record rec;
				rec.owner = list[0].toLatin1();
				rec.type = QJDns::A;
				rec.ttl = 120;
				rec.haveKnown = true;
				rec.address = host;
				pubitems += rec;
			}
			else if(type == "aaaa")
			{
				QStringList list = val.split(',');
				if(list.count() != 2)
				{
					printf("bad format for AAAA type\n");
					usage();
					return 1;
				}
				QHostAddress host(list[1]);
				if(host.isNull() || host.protocol() != QAbstractSocket::IPv6Protocol)
				{
					printf("bad format for AAAA type IP address\n");
					usage();
					return 1;
				}

				QJDns::Record rec;
				rec.owner = list[0].toLatin1();
				rec.type = QJDns::Aaaa;
				rec.ttl = 120;
				rec.haveKnown = true;
				rec.address = host;
				pubitems += rec;
			}
			else if(type == "srv")
			{
				QStringList list = val.split(',');
				if(list.count() != 3)
				{
					printf("bad format for SRV type\n");
					usage();
					return 1;
				}

				QJDns::Record rec;
				rec.owner = list[0].toLatin1();
				rec.type = QJDns::Srv;
				rec.ttl = 120;
				rec.haveKnown = true;
				rec.name = list[1].toLatin1();
				rec.priority = 0;
				rec.weight = 0;
				rec.port = list[2].toInt();
				pubitems += rec;
			}
			else if(type == "ptr")
			{
				QStringList list = val.split(',');
				if(list.count() != 2)
				{
					printf("bad format for PTR type\n");
					usage();
					return 1;
				}

				QJDns::Record rec;
				rec.owner = list[0].toLatin1();
				rec.type = QJDns::Ptr;
				rec.ttl = 120;
				rec.haveKnown = true;
				rec.name = list[1].toLatin1();
				pubitems += rec;
			}
			else if(type == "txt")
			{
				QStringList list = val.split(',');
				QList<QByteArray> texts;
				for(int n = 1; n < list.count(); ++n)
					texts += list[n].toLatin1();

				QJDns::Record rec;
				rec.owner = list[0].toLatin1();
				rec.type = QJDns::Txt;
				rec.ttl = 120;
				rec.haveKnown = true;
				rec.texts = texts;
				pubitems += rec;
			}
			else
			{
				printf("bad record type [%s]\n", qPrintable(type));
				usage();
				return 1;
			}
		}
	}
	else if(mode == "sys")
	{
		QJDns::SystemInfo info = QJDns::systemInfo();

		printf("DNS System Information\n");
		printf("  Name Servers:\n");
		if(!info.nameServers.isEmpty())
		{
			for(int n = 0; n < info.nameServers.count(); ++n)
				printf("    %s\n", qPrintable(info.nameServers[n].address.toString()));
		}
		else
			printf("    (None)\n");

		printf("  Domains:\n");
		if(!info.domains.isEmpty())
		{
			for(int n = 0; n < info.domains.count(); ++n)
				printf("    [%s]\n", info.domains[n].data());
		}
		else
			printf("    (None)\n");

		printf("  Hosts:\n");
		if(!info.hosts.isEmpty())
		{
			for(int n = 0; n < info.hosts.count(); ++n)
			{
				const QJDns::DnsHost &h = info.hosts[n];
				printf("    [%s] -> %s\n", h.name.data(), qPrintable(h.address.toString()));
			}
		}
		else
			printf("    (None)\n");

		QHostAddress addr;
		printf("Primary IPv4 Multicast Address: ");
		addr = QJDns::detectPrimaryMulticast(QHostAddress::Any);
		if(!addr.isNull())
			printf("%s\n", qPrintable(addr.toString()));
		else
			printf("(None)\n");
		printf("Primary IPv6 Multicast Address: ");
		addr = QJDns::detectPrimaryMulticast(QHostAddress::AnyIPv6);
		if(!addr.isNull())
			printf("%s\n", qPrintable(addr.toString()));
		else
			printf("(None)\n");

		return 0;
	}
	else
	{
		usage();
		return 1;
	}

	App a;
	a.opt_debug = opt_debug;
	a.opt_ipv6 = opt_ipv6;
	a.opt_quit = opt_quit;
	a.quit_time = quit_time;
	a.mode = mode;
	a.type = type.toLower();
	a.name = name;
	a.ipaddr = ipaddr;
	a.nslist = nslist;
	a.pubitems = pubitems;
	QObject::connect(&a, SIGNAL(quit()), &app, SLOT(quit()));
	QTimer::singleShot(0, &a, SLOT(start()));
	app.exec();
	return 0;
}
