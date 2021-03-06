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
import 'scripts/utils.js' as Utils

Item {
    id: block

    property alias count: view.count
    property alias model: view.model
    property string style: ''

    Rectangle {
        id: background

        anchors.fill: parent
        color: '#ffffff'
    }

    Rectangle {
        id: border

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: '#93b9f2'
        height: 1
    }

    ScrollView {
        id: view

        anchors.top: border.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: appStyle.spacing.vertical

        delegate: Item {
            width: parent.width
            height: appStyle.icon.smallSize

            Image {
                id: thumbnail

                anchors.left: parent.left
                anchors.leftMargin: appStyle.margin.normal
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.PreserveAspectFit
                source: block.style == 'shares' ? (model.isDir ? (model.node.length ? 'image://icon/album' : 'image://icon/peer') : 'image://icon/file') : model.avatar
                sourceSize.width: appStyle.icon.smallSize
                sourceSize.height: appStyle.icon.smallSize
            }

            ProgressBar {
                id: progress

                anchors.left: thumbnail.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: cancelButton.left
                anchors.rightMargin: appStyle.spacing.horizontal
                anchors.top: parent.top
                anchors.topMargin: appStyle.margin.normal
                anchors.bottom: parent.bottom
                anchors.bottomMargin: appStyle.margin.normal
                maximumValue: model.totalBytes
                value: model.doneBytes

                Label {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    text: {
                        var text = model.name;
                        if (model.totalFiles > 1)
                            text += ', ' + qsTr('%1 files').replace('%1', model.totalFiles);
                        if (model.totalBytes > 0)
                            text += ' - ' + Utils.formatSize(model.totalBytes);
                        if (model.speed > 0)
                            text += ' - ' + Utils.formatSpeed(model.speed);
                        return text;
                    }
                }
            }

            Button {
                id: cancelButton

                anchors.right: parent.right
                anchors.rightMargin: appStyle.margin.normal
                anchors.verticalCenter: parent.verticalCenter
                iconStyle: 'icon-remove'
                text: qsTr('Cancel')

                onClicked: view.model.cancel(model.index)
            }
        }
    }
}

