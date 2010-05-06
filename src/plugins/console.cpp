/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QShortcut>
#include <QTextBrowser>

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "console.h"
#include "utils.h"

ChatConsole::ChatConsole(QXmppLogger *logger, QWidget *parent)
    : ChatPanel(parent), connected(false), currentLogger(logger)
{
    setWindowIcon(QIcon(":/options.png"));
    setWindowTitle(tr("Debugging console"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    layout->addItem(headerLayout());

    browser = new QTextBrowser;
    layout->addWidget(browser);
    Highlighter *highlighter = new Highlighter(browser->document());

    QHBoxLayout *hbox = new QHBoxLayout;
    showPackets = new QCheckBox(tr("Show packets"));
    showPackets->setCheckState(Qt::Checked);
    hbox->addWidget(showPackets);

    hbox->addSpacing(16);

    showMessages = new QCheckBox(tr("Show messages"));
    showMessages->setCheckState(Qt::Checked);
    hbox->addWidget(showMessages);

    hbox->addStretch();

    QPushButton *clearButton = new QPushButton(QIcon(":/close.png"), tr("Clear"));
    connect(clearButton, SIGNAL(clicked()), browser, SLOT(clear()));
    hbox->addWidget(clearButton);

    layout->addItem(hbox);

    setLayout(layout);

    /* connect signals */
    connect(this, SIGNAL(hidePanel()), this, SLOT(slotHide()));
    connect(this, SIGNAL(showPanel()), this, SLOT(slotShow()));
}

void ChatConsole::message(QXmppLogger::MessageType type, const QString &msg)
{
    if (showMessages->checkState() != Qt::Checked &&
        (type == QXmppLogger::DebugMessage || type == QXmppLogger::InformationMessage || type == QXmppLogger::WarningMessage))
        return;

    if (showPackets->checkState() != Qt::Checked &&
        (type == QXmppLogger::ReceivedMessage || type == QXmppLogger::SentMessage))
        return;

    QColor color;
    QString message;
    if (type == QXmppLogger::SentMessage || type == QXmppLogger::ReceivedMessage)
    {
        color = (type == QXmppLogger::SentMessage) ? QColor(0xcc, 0xcc, 0xff) : QColor(0xcc, 0xff, 0xcc);
        message = indentXml(msg);
    }
    else
    {
        color = (type == QXmppLogger::WarningMessage) ? QColor(0xff, 0x95, 0x95) : Qt::white;
        message = msg;
    }

    if (!message.isEmpty())
    {
        const QTextCursor savedCursor = browser->textCursor();

        QTextCursor cursor = browser->textCursor();
        cursor.movePosition(QTextCursor::End);
        browser->setTextCursor(cursor);
        browser->setTextBackgroundColor(color);
        browser->insertPlainText(message + "\n");

        browser->setTextCursor(savedCursor);
    }
}

void ChatConsole::slotHide()
{
    if (!connected)
        return;
    disconnect(currentLogger, SIGNAL(message(QXmppLogger::MessageType,QString)), this, SLOT(message(QXmppLogger::MessageType,QString)));
    connected = false;
}

void ChatConsole::slotShow()
{
    if (connected)
        return;
    connect(currentLogger, SIGNAL(message(QXmppLogger::MessageType,QString)), this, SLOT(message(QXmppLogger::MessageType,QString)));
    connected = true;
}


Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    tagFormat.setForeground(Qt::darkBlue);
    tagFormat.setFontWeight(QFont::Bold);

    rule.pattern = QRegExp("</?([a-z:]+)[^>]*>");
    rule.format = tagFormat;
    highlightingRules.append(rule);

    quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("\"[^\"]*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegExp("'[^']*'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
}

void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            QStringList captures = expression.capturedTexts();
            if (captures.size() > 1)
                setFormat(expression.pos(1), captures[1].size(), rule.format);
            else
                setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}

// PLUGIN

class ConsolePlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool ConsolePlugin::initialize(Chat *chat)
{
    /* register panel */
    ChatConsole *console = new ChatConsole(chat->client()->logger());
    console->setObjectName("console");
    chat->addPanel(console);

    /* register shortcut */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_D), chat);
    connect(shortcut, SIGNAL(activated()), console, SIGNAL(showPanel()));
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(console, ConsolePlugin)
