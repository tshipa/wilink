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

#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDomDocument>
#include <QMenu>
#include <QMenuBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include "QXmppClient.h"

#include "qnetio/wallet.h"

#include "application.h"
#include "chat.h"
#include "chat_plugin.h"
#include "declarative.h"
#include "menu.h"

static const QUrl baseUrl("https://www.wifirst.net/wilink/menu/1");

Menu::Menu(Chat *window)
    : QObject(window),
    refreshInterval(300000),
    chatWindow(window),
    servicesMenu(0)
{
    bool check;

    servicesMenu = chatWindow->menuBar()->addMenu(tr("&Services"));

    /* prepare network manager */
    network = new NetworkAccessManager(this);

    /* prepare timer */
    timer = new QTimer(this);
    timer->setSingleShot(true);
    check = connect(timer, SIGNAL(timeout()),
                    this, SLOT(fetchMenu()));
    Q_ASSERT(check);

    /* wait for initial connection to the XMPP server */
    check = connect(chatWindow->client(), SIGNAL(connected()),
                    this, SLOT(connected()));
    Q_ASSERT(check);
}

void Menu::connected()
{
    /* we are no longer interested in connected events */
    disconnect(chatWindow->client(), SIGNAL(connected()),
               this, SLOT(connected()));

    /* initial menu fetch */
    timer->start(0);
}

void Menu::fetchIcon(const QUrl &url, QAction *action)
{
    QNetworkRequest req(url);
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(showIcon()));
    icons[reply] = action;
}

void Menu::fetchMenu()
{
    QNetworkRequest req(baseUrl);
    req.setRawHeader("Accept", "application/xml");
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(showMenu()));
}

void Menu::openUrl()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;

    const QUrl linkUrl = action->data().toUrl();
    QDesktopServices::openUrl(linkUrl);
}

void Menu::showIcon()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || !icons.contains(reply))
        return;

    QAction *action = icons.take(reply);
    QPixmap pixmap;
    const QByteArray data = reply->readAll();
    if (!pixmap.loadFromData(data, 0))
        return;
    action->setIcon(QIcon(pixmap));
}

void Menu::showMenu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning("Failed to retrieve menu: %s", qPrintable(reply->errorString()));
        if (refreshInterval > 0)
            timer->start(refreshInterval);
        return;
    }

    /* clear menu bar */
    icons.clear();
    servicesMenu->clear();

    QAction *action;

    /* parse menu */
    QDomDocument doc;
    doc.setContent(reply);
    QDomElement item = doc.documentElement().firstChildElement("menu").firstChildElement("menu");
    while (!item.isNull())
    {
        const QString image = item.firstChildElement("image").text();
        const QString link = item.firstChildElement("link").text();
        const QString text = item.firstChildElement("label").text();
        QUrl linkUrl(link);
        if (text.isEmpty())
            servicesMenu->addSeparator();
        else if (linkUrl.isValid())
        {
            if (linkUrl.scheme() == "xmpp" && linkUrl.hasQueryItem("join"))
            {
                const QString roomJid = linkUrl.path();

                // FIXME: add or update room entry
#if 0
                ChatRosterModel *model = chatWindow->rosterModel();
                QModelIndex index = model->findItem(roomJid);
                if (!index.isValid()) {
                    roomWatcher->joinRoom(roomJid, false);
                    index = model->findItem(roomJid);
                }
                model->setData(index, QPixmap(":/home.png"), Qt::DecorationRole);
                model->setData(index, true, ChatRosterModel::PersistentRole);
#endif
            }
            action = servicesMenu->addAction(text);

            /* build full URL */
            QUrl url = baseUrl.resolved(link);
            if (url.scheme() == "xmpp" && url.authority().isEmpty())
                url.setAuthority(chatWindow->client()->configuration().jidBare());
            action->setData(url);
            connect(action, SIGNAL(triggered(bool)), this, SLOT(openUrl()));

            /* request icon */
            if (!image.isEmpty())
                fetchIcon(baseUrl.resolved(image), action);
        }
        item = item.nextSiblingElement("menu");
    }

    /* parse preferences */
    QDomElement preferences = doc.documentElement().firstChildElement("preferences");
    refreshInterval = preferences.firstChildElement("refresh").text().toInt() * 1000;
    if (refreshInterval > 0)
        timer->start(refreshInterval);
}

// PLUGIN

class MenuPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    QString name() const { return "Menu"; };
};

bool MenuPlugin::initialize(Chat *chat)
{
    const QString domain = chat->client()->configuration().domain();
    if (domain != "wifirst.net")
        return false;

    new Menu(chat);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(menu, MenuPlugin)

