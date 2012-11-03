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

#include "XmlConsole.h"
#include "ui_XmlConsole.h"

#include "utils/Logger.h"

#include <QMenu>
#include <QActionGroup>
#include <QStringBuilder>
#include <QTextLayout>
#include <QPlainTextDocumentLayout>
#include <QFileDialog>
#include <QTextDocumentWriter>

using namespace Jreen;


XmlConsole::XmlConsole(Client* client, QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::XmlConsole),
	m_client(client),	m_filter(0x1f)
{
	m_ui->setupUi(this);

    m_client->addXmlStreamHandler(this);

	QPalette pal = palette();
	pal.setColor(QPalette::Base, Qt::black);
	pal.setColor(QPalette::Text, Qt::white);
	m_ui->xmlBrowser->viewport()->setPalette(pal);
	QTextDocument *doc = m_ui->xmlBrowser->document();
	doc->setDocumentLayout(new QPlainTextDocumentLayout(doc));
	doc->clear();

	QTextFrameFormat format = doc->rootFrame()->frameFormat();
	format.setBackground(QColor(Qt::black));
	format.setMargin(0);
	doc->rootFrame()->setFrameFormat(format);
	QMenu *menu = new QMenu(m_ui->filterButton);
	menu->setSeparatorsCollapsible(false);
	menu->addSeparator()->setText(tr("Filter"));
	QActionGroup *group = new QActionGroup(menu);
	QAction *disabled = group->addAction(menu->addAction(tr("Disabled")));
	disabled->setCheckable(true);
	disabled->setData(Disabled);
	QAction *jid = group->addAction(menu->addAction(tr("By JID")));
	jid->setCheckable(true);
	jid->setData(ByJid);
	QAction *xmlns = group->addAction(menu->addAction(tr("By namespace uri")));
	xmlns->setCheckable(true);
	xmlns->setData(ByXmlns);
	QAction *attrb = group->addAction(menu->addAction(tr("By all attributes")));
	attrb->setCheckable(true);
	attrb->setData(ByAllAttributes);
	disabled->setChecked(true);
	connect(group, SIGNAL(triggered(QAction*)), this, SLOT(onActionGroupTriggered(QAction*)));
	menu->addSeparator()->setText(tr("Visible stanzas"));
	group = new QActionGroup(menu);
	group->setExclusive(false);
	QAction *iq = group->addAction(menu->addAction(tr("Information query")));
	iq->setCheckable(true);
	iq->setData(XmlNode::Iq);
	iq->setChecked(true);
	QAction *message = group->addAction(menu->addAction(tr("Message")));
	message->setCheckable(true);
	message->setData(XmlNode::Message);
	message->setChecked(true);
	QAction *presence = group->addAction(menu->addAction(tr("Presence")));
	presence->setCheckable(true);
	presence->setData(XmlNode::Presence);
	presence->setChecked(true);
	QAction *custom = group->addAction(menu->addAction(tr("Custom")));
	custom->setCheckable(true);
	custom->setData(XmlNode::Custom);
	custom->setChecked(true);
	connect(group, SIGNAL(triggered(QAction*)), this, SLOT(onActionGroupTriggered(QAction*)));
	m_ui->filterButton->setMenu(menu);
	m_stackBracketsColor = QColor(0x666666);
	m_stackIncoming.bodyColor = QColor(0xbb66bb);
	m_stackIncoming.tagColor = QColor(0x006666);
	m_stackIncoming.attributeColor = QColor(0x009933);
	m_stackIncoming.paramColor = QColor(0xcc0000);
	m_stackOutgoing.bodyColor = QColor(0x999999);
	m_stackOutgoing.tagColor = QColor(0x22aa22);
	m_stackOutgoing.attributeColor = QColor(0xffff33);
	m_stackOutgoing.paramColor = QColor(0xdd8811);

	QAction *action = new QAction(tr("Close"),this);
	action->setSoftKeyRole(QAction::NegativeSoftKey);
	connect(action, SIGNAL(triggered()), SLOT(close()));
	addAction(action);
}

XmlConsole::~XmlConsole()
{
	delete m_ui;
}

void XmlConsole::handleStreamBegin()
{
	m_stackIncoming.reader.clear();
	m_stackOutgoing.reader.clear();
	m_stackIncoming.depth = 0;
	m_stackOutgoing.depth = 0;
	qDeleteAll(m_stackIncoming.tokens);
	qDeleteAll(m_stackOutgoing.tokens);
	m_stackIncoming.tokens.clear();
	m_stackOutgoing.tokens.clear();
}

