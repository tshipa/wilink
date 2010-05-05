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

#ifndef __WILINK_CHAT_DIALOG_H__
#define __WILINK_CHAT_DIALOG_H__

#include <QWidget>

#include "qxmpp/QXmppMessage.h"

#include "chat_conversation.h"

class ChatRosterModel;
class QXmppArchiveChat;
class QXmppClient;

class ChatDialog : public ChatConversation
{
    Q_OBJECT

public:
    ChatDialog(QXmppClient *xmppClient, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent = NULL);

public slots:
    void messageReceived(const QXmppMessage &msg);

private slots:
    void archiveChatReceived(const QXmppArchiveChat &chat);
    void archiveListReceived(const QList<QXmppArchiveChat> &chats);
    void chatStateChanged(QXmppMessage::State state);
    void disconnected();
    void join();
    void leave();

private:
    virtual void sendMessage(const QString &body);

    QString chatLocalName;
    QXmppClient *client;
    bool joined;
    ChatRosterModel *rosterModel;
    QStringList chatStatesJids;
};

#endif
