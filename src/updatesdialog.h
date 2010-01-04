/*
 * wDesktop
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

#ifndef __UPDATES_DIALOG_H__
#define __UPDATES_DIALOG_H__

#include <QDialog>

#include "updates.h"

class QLabel;
class QProgressBar;
class QUrl;

class UpdatesDialog : public QDialog
{
    Q_OBJECT

public:
    UpdatesDialog(QWidget *parent = NULL);
    void setUrl(const QUrl &url);

public slots:
    void check();

protected slots:
    void updateAvailable(const Release &release);
    void updateDownloaded(const QUrl &url);
    void updateFailed();
    void updateProgress(qint64 done, qint64 total);

signals:
    void installRelease(const Release &release);

private:
    QProgressBar *progressBar;
    QLabel *statusLabel;
    Updates *updates;
};

#endif
