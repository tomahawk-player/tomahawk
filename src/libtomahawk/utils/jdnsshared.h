/*
 * Copyright (C) 2006,2007  Justin Karneges
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

#ifndef JDNSSHARED_H
#define JDNSSHARED_H

#include "qjdns.h"

class JDnsShared;
class JDnsSharedPrivate;
class JDnsSharedRequestPrivate;
class JDnsSharedDebugPrivate;

/**
   \brief Collects debugging information from JDnsShared

   \note Iris users should utilize NetNames for DNS capabilities, <i>not</i> JDnsSharedDebug.  See the JDnsShared documentation for more information.

   JDnsSharedDebug is used to collect debugging information from one or many JDnsShared objects.  To use it, simply create it and pass it to JDnsShared::setDebug().

   Example use:

\code
JDnsSharedDebug *db = new JDnsSharedDebug;
connect(db, SIGNAL(debugLinesReady(const QStringList &)),
	SLOT(db_debugLinesReady(const QStringList &)));

JDnsShared *jdnsShared1 = new JDnsShared(JDnsShared::UnicastInternet);
jdnsShared1->setDebug(db, "U");

JDnsShared *jdnsShared2 = new JDnsShared(JDnsShared::UnicastLocal);
jdnsShared2->setDebug(db, "L");
...
void db_debugLinesReady(const QStringList &lines)
{
	foreach(QString line, lines)
		printf("%s\n", qPrintable(line));
}
\endcode

   JDnsShared reports debug lines with the name and interface number prepended to each line.  For example, if there is debug information to report about the second interface added to \a jdnsShared2 in the above example, the lines would be prepended with "L1: ".

   Do not destroy JDnsSharedDebug until all of the JDnsShared objects associated with it have been destroyed.

   \sa JDnsShared JDnsSharedRequest
*/
class JDnsSharedDebug : public QObject
{
	Q_OBJECT
public:
	/**
	   \brief Constructs a new object with the given \a parent
	*/
	JDnsSharedDebug(QObject *parent = 0);

	/**
	   \brief Destroys the object
	*/
	~JDnsSharedDebug();

	/**
	   \brief Read the available debug information

	   Debug information is reported as a series of lines.  The lines are of reasonable length, and so if you're storing a backlog of the most recent debug information, it should be safe to make the cut-off point based on lines.

	   \sa readyRead
	*/
	QStringList readDebugLines();

signals:
	/**
	   \brief Emitted when there is debug information to report

	   \sa readDebugLines
	*/
	void readyRead();

private:
	friend class JDnsShared;
	friend class JDnsSharedPrivate;
	friend class JDnsSharedDebugPrivate;
	JDnsSharedDebugPrivate *d;
};

