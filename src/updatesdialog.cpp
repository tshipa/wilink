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

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QTimer>

#include "systeminfo.h"
#include "updatesdialog.h"

UpdatesDialog::UpdatesDialog(QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;

    /* status */
    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *statusIcon = new QLabel;
    statusIcon->setPixmap(QPixmap(":/wiLink.png"));
    hbox->addWidget(statusIcon);
    hbox->addSpacing(10);
    statusLabel = new QLabel;
    statusLabel->setWordWrap(true);
    hbox->addWidget(statusLabel);
    layout->addItem(hbox);

    /* progress */
    progressBar = new QProgressBar;
    layout->addWidget(progressBar);
    setLayout(layout);

    /* updates */
    updates = new Updates(this);
    updates->setCacheDirectory(SystemInfo::storageLocation(SystemInfo::DownloadsLocation));

    bool check;
    check = connect(updates, SIGNAL(checkStarted()),
                    this, SLOT(checkStarted()));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(checkFinished(Release)),
                    this, SLOT(checkFinished(Release)));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(downloadStarted(Release)),
                    this, SLOT(downloadStarted(Release)));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(installStarted(Release)),
                    this, SLOT(installStarted(Release)));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(downloadFinished(Release)),
                    this, SLOT(downloadFinished(Release)));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(error(Updates::UpdatesError, const QString&)),
                    this, SLOT(error(Updates::UpdatesError, const QString&)));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(downloadProgress(qint64, qint64)),
                    this, SLOT(downloadProgress(qint64, qint64)));
    Q_ASSERT(check);
}

void UpdatesDialog::check()
{
    show();
    updates->check();
}

void UpdatesDialog::checkStarted()
{
    statusLabel->setText(tr("Checking for updates.."));
}

void UpdatesDialog::checkFinished(const Release &release)
{
    if (!release.isValid())
    {
        statusLabel->setText(tr("No update available"));
    } else {
        statusLabel->setText(tr("Update available"));
    }
    updates->download(release);
}

void UpdatesDialog::downloadStarted(const Release &release)
{
    Q_UNUSED(release);
    statusLabel->setText(tr("Downloading update.."));
}

void UpdatesDialog::downloadProgress(qint64 done, qint64 total)
{
    progressBar->setMaximum(total);
    progressBar->setValue(done);
}

void UpdatesDialog::installStarted(const Release &release)
{
    Q_UNUSED(release);
    statusLabel->setText(tr("Installing update.."));
}

void UpdatesDialog::downloadFinished(const Release &release)
{
    const QString message = QString("<p>%1</p><p><b>%2</b></p><pre>%3</pre><p>%4</p>")
            .arg(tr("Version %1 of %2 is available. Do you want to install it?")
                .arg(release.version)
                .arg(release.package))
            .arg(tr("Changes:"))
            .arg(release.changes)
            .arg(tr("%1 will automatically exit to allow you to install the new version.")
                .arg(release.package));
    if (QMessageBox::question(NULL,
        tr("Update available"),
        message,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
        show();
        updates->install(release);
    }
}

void UpdatesDialog::error(Updates::UpdatesError error, const QString &errorString)
{
    statusLabel->setText(tr("Could not run update, please try again later.") + "\n\n" + errorString);
}

