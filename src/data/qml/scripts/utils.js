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

.pragma library

function datetimeFromString(text)
{
    function parseZeroInt(txt) {
        return parseInt(txt.replace(/^0/, ''));
    }

    var cap = text.match(/([0-9]{4})-([0-9]{2})-([0-9]{2})T([0-9]{2}):([0-9]{2}):([0-9]{2})Z/);
    if (cap) {
        var dt = new Date();
        dt.setUTCFullYear(parseInt(cap[1]));
        dt.setUTCMonth(parseZeroInt(cap[2]) - 1);
        dt.setUTCDate(parseZeroInt(cap[3]));
        dt.setUTCHours(parseZeroInt(cap[4]));
        dt.setUTCMinutes(parseZeroInt(cap[5]));
        dt.setUTCSeconds(parseZeroInt(cap[6]));
        return dt;
    }
}

function escapeRegExp(text) {
    return text.replace(/[-[\]{}()*+?.,\\^$|#\s]/g, '\\$&');
}

function formatDateTime(dateTime) {
    return Qt.formatDateTime(dateTime, 'dd MMM hh:mm');
}

function formatDuration(duration) {
    return duration + 's';
}

function formatNumber(size) {
    var KILO = 1000;
    var MEGA = KILO * 1000;
    var GIGA = MEGA * 1000;
    var TERA = GIGA * 1000;

    if (size < KILO)
        return size + ' ';
    else if (size < MEGA)
        return Math.floor(size / KILO) + ' K';
    else if (size < GIGA)
        return Math.floor(size / MEGA) + ' M';
    else if (size < TERA)
        return Math.floor(size / GIGA) + ' G';
    else
        return Math.floor(size / TERA) + ' T';
}

function formatSpeed(size) {
    return formatNumber(size * 8) + 'b/s';
}

function formatSize(size) {
    return formatNumber(size) + 'B';
}

function jidToBareJid(jid) {
    var pos = jid.indexOf('/');
    if (pos < 0)
        return jid;
    return jid.substring(0, pos);
}

function jidToDomain(jid) {
    var bareJid = jidToBareJid(jid);
    var pos = bareJid.indexOf('@');
    if (pos < 0)
        return bareJid;
    return bareJid.substring(pos + 1);
}

function jidToResource(jid) {
    var pos = jid.indexOf('/');
    if (pos < 0)
        return '';
    return jid.substring(pos + 1);
}

function jidToUser(jid) {
    var pos = jid.indexOf('@');
    if (pos < 0)
        return '';
    return jid.substring(0, pos);
}

// Compare the two object's properties and returns
// true if they are equal, false otherwise.

function equalProperties(a, b) {
    if (a.length != b.length)
        return false;
    for (var key in a) {
        if (a[key] != b[key])
            return false;
    }
    return true;
}

// Returns a dump of the object's properties as a string.

function dumpProperties(a) {
    var dump = '';
    for (var key in a) {
        if (key != 'loadScript')
            dump += (dump.length > 0 ? ', ' : '') + key + ': ' + a[key];
    }
    return '{' + dump + '}';
}

// There's no getElementsByTagName method in QML
// ([http://www.mail-archive.com/qt-qml@trolltech.com/msg01024.html]), so we'll
// implement our own version.
function getElementsByTagName(rootElement, tagName) {
    var childNodes = rootElement.childNodes;
    var elements = [];
    for (var i = 0; i < childNodes.length; i++) {
        if (childNodes[i].nodeName == tagName) {
            elements.push(childNodes[i]);
        }
    }
    return elements;
}

function getSocialIcon(email) {
    var result = {};
    switch (true) {
    case /chat\.facebook\.com/.test(email):
        result = {color: '#3d589d', style: 'icon-facebook-sign', opacity: 1, visible: true};
        break;
    case /gmail\.com/.test(email):
        result = {color: '#f71100', style: 'icon-google-plus-sign', opacity: 1, visible: true};
        break;
    default:
        result = {color: 'black', style:'icon-sign-blank', opacity: 0, visible: false};
        break;
    }
    return result;
}
