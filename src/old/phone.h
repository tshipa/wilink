/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#ifndef __WILINK_PHONE_H__
#define __WILINK_PHONE_H__

#include "chat_panel.h"

#include "plugins/phone/models.h"
#include "plugins/phone/sip.h"

class QAbstractButton;
class QDeclarativeView;
class Chat;
class PhoneCallsModel;
class SipCall;

class PhonePanel : public ChatPanel
{
    Q_OBJECT

public:
    PhonePanel(Chat *chatWindow, QWidget *parent = NULL);

private slots:
    void callButtonClicked(QAbstractButton *button);
    void callReceived(SipCall *call);

private:
    PhoneCallsModel *m_callsModel;
    QDeclarativeView *declarativeView;
    Chat *m_window;
};

#endif