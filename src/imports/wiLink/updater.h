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

#ifndef __UPDATES_H__
#define __UPDATES_H__

#include <QMap>
#include <QUrl>

class QNetworkReply;

class UpdaterPrivate;

/** The Updater class handling checking for software updates
 *  and installing them.
 */
class Updater : public QObject
{
    Q_OBJECT
    Q_ENUMS(Error State)
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    Q_PROPERTY(QString applicationVersion READ applicationVersion CONSTANT)
    Q_PROPERTY(Error error READ error NOTIFY error)
    Q_PROPERTY(QString errorString READ errorString NOTIFY error)
    Q_PROPERTY(int progressMaximum READ progressMaximum CONSTANT)
    Q_PROPERTY(int progressValue READ progressValue NOTIFY progressValueChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString updateChanges READ updateChanges NOTIFY updateChanged)
    Q_PROPERTY(QString updateVersion READ updateVersion NOTIFY updateChanged)

public:
    enum Error {
        NoError = 0,
        NoUpdateError,
        IntegrityError,
        FileError,
        NetworkError,
        SecurityError,
    };

    enum State {
        IdleState = 0,
        CheckState,
        DownloadState,
        PromptState,
        InstallState,
    };

    Updater(QObject *parent = 0);
    ~Updater();

    QString applicationName() const;
    QString applicationVersion() const;
    Error error() const;
    QString errorString() const;
    int progressMaximum() const;
    int progressValue() const;
    State state() const;
    QString updateChanges() const;
    QString updateVersion() const;

    static int compareVersions(const QString &v1, const QString v2);

public slots:
    void check();
    void install();
    void refuse();

signals:
    void error(Updater::Error error, const QString &errorString);
    void progressValueChanged(int value);
    void stateChanged(Updater::State state);
    void updateChanged();

private slots:
    void _q_downloadProgress(qint64 done, qint64 total);
    void _q_saveUpdate();
    void _q_processStatus();

private:
    UpdaterPrivate *d;
    friend class UpdaterPrivate;
};

#endif
