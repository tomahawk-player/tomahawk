/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Ruslan Nigmatullin <euroelessar@ya.ru>
 *   Copyright 2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *   Copyright 2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef XMLCONSOLE_H
#define XMLCONSOLE_H

#include <jreen/client.h>
#include <jreen/jid.h>

#include <QWidget>
#include <QXmlStreamReader>
#include <QDateTime>
#include <QTextBlock>

namespace Ui {
class XmlConsole;
}

#include "accounts/AccountDllMacro.h"

class ACCOUNTDLLEXPORT XmlConsole : public QWidget, public Jreen::XmlStreamHandler
{
    Q_OBJECT

public:
	XmlConsole(Jreen::Client* client, QWidget *parent = 0);
	~XmlConsole();

	virtual void handleStreamBegin();
	virtual void handleStreamEnd();
	virtual void handleIncomingData(const char *data, qint64 size);
	virtual void handleOutgoingData(const char *data, qint64 size);

protected:
	void changeEvent(QEvent *e);
private:
	void stackProcess(const QByteArray &data, bool incoming);

	struct XmlNode
	{
		enum Type
		{
			Iq = 1,
			Presence = 2,
			Message = 4,
			Custom = 8
		};
		QDateTime time;
		Type type;
		bool incoming;
		QSet<QString> xmlns;
		Jreen::JID jid;
		QSet<QString> attributes;
		QTextBlock block;
		int lineCount;
	};
	enum FilterType
	{
		Disabled = 0x10,
		ByJid = 0x20,
		ByXmlns = 0x30,
		ByAllAttributes = 0x40
	};

	enum State
	{
		WaitingForStanza,
		ReadFeatures,
		ReadStanza,
		ReadCustom
	};

	struct StackToken
	{
		StackToken(QXmlStreamReader &reader)
		{
			type = reader.tokenType();
			if (type == QXmlStreamReader::StartElement) {
				QStringRef tmp = reader.name();
				startTag.namePointer = new QString(*tmp.string());
				startTag.name = new QStringRef(startTag.namePointer, tmp.position(), tmp.length());
				tmp = reader.namespaceUri();
				startTag.xmlnsPointer = new QString(*tmp.string());
				startTag.xmlns = new QStringRef(startTag.xmlnsPointer, tmp.position(), tmp.length());
				startTag.attributes = new QXmlStreamAttributes(reader.attributes());
				startTag.empty = false;
			} else if (type == QXmlStreamReader::Characters) {
				QStringRef tmp = reader.text();
				charachters.textPointer = new QString(*tmp.string());
				charachters.text = new QStringRef(charachters.textPointer, tmp.position(), tmp.length());
			} else if (type == QXmlStreamReader::EndElement) {
				QStringRef tmp = reader.name();
				endTag.namePointer = new QString(*tmp.string());
				endTag.name = new QStringRef(endTag.namePointer, tmp.position(), tmp.length());
			}
		}

		StackToken(const QString &name)
		{
			type = QXmlStreamReader::Characters;
			charachters.textPointer = new QString(name);
			charachters.text = new QStringRef(charachters.textPointer);
		}

		~StackToken()
		{
			if (type == QXmlStreamReader::StartElement) {
				delete startTag.namePointer;
				delete startTag.name;
				delete startTag.xmlnsPointer;
				delete startTag.xmlns;
				delete startTag.attributes;
			} else if (type == QXmlStreamReader::Characters) {
				delete charachters.textPointer;
				delete charachters.text;
			} else if (type == QXmlStreamReader::EndElement) {
				delete endTag.namePointer;
				delete endTag.name;
			}
		}

		QXmlStreamReader::TokenType type;
		union {
			struct {
				QString *namePointer;
				QStringRef *name;
				QString *xmlnsPointer;
				QStringRef *xmlns;
				QXmlStreamAttributes *attributes;
				bool empty;
			} startTag;
			struct {
				QString *textPointer;
				QStringRef *text;
			} charachters;
			struct {
				QString *namePointer;
				QStringRef *name;
			} endTag;
		};
	};

	struct StackEnvironment
	{
		QXmlStreamReader reader;
		State state;
		int depth;
		QList<StackToken*> tokens;
		QColor bodyColor;
		QColor tagColor;
		QColor attributeColor;
		QColor paramColor;
	};

	Ui::XmlConsole *m_ui;
	Jreen::Client *m_client;
	QList<XmlNode> m_nodes;
	StackEnvironment m_stackIncoming;
	StackEnvironment m_stackOutgoing;
	QColor m_stackBracketsColor;
	int m_filter;

private slots:
	void on_lineEdit_textChanged(const QString &);
	void onActionGroupTriggered(QAction *action);
	void on_saveButton_clicked();
};


#endif // XMLCONSOLE_H
