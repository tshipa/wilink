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

Plugin {
    name: qsTr('Web')
    description: qsTr('This plugin allows you to access the web.')
    iconStyle: 'icon-globe'

    onLoaded: {
        dock.model.add({
            iconStyle: iconStyle,
            panelSource: 'WebPanel.qml',
            priority: 11,
            text: qsTr('Web'),
            visible: true,
        });
        swapper.showPanel('WebPanel.qml');
    }

    onUnloaded: {
        dock.model.removePanel('WebPanel.qml');
    }
}
