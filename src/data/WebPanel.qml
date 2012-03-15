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
import QtWebKit 1.0
import wiLink 2.0

Panel {
    id: panel

    property variant url

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        iconSource: 'web.png'
        title: qsTr('Web') + ' - ' + webView.title
        subTitle: webView.url
        toolBar: ToolBar {
            ToolButton {
                action: webView.back
                iconSource: 'back.png'
            }
            ToolButton {
                action: webView.forward
                iconSource: 'forward.png'
            }
        }
        z: 3
    }

    FocusScope {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        Flickable {
            id: webFlickable

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: scrollBar.left
            anchors.margins: appStyle.margin.small
            contentWidth: webView.width
            contentHeight: webView.height

            WebView {
                id: webView

                focus: true
                preferredHeight: webFlickable.height
                preferredWidth: webFlickable.width
                url: 'http://www.wifirst.net/'
            }
        }

        ScrollBar {
            id: scrollBar

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            flickableItem: webFlickable
        }
    }
}