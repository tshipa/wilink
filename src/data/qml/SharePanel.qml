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

Panel {
    id: panel

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        iconStyle: 'icon-share'
        title: qsTr('Shares')
        toolBar: ToolBar {
            ToolButton {
                iconStyle: 'icon-chevron-left'
                text: qsTr('Go back')
                enabled: crumbBar.model.count > 1

                onClicked: crumbBar.pop()
            }

            ToolButton {
                id: preferenceButton

                iconStyle: 'icon-wrench'
                text: qsTr('Options')

                onClicked: {
                    dialogSwapper.showPanel('PreferenceDialog.qml', {initialPanel: 'SharePreferencePanel.qml'});
                }

                // run one-time configuration dialog
                Component.onCompleted: {
                    if (!appSettings.sharesConfigured && !appSettings.isMobile) {
                        preferenceButton.clicked();
                        appSettings.sharesConfigured = true;
                    }
                }
            }
        }
    }

    PanelHelp {
        id: help

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        text: qsTr('You can select the folders you want to share with other users from the shares options.')
        z: 1
    }

    Rectangle {
        id: searchBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: help.bottom
        color: '#e1eafa'
        height: 32
        z: 1

        Icon {
            id: searchIcon

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            style: 'icon-search'
        }

        InputBar {
            id: searchEdit

            anchors.left: searchIcon.right
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 8
            focus: true
            hintText: qsTr('Enter the name of the file you are looking for.')
            state: (view.model.count == 0 && !view.model.busy && searchEdit.text.length > 0) ? 'error' : ''

            onTextChanged: searchTimer.restart()

            Timer {
                id: searchTimer
                interval: 500
                onTriggered: view.model.filter = searchEdit.text
            }
        }
    }

    CrumbBar {
        id: crumbBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: searchBar.bottom

        onLocationChanged: {
            view.model.rootUrl = location.url;
        }

        Component.onCompleted: {
            crumbBar.push({name: qsTr('Home'), url: 'share:'});
        }
        z: 1
    }

    ShareView {
        id: view

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: crumbBar.bottom
        anchors.bottom: queueHelp.top

        model: FolderModel {
            onCountChanged: {
                // show dock icon
                if (rootUrl == 'share:' && count > 0) {
                    for (var i = 0; i < dock.model.count; i++) {
                        if (dock.model.get(i).panelSource == 'SharePanel.qml') {
                            dock.model.setProperty(i, 'visible', true);
                            break;
                        }
                    }
                }
            }
        }

        Spinner {
            anchors.centerIn: parent
            busy: view.model.busy
        }
    }

    PanelHelp {
        id: queueHelp

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: queueView.top
        text: qsTr('Received files are stored in your <a href="%1">downloads folder</a>.').replace('%1', appSettings.sharesUrl)
        z: 1
    }

    FolderQueueView {
        id: queueView

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        height: queueView.count > 0 ? 120 : 0
        model: view.model.uploads
        style: 'shares'
    }
}
