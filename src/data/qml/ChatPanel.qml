/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

import QtQuick 1.1
import wiLink 2.4
import 'scripts/storage.js' as Storage
import 'scripts/utils.js' as Utils

Panel {
    id: chatPanel

    property bool pendingMessages: (roomListModel.pendingMessages + rosterModel.pendingMessages) > 0
    property variant delayedOpening: {}
    property bool wifirstRosterReceived: false

    function isFacebook(jid) {
       return Utils.jidToDomain(jid) == 'chat.facebook.com';
    }

    function isWindowsLive(jid) {
       return Utils.jidToDomain(jid) == 'messenger.live.com';
    }

    Repeater {
        id: chatClients

        model: ListModel {}

        Item {
            id: item

            property string conferenceJid
            property string conferenceName
            property QtObject client: Client {
                diagnosticManager.pingHosts: ['213.91.4.201', '8.8.8.8', 'wireless.wifirst.net', 'www.wifirst.net', 'www.google.fr']
                diagnosticManager.tracerouteHosts: ['213.91.4.201']
                diagnosticManager.transferUrl: 'http://wireless.wifirst.net:8080/speed/'
                logger: appLogger
            }

            Connections {
                target: item.client

                onAuthenticationFailed: {
                    var domain = Utils.jidToDomain(item.client.jid);
                    console.log("Failed to authenticate with chat server for domain: " + domain);
                    if (isFacebook(item.client.jid)) {
                        if (Storage.getSetting('facebookTokenRefused', '0') != '1') {
                            dialogSwapper.showPanel('FacebookTokenNotification.qml');
                        }
                        item.client.disconnectFromServer();
                        chatClients.model.remove(model.index);
                    } else if (isWindowsLive(item.client.jid)) {
                        item.client.disconnectFromServer();
                        chatClients.model.remove(model.index);
                    } else if (domain != 'wifirst.net') {
                        var jid = Utils.jidToBareJid(item.client.jid);
                        dialogSwapper.showPanel('AccountPasswordDialog.qml', {client: item.client, jid: jid});
                        item.client.disconnectFromServer();
                    }
                }

                onConflictReceived: {
                    console.log("Received a resource conflict from chat server");
                    Qt.quit();
                }

                onConnected: {
                    if (isFacebook(item.client.jid)) {
                        Storage.removeSetting('facebookTokenRefused');
                    }
                    if (item.conferenceJid) {
                        chatSwapper.showPanel('RoomPanel.qml', { jid: item.conferenceJid });
                        item.conferenceJid = '';
                    }
                }

                // FIXME : this is a hack to replay received messages after
                // adding the appropriate conversation
                onMessageReceived: {
                    var opts = { jid: Utils.jidToBareJid(from) };
                    if (!chatSwapper.findPanel('ConversationPanel.qml', opts)) {
                        chatSwapper.addPanel('ConversationPanel.qml', opts);
                        item.client.replayMessage();
                    }
                }
            }

            Connections {
                target: item.client.callManager
                onCallReceived: {
                    dialogSwapper.showPanel('CallNotification.qml', {
                        call: call,
                        panel: chatPanel});
                }
            }

            Connections {
                target: item.client.mucManager

                onInvitationReceived: {
                    dialogSwapper.showPanel('RoomInviteNotification.qml', {
                        jid: Utils.jidToBareJid(inviter),
                        panel: chatPanel,
                        roomJid: roomJid});
                }
            }

            Connections {
                target: item.client.rosterManager

                onSubscriptionReceived: {
                    // If we have a subscription to the requester, accept
                    // reciprocal subscription.
                    //
                    // FIXME: use QXmppRosterIq::Item::To and QXmppRosterIq::Item::Both
                    var subscription = item.client.subscriptionType(bareJid);
                    if (subscription == 2 || subscription == 3) {
                        // accept subscription
                        item.client.rosterManager.acceptSubscription(bareJid);
                        return;
                    }

                    dialogSwapper.showPanel('ContactAddNotification.qml', {jid: bareJid, rosterManager: item.client.rosterManager});
                }

                onRosterReceived: {
                    if (Utils.jidToDomain(item.client.jid) == 'wifirst.net') {
                        // Wifirst roster has been received
                        chatPanel.wifirstRosterReceived = true;
                        switch (chatPanel.delayedOpening.action) {
                        case 'open_conversation':
                            var jid = chatPanel.delayedOpening.jid;
                            chatPanel.showConversation(jid);
                            break;
                        case 'open_room':
                            chatPanel.showRoom(chatPanel.delayedOpening.jid);
                            break;
                        }
                        chatPanel.delayedOpening = {};
                    }
                }
            }

            Connections {
                target: item.client.transferManager
                onFileReceived: {
                    dialogSwapper.showPanel('TransferNotification.qml', {
                        job: job,
                        panel: chatPanel});
                }
            }
        }

        onItemAdded: {
            var data = model.get(index);
            if (data.facebookAppId && data.facebookAccessToken) {
                console.log("connecting to facebook: " + data.facebookAppId);
                item.client.connectToFacebook(data.facebookAppId, data.facebookAccessToken);
            } else if (data.windowsLiveAccessToken) {
                console.log("connecting to windows live");
                item.client.connectToWindowsLive(data.windowsLiveAccessToken);
            } else {
                console.log("connecting to: " + data.jid);
                if (data.conferenceJid) {
                    item.conferenceJid = data.conferenceJid;
                }
                item.client.connectToServer(data.jid, data.password);
            }
            statusBar.addClient(item.client);
        }

        onItemRemoved: {
            console.log("removing client: " + item.client.jid);
            statusBar.removeClient(item.client);
        }
    }

    /** Convenience method to show a conversation panel.
     */
    function showConversation(jid) {
        swapper.showPanel('ChatPanel.qml');

        if (Utils.jidToDomain(jid) == 'wifirst.net') {
            for (var i = 0; i < chatClients.count; ++i) {
                var client = chatClients.itemAt(i).client;
                if (Utils.jidToDomain(client.jid) == 'wifirst.net') {
                    // FIXME: use QXmppRosterIq.SubscriptionType
                    // "2" means the user has a subscription to the contact's presence information
                    if ((client.subscriptionType(jid) & 2) == 0) {
                        if (client.subscriptionStatus(jid) == 'subscribe') {
                            console.log('Already sent subscribe request to ' + jid);
                        } else {
                            console.log('Sending subscribe request to ' + jid);
                            dialogSwapper.showPanel('ContactSendSubscriptionDialog.qml', {jid: jid, client: client});
                        }
                        return;
                    }
                    break;
                }
            }
        }

        chatSwapper.showPanel('ConversationPanel.qml', { jid: Utils.jidToBareJid(jid) });
        if (chatPanel.singlePanel)
            chatPanel.state = 'no-sidebar';
    }

    /** Convenience method to show a chat room panel.
     */
    function showRoom(jid) {
        swapper.showPanel('ChatPanel.qml');
        chatSwapper.showPanel('RoomPanel.qml', { jid: jid });
        if (chatPanel.singlePanel)
            chatPanel.state = 'no-sidebar';
    }

    Rectangle {
        id: sidebar

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: width > 0
        width: chatPanel.singlePanel ? parent.width : appStyle.sidebarWidth
        color: '#474747'
        z: 2

        RoomContactView {
            id: rooms

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            currentJid: (Qt.application.active && swapper.currentItem == chatPanel && Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            model: ListModel {
                id: roomListModel

                property int pendingMessages: 0

                function addRoom(jid, name) {
                    for (var i = 0; i < count; ++i) {
                        if (get(i).jid == jid) {
                            return;
                        }
                    }
                    append({jid: jid, name: name, messages: 0});
                }

                function addPendingMessage(jid) {
                    for (var i = 0; i < count; ++i) {
                        var data = get(i);
                        if (data.jid == jid) {
                            pendingMessages++;
                            setProperty(i, 'messages', data.messages + 1);
                            break;
                        }
                    }
                }

                function clearPendingMessages(jid) {
                    for (var i = 0; i < count; ++i) {
                        var data = get(i);
                        if (data.jid == jid) {
                            pendingMessages -= data.messages;
                            setProperty(i, 'messages', 0);
                            break;
                        }
                    }
                }

                function removeRoom(jid) {
                    for (var i = 0; i < count; ++i) {
                        var data = get(i);
                        if (data.jid == jid) {
                            pendingMessages -= data.messages;
                            remove(i);
                            break;
                        }
                    }
                }

                function renameRoom(jid, name) {
                    for (var i = 0; i < count; ++i) {
                        var data = get(i);
                        if (data.jid == jid) {
                            setProperty(i, 'name', name);
                            break;
                        }
                    }
                }
            }
            height: headerHeight + rowHeight * model.count

            onAddClicked: {
                // FIXME: we only support default client
                dialogSwapper.showPanel('RoomJoinDialog.qml', {client: accountModel.clientForJid('wifirst.net')});
            }

            onCurrentJidChanged: {
                rooms.model.clearPendingMessages(rooms.currentJid);
            }

            onItemClicked: showRoom(model.jid)
        }

        ChatContactView {
            id: contacts

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: rooms.bottom
            anchors.bottom: statusBar.top
            anchors.bottomMargin: appStyle.margin.normal
            currentJid: (Qt.application.active && swapper.currentItem == chatPanel && Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            model: sortedContacts

            onAddClicked: {
                var clients = [];
                for (var i = 0; i < chatClients.count; ++i) {
                    var client = chatClients.itemAt(i).client;
                    if (!isFacebook(client.jid) && !isWindowsLive(client.jid))
                        clients.push(client);
                }
                dialogSwapper.showPanel('ContactAddDialog.qml', {clients: clients});
            }

            onCurrentJidChanged: {
                rosterModel.clearPendingMessages(contacts.currentJid);
            }

            onItemClicked: {
                showConversation(model.jid);
            }

            onItemContextMenu: {
                var pos = mapToItem(menuLoader.parent, point.x, point.y);
                menuLoader.sourceComponent = contactMenu;
                menuLoader.item.jid = model.jid;
                menuLoader.show(pos.x, pos.y);
            }
        }

        StatusBar {
            id: statusBar

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }

        // FIXME : this is a hack to make sure ConversationPanel loads fast
        ConversationPanel {
            opacity: 0
        }

    }

    Rectangle {
        id: tabBox

        anchors.top: parent.top
        anchors.left: chatPanel.singlePanel ? parent.left : sidebar.right
        anchors.right: parent.right
        gradient: Gradient {
            GradientStop { position: 0; color: '#FAFAFA' }
            GradientStop { position: 1; color: '#E9E9E9' }
        }
        height: (appStyle.font.largeSize + 2 * appStyle.margin.large)
        property int radius: appStyle.margin.small

        Button {
            id: prevButton

            anchors.left: parent.left
            iconStyle: 'icon-chevron-left'
            z: 2

            onClicked: chatSwapper.decrementCurrentIndex()
        }

        ChatTabs {
            id: tabView

            anchors.left: prevButton.right
            anchors.right: nextButton.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            panelSwapper: chatSwapper
            z: 1
        }

        Button {
            id: nextButton

            anchors.right: parent.right
            iconStyle: 'icon-chevron-right'
            z: 2

            onClicked: chatSwapper.incrementCurrentIndex()
        }

        states: State {
            when: chatSwapper.model.count < 2

            PropertyChanges { target: prevButton; visible: false }
            PropertyChanges { target: nextButton; visible: false }
        }
    }

    PanelSwapper {
        id: chatSwapper

        anchors.top: tabBox.bottom
        anchors.bottom: parent.bottom
        anchors.left: chatPanel.singlePanel ? parent.left : sidebar.right
        anchors.margins: appStyle.margin.normal
        anchors.right: parent.right
        focus: true
        visible: width > 0
    }

    onPendingMessagesChanged: {
        for (var i = 0; i < dock.model.count; i++) {
            if (dock.model.get(i).panelSource == 'ChatPanel.qml') {
                dock.model.setProperty(i, 'notified', pendingMessages);
                break;
            }
        }
    }

    resources: [
        SortFilterProxyModel {
            id: onlineContacts

            dynamicSortFilter: true
            filterRole: RosterModel.StatusRole
            filterRegExp: /^(?!offline)/
            sourceModel: RosterModel {
                id: rosterModel
            }
        },

        SortFilterProxyModel {
            id: sortedContacts

            dynamicSortFilter: true
            sortCaseSensitivity: Qt.CaseInsensitive
            sortRole: appSettings.sortContactsByStatus ? RosterModel.StatusSortRole : RosterModel.NameRole
            sourceModel: appSettings.showOfflineContacts ? rosterModel : onlineContacts
            Component.onCompleted: sort(0)
        }
    ]

    states: State {
        name: 'no-sidebar'

        PropertyChanges { target: sidebar; width: 0 }
    }

    transitions: Transition {
        PropertyAnimation { target: sidebar; properties: 'width'; duration: appStyle.animation.normalDuration }
    }

    Component {
        id: contactMenu
        Menu {
            id: menu

            property alias jid: vcard.jid
            property bool changeEnabled: !isFacebook(jid) && !isWindowsLive(jid)
            property bool profileEnabled: (profileUrl != undefined && profileUrl != '')
            property string profileUrl: isFacebook(vcard.jid) ? ('https://www.facebook.com/' + Utils.jidToUser(vcard.jid).substr(1)) : vcard.url

            onProfileEnabledChanged: {
                if (menu.model.count > 0) {
                    menu.model.setProperty(0, 'enabled', profileEnabled);
                }
            }

            onChangeEnabledChanged: {
                if (menu.model.count > 0) {
                    menu.model.setProperty(1, 'enabled', changeEnabled);
                    menu.model.setProperty(2, 'enabled', changeEnabled);
                }
            }

            VCard {
                id: vcard
            }

            onItemClicked: {
                var item = menu.model.get(index);
                var client = accountModel.clientForJid(jid);
                if (item.action == 'profile') {
                    Qt.openUrlExternally(profileUrl);
                } else if (item.action == 'rename') {
                    dialogSwapper.showPanel('ContactRenameDialog.qml', {jid: jid, rosterManager: client.rosterManager});
                } else if (item.action == 'remove') {
                    dialogSwapper.showPanel('ContactRemoveDialog.qml', {jid: jid, rosterManager: client.rosterManager});
                }
            }

            Component.onCompleted: {
                menu.model.append({
                    action: 'profile',
                    enabled: profileEnabled,
                    iconStyle: 'icon-info-sign',
                    text: qsTr('Show profile')});
                menu.model.append({
                    action: 'rename',
                    enabled: changeEnabled,
                    iconStyle: 'icon-wrench',
                    name: model.name,
                    text: qsTr('Rename contact')});
                menu.model.append({
                    action: 'remove',
                    enabled: changeEnabled,
                    iconStyle: 'icon-minus',
                    name: model.name,
                    text: qsTr('Remove contact')});
            }
        }
    }

    onDockClicked: {
        chatPanel.state = (chatPanel.state == 'no-sidebar') ? '' : 'no-sidebar';
    }

    Component.onCompleted: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);
            if (account.type == 'xmpp') {
                chatClients.model.append({jid: account.username, password: account.password});
            } else if (account.type == 'web' && account.realm == 'www.google.com') {
                chatClients.model.append({jid: account.username, password: account.password});
            } else if (account.type == 'web' && account.realm == 'www.wifirst.net') {
                var xhr = new XMLHttpRequest();
                xhr.onreadystatechange = function() {
                    if (xhr.readyState == 4) {
                        if (xhr.status == 200) {
                            var conferenceJid = '',
                                facebookAppId = '',
                                facebookAccessToken = '',
                                windowsLiveAccessToken = '',
                                xmppJid = '',
                                xmppPassword = '';
                            var doc = xhr.responseXML.documentElement;
                            for (var i = 0; i < doc.childNodes.length; ++i) {
                                var node = doc.childNodes[i];
                                if (node.nodeName == 'id' && node.firstChild) {
                                    xmppJid = node.firstChild.nodeValue;
                                } else if (node.nodeName == 'password' && node.firstChild) {
                                    xmppPassword = node.firstChild.nodeValue;
                                } else if (node.nodeName == 'conference') {
                                    for (var j = 0; j < node.childNodes.length; ++j) {
                                        var child = node.childNodes[j];
                                        if (child.nodeName == 'jid' && child.firstChild)
                                            conferenceJid = child.firstChild.nodeValue;
                                    }
                                } else if (node.nodeName == 'facebook') {
                                    for (var j = 0; j < node.childNodes.length; ++j) {
                                        var child = node.childNodes[j];
                                        if (child.nodeName == 'app-id' && child.firstChild)
                                            facebookAppId = child.firstChild.nodeValue;
                                        else if (child.nodeName == 'access-token' && child.firstChild)
                                            facebookAccessToken = child.firstChild.nodeValue;
                                    }
                                } else if (node.nodeName == 'windowslive') {
                                    for (var j = 0; j < node.childNodes.length; ++j) {
                                        var child = node.childNodes[j];
                                        if (child.nodeName == 'access-token' && child.firstChild)
                                            windowsLiveAccessToken = child.firstChild.nodeValue;
                                    }
                                }
                            }

                            // connect to XMPP
                            if (xmppJid && xmppPassword) {
                                chatClients.model.append({
                                    jid: xmppJid,
                                    password: xmppPassword,
                                    conferenceJid: conferenceJid});
                            }

                            // connect to facebook
                            if (facebookAppId && facebookAccessToken) {
                                chatClients.model.append({
                                    facebookAppId: facebookAppId,
                                    facebookAccessToken: facebookAccessToken});
                            }

                            // connect to windows live
                            if (windowsLiveAccessToken) {
                                chatClients.model.append({
                                    windowsLiveAccessToken: windowsLiveAccessToken});
                            }
                        }
                    }
                };
                xhr.open('GET', appSettings.wifirstBaseUrl + '/wilink/credentials', true, account.username, account.password);
                xhr.setRequestHeader('Accept', 'application/xml');
                xhr.send();
            }
        }
    }

    Keys.onPressed: {
        if (((event.modifiers == Qt.ControlModifier) && (event.key == Qt.Key_W))
         || ((event.modifiers == Qt.ControlModifier) && (event.key == Qt.Key_F4))) {
            chatSwapper.model.get(chatSwapper.currentIndex).panel.close();
        } else if (((event.modifiers & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && (event.key == Qt.Key_Left))
                || ((event.modifiers & Qt.ControlModifier) && (event.key == Qt.Key_PageUp))) {
            chatSwapper.decrementCurrentIndex();
        } else if (((event.modifiers & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && (event.key == Qt.Key_Right))
                || ((event.modifiers & Qt.ControlModifier) && (event.key == Qt.Key_PageDown))) {
            chatSwapper.incrementCurrentIndex();
        }
    }
}
