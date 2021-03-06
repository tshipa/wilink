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
import 'scripts/utils.js' as Utils

Dialog {
    id: dialog

    property bool accountChoice: true
    property QtObject accountModel: AccountModel {}
    property bool accountSlave: false

    title: qsTr('Add an account')
    minimumWidth: 440
    minimumHeight: 270

    function save() {
        if (!dialog.accountSlave) {
            accountModel.submit();
            window.reload();
        }

        dialog.close();
    }

    Item {
        anchors.fill: parent
        anchors.leftMargin: appStyle.margin.normal
        anchors.rightMargin: appStyle.margin.normal

        PanelHelp {
            id: topHelp

            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr('Enter the address and password for your account.')
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing:  appStyle.spacing.vertical

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: accountCombo.height
                visible: accountChoice

                Label {
                    id: accountLabel

                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    font.bold: true
                    text: qsTr('Account')
                    width: 100
                }

                ComboBox {
                    id: accountCombo

                    anchors.top: parent.top
                    anchors.left: accountLabel.right
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: parent.right
                    model: ListModel {}

                    Component.onCompleted: {
                        // make a note of existing accounts
                        var hasWifirst = false;
                        var hasGoogle = false;
                        for (var i = 0; i < accountModel.count; i++) {
                            var account = accountModel.get(i);
                            if (account.type == 'web' && account.realm == 'www.wifirst.net') {
                                hasWifirst = true;
                            } else if (account.type == 'web' && account.realm == 'www.google.com') {
                                hasGoogle = true;
                            }
                        }

                        // populate account type combo
                        if (!hasWifirst)
                            model.append({iconStyle: 'icon-home', text: 'Wifirst', type: 'wifirst'});
                        if (!hasGoogle)
                            model.append({iconStyle: 'icon-google-plus', text: 'Google', type: 'google'});
                        model.append({iconStyle: 'icon-user', text: qsTr('Other'), type: 'other'});
                        accountCombo.currentIndex = 0;
                    }
                }
            }

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: usernameInput.height

                Label {
                    id: usernameLabel

                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    font.bold: true
                    text: qsTr('Address')
                    width: 100
                }

                InputBar {
                    id: usernameInput

                    anchors.top: parent.top
                    anchors.left: usernameLabel.right
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: parent.right
                    focus: true
                    validator: RegExpValidator {
                        regExp: /.+/
                    }
                    onTextChanged: dialog.state = ''
                }
            }

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: passwordInput.height

                Label {
                    id: passwordLabel

                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    font.bold: true
                    text: qsTr('Password')
                    width: 100
                }

                InputBar {
                    id: passwordInput

                    anchors.top: parent.top
                    anchors.left: passwordLabel.right
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: parent.right
                    echoMode: TextInput.Password
                    validator: RegExpValidator {
                        regExp: /.+/
                    }
                    onTextChanged: dialog.state = ''
                }
            }

            Label {
                id: statusLabel

                property string authErrorText: qsTr('Could not connect, please check your username and password.')
                property string unknownErrorText: qsTr('An unknown error occured.')
                property string dupeText: qsTr("You already have an account for '%1'.").replace('%1', Utils.jidToDomain(usernameInput.text));
                property string incompleteText: qsTr('Please enter a valid username and password.')
                property string testingText: qsTr('Checking your username and password..')

                anchors.left: parent.left
                anchors.right: parent.right
                visible: text != ''
                wrapMode: Text.WordWrap
            }
        }

        PanelHelp {
            id: bottomHelp

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            text: qsTr('If you need help, please refer to the <a href="%1">wiLink FAQ</a>.').replace('%1', appSettings.wifirstBaseUrl + '/wilink/faq')
        }
    }

    onAccepted: {
        if (!usernameInput.acceptableInput || !passwordInput.acceptableInput) {
            dialog.state = 'incomplete';
            return;
        }
        dialog.state = 'testing';

        var accountType = accountCombo.model.get(accountCombo.currentIndex).type;
        if (accountType == 'wifirst') {
            // validate input
            if (usernameInput.text.indexOf('@') < 0) {
                usernameInput.text += '@wifirst.net';
            }
            var username = usernameInput.text;
            var password = passwordInput.text;

            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    if (xhr.status == 200) {
                        accountModel.append({type: 'web',
                            username: username,
                            password: password,
                            realm: 'www.wifirst.net'});
                        dialog.save();
                    } else if (xhr.status == 401) {
                        dialog.state = 'authError';
                    } else {
                        dialog.state = 'unknownError';
                    }
                }
            }
            xhr.open('GET', 'https://apps.wifirst.net/wilink/credentials', true, username, password);
            xhr.setRequestHeader('Accept', 'application/xml');
            xhr.send();
        } else if (accountType == 'google') {
            // validate input
            if (usernameInput.text.indexOf('@') < 0) {
                usernameInput.text += '@gmail.com';
            }
            var username = usernameInput.text;
            var password = passwordInput.text;

            // test credentials
            dialog.state = 'testing';
            console.log("connecting: " + username);
            testClient.connectToServer(username + '/AccountCheck', password);
        } else {
            // validate input
            if (usernameInput.text.indexOf('@') < 0) {
                dialog.state = 'incomplete';
                return;
            }
            var username = usernameInput.text;
            var password = passwordInput.text;

            // check for duplicate account
            for (var i = 0; i < accountModel.count; i++) {
                var account = accountModel.get(i);
                if (account.type == 'xmpp' && account.realm == Utils.jidToDomain(username)) {
                    dialog.state = 'dupe';
                    return;
                }
            }

            // test credentials
            console.log("connecting: " + username);
            testClient.connectToServer(username + '/AccountCheck', password);
        }
    }

    resources: [
        Client {
            id: testClient

            logger: QXmppLogger {
                loggingType: QXmppLogger.StdoutLogging
            }

            onConnected: {
                var accountType = accountCombo.model.get(accountCombo.currentIndex).type;
                if (accountType == 'google') {
                    console.log("google ok");
                    accountModel.append({type: 'web',
                        username: usernameInput.text,
                        password: passwordInput.text,
                        realm: 'www.google.com'});
                } else {
                    console.log("other ok");
                    accountModel.append({type: 'xmpp',
                        username: usernameInput.text,
                        password: passwordInput.text,
                        realm: Utils.jidToDomain(usernameInput.text)});
                }
                dialog.save();
            }

            onDisconnected: {
                if (dialog.state == 'testing') {
                    dialog.state = 'authError';
                }
            }
        }
    ]

    states: [
        State {
            name: 'authError'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.authErrorText }
        },

        State {
            name: 'unknownError'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.unknownErrorText }
        },

        State {
            name: 'dupe'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.dupeText }
        },

        State {
            name: 'incomplete'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.incompleteText }
        },

        State {
            name: 'testing'
            PropertyChanges { target: statusLabel; text: statusLabel.testingText }
            PropertyChanges { target: usernameInput; enabled: false }
            PropertyChanges { target: passwordInput; enabled: false }
        }
    ]

    onRejected: {
        if (dialog.accountSlave)
            dialog.close()
        else
            Qt.quit()
    }

    Keys.onTabPressed: {
        if (usernameInput.activeFocus)
            passwordInput.forceActiveFocus();
        else
            usernameInput.forceActiveFocus();
    }
}