/**
   \brief Performs a DNS operation using JDnsShared

   \note Iris users should utilize NetNames for DNS capabilities, <i>not</i> JDnsSharedRequest.  See the JDnsShared documentation for more information.

   JDnsSharedRequest is used to perform DNS operations on a JDnsShared object.  Many requests may be performed simultaneously, such that a single JDnsShared object can be "shared" across the application.  Please see the JDnsShared documentation for more complete information about how the overall system works.

   Call query() to perform a query.  Call publish() (or publishUpdate()) to make DNS records available on the local network (JDnsShared::Multicast mode only).  When the operation has something to report, the resultsReady() signal is emitted.  Call success() to determine the status of the operation.  If success() returns false, then the operation has failed and the reason for the failure can be determined with error().  If success() returns true, then the meaning differs depending on the type of operation being performed:
   <ul>
     <li>For JDnsShared::UnicastInternet and JDnsShared::UnicastLocal modes, call results() to obtain the records obtained by the query.  In these modes, resultsReady() is only emitted once, at which point the operation is no longer active.</li>
     <li>For JDnsShared::Multicast, operations are long-lived.  Query operations never timeout, and resultsReady() may be emitted multiple times.  In order to stop the query, either call cancel() or destroy the JDnsSharedRequest object.  Similarly, publishing is long-lived.  The record stays published as long as the JDnsSharedRequest has not been cancelled or destroyed.</li>
   </ul>

   Here is how you might look up an A record:

\code
JDnsSharedRequest *req = new JDnsSharedRequest(jdnsShared);
connect(req, SIGNAL(resultsReady()), SLOT(req_resultsReady()));
req->query("psi-im.org", QJDns::A);
...
void req_resultsReady()
{
	if(req->success())
	{
		// print all of the IP addresses obtained
		QList<QJDns::Record> results = req->results();
		foreach(QJDns::Record r, results)
		{
			if(r.type == QJDns::A)
				printf("%s\n", qPrintable(r.address.toString());
		}
	}
	else
		printf("Error resolving!\n");
}
\endcode

   Here is an example of publishing a record:

\code
JDnsSharedRequest *pub = new JDnsSharedRequest(jdnsShared);
connect(pub, SIGNAL(resultsReady()), SLOT(pub_resultsReady()));

// let's publish an A record
QJDns::Record rec;
rec.owner = "SomeComputer.local.";
rec.type = QJDns::A;
rec.ttl = 120;
rec.haveKnown = true;
rec.address = QHostAddress("192.168.0.32");

pub->publish(QJDns::Unique, rec);
...
void pub_resultsReady()
{
	if(pub->success())
		printf("Record published\n");
	else
		printf("Error publishing!\n");
}
\endcode

   To update an existing record, use publishUpdate():

\code
// the IP address of the host changed, so make a new record
QJDns::Record rec;
rec.owner = "SomeComputer.local.";
rec.type = QJDns::A;
rec.ttl = 120;
rec.haveKnown = true;
rec.address = QHostAddress("192.168.0.64");

// update it
pub->publishUpdate(rec);
\endcode

   As a special exception, the address value can be left unspecified for A and Aaaa record types, which tells JDnsShared to substitute the address value with the address of whatever interfaces the record gets published on.  This is the preferred way to publish the IP address of your own machine, and in fact it is the only way to do so if you have multiple interfaces, because there will likely be a different IP address value for each interface (the record resolves to a different answer depending on which interface a query comes from).

\code
// let's publish our own A record
QJDns::Record rec;
rec.owner = "MyComputer.local.";
rec.type = QJDns::A;
rec.ttl = 120;
rec.haveKnown = true;
rec.address = QHostAddress();

pub->publish(QJDns::Unique, rec);
\endcode

   When you want to unpublish, call cancel() or destroy the JDnsSharedRequest.

   \sa JDnsShared
*/
class JDnsSharedRequest : public QObject
{
	Q_OBJECT
public:
	/**
	   \brief Operation type
	*/
	enum Type
	{
		Query,   ///< Query operation, initiated by query()
		Publish  ///< Publish operation, initiated by publish() or publishUpdate()
	};

	/**
	   \brief Request error
	*/
	enum Error
	{
		ErrorNoNet,     ///< There are no available network interfaces to operate on.  This happens if JDnsShared::addInterface() was not called.
		ErrorGeneric,   ///< Generic error during the operation.
		ErrorNXDomain,  ///< The name looked up does not exist.
		ErrorTimeout,   ///< The operation timed out.
		ErrorConflict   ///< Attempt to publish an already published unique record.
	};

	/**
	   \brief Constructs a new object with the given \a jdnsShared and \a parent
	*/
	JDnsSharedRequest(JDnsShared *jdnsShared, QObject *parent = 0);

	/**
	   \brief Destroys the object

	   If there is an active operation, it is cancelled.
	*/
	~JDnsSharedRequest();

	/**
	   \brief The type of operation being performed
	*/
	Type type();

	/**
	   \brief Perform a query operation
	*/
	void query(const QByteArray &name, int type);

