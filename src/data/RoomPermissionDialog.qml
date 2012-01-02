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

import QtQuick 1.1
import QXmpp 0.4
import wiLink 2.0

Dialog {
    id: dialog

    property alias room: permissionModel.room

    minimumWidth: 280
    minimumHeight: 150
    title: qsTr('Chat room permissions')

    RoomPermissionModel {
        id: permissionModel
    }

    ListModel {
        id: affiliationModel

        Component.onCompleted: {
            append({'text': qsTr('member'), 'value': QXmppMucItem.MemberAffiliation});
            append({'text': qsTr('administrator'), 'value': QXmppMucItem.AdminAffiliation});
            append({'text': qsTr('owner'), 'value': QXmppMucItem.OwnerAffiliation});
            append({'text': qsTr('banned'), 'value': QXmppMucItem.OutcastAffiliation});

            combo.currentIndex = 0;
        }
    }

    Item {
        anchors.fill: contents

        ListView {
            id: view

            anchors.top: parent.top
            anchors.bottom: addRow.top
            anchors.bottomMargin: appStyle.spacing.vertical
            anchors.left: parent.left
            anchors.right: scrollBar.left
            clip: true

            model: permissionModel
            delegate: Item {
                id: item

                property string jid: model.jid

                width: parent.width
                height: combo.height

                Label {
                    id: label

                    property int affiliation: model.affiliation

                    anchors.left: parent.left
                    anchors.right: combo.left
                    anchors.rightMargin: appStyle.spacing.horizontal
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    text: model.jid
                    verticalAlignment: Text.AlignVCenter
                }

                ComboBox {
                    id: combo

                    anchors.top: parent.top
                    anchors.right: button.left
                    anchors.rightMargin: appStyle.spacing.horizontal
                    model: affiliationModel
                    width: 120

                    onCurrentIndexChanged: {
                        if (currentIndex > 0) {
                            permissionModel.setPermission(item.jid, model.get(currentIndex).value);
                        }
                    }

                    Component.onCompleted: {
                        for (var i = 0; i < model.count; i++) {
                            if (label.affiliation == model.get(i).value) {
                                combo.currentIndex = i;
                                break;
                            }
                        }
                    }
                }

                Button {
                    id: button

                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    iconSource: 'remove.png'
                    onClicked: {
                        permissionModel.removePermission(model.jid);
                    }
                }
            }
        }

        ScrollBar {
            id: scrollBar

            anchors.top: parent.top
            anchors.bottom: addRow.top
            anchors.bottomMargin: appStyle.spacing.vertical
            anchors.right: parent.right
            flickableItem: view
        }

        Item {
            id: addRow

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: input.height

            InputBar {
                id: input

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: combo.left
                anchors.rightMargin: appStyle.spacing.horizontal
                validator: RegExpValidator {
                    regExp: /^[^@/ ]+@[^@/ ]+$/
                }
            }

            ComboBox {
                id: combo

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: button.left
                anchors.rightMargin: appStyle.spacing.horizontal
                model: affiliationModel
                width: 120
            }

            Button {
                id: button

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                iconSource: 'add.png'

                onClicked: {
                    if (input.acceptableInput) {
                        permissionModel.setPermission(input.text, affiliationModel.get(combo.currentIndex).value);
                        input.text = '';
                    }
                }
            }
        }
    }

    onAccepted: {
        permissionModel.save();
        dialog.close();
    }
}