void XmlConsole::handleStreamEnd()
{
	m_stackIncoming.reader.clear();
	m_stackOutgoing.reader.clear();
	m_stackIncoming.depth = 0;
	m_stackOutgoing.depth = 0;
	qDeleteAll(m_stackIncoming.tokens);
	qDeleteAll(m_stackOutgoing.tokens);
	m_stackIncoming.tokens.clear();
	m_stackOutgoing.tokens.clear();
}

void XmlConsole::handleIncomingData(const char *data, qint64 size)
{
	stackProcess(QByteArray::fromRawData(data, size), true);
}

void XmlConsole::handleOutgoingData(const char *data, qint64 size)
{
	stackProcess(QByteArray::fromRawData(data, size), false);
}

QString generate_stacked_space(int depth)
{
	return QString(depth * 2, QLatin1Char(' '));
}

void XmlConsole::stackProcess(const QByteArray &data, bool incoming)
{
	StackEnvironment *d = &(incoming ? m_stackIncoming : m_stackOutgoing);
	d->reader.addData(data);
	StackToken *token;
//	debug() << incoming << data;
//	debug() << "==================================================================";
	while (d->reader.readNext() > QXmlStreamReader::Invalid) {
//		qDebug() << incoming << d->reader.tokenString();
		switch(d->reader.tokenType()) {
		case QXmlStreamReader::StartElement:
//			dbg << d->reader.name().toString() << d->depth
//					<< d->reader.attributes().value(QLatin1String("from")).toString();
			d->depth++;
			if (d->depth > 1) {
				if (!d->tokens.isEmpty() && d->tokens.last()->type == QXmlStreamReader::Characters)
					delete d->tokens.takeLast();
				d->tokens << new StackToken(d->reader);
			}
			break;
		case QXmlStreamReader::EndElement:
//			dbg << d->reader.name().toString() << d->depth;
			if (d->tokens.isEmpty())
				break;
			token = d->tokens.last();
			if (token->type == QXmlStreamReader::StartElement && !token->startTag.empty)
				token->startTag.empty = true;
			else if (d->depth > 1)
				d->tokens << new StackToken(d->reader);
			if (d->depth == 2) {
				QTextCursor cursor(m_ui->xmlBrowser->document());
				cursor.movePosition(QTextCursor::End);
				cursor.beginEditBlock();
				QTextCharFormat zeroFormat = cursor.charFormat();
				zeroFormat.setForeground(QColor(Qt::white));
				QTextCharFormat bodyFormat = zeroFormat;
				bodyFormat.setForeground(d->bodyColor);
				QTextCharFormat tagFormat = zeroFormat;
				tagFormat.setForeground(d->tagColor);
				QTextCharFormat attributeFormat = zeroFormat;
				attributeFormat.setForeground(d->attributeColor);
				QTextCharFormat paramsFormat = zeroFormat;
				paramsFormat.setForeground(d->paramColor);
				QTextCharFormat bracketFormat = zeroFormat;
				bracketFormat.setForeground(m_stackBracketsColor);
				QString singleSpace = QLatin1String(" ");
				cursor.insertBlock();
				int depth = 0;
				QString currentXmlns;
				QXmlStreamReader::TokenType lastType = QXmlStreamReader::StartElement;
				for (int i = 0; i < d->tokens.size(); i++) {
					token = d->tokens.at(i);
					if (token->type == QXmlStreamReader::StartElement) {
						QString space = generate_stacked_space(depth);
						cursor.insertText(QLatin1String("\n"));
						cursor.insertText(space);
						cursor.insertText(QLatin1String("<"), bracketFormat);
						cursor.insertText(token->startTag.name->toString(), tagFormat);
						const QStringRef &xmlns = *token->startTag.xmlns;
						if (i == 0 || xmlns != currentXmlns) {
							currentXmlns = xmlns.toString();
							cursor.insertText(singleSpace);
							cursor.insertText(QLatin1String("xmlns"), attributeFormat);
							cursor.insertText(QLatin1String("="), zeroFormat);
							cursor.insertText(QLatin1String("'"), paramsFormat);
							cursor.insertText(currentXmlns, paramsFormat);
							cursor.insertText(QLatin1String("'"), paramsFormat);
						}
						QXmlStreamAttributes *attributes = token->startTag.attributes;
						for (int j = 0; j < attributes->count(); j++) {
							const QXmlStreamAttribute &attr = attributes->at(j);
							cursor.insertText(singleSpace);
							cursor.insertText(attr.name().toString(), attributeFormat);
							cursor.insertText(QLatin1String("="), zeroFormat);
							cursor.insertText(QLatin1String("'"), paramsFormat);
							cursor.insertText(attr.value().toString(), paramsFormat);
							cursor.insertText(QLatin1String("'"), paramsFormat);
						}
						if (token->startTag.empty) {
							cursor.insertText(QLatin1String("/>"), bracketFormat);
						} else {
							cursor.insertText(QLatin1String(">"), bracketFormat);
							depth++;
						}
					} else if (token->type == QXmlStreamReader::EndElement) {
						if (lastType == QXmlStreamReader::EndElement) {
							QString space = generate_stacked_space(depth - 1);
							cursor.insertText(QLatin1String("\n"));
							cursor.insertText(space);
						}
						cursor.insertText(QLatin1String("</"), bracketFormat);
						cursor.insertText(token->endTag.name->toString(), tagFormat);
						cursor.insertText(QLatin1String(">"), bracketFormat);
						depth--;
					} else if (token->type == QXmlStreamReader::Characters) {
						cursor.setCharFormat(bodyFormat);
						QString text = token->charachters.text->toString();
						if (text.contains(QLatin1Char('\n'))) {
							QString space = generate_stacked_space(depth);
							space.prepend(QLatin1Char('\n'));
							QStringList lines = text.split(QLatin1Char('\n'));
							for (int j = 0; j < lines.size(); j++) {
								cursor.insertText(space);
								cursor.insertText(lines.at(j));
							}
							space.chop(1);
							cursor.insertText(space);
						} else {
							cursor.insertText(text);
						}
					}
					lastType = token->type;
					if (lastType == QXmlStreamReader::StartElement && token->startTag.empty)
						lastType = QXmlStreamReader::EndElement;
					delete token;
				}
				cursor.endEditBlock();
				d->tokens.clear();
			}
			d->depth--;
			break;
		case QXmlStreamReader::Characters:
			token = d->tokens.isEmpty() ? 0 : d->tokens.last();
			if (token && token->type == QXmlStreamReader::StartElement && !token->startTag.empty) {
				if (*token->startTag.name == QLatin1String("auth")
						&& *token->startTag.xmlns == QLatin1String("urn:ietf:params:xml:ns:xmpp-sasl")) {
					d->tokens << new StackToken(QLatin1String("<<Private data>>"));
				} else {
					d->tokens << new StackToken(d->reader);
				}
			}
			break;
		default:
			break;
		}
	}
//	qDebug() << d->reader.tokenString();
//	if (d->reader.tokenType() == QXmlStreamReader::Invalid)
//		dbg << d->reader.error() << d->reader.errorString();
	if (!incoming && d->depth > 1) {
		qFatal("outgoing depth %d on\n\"%s\"", d->depth,
			   qPrintable(QString::fromUtf8(data, data.size())));
	}
}