	/**
	   \brief Perform a publish operation
	*/
	void publish(QJDns::PublishMode m, const QJDns::Record &record);

	/**
	   \brief Update a record that is currently published
	*/
	void publishUpdate(const QJDns::Record &record);

	/**
	   \brief Cancels the current operation
	*/
	void cancel();

	/**
	   \brief Indicates whether or not the operation was successful
	*/
	bool success() const;

	/**
	   \brief Returns the reason for error
	*/
	Error error() const;

	/**
	   \brief Returns the results of the operation
	*/
	QList<QJDns::Record> results() const;

signals:
	/**
	   \brief Indicates that the operation has something to report

	   After receiving this signal, call success() to check on the status of the operation, followed by results() or error() as appropriate.
	*/
	void resultsReady();

private:
	friend class JDnsShared;
	friend class JDnsSharedPrivate;
	friend class JDnsSharedRequestPrivate;
	JDnsSharedRequestPrivate *d;
};

/**
   \brief Abstraction layer on top of QJDns

   \note Iris users should utilize NetNames for DNS capabilities, <i>not</i> JDnsShared.  JDnsShared is provided for non-Iris users (and it is also used internally by NetNames).  To use JDnsShared by itself, simply drop the jdnsshared.h and jdnsshared.cpp files, along with JDNS, into your project.  It is not a full replacement for Qt's Q3Dns, as some tasks are left to you, but it covers most of it.

   QJDns supports everything a typical application should ever need in DNS.  However, it is expected that modern applications will need to maintain multiple QJDns instances at the same time, and this is where things can get complicated.  For example, most applications will want at least two QJDns instances: one for IPv4 unicast and one for IPv6 unicast.

   A single JDnsShared object encapsulates multiple instances of QJDns that are related.  For example, an IPv4 unicast instance and an IPv6 unicast instance could be coupled within JDnsShared.  Then, when a unicast operation is performed on the JDnsShared object, both underlying instances will be queried as appropriate.  The application would not need to perform two resolutions itself, nor deal with any related complexity.

   Further, individual operations are performed using a separate class called JDnsSharedRequest, eliminating the need for the application to directly interface with a central QJDns object or track integer handles.  This makes it easier for individual parts of the application to "share" the same instance (or set of instances) of QJDns, hence the name.

   JDnsShared is a thin abstraction.  QJDns subtypes (e.g. QJDns::Type, QJDns::Record, etc) are still used with JDnsShared.  Because of the duplication of documentation effort between NetNames and QJDns, there is no formal documentation for QJDns.  Users of JDnsShared will need to read qjdns.h, although a basic explanation of the elements can be found below.

   Types:
   <table>
     <tr><td>QJDns::Type</td><td>This is a convenience enumeration for common DNS record types.  For example: A, Aaaa, Srv, etc.  The values directly map to the integer values of the DNS protocol (e.g. Srv = 33).  See qjdns.h for all of the types and values.</td></tr>
     <tr><td>QJDns::Record</td><td>This class holds a DNS record.  The main fields are <i>type</i> (integer type, probably something listed in QJDns::Type), <i>rdata</i> (QByteArray of the record value), and <i>haveKnown</i> (boolean to indicate if a decoded form of the record value is also available).  See qjdns.h for the possible known fields.  You will most-likely always work with known types.  Received records that have a type listed in QJDns::Type are guaranteed to be known and will provide a decoded value.  If you are creating a record for publishing, you will need to set <i>owner</i>, <i>ttl</i>, and <i>type</i>.  If the type to be published is listed in QJDns::Type, then you will need to set <i>haveKnown</i> to true and set the known fields as appropriate, otherwise you need to set <i>rdata</i>.  You do not need to supply an encoded form in <i>rdata</i> for known types, it can be left empty in that case.</td></tr>
     <tr><td>QJDns::PublishMode</td><td>This is for Multicast DNS, and can either be Unique or Shared.  A shared record can be published by multiple owners (for example, a "_ssh._tcp.local." PTR record might resolve to many different SSH services owned by different machines).  A unique record can only have one owner (for example, a "mycomputer.local." A record would resolve to the IP address of the machine that published it).  Attempting to publish a record on a network where a unique record is already present will result in a conflict error.</td></tr>
   </table>

   Functions:
   <table>
     <tr><td>QJDns::detectPrimaryMulticast()</td><td>Detects a multicast interface.  Pass QHostAddress::Any or QHostAddress::AnyIPv6, depending on which type of interface is desired.</td></tr>
   </table>

   To use JDnsShared, first create an instance of it, set it up by calling addInterface() as necessary, and then use JDnsSharedRequest to perform operations on it.

   Here is an example of how to create and set up a JDnsShared object for typical DNS resolution:

\code
// construct
JDnsShared *dns = new JDnsShared(JDnsShared::UnicastInternet);

// add IPv4 and IPv6 interfaces
dns->addInterface(QHostAddress::Any);
dns->addInterface(QHostAddress::AnyIPv6);

// at this point, the object is ready for operation
\endcode

   Perform a resolution like this:

\code
JDnsSharedRequest *req = new JDnsSharedRequest(dns);
connect(req, SIGNAL(resultsReady()), SLOT(req_resultsReady()));
req->query("psi-im.org", QJDns::A);
...
void req_resultsReady()
{
	if(req->success())
	{
		// print all of the IP addresses obtained
		QList<QJDns::Record> results = req->results();
		foreach(QJDns::Record r, results)
		{
			if(r.type == QJDns::A)
				printf("%s\n", qPrintable(r.address.toString());
		}
	}
	else
		printf("Error resolving!\n");
}
\endcode

   It is important to filter the results as shown in the above example.  QJDns guarantees at least one record in the results will be of the type queried for, but there may also be CNAME records present (of course, if the query was for a CNAME type, then the results will only be CNAME records).  The recommended approach is to simply filter for the record types desired, as shown, rather than single out CNAME specifically.

   When you are finished with a JDnsShared object, it should be shut down before deleting:

\code
connect(dns, SIGNAL(shutdownFinished()), SLOT(dns_shutdownFinished()));
dns->shutdown();
...
void dns_shutdownFinished()
{
	delete dns;
}
\endcode

   Setting up JDnsShared for UnicastLocal and Multicast mode is done the same way as with UnicastInternet.

   For example, here is how Multicast mode could be set up:

\code
// construct
JDnsShared *dns = new JDnsShared(JDnsShared::Multicast);

// add IPv4 interface
QHostAddress addr = QJDns::detectPrimaryMulticast(QHostAddress::Any);
dns->addInterface(addr);

// at this point, the object is ready for operation
\endcode

   JDnsShared provides a lot of functionality, but certain aspects of DNS are deemed out of its scope.  Below are the responsibilities of the user of JDnsShared, if a more complete DNS behavior is desired:
   <ul>
     <li>Querying for several "qualified" names.  You should first query for the name as provided, and if that fails then query for name + ".domain" (for every domain the computer is in).  See domains().</li>
     <li>Detecting for ".local" in the name to be queried, and using that to decide whether to query via Multicast/UnicastLocal or UnicastInternet.</li>
     <li>For zeroconf/Bonjour, keep in mind that JDnsShared only provides low-level record queries.  DNS-SD and any higher layers would be your job.</li>
   </ul>

   Using a custom DNS implementation, such as JDnsShared, has the drawback that it is difficult to take advantage of platform-specific features (for example, an OS-wide DNS cache or LDAP integration).  An application strategy for normal DNS should probably be:
   <ul>
     <li>If an A or AAAA record is desired, use a native lookup.</li>
     <li>Else, if the platform has advanced DNS features already (ie, res_query), use those.</li>
     <li>Else, use JDnsShared.</li>
   </ul>

   For Multicast DNS, awareness of the platform is doubly important.  There should only be one Multicast DNS "Responder" per computer, and using JDnsShared in Multicast mode at the same time could result in a conflict.  An application strategy for Multicast DNS should be:
   <ul>
     <li>If the platform has a Multicast DNS daemon installed already, use it somehow.</li>
     <li>Else, use JDnsShared.</li>
   </ul>

   \sa JDnsSharedRequest
*/
class JDnsShared : public QObject
{
	Q_OBJECT
public:
	/**
	   \brief The mode to operate in
	*/
	enum Mode
	{
		/**
		   For regular DNS resolution.  In this mode, lookups are performed on all interfaces, and the first returned result is used.
		*/
		UnicastInternet,

