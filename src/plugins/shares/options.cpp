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

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>

#include "QXmppShareDatabase.h"

#include "options.h"

ChatSharesOptions::ChatSharesOptions(QXmppShareDatabase *database, QWidget *parent)
    : QDialog(parent),
    m_database(database)
{
    QVBoxLayout *layout = new QVBoxLayout;

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("Shares folder")));
    m_directoryEdit = new QLineEdit;
    m_directoryEdit->setEnabled(false);
    hbox->addWidget(m_directoryEdit);
    QPushButton *directoryButton = new QPushButton;
    directoryButton->setIcon(QIcon(":/album.png"));
    connect(directoryButton, SIGNAL(clicked()), this, SLOT(browse()));
    hbox->addWidget(directoryButton);
    layout->addItem(hbox);

    m_listWidget = new QListWidget;
    layout->addWidget(m_listWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(validate()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox);

    setLayout(layout);
    setWindowTitle(tr("Shares options"));

    /* load preferences */
    m_directoryEdit->setText(m_database->directory());
    foreach (const QString &dir, m_database->mappedDirectories())
        m_listWidget->addItem(dir);
}

void ChatSharesOptions::browse()
{
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setDirectory(m_directoryEdit->text());
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setWindowTitle(tr("Shares folder"));
    dialog->show();

    bool check;
    check = connect(dialog, SIGNAL(finished(int)),
                    dialog, SLOT(deleteLater()));
    Q_ASSERT(check);

    check = connect(dialog, SIGNAL(fileSelected(QString)),
                    this, SLOT(directorySelected(QString)));
    Q_ASSERT(check);
}

void ChatSharesOptions::directorySelected(const QString &path)
{
    m_directoryEdit->setText(path);
}

void ChatSharesOptions::validate()
{
    const QString path = m_directoryEdit->text();
    QStringList mapped;
    for (int i = 0; i < m_listWidget->count(); ++i)
        mapped << m_listWidget->item(i)->text();

    // remember directory
    QSettings settings;
    settings.setValue("SharesLocation", path);
    settings.setValue("SharesDirectories", mapped);

    m_database->setDirectory(path);
    m_database->setMappedDirectories(mapped);
    accept();
}