void XmlConsole::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		m_ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void XmlConsole::onActionGroupTriggered(QAction *action)
{
	int type = action->data().toInt();
	if (type >= 0x10) {
		m_filter = (m_filter & 0xf) | type;
		m_ui->lineEdit->setEnabled(type != 0x10);
	} else {
		m_filter = m_filter ^ type;
	}
	on_lineEdit_textChanged(m_ui->lineEdit->text());
}

void XmlConsole::on_lineEdit_textChanged(const QString &text)
{
	int filterType = m_filter & 0xf0;
	JID filterJid = (filterType == ByJid) ? text : QString();
    for (int i = 0; i < m_nodes.size(); i++) {
		XmlNode &node = m_nodes[i];
		bool ok = true;
		switch (filterType) {
		case ByXmlns:
			ok = node.xmlns.contains(text);
			break;
		case ByJid:
			ok = node.jid.full() == filterJid.full() || node.jid.bare() == filterJid.full();
			break;
		case ByAllAttributes:
			ok = node.attributes.contains(text);
			break;
		default:
			break;
		}
		ok &= bool(node.type & m_filter);
		node.block.setVisible(ok);
		node.block.setLineCount(ok ? node.lineCount : 0);
		//		qDebug() << node.block.lineCount();
	}
	QAbstractTextDocumentLayout *layout = m_ui->xmlBrowser->document()->documentLayout();
	Q_ASSERT(qobject_cast<QPlainTextDocumentLayout*>(layout));
	qobject_cast<QPlainTextDocumentLayout*>(layout)->requestUpdate();
}

void XmlConsole::on_saveButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save XMPP log to file"),
	                                                QString(), tr("OpenDocument Format (*.odf);;HTML file (*.html);;Plain text (*.txt)"));
	if (!fileName.isEmpty()) {
		QTextDocumentWriter writer(fileName);
		writer.write(m_ui->xmlBrowser->document());
	}
}
