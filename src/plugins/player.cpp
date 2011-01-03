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

#include <QFileInfo>
#include <QHeaderView>
#include <QLayout>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>

#include "application.h"
#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"

#include "qsound/QSoundFile.h"
#include "qsound/QSoundPlayer.h"
#include "player.h"

#define DURATION_WIDTH 60
#define PLAYER_ROSTER_ID "0_player"

enum PlayerColumns {
    ArtistColumn = 0,
    TitleColumn,
    DurationColumn,
    MaxColumn
};

PlayerModel::Item::Item()
    : duration(0),
    parent(0)
{
}

PlayerModel::Item::~Item()
{
    foreach (Item *item, children)
        delete item;
}

int PlayerModel::Item::row() const
{
    if (!parent)
        return -1;
    return parent->children.indexOf((Item*)this);
}

PlayerModel::PlayerModel(QObject *parent)
    : QAbstractItemModel(parent),
    m_cursorItem(0)
{
    m_rootItem = new Item;

    // load saved playlist
    QSettings settings;
    QStringList values = settings.value("PlayerUrls").toStringList();
    foreach (const QString &value, values)
        addUrl(QUrl(value));
}

PlayerModel::~PlayerModel()
{
    delete m_rootItem;
}

bool PlayerModel::addUrl(const QUrl &url)
{
    QSoundFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    Item *item = new Item;
    item->parent = m_rootItem;
    item->url = url;
    item->duration = file.duration();

    QStringList values = file.metaData(QSoundFile::TitleMetaData);
    if (!values.isEmpty())
        item->title = values.first();
    else
        item->title = QFileInfo(url.toLocalFile()).baseName();

    values = file.metaData(QSoundFile::ArtistMetaData);
    if (!values.isEmpty())
        item->artist = values.first();

    beginInsertRows(createIndex(item->parent), item->parent->children.size(), item->parent->children.size());
    m_rootItem->children.append(item);
    endInsertRows();

    // save playlist
    save();
    return true;
}

int PlayerModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QModelIndex PlayerModel::cursor() const
{
    return createIndex(m_cursorItem);
}

void PlayerModel::setCursor(const QModelIndex &index)
{
    Item *item = static_cast<Item*>(index.internalPointer());
    if (item != m_cursorItem) {
        Item *oldItem = m_cursorItem;
        m_cursorItem = item;
        if (oldItem)
            emit dataChanged(createIndex(oldItem, 0), createIndex(oldItem, MaxColumn));
        if (item)
            emit dataChanged(createIndex(item, 0), createIndex(item, MaxColumn));
        emit cursorChanged(index);
    }
}

QVariant PlayerModel::data(const QModelIndex &index, int role) const
{
    Item *item = static_cast<Item*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == Qt::UserRole)
        return item->url;

    if (index.column() == ArtistColumn) {
        if (role == Qt::DisplayRole)
            return item->artist;
        else if (role == Qt::DecorationRole && item == m_cursorItem)
            return QPixmap(":/start.png");
    } else if (index.column() == TitleColumn) {
        if (role == Qt::DisplayRole)
            return item->title;
    } else if (index.column() == DurationColumn) {
        if (role == Qt::DisplayRole) {
            return QTime().addMSecs(item->duration).toString("m:ss");
        }
    }
    return QVariant();
}

QVariant PlayerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == ArtistColumn)
            return tr("Artist");
        else if (section == TitleColumn)
            return tr("Title");
        else if (section == DurationColumn)
            return tr("Duration");
    }
    return QVariant();
}

QModelIndex PlayerModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : m_rootItem;
    return createIndex(parentItem->children[row], column);
}

QModelIndex PlayerModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    Item *item = static_cast<Item*>(index.internalPointer());
    return createIndex(item->parent);
}

bool PlayerModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : m_rootItem;

    const int minIndex = qMax(0, row);
    const int maxIndex = qMin(row + count, parentItem->children.size()) - 1;
    beginRemoveRows(parent, minIndex, maxIndex);
    for (int i = maxIndex; i >= minIndex; --i)
        parentItem->children.removeAt(i);
    endRemoveRows();

    // save playlist
    save();
    return true;
}

int PlayerModel::rowCount(const QModelIndex &parent) const
{
    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : m_rootItem;
    return parentItem->children.size();
}

void PlayerModel::save()
{
    QSettings settings;
    QStringList values;
    foreach (Item *item, m_rootItem->children)
        values << item->url.toString();
    settings.setValue("PlayerUrls", values);
}

