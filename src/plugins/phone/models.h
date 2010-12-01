/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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

#ifndef __WILINK_PHONE_MODELS_H__
#define __WILINK_PHONE_MODELS_H__

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>

class QNetworkAccessManager;
class PhoneCallsItem;

class PhoneCallsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    PhoneCallsModel(QNetworkAccessManager *network, QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    void setUrl(const QUrl &url);

private slots:
    void handleList();

private:
    class PhoneCallsItem
    {
    public:
        QString address;
        QDateTime date;
        int duration;
    };

    QList<PhoneCallsItem*> m_calls;
    QNetworkAccessManager *m_network;
};

#endif
