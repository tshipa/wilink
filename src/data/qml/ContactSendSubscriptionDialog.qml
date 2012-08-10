/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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
import 'scripts/utils.js' as Utils

Dialog {
    id: dialog

    property string jid
    property string nickname

    property QtObject client

    title: qsTr('Add a contact')

    function isWifirst(jid) {
       return Utils.jidToDomain(jid) == 'wifirst.net';
    }

    Column {
        anchors.fill: parent
        spacing: 8

        PanelHelp {
            id: help

            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr('The contact you tried to reach is not in your contact list.')
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            height: 25

            Label {
                anchors.fill: parent
                anchors.topMargin: 15
                horizontalAlignment: Text.Center
                verticalAlignment: Text.Center
                text: qsTr('Send a subscription request to %1 ?').replace('%1', nickname);
            }
        }
    }

    onAccepted: {
        console.log("Add contact " + jid + " to account " + client.jid);
        client.rosterManager.subscribe(jid);
        dialog.close();
    }
}