		/**
		   Perform regular DNS resolution using the Multicast DNS address.  This is used to resolve large and/or known Multicast DNS names without actually multicasting anything.
		*/
		UnicastLocal,

		/**
		   Multicast DNS querying and publishing.

		   \note For Multicast mode, JDnsShared supports up to one interface for each IP version (e.g. one IPv4 interface and one IPv6 interface), and expects the default/primary multicast interface for that IP version to be used.
		*/
		Multicast
	};

	/**
	   \brief Constructs a new object with the given \a mode and \a parent
	*/
	JDnsShared(Mode mode, QObject *parent = 0);

	/**
	   \brief Destroys the object
	*/
	~JDnsShared();

	/**
	   \brief Sets the debug object to report to

	   If a debug object is set using this function, then JDnsShared will send output text to it, prefixing each line with \a name.
	*/
	void setDebug(JDnsSharedDebug *db, const QString &name);

	/**
	   \brief Adds an interface to operate on

	   For UnicastInternet and UnicastLocal, these will almost always be QHostAddress::Any or QHostAddress::AnyIPv6 (operate on the default interface for IPv4 or IPv6, respectively).

	   For Multicast, it is expected that the default/primary multicast interface will be used here.  Do not pass QHostAddress::Any (or AnyIPv6) with Multicast mode.

	   Returns true if the interface was successfully added, otherwise returns false.
	*/
	bool addInterface(const QHostAddress &addr);