PlayerPanel::PlayerPanel(Chat *chatWindow)
    : ChatPanel(chatWindow),
    m_chat(chatWindow),
    m_playId(-1),
    m_playStop(false)
{
    bool check;

    setObjectName(PLAYER_ROSTER_ID);
    setWindowIcon(QIcon(":/start.png"));
    setWindowTitle(tr("Media player"));

    m_model = new PlayerModel(this);
    check = connect(m_model, SIGNAL(cursorChanged(QModelIndex)),
                    this, SLOT(cursorChanged(QModelIndex)));
    Q_ASSERT(check);

    Application *wApp = qobject_cast<Application*>(qApp);
    m_player = wApp->soundPlayer();
    check = connect(m_player, SIGNAL(finished(int)),
                    this, SLOT(finished(int)));
    Q_ASSERT(check);

    // build layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());
    layout->addSpacing(5);

    // controls
    QHBoxLayout *controls = new QHBoxLayout;
    controls->addStretch();

    m_playButton = new QPushButton(QIcon(":/start.png"), tr("Play"));
    check = connect(m_playButton, SIGNAL(clicked()),
                    this, SLOT(play()));
    Q_ASSERT(check);
    controls->addWidget(m_playButton);

    m_stopButton = new QPushButton(QIcon(":/stop.png"), tr("Stop"));
    m_stopButton->hide();
    check = connect(m_stopButton, SIGNAL(clicked()),
                    this, SLOT(stop()));
    Q_ASSERT(check);
    controls->addWidget(m_stopButton);

    controls->addStretch();
    layout->addLayout(controls);
    layout->addSpacing(10);

    // playlist
    m_view = new PlayerView;
    m_view->setModel(m_model);
    m_view->setIconSize(QSize(32, 32));
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    check = connect(m_view, SIGNAL(doubleClicked(QModelIndex)),
                    this, SLOT(doubleClicked(QModelIndex)));
    Q_ASSERT(check);
    layout->addWidget(m_view);

    setLayout(layout);

    // register panel
    filterDrops(m_view);

    check = connect(m_chat, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)),
                    this, SLOT(rosterDrop(QDropEvent*, QModelIndex)));
    Q_ASSERT(check);

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_M), m_chat);
    check = connect(shortcut, SIGNAL(activated()),
                    this, SIGNAL(showPanel()));
    Q_ASSERT(check);
}

void PlayerPanel::cursorChanged(const QModelIndex &index)
{
    if (index.isValid()) {
        m_playButton->hide();
        m_stopButton->show();
    } else {
        m_stopButton->hide();
        m_playButton->show();
    }
}

void PlayerPanel::doubleClicked(const QModelIndex &index)
{
    if (m_playId >= 0)
        m_player->stop(m_playId);

    QUrl audioUrl = index.data(Qt::UserRole).toUrl();
    if (audioUrl.isValid()) {
        m_model->setCursor(index);
        m_playId = m_player->play(audioUrl.toLocalFile());
    } else {
        m_model->setCursor(QModelIndex());
        m_playId = -1;
    }
}

void PlayerPanel::finished(int id)
{
    if (id != m_playId)
        return;
    m_playId = -1;
    if (m_playStop) {
        m_model->setCursor(QModelIndex());
        m_stopButton->setEnabled(true);
        m_playStop = false;
    } else {
        const QModelIndex cursor = m_model->cursor();
        QModelIndex nextIndex = cursor.sibling(cursor.row() + 1, cursor.column());
        doubleClicked(nextIndex);
    }
}

void PlayerPanel::play()
{
    QList<QModelIndex> selected = m_view->selectionModel()->selectedIndexes();
    if (!selected.isEmpty())
        doubleClicked(selected.first());
    else if (m_model->rowCount(QModelIndex()))
        doubleClicked(m_model->index(0, 0, QModelIndex()));
}

/** Handle a drop event on a roster entry.
 */
void PlayerPanel::rosterDrop(QDropEvent *event, const QModelIndex &index)
{
    if (index.data(ChatRosterModel::IdRole).toString() != objectName())
        return;

    int found = 0;
    foreach (const QUrl &url, event->mimeData()->urls())
    {
        if (url.scheme() != "file")
            continue;
        if (event->type() == QEvent::Drop)
            m_model->addUrl(url);
        found++;
    }
    if (found)
        event->acceptProposedAction();
}

void PlayerPanel::stop()
{
    if (m_playId >= 0) {
        m_player->stop(m_playId);
        m_playStop = true;
        m_stopButton->setEnabled(false);
    }
}

PlayerView::PlayerView(QWidget *parent)
    : QTreeView(parent)
{
    header()->setResizeMode(QHeaderView::Fixed);
}

void PlayerView::setModel(PlayerModel *model)
{
    QTreeView::setModel(model);
    setColumnWidth(DurationColumn, 40);
}

void PlayerView::keyPressEvent(QKeyEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (index.isValid()) {
        switch (event->key())
        {
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            model()->removeRow(index.row());
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            emit doubleClicked(index);
            break;
        }
    }

    QTreeView::keyPressEvent(event);
}

void PlayerView::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);

    const int available = e->size().width() - DURATION_WIDTH;
    setColumnWidth(ArtistColumn, available/2);
    setColumnWidth(TitleColumn, available/2);
}

// PLUGIN

class PlayerPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool PlayerPlugin::initialize(Chat *chat)
{
    PlayerPanel *panel = new PlayerPanel(chat);
    chat->addPanel(panel);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(player, PlayerPlugin)

