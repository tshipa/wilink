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

#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QFile>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QtCore/qmath.h>
#include <QtCore/qendian.h>
#include <QTimer>

#include "QXmppCallManager.h"
#include "QXmppUtils.h"

#include "calls.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "chat_roster.h"

const int ToneFrequencyHz = 600;

static QAudioFormat formatFor(const QXmppJinglePayloadType &type)
{
    QAudioFormat format;
    format.setFrequency(type.clockrate());
    format.setChannels(type.channels());
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    return format;
}

Reader::Reader(const QAudioFormat &format, QObject *parent)
    : QObject(parent)
{
    int durationMs = 100;

    // 100ms
    m_block = (format.frequency() * format.channels() * (format.sampleSize() / 8)) * durationMs / 1000;
    m_input = new QFile("test.raw");
    m_input->open(QIODevice::ReadOnly);
    m_timer = new QTimer(this);
    m_timer->setInterval(durationMs);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(tick()));
}

void Reader::start(QIODevice *device)
{
    m_output = device;
    m_timer->start();
}

void Reader::tick()
{
    QByteArray block = m_input->read(m_block);
    m_output->write(block);
}

CallPanel::CallPanel(QXmppCall *call, QWidget *parent)
    : ChatPanel(parent), m_call(call)
{
    setWindowIcon(QIcon(":/chat.png"));
    setWindowTitle(tr("Call"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    // HEADER

    layout->addItem(headerLayout());

    QHBoxLayout *hbox = new QHBoxLayout;
    QPushButton *hangupButton = new QPushButton(tr("Hang up"));
    connect(hangupButton, SIGNAL(clicked()), m_call, SLOT(hangup()));
    hbox->addWidget(hangupButton);

    layout->addItem(hbox);

    setLayout(layout);

    connect(call, SIGNAL(buffered()), this, SLOT(callBuffered()));
    connect(call, SIGNAL(connected()), this, SLOT(callConnected()));

    QTimer::singleShot(0, this, SIGNAL(showPanel()));
}

void CallPanel::callBuffered()
{
    QAudioFormat format = formatFor(m_call->payloadType());
    QAudioOutput *audioOutput = new QAudioOutput(format, this);
    connect(audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(stateChanged(QAudio::State)));

    if (m_call->direction() == QXmppCall::IncomingDirection)
    {
        qDebug() << "start playback";
        audioOutput->start(m_call);
    }
}

void CallPanel::callConnected()
{
    QAudioFormat format = formatFor(m_call->payloadType());
    QAudioInput *audioInput = new QAudioInput(format, this);

    if (m_call->direction() == QXmppCall::OutgoingDirection)
    {
        qDebug() << "start capture";
        audioInput->start(m_call);
    }
}

void CallPanel::stateChanged(QAudio::State state)
{
    qDebug() << "state changed" << state;
}

CallWatcher::CallWatcher(Chat *chatWindow)
    : QObject(chatWindow), m_window(chatWindow)
{
    m_client = chatWindow->client();
    m_roster = chatWindow->rosterModel();

    connect(&m_client->callManager(), SIGNAL(callReceived(QXmppCall*)),
            this, SLOT(callReceived(QXmppCall*)));
}

void CallWatcher::callClicked(QAbstractButton * button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    QXmppCall *call = qobject_cast<QXmppCall*>(box->property("call").value<QObject*>());
    if (!call)
        return;

    if (box->standardButton(button) == QMessageBox::Yes)
    {
        CallPanel *panel = new CallPanel(call, m_window);
        m_window->addPanel(panel);
        call->accept();
    } else
        call->hangup();
}

void CallWatcher::callContact()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString fullJid = action->data().toString();

    QXmppCall *call = m_client->callManager().call(fullJid);
    CallPanel *panel = new CallPanel(call, m_window);
    m_window->addPanel(panel);
}

void CallWatcher::callReceived(QXmppCall *call)
{
    const QString bareJid = jidToBareJid(call->jid());
    const QString contactName = m_roster->contactName(bareJid);

    QMessageBox *box = new QMessageBox(QMessageBox::Question,
        tr("Call from %1").arg(contactName),
        tr("%1 wants to talk to you.\n\nDo you accept?").arg(contactName),
        QMessageBox::Yes | QMessageBox::No, m_window);
    box->setDefaultButton(QMessageBox::NoButton);
    box->setProperty("call", qVariantFromValue(qobject_cast<QObject*>(call)));

    /* connect signals */
    connect(box, SIGNAL(buttonClicked(QAbstractButton*)),
        this, SLOT(callClicked(QAbstractButton*)));
    box->show();
}

void CallWatcher::rosterMenu(QMenu *menu, const QModelIndex &index)
{
    if (!m_client->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    if (type == ChatRosterItem::Contact)
    {
        QStringList fullJids = m_roster->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
        if (fullJids.isEmpty())
            return;

        QAction *action = menu->addAction(tr("Call"));
        action->setData(fullJids.first());
        connect(action, SIGNAL(triggered()), this, SLOT(callContact()));
    }
}

// PLUGIN

class CallsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool CallsPlugin::initialize(Chat *chat)
{
    CallWatcher *watcher = new CallWatcher(chat);

    /* add roster hooks */
    connect(chat, SIGNAL(rosterMenu(QMenu*, QModelIndex)),
            watcher, SLOT(rosterMenu(QMenu*, QModelIndex)));

    return true;
}

Q_EXPORT_STATIC_PLUGIN2(calls, CallsPlugin)