	/**
	   \brief Removes a previously-added interface
	*/
	void removeInterface(const QHostAddress &addr);

	/**
	   \brief Shuts down the object

	   This operation primarily exists for Multicast mode, so that any published records have a chance to be unpublished.  If the JDnsShared object is simply deleted without performing a shutdown, then published records will linger on the network until their TTLs expire.

	   When shutdown is complete, the shutdownFinished() signal will be emitted. 
	*/
	void shutdown();

	/**
	   \brief The domains to search in

	   You should perform a separate resolution for every domain configured on this machine.
	*/
	static QList<QByteArray> domains();

	/**
	   \brief Performs a blocking shutdown of many JDnsShared instances

	   This function is a convenient way to shutdown multiple JDnsShared instances synchronously.  The internal shutdown procedure uses no more than a few cycles of the eventloop, so it should be safe to call without worry of the application being overly stalled.  This function takes ownership of the instances passed to it, and will delete them upon completion.

	   It is worth noting that this function is implemented without the use of a nested eventloop.  All of the JDnsShared instances are moved into a temporary thread to perform the shutdown procedure, which should not cause any unexpected behavior in the current thread.

	   \code
QList<JDnsShared*> list;
list += jdnsShared_unicast;
list += jdnsShared_multicast;
JDnsShared::waitForShutdown(list);

// collect remaining debug information
QStringList finalDebugLines = jdnsSharedDebug.readDebugLines();
	   \endcode
	*/
	static void waitForShutdown(const QList<JDnsShared*> &instances);

signals:
	/**
	   \brief Indicates the object has been shut down
	*/
	void shutdownFinished();

private:
	friend class JDnsSharedRequest;
	friend class JDnsSharedPrivate;
	JDnsSharedPrivate *d;
};

#endif
