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
import wiLink 2.4
import 'utils.js' as Utils

Panel {
    id: panel

    property string accountJid
    property QtObject soundJob
    property string soundUrl

    function finished() {
        panel.soundJob = null;
        panel.soundUrl = '';
    }

    function play(model) {
        stop();
        panel.soundJob = appSoundPlayer.play(decodeURIComponent(model.audioSource));
        panel.soundJob.finished.connect(panel.finished);
        panel.soundUrl = model.audioSource;
    }

    function stop() {
        if (panel.soundJob) {
            panel.soundJob.finished.disconnect(panel.finished);
            panel.soundJob.stop();
            panel.soundJob = null;
            panel.soundUrl = '';
        }
    }

    NewsListModel {
        id: newsListModel

    }

    ContactView {
        id: sidebar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        title: qsTr('My news')
        visible: width > 0
        width: panel.singlePanel ? parent.width : appStyle.sidebarWidth
        z: 1

        delegate: Item {
            id: item

            height: appStyle.icon.normalSize
            width: parent.width

            Image {
                id: image

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.leftMargin: appStyle.spacing.horizontal
                height: appStyle.icon.normalSize
                width: appStyle.icon.normalSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                source: model.avatar
            }

            Label {
                id: titleLabel

                anchors.verticalCenter: image.verticalCenter
                anchors.left: image.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: parent.right
                anchors.rightMargin: appStyle.spacing.horizontal
                elide: Text.ElideRight
                font.bold: true
                text: model.name
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent

                onClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        sidebar.currentIndex = model.index;
                        if (panel.singlePanel)
                            panel.state = 'no-sidebar';
                    } else if (mouse.button == Qt.RightButton) {
                        // show context menu
                        var pos = mapToItem(menuLoader.parent, mouse.x, mouse.y);
                        menuLoader.sourceComponent = newsListMenu;
                        menuLoader.item.bookmarkName = model.name;
                        menuLoader.item.bookmarkUrl = model.url;
                        menuLoader.show(pos.x, pos.y);
                    }
                }
            }
        }

        model: newsListModel

        onAddClicked: {
            dialogSwapper.showPanel('NewsDialog.qml', {'model': newsListModel});
        }

        onCurrentIndexChanged: {
            if (currentIndex >= 0) {
                var item = newsListModel.get(currentIndex);
                main.subTitle = item.name;
                mainView.model.source = item.url;
            }
        }

        Component {
            id: newsListMenu

            Menu {
                id: menu

                property string bookmarkName
                property url bookmarkUrl

                onItemClicked: {
                    var item = menu.model.get(index);
                    if (item.action == 'edit') {
                        dialogSwapper.showPanel('NewsDialog.qml', {
                            'editUrl': bookmarkUrl,
                            'model': newsListModel,
                            'name': bookmarkName,
                            'url': bookmarkUrl
                        });
                    } else if (item.action == 'remove') {
                        newsListModel.removeBookmark(bookmarkUrl);
                    }
                }

                Component.onCompleted: {
                    menu.model.append({
                        action: 'edit',
                        iconStyle: 'icon-wrench',
                        text: qsTr('Modify')});
                    menu.model.append({
                        action: 'remove',
                        iconStyle: 'icon-minus',
                        text: qsTr('Remove')});
                }
            }
        }
    }

    Item {
        id: main

        property alias subTitle: header.subTitle

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: panel.singlePanel ? parent.left : sidebar.right
        anchors.right: parent.right
        visible: width > 0

        PanelHeader {
            id: header

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            iconStyle: 'icon-rss'
            title: qsTr('News reader')
        }

        ScrollView {
            id: mainView

            property QtObject soundJob

            function resolvedUrl(url) {
                // FIXME: handle relative urls
                return url;
            }

            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: appStyle.spacing.horizontal
            focus: true
            delegate: Item {
                id: item

                height: appStyle.icon.normalSize
                width: parent.width

                Image {
                    id: image

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.leftMargin: appStyle.spacing.horizontal
                    height: appStyle.icon.normalSize
                    width: appStyle.icon.normalSize
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    source: {
                        if (model.thumbnailSource)
                            return resolvedUrl(model.thumbnailSource);
                        else if (model.imageSource)
                            return resolvedUrl(model.imageSource);
                        else
                            return 'image://icon/rss';
                    }
                }

                Column {
                    anchors.verticalCenter: image.verticalCenter

                    anchors.left: image.right
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: parent.right
                    anchors.rightMargin: appStyle.spacing.horizontal

                    Label {
                        id: titleLabel

                        anchors.left: parent.left
                        anchors.right: parent.right
                        elide: Text.ElideRight
                        font.bold: true
                        text: model.title
                    }

                    Label {
                        id: dateLabel

                        anchors.left: parent.left
                        anchors.right: parent.right
                        elide: Text.ElideRight
                        text: Utils.formatDateTime(new Date(model.pubDate))
                    }
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        if (item.state == 'details')
                            Qt.openUrlExternally(model.link);
                        else
                            mainView.currentIndex = model.index;
                    }
                }

                Column {
                    id: descriptionColumn

                    anchors.margins: appStyle.margin.normal
                    anchors.top: image.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: appStyle.margin.normal
                    visible: false

                    Label {
                        id: descriptionLabel

                        anchors.left: parent.left
                        anchors.right: parent.right
                        wrapMode: Text.WordWrap

                        onLinkActivated: Qt.openUrlExternally(link)
                    }

                    Row {
                        spacing: appStyle.margin.normal

                        Button {
                            iconStyle: 'icon-play'
                            text: qsTr('Play sound')
                            visible: model.audioSource ? (panel.soundUrl != model.audioSource) : false

                            onClicked: panel.play(model)
                        }

                        Button {
                            iconStyle: 'icon-stop'
                            text: qsTr('Stop sound')
                            visible: model.audioSource ? (panel.soundUrl == model.audioSource) : false

                            onClicked: panel.stop();
                        }

                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: {
                                if (panel.soundJob && panel.soundUrl == model.audioSource) {
                                    switch (panel.soundJob.state)
                                    {
                                    case SoundPlayerJob.DownloadingState:
                                        return 'Downloading';
                                    case SoundPlayerJob.PlayingState:
                                        return 'Playing';
                                    }
                                }
                                return '';

                            }
                        }
                    }
                }

                states: State {
                    name: 'details'
                    when: mainView.currentIndex == model.index

                    PropertyChanges {
                        target: descriptionLabel
                        text: {
                            // resolve urls to absolute urls
                            var text = model.description.replace(/src=(['"])([^'"]+)(['"])/, function(match, quote, url) {
                                return 'src=' + quote + resolvedUrl(url) + quote;
                            });

                            return '<p>' + text + '</p>';
                        }
                    }

                    PropertyChanges {
                        target: descriptionColumn
                        visible: true
                    }

                    PropertyChanges {
                        target: item
                        height: image.height + 2*appStyle.margin.normal + descriptionColumn.height
                    }
                }
            }

            model: XmlListModel {
                id: newsModel

                query: '/rss/channel/item'

                XmlRole { name: 'description'; query: 'description/string()' }
                XmlRole { name: 'link'; query: 'link/string()' }
                XmlRole { name: 'pubDate'; query: 'pubDate/string()' }
                XmlRole { name: 'audioSource'; query: 'enclosure[@type="audio/mpeg"]/@url/string()' }
                XmlRole { name: 'imageSource'; query: 'enclosure[@type="image/jpeg"]/@url/string()' }
                XmlRole { name: 'thumbnailSource'; query: '*:thumbnail[1]/@url/string()' }
                XmlRole { name: 'title'; query: 'title/string()' }
            }

            Keys.onReturnPressed: {
                var item = model.get(currentIndex);
                Qt.openUrlExternally(item.link);
            }
        }
    }

    onAccountJidChanged: {
        newsListModel.client = accountModel.clientForJid(panel.accountJid);
    }

    onDockClicked: {
        panel.state = (panel.state == 'no-sidebar') ? '' : 'no-sidebar';
    }

    states: State {
        name: 'no-sidebar'

        PropertyChanges { target: sidebar; width: 0 }
    }

    transitions: Transition {
        PropertyAnimation { target: sidebar; properties: 'width'; duration: appStyle.animation.normalDuration }
    }
}

