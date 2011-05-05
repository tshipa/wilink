/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#include "chat_history.h"
#include "chat_roster.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QMenu>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QStyle>
#include <QShortcut>
#include <QTextBlock>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>

#define AVATAR_WIDTH 40
#define BODY_OFFSET 5
#define BUBBLE_RADIUS 8
#define FOOTER_HEIGHT 5
#define MESSAGE_MAX 100

#ifdef WILINK_EMBEDDED
#define BODY_FONT 18
#define DATE_FONT 15
#define HEADER_HEIGHT 25
#else
#define BODY_FONT 12
#define DATE_FONT 10
#define HEADER_HEIGHT 20
#endif

typedef QPair<QRegExp, QString> TextTransform;
static QList<TextTransform> textTransforms;

static const QRegExp linkRegex = QRegExp("\\b((ftp|http|https)://[^ ]+)\\b");
static const QRegExp meRegex = QRegExp("^/me( .*)");

class ChatHistoryItem : public ChatModelItem
{
public:
    ChatMessage message;
};

/** Constructs a new ChatMessage.
 */
ChatMessage::ChatMessage()
    : archived(false), received(true)
{
}

/** Adds a transformation to be applied to message bodies.
 *
 * @param match
 * @param replacement
 */
void ChatMessage::addTransform(const QRegExp &match, const QString &replacement)
{
    textTransforms.append(qMakePair(match, replacement));
}

/** Returns true if the two messages should be grouped in the same bubble.
 *
 * @param other
 */
bool ChatMessage::groupWith(const ChatMessage &other) const
{
    return jid == other.jid &&
        !isAction() &&
        !other.isAction() &&
        qAbs(date.secsTo(other.date)) < 3600; // 1 hour
}

/** Returns true if the message is an "action" message, such
 *  as /me.
 */
bool ChatMessage::isAction() const
{
    return body.indexOf(meRegex) >= 0;
}

/** Constructs a new ChatHistoryBubble.
 *
 * @param parent
 */
ChatHistoryBubble::ChatHistoryBubble(ChatHistoryWidget *parent)
    : QGraphicsWidget(parent),
    m_history(parent)
{
    // avatar
    m_avatar = new QGraphicsPixmapItem(this);
    m_avatar->installSceneEventFilter(this);

    // from
    m_from = new QGraphicsTextItem(this);
    QFont font = m_from->font();
    font.setPixelSize(DATE_FONT);
    m_from->setFont(font);
    m_from->installSceneEventFilter(this);

    // date
    m_date = new QGraphicsTextItem(this);
    m_date->setFont(font);

    // bubble frame
    m_frame = new QGraphicsPathItem(this);
    m_frame->setZValue(-1);
    m_frame->hide();

    // body
    m_body = new QGraphicsTextItem(this);
    font.setPixelSize(BODY_FONT);
    m_body->setFont(font);
    m_body->installSceneEventFilter(this);

#if 0
    // bubble shadow
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(8);
    effect->setOffset(QPointF(2, 2));
    m_frame->setGraphicsEffect(effect);
#endif
}

/** Retrieves updated data from the model.
 */
void ChatHistoryBubble::dataChanged()
{
    QModelIndex idx = index();

    // empty bubble
    if (!m_history->model()->rowCount(idx)) {
        m_body->setHtml(QString());
        m_frame->hide();
        updateGeometry();
        return;
    }

    const bool isAction = idx.data(ChatHistoryModel::ActionRole).toBool();
    const bool isReceived = idx.data(ChatHistoryModel::ReceivedRole).toBool();
    const QString jid = idx.data(ChatHistoryModel::JidRole).toString();

    // avatar
    const QPixmap pixmap = m_history->model()->rosterModel()->contactAvatar(jid);
    if (pixmap.isNull())
        m_avatar->setPixmap(QPixmap());
    else
        m_avatar->setPixmap(pixmap.scaled(32, 32, Qt::KeepAspectRatio));

    QColor baseColor = isReceived ? QColor(0x26, 0x89, 0xd6) : QColor(0x7b, 0x7b, 0x7b);
    QColor backgroundColor = isReceived ? QColor(0xe7, 0xf4, 0xfe) : QColor(0xfa, 0xfa, 0xfa);

    // from
    m_from->setDefaultTextColor(baseColor);
    m_from->setPlainText(idx.data(ChatHistoryModel::FromRole).toString());
    m_from->show();

    // date
    m_date->setDefaultTextColor(baseColor);
    const QDateTime datetime = idx.data(ChatHistoryModel::DateRole).toDateTime().toLocalTime();
    m_date->setPlainText(datetime.date() == QDate::currentDate() ?
        datetime.toString("hh:mm") : datetime.toString("dd MMM hh:mm"));
    m_date->show();

    // bubble frame
    m_frame->setPen(baseColor);
    m_frame->setBrush(backgroundColor);
    if (isAction) {
        m_frame->hide();
    } else {
        m_frame->show();
    }

    // smileys
    QString text = idx.data(ChatHistoryModel::HtmlRole).toString();
    text.replace(":@", "<img src=\":/smiley-angry.png\" />");
    text.replace(":s", "<img src=\":/smiley-confused.png\" />");
    text.replace(":)", "<img src=\":/smiley-happy.png\" />");
    text.replace(":|", "<img src=\":/smiley-neutral.png\" />");
    text.replace(":(", "<img src=\":/smiley-sad.png\" />");

    // body
    m_body->setHtml(text);

    updateGeometry();
}

/** Returns the model index which this widget displays.
 */
QModelIndex ChatHistoryBubble::index() const
{
    int row = m_history->indexOf((ChatHistoryBubble*)this);
    return m_history->model()->index(row, 0, QModelIndex());
}

/** Returns the underlying data model.
 */
ChatHistoryModel *ChatHistoryBubble::model()
{
    return m_history->model();
}

/** Filters events on the sender and body labels.
 *
 * @param item
 * @param event
 */
bool ChatHistoryBubble::sceneEventFilter(QGraphicsItem *item, QEvent *event)
{
    if (item == m_avatar || item == m_from) {
        if (event->type() == QEvent::GraphicsSceneMousePress)
            emit messageClicked(index());
    } else if (item == m_body) {
        if (event->type() == QEvent::GraphicsSceneHoverLeave) {
            m_bodyAnchor = QString();
            m_body->setCursor(Qt::ArrowCursor);
        } else if (event->type() == QEvent::GraphicsSceneHoverMove) {
            QGraphicsSceneHoverEvent *hoverEvent = static_cast<QGraphicsSceneHoverEvent*>(event);
            m_bodyAnchor = m_body->document()->documentLayout()->anchorAt(hoverEvent->pos());
            m_body->setCursor(m_bodyAnchor.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
        } else if (event->type() == QEvent::GraphicsSceneMousePress) {
            QGraphicsSceneMouseEvent *mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (!m_bodyAnchor.isEmpty())
                    QDesktopServices::openUrl(m_bodyAnchor);
            }
        }
    }
    return false;
}

/** Sets the bubble's geometry.
 */
void ChatHistoryBubble::setGeometry(const QRectF &baseRect)
{
    QGraphicsWidget::setGeometry(baseRect);

    QRectF rect(baseRect);
    rect.moveLeft(0);
    rect.moveTop(0);

    // header
    rect.adjust(0.5 + AVATAR_WIDTH, 0.5, -0.5, -0.5);
    m_from->setPos(rect.left(), rect.top());
    m_date->setPos(rect.right() - (80 + m_date->document()->idealWidth())/2, rect.top());
    rect.adjust(0, HEADER_HEIGHT, 0, 0);

    // avatar
    m_avatar->setPos(0, rect.top());

    // bubble
    if (m_frame) {
        QRectF arc(0, 0, 2 * BUBBLE_RADIUS, 2 * BUBBLE_RADIUS);
        rect.adjust(0, 0, 0, -FOOTER_HEIGHT);

        // bubble top
        QPainterPath path;
        path.moveTo(rect.left(), rect.top() + BUBBLE_RADIUS);
        arc.moveTopLeft(rect.topLeft());
        path.arcTo(arc, 180, -90);
        path.lineTo(rect.left() + 20, rect.top());
        path.lineTo(rect.left() + 17, rect.top() - 5);
        path.lineTo(rect.left() + 27, rect.top());
        path.lineTo(rect.right() - BUBBLE_RADIUS, rect.top());
        arc.moveRight(rect.right());
        path.arcTo(arc, 90, -90);

        // bubble right, bottom, left
        path.lineTo(rect.right(), rect.bottom() - BUBBLE_RADIUS);
        arc.moveBottom(rect.bottom());
        path.arcTo(arc, 0, -90);
        path.lineTo(rect.left() + BUBBLE_RADIUS, rect.bottom());
        arc.moveLeft(rect.left());
        path.arcTo(arc, -90, -90);
        path.closeSubpath();
        m_frame->setPath(path);
    }

    // messages
    rect.adjust(0.5, 0.5, -0.5, -0.5);
    m_body->setPos(rect.left() + BODY_OFFSET, rect.top());
}

/** Sets the bubble's maximum width.
 *
 * @param widget
 */
void ChatHistoryBubble::setMaximumWidth(qreal width)
{
    m_maximumWidth = width;
    QGraphicsWidget::setMaximumWidth(width);
    m_body->document()->setTextWidth(m_maximumWidth - BODY_OFFSET - AVATAR_WIDTH - 2);
    updateGeometry();
}

/** Returns a size hint for the bubble.
 *
 * @param which
 * @param constraint
 */
QSizeF ChatHistoryBubble::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    switch (which)
    {
        case Qt::MinimumSize:
        case Qt::PreferredSize:
        {
            qreal height = HEADER_HEIGHT + (m_frame ? FOOTER_HEIGHT : 0) + 2;
            height += m_body->document()->size().height();
            return QSizeF(m_maximumWidth, height);
        }
        default:
            return constraint;
    }
}

/** Returns the bubble's main text item.
 */
QGraphicsTextItem *ChatHistoryBubble::textItem()
{
    return m_body;
}

/** Break a text selection into a list of rectangles, which one rectangle per line.
 *
 * @param textItem
 * @param cursor
 */
static QList<RectCursor> chunkSelection(QGraphicsTextItem *textItem, const QTextCursor &cursor)
{
    QList<RectCursor> rectangles;

    const QTextLayout *layout = cursor.block().layout();
    const QPointF blockOffset = layout->position();
    const int startPos = cursor.anchor() - cursor.block().position();
    const int endPos = cursor.position() - cursor.block().position();
    for (int i = 0; i < layout->lineCount(); ++i)
    {
        QTextLine line = layout->lineAt(i);
        const int lineEnd = line.textStart() + line.textLength();

        // skip lines before selection
        if (lineEnd <= startPos)
            continue;

        QRectF localRect = line.rect();
        QTextCursor localCursor = cursor;

        // position left edge
        if (line.textStart() < startPos)
        {
            QPointF topLeft = line.rect().topLeft();
            topLeft.setX(topLeft.x() + line.cursorToX(startPos));
            localRect.setTopLeft(topLeft);
            localCursor.setPosition(cursor.anchor());
        } else {
            localCursor.setPosition(line.textStart() + cursor.block().position());
        }

        // position right edge
        QPointF bottomRight = line.rect().bottomLeft();
        if (lineEnd > endPos)
        {
            bottomRight.setX(bottomRight.x() + line.cursorToX(endPos));
            localCursor.setPosition(cursor.position(), QTextCursor::KeepAnchor);
        } else {
            bottomRight.setX(bottomRight.x() + line.cursorToX(lineEnd));
            localCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        }
        localRect.setBottomRight(bottomRight);

        // map to scene coordinates
        rectangles << qMakePair(textItem->mapRectToScene(localRect.translated(blockOffset)), localCursor);

        if (lineEnd > endPos)
            break;
    }

    return rectangles;
}

/** Sets the text cursor to select the given rectangle.
 *
 * @param textItem
 * @param rect
 */
static void setSelection(QGraphicsTextItem *textItem, const QRectF &rect)
{
    QRectF localRect = textItem->boundingRect().intersected(textItem->mapRectFromScene(rect));

    // determine selected text
    QAbstractTextDocumentLayout *layout = textItem->document()->documentLayout();
    int startPos = layout->hitTest(localRect.topLeft(), Qt::FuzzyHit);
    int endPos = layout->hitTest(localRect.bottomRight(), Qt::FuzzyHit);

    // set cursor
    QTextCursor cursor(textItem->document());
    cursor.setPosition(startPos);
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    textItem->setTextCursor(cursor);
}

/** Constructs a new ChatHistoryHelper.
 *
 * @param parent
 */
ChatHistoryHelper::ChatHistoryHelper(QObject *parent)
    : QObject(parent)
{
}

/** Opens the given \a url using QDesktopServices.
 *
 * @param url
 */
void ChatHistoryHelper::openUrl(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

class ChatHistoryModelPrivate
{
public:
    ChatHistoryModelPrivate(ChatHistoryModel *qq);
    QString html(ChatHistoryItem *item) const;
    void rosterChanged(const QString &jid);

    ChatRosterModel *rosterModel;

private:
    ChatHistoryModel *q;
};

ChatHistoryModelPrivate::ChatHistoryModelPrivate(ChatHistoryModel *qq)
    : rosterModel(0),
    q(qq)
{
}

/** Returns the HTML for the message body.
 */
QString ChatHistoryModelPrivate::html(ChatHistoryItem *item) const
{
    QString bodyHtml = Qt::escape(item->message.body);
    bodyHtml.replace("\n", "<br/>");
    bodyHtml.replace(linkRegex, "<a href=\"\\1\">\\1</a>");
    if (rosterModel)
        bodyHtml.replace(meRegex, "<b>" + rosterModel->contactName(item->message.jid) + "\\1</b>");
    foreach (const TextTransform &transform, textTransforms)
        bodyHtml.replace(transform.first, transform.second);
    return bodyHtml;
}

/** Handles a roster update.
 *
 * @param jid
 */
void ChatHistoryModelPrivate::rosterChanged(const QString &jid)
{
    foreach (ChatModelItem *it, q->rootItem->children) {
        if (it->children.isEmpty())
            continue;
        ChatHistoryItem *item = static_cast<ChatHistoryItem*>(it->children.first());
        if (item->message.jid == jid)
            emit q->dataChanged(q->createIndex(it), q->createIndex(it));
    }
}

/** Constructs a new ChatHistoryModel.
 *
 * @param parent
 */
ChatHistoryModel::ChatHistoryModel(QObject *parent)
    : ChatModel(parent)
{
    d = new ChatHistoryModelPrivate(this);

    rootItem = new ChatHistoryItem;

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(ActionRole, "action");
    roleNames.insert(AvatarRole, "avatar");
    roleNames.insert(BodyRole, "body");
    roleNames.insert(DateRole, "date");
    roleNames.insert(FromRole, "from");
    roleNames.insert(HtmlRole, "html");
    roleNames.insert(JidRole, "jid");
    roleNames.insert(ReceivedRole, "received");
    setRoleNames(roleNames);
}

/** Adds a message in the chat history.
 *
 * The message is checked for duplicates, inserted in chronological order
 * and grouped with related messages.
 *
 * @param message
 */
void ChatHistoryModel::addMessage(const ChatMessage &message)
{
    if (message.body.isEmpty())
        return;

    // position cursor
    ChatHistoryItem *prevMsg = 0;
    ChatHistoryItem *nextMsg = 0;
    foreach (ChatModelItem *bubble, rootItem->children) {
        foreach (ChatModelItem *childPtr, bubble->children) {
            ChatHistoryItem *child = static_cast<ChatHistoryItem*>(childPtr);
            // check for collision
            if (message.archived != child->message.archived &&
                message.jid == child->message.jid &&
                message.body == child->message.body &&
                qAbs(message.date.secsTo(child->message.date)) < 10)
                return;

            // we use greater or equal comparison (and not strictly greater) dates
            // because messages are usually received in chronological order
            if (message.date >= child->message.date) {
                prevMsg = child;
            } else if (!nextMsg) {
                nextMsg = child;
            }
        }
    }

    // prepare message
    ChatHistoryItem *msg = new ChatHistoryItem;
    msg->message = message;

    if (prevMsg && prevMsg->message.groupWith(message) &&
        nextMsg && nextMsg->message.groupWith(message) &&
        prevMsg->parent != nextMsg->parent) {
        ChatModelItem *prevBubble = prevMsg->parent;
        ChatModelItem *nextBubble = nextMsg->parent;
        int row = prevMsg->row() + 1;
        addItem(msg, prevBubble, row++);

        // message belongs both to the previous and next bubble, merge
        const int lastRow = nextBubble->children.size() - 1;
        beginMoveRows(createIndex(nextBubble), 0, lastRow,
                      createIndex(prevBubble), row);
        for (int i = lastRow; i >= 0; --i) {
            ChatModelItem *item = nextBubble->children.takeAt(i);
            item->parent = prevBubble;
            prevBubble->children.insert(row, item);
        }
        endMoveRows();
        emit dataChanged(createIndex(prevBubble), createIndex(prevBubble));
        removeRow(nextBubble->row());
    }
    else if (prevMsg && prevMsg->message.groupWith(message))
    {
        // message belongs to the same bubble as previous message
        ChatModelItem *bubble = prevMsg->parent;
        addItem(msg, bubble, prevMsg->row() + 1);
        emit dataChanged(createIndex(bubble), createIndex(bubble));
    }
    else if (nextMsg && nextMsg->message.groupWith(message))
    {
        // message belongs to the same bubble as next message
        ChatModelItem *bubble = nextMsg->parent;
        addItem(msg, bubble, nextMsg->row());
        emit dataChanged(createIndex(bubble), createIndex(bubble));
    }
    else
    {
        // split the previous bubble if needed
        int bubblePos = 0;
        if (prevMsg)
        {
            ChatModelItem *prevBubble = prevMsg->parent;
            bubblePos = prevBubble->row() + 1;

            // if the previous message is not the last in its bubble, split
            const int index = prevMsg->row();
            const int lastRow = prevBubble->children.size() - 1;
            if (index < lastRow) {
                ChatHistoryItem *bubble = new ChatHistoryItem;
                addItem(bubble, rootItem, bubblePos);

                const int firstRow = index + 1;
                beginMoveRows(createIndex(prevBubble), firstRow, lastRow,
                              createIndex(bubble), 0);
                for (int i = lastRow; i >= firstRow; --i) {
                    ChatModelItem *item = prevBubble->children.takeAt(i);
                    item->parent = bubble;
                    bubble->children.insert(0, item);
                }
                endMoveRows();
                emit dataChanged(createIndex(prevBubble), createIndex(prevBubble));
            }
        }

        // insert the new bubble
        ChatHistoryItem *bubble = new ChatHistoryItem;
        addItem(bubble, rootItem, bubblePos);
        addItem(msg, bubble);
        emit dataChanged(createIndex(bubble), createIndex(bubble));
    }
}

/** Clears all messages.
 */
void ChatHistoryModel::clear()
{
    int rows = rowCount(QModelIndex());
    if (rows > 0)
        removeRows(0, rows);
}

/** Returns the number of columns for the children of the given parent.
 *
 * @param parent
 */
int ChatHistoryModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

/** Returns the data stored under the given \a role for the item referred to
 *  by the \a index.
 *
 * @param index
 * @param role
 */
QVariant ChatHistoryModel::data(const QModelIndex &index, int role) const
{
    ChatHistoryItem *item = static_cast<ChatHistoryItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    ChatHistoryItem *msg = static_cast<ChatHistoryItem*>(item->children.isEmpty() ? item : item->children.first());
    if (role == ActionRole) {
        return msg->message.isAction();
    } else if (role == AvatarRole) {
        if (msg->message.jid.isEmpty())
            return QUrl("qrc://peer.png");
        return QUrl("image://roster/" + msg->message.jid);
    } else if (role == BodyRole) {
        if (item->children.isEmpty())
            return item->message.body;
        else {
            QStringList bodies;
            foreach (ChatModelItem *ptr, item->children) {
                ChatHistoryItem *child = static_cast<ChatHistoryItem*>(ptr);
                bodies << child->message.body;
            }
            return bodies.join("\n");
        }
    } else if (role == DateRole) {
        return msg->message.date;
    } else if (role == FromRole) {
        if (!d->rosterModel)
            return QVariant();
        return d->rosterModel->contactName(msg->message.jid);
    } else if (role == HtmlRole) {
        if (item->children.isEmpty())
            return d->html(item);
        else {
            QString bodies;
            foreach (ChatModelItem *ptr, item->children) {
                ChatHistoryItem *child = static_cast<ChatHistoryItem*>(ptr);
                bodies += "<p style=\"margin-top: 0; margin-bottom: 2\">" + d->html(child) + "</p>";
            }
            return bodies;
        }
    } else if (role == JidRole) {
        return msg->message.jid;
    } else if (role == ReceivedRole) {
        return msg->message.received;
    }

    return QVariant();
}

/** Handles a roster change.
 *
 * @param topLeft
 * @param bottomRight
 */
void ChatHistoryModel::rosterChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_ASSERT(topLeft.parent() == bottomRight.parent());
    const QModelIndex parent = topLeft.parent();
    for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
        const QString jid = d->rosterModel->index(i, 0, parent).data(ChatRosterModel::IdRole).toString();
        d->rosterChanged(jid);
    }
}

/** Handles a roster insertion.
 *
 * @param parent
 * @param start
 * @param end
 */
void ChatHistoryModel::rosterInserted(const QModelIndex &parent, int start, int end)
{
    for (int i = start; i <= end; ++i) {
        const QString jid = d->rosterModel->index(i, 0, parent).data(ChatRosterModel::IdRole).toString();
        d->rosterChanged(jid);
    }
}

/** Returns the roster model.
 */
ChatRosterModel *ChatHistoryModel::rosterModel()
{
    return d->rosterModel;
}

/** Sets the roster model.
 *
 * @param rosterModel
 */
void ChatHistoryModel::setRosterModel(ChatRosterModel *rosterModel)
{
    d->rosterModel = rosterModel;
    connect(d->rosterModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(rosterChanged(QModelIndex,QModelIndex)));
    connect(d->rosterModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(rosterInserted(QModelIndex,int,int)));
}

/** Constructs a new ChatHistoryWidget.
 *
 * @param parent
 */
ChatHistoryWidget::ChatHistoryWidget(QGraphicsItem *parent)
    : QGraphicsWidget(parent),
    m_followEnd(true),
    m_view(0),
    m_lastFindWidget(0)
{
    m_layout = new QGraphicsLinearLayout(Qt::Vertical);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    setLayout(m_layout);

    m_trippleClickTimer = new QTimer(this);
    m_trippleClickTimer->setSingleShot(true);
    m_trippleClickTimer->setInterval(QApplication::doubleClickInterval());
}

/** Resizes the ChatHistoryWidget to its preferred size hint.
 */
void ChatHistoryWidget::adjustSize()
{
    QGraphicsWidget::adjustSize();

    // reposition search bubbles
    if (!m_glassItems.isEmpty() && m_lastFindWidget)
    {
        findClear();
        foreach (RectCursor textRect, chunkSelection(m_lastFindWidget, m_lastFindCursor)) {
            ChatSearchBubble *glass = new ChatSearchBubble;
            glass->setSelection(textRect);
            scene()->addItem(glass);
            m_glassItems << glass;
        }
    }

    // adjust viewed rectangle
    if (m_bubbles.isEmpty())
        m_view->setSceneRect(0, 0, m_maximumWidth, 50);
    else
        m_view->setSceneRect(boundingRect());

    // scroll to end
    QScrollBar *scrollBar = m_view->verticalScrollBar();
    if (m_followEnd && scrollBar->value() < scrollBar->maximum())
        scrollBar->setSliderPosition(scrollBar->maximum());
}

/** Clears all messages.
 */
void ChatHistoryWidget::clear()
{
    m_model->clear();
}

/** Copies the selected text to the clipboard.
 */
void ChatHistoryWidget::copy()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(selectedText(), QClipboard::Clipboard);
}

/** When the viewport is resized, adjust the history widget's width.
 *
 * @param watched
 * @param event
 */
bool ChatHistoryWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view->viewport() && event->type() == QEvent::Resize)
    {
        m_maximumWidth = m_view->viewport()->width() - 15;
        foreach (ChatHistoryBubble *bubble, m_bubbles)
            bubble->setMaximumWidth(m_maximumWidth);
        adjustSize();
    }
    return false;
}

/** Find and highlight the given text.
 *
 * @param needle
 * @param flags
 * @param changed
 */
void ChatHistoryWidget::find(const QString &needle, QTextDocument::FindFlags flags, bool changed)
{
    // clear search bubbles
    findClear();

    // handle empty search
    if (needle.isEmpty())
    {
        emit findFinished(true);
        return;
    }

    // retrieve previous cursor
    QList<QGraphicsTextItem*> m_messages;
    foreach (ChatHistoryBubble *bubble, m_bubbles)
        m_messages << bubble->textItem();

    QTextCursor cursor;
    int startIndex = (flags && QTextDocument::FindBackward) ? m_messages.size() -1 : 0;
    if (m_lastFindWidget)
    {
        for (int i = 0; i < m_messages.size(); ++i)
        {
            if (m_messages[i] == m_lastFindWidget)
            {
                startIndex = i;
                cursor = m_lastFindCursor;
                break;
            }
        }
    }
    if (changed)
        cursor.setPosition(cursor.anchor());

    // perform search
    bool looped = false;
    int i = startIndex;
    while (i >= 0 && i < m_messages.size())
    {
        QGraphicsTextItem *textItem = m_messages[i];
        QTextDocument *document = textItem->document();

        // position cursor
        if (cursor.isNull())
        {
            cursor = QTextCursor(document);
            if (flags && QTextDocument::FindBackward)
                cursor.movePosition(QTextCursor::End);
        }

        cursor = document->find(needle, cursor, flags);
        if (!cursor.isNull())
        {
            // build new glass
            QRectF boundingRect;
            foreach (const RectCursor &textRect, chunkSelection(textItem, cursor))
            {
                ChatSearchBubble *glass = new ChatSearchBubble;
                glass->setSelection(textRect);
                glass->bounce();
                scene()->addItem(glass);
                m_glassItems << glass;

                if (boundingRect.isEmpty())
                    boundingRect = glass->boundingRect();
                else
                    boundingRect = boundingRect.united(glass->boundingRect());
            }
            ensureVisible(boundingRect);

            m_lastFindCursor = cursor;
            m_lastFindWidget = textItem;
            emit findFinished(true);
            return;
        } else {
            if (looped)
                break;
            if (flags && QTextDocument::FindBackward) {
                if (--i < 0)
                    i = m_messages.size() - 1;
            } else {
                if (++i >= m_messages.size())
                    i = 0;
            }
            if (i == startIndex)
                looped = true;
        }
    }

    m_lastFindWidget = 0;
    m_lastFindCursor = QTextCursor();
    emit findFinished(false);
}

/** Clear all the search bubbles.
 */
void ChatHistoryWidget::findClear()
{
    foreach (ChatSearchBubble *item, m_glassItems)
    {
        scene()->removeItem(item);
        delete item;
    }
    m_glassItems.clear();
}

/** Returns the row of the given history bubble.
 *
 * @param bubble
 */
int ChatHistoryWidget::indexOf(ChatHistoryBubble *bubble)
{
    return m_bubbles.indexOf(bubble);
}

/** Returns the text item at the given scene position.
 *
 * @param pos
 */
QGraphicsTextItem *ChatHistoryWidget::textItemAt(const QPointF &pos) const
{
    QGraphicsItem *hit = scene()->itemAt(pos);
    while (hit) {
        ChatHistoryBubble *widget = static_cast<ChatHistoryBubble*>(hit);
        if (m_bubbles.contains(widget))
            return widget->textItem();
        hit = hit->parentItem();
    }
    return 0;
}

/** Returns the underlying data model.
 */
ChatHistoryModel *ChatHistoryWidget::model()
{
    return m_model;
}

/** Sets the underlying data model.
 *
 * @param model
 */
void ChatHistoryWidget::setModel(ChatHistoryModel *model)
{
    bool check;

    m_model = model;
    check = connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                    this, SLOT(rowsChanged(QModelIndex,QModelIndex)));
    Q_ASSERT(check);
    check = connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                    this, SLOT(rowsInserted(QModelIndex,int,int)));
    Q_ASSERT(check);
    check = connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                    this, SLOT(rowsRemoved(QModelIndex,int,int)));
    Q_ASSERT(check);
}

/** Handles mouse double click events to update text selection.
 *
 * @param event
 */
void ChatHistoryWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsTextItem *textItem = textItemAt(event->pos());
    if (event->buttons() != Qt::LeftButton || !textItem)
        return;

    QAbstractTextDocumentLayout *layout = textItem->document()->documentLayout();
    int cursorPos = layout->hitTest(textItem->mapFromScene(event->pos()), Qt::FuzzyHit);
    if (cursorPos != -1)
    {
        QTextCursor cursor = textItem->textCursor();
        cursor.setPosition(cursorPos);
        cursor.select(QTextCursor::WordUnderCursor);
        textItem->setTextCursor(cursor);

        // update selection
        foreach (QGraphicsTextItem *child, m_selectedMessages)
            if (child != textItem)
                setSelection(child, QRectF());
        m_selectedMessages.clear();
        m_selectedMessages << textItem;
        QApplication::clipboard()->setText(selectedText(), QClipboard::Selection);

        m_trippleClickTimer->start();
        event->accept();
    }
}

/** Handles mouse move events to update text selection.
 *
 * @param event
 */
void ChatHistoryWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton)
    {
        const QRectF rect(m_selectionStart, event->scenePos());

        // update the selected items
        QList<QGraphicsTextItem*> newSelection;
        foreach (ChatHistoryBubble *bubble, m_bubbles) {
            QGraphicsTextItem *textItem = bubble->textItem();
            if (textItem->boundingRect().intersects(textItem->mapRectFromScene(rect))) {
                newSelection << textItem;
                setSelection(textItem, rect);
            } else if (m_selectedMessages.contains(textItem)) {
                setSelection(textItem, QRectF());
            }
        }
        m_selectedMessages = newSelection;

        // for X11, copy the selected text to the selection buffer
        if (!m_selectedMessages.isEmpty())
            QApplication::clipboard()->setText(selectedText(), QClipboard::Selection);

        event->accept();
    }
}

/** Handles mouse click and tripple click events to update text selection.
 *
 * @param event
 */
void ChatHistoryWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() != Qt::LeftButton)
        return;

    // handle tripple click
    QGraphicsTextItem *textItem = textItemAt(event->pos());
    if (textItem && m_trippleClickTimer->isActive())
    {
        QTextCursor cursor = textItem->textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        textItem->setTextCursor(cursor);

        // update selection
        foreach (QGraphicsTextItem *child, m_selectedMessages)
            if (child != textItem)
                setSelection(child, QRectF());
        m_selectedMessages.clear();
        m_selectedMessages << textItem;
        QApplication::clipboard()->setText(selectedText(), QClipboard::Selection);
    } else {
        m_selectionStart = event->scenePos();
        foreach (QGraphicsTextItem *child, m_selectedMessages)
            setSelection(child, QRectF());
        m_selectedMessages.clear();
    }
    event->accept();
}

void ChatHistoryWidget::rowsChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_ASSERT(topLeft.parent() == bottomRight.parent());
    if (!topLeft.parent().isValid()) {
        //qDebug("bubbles changed: %i-%i", topLeft.row(), bottomRight.row());
        for (int i = topLeft.row(); i <= bottomRight.row(); ++i)
            m_bubbles.at(i)->dataChanged();
        adjustSize();
    }
}

void ChatHistoryWidget::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if (!parent.isValid()) {
        bool check;
        //qDebug("bubbles inserted: %i-%i", start, end);
        for (int i = start; i <= end; ++i) {
            ChatHistoryBubble *bubble = new ChatHistoryBubble(this);
            bubble->setMaximumWidth(m_maximumWidth);
            m_bubbles.insert(i, bubble);
            m_layout->insertItem(i, bubble);

            check = connect(bubble, SIGNAL(messageClicked(QModelIndex)),
                            this, SIGNAL(messageClicked(QModelIndex)));
            Q_ASSERT(check);
        }
        adjustSize();
    }
}

void ChatHistoryWidget::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    if (!parent.isValid()) {
        //qDebug("bubbles removed: %i-%i", start, end);
        for (int i = end; i >= start; --i) {
            ChatHistoryBubble *bubble = m_bubbles.takeAt(i);
            bubble->deleteLater();
        }
        adjustSize();
    }
}

/** Selects all the messages.
 */
void ChatHistoryWidget::selectAll()
{
    const QRectF rect = scene()->sceneRect();

    m_selectedMessages.clear();
    foreach (ChatHistoryBubble *bubble, m_bubbles) {
        QGraphicsTextItem *child = bubble->textItem();
        setSelection(child, rect);
        m_selectedMessages << child;
    }
}

/** Retrieves the selected text.
 */
QString ChatHistoryWidget::selectedText() const
{
    QString copyText;

    // gather the message senders
    QMap<QGraphicsTextItem*, QString> senders;
    foreach (QGraphicsTextItem *textItem, m_selectedMessages) {
        // FIXME : cannot be reused in QML
        ChatHistoryBubble *bubble = static_cast<ChatHistoryBubble*>(textItem->parentItem());
        senders.insert(textItem, bubble->index().data(ChatHistoryModel::FromRole).toString());
    }

    // copy selected messages
    foreach (QGraphicsTextItem *textItem, m_selectedMessages) {
        if (!copyText.isEmpty())
            copyText += "\n";

        // if this is a conversation, prefix the message with its sender
        if (senders.size() > 1)
            copyText += senders.value(textItem) + "> ";

        copyText += textItem->textCursor().selectedText().replace("\r\n", "\n");
    }

#ifdef Q_OS_WIN
    return copyText.replace("\n", "\r\n");
#else
    return copyText;
#endif
}

/** Sets the \a view which holds this history widget.
 *
 * This is used to monitor scroll events and setup keyboard
 * shortcuts.
 *
 * @param view
 */
void ChatHistoryWidget::setView(QGraphicsView *view)
{
    bool check;
    m_view = view;

    /* set up hooks */
    m_view->viewport()->installEventFilter(this);

    check = connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)),
                    this, SLOT(slotScrollChanged()));
    Q_ASSERT(check);

    /* set up keyboard shortcuts */
    QAction *action = new QAction(QIcon::fromTheme("edit-copy"), tr("&Copy"), this);
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_C));
    check = connect(action, SIGNAL(triggered(bool)),
                    this, SLOT(copy()));
    Q_ASSERT(check);
    m_view->addAction(action);

    action = new QAction(QIcon::fromTheme("edit-select-all"), tr("Select &All"), this);
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_A));
    check = connect(action, SIGNAL(triggered(bool)),
                    this, SLOT(selectAll()));
    Q_ASSERT(check);
    m_view->addAction(action);

    action = new QAction(QIcon::fromTheme("edit-clear"), tr("Clear"), this);
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_X));
    check = connect(action, SIGNAL(triggered(bool)),
                    this, SLOT(clear()));
    Q_ASSERT(check);
    m_view->addAction(action);

    // initialise size
    adjustSize();
}

/** When the scroll value changes, remember whether we were at the end
 *  of the view.
 */
void ChatHistoryWidget::slotScrollChanged()
{
    QScrollBar *scrollBar = m_view->verticalScrollBar();
    m_followEnd = scrollBar->value() >= scrollBar->maximum() - 16;
}

/** Constructs a new ChatSearchBubble.
 */
ChatSearchBubble::ChatSearchBubble()
    : m_margin(3), m_radius(4)
{
    // shadow
    shadow = new QGraphicsPathItem;
    QColor shadowColor(0x99, 0x99, 0x99);
    QLinearGradient shadowGradient(QPointF(0, 0), QPointF(0, 1));
    shadowGradient.setColorAt(0, shadowColor);
    shadowColor.setAlpha(0xff);
    shadowGradient.setColorAt(.95, shadowColor);
    shadowColor.setAlpha(0x0);
    shadowGradient.setColorAt(1, shadowColor);
    shadowGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    shadowGradient.setSpread(QGradient::PadSpread);
    shadow->setPen(Qt::NoPen);
    shadow->setBrush(shadowGradient);
    addToGroup(shadow);

    // bubble
    bubble = new QGraphicsPathItem;
    bubble->setPen(QColor(63, 63, 63, 95));
    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0, QColor(255, 255, 0, 255));
    gradient.setColorAt(1, QColor(228, 212, 0, 255));
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setSpread(QGradient::PadSpread);
    bubble->setBrush(gradient);
    addToGroup(bubble);

    // text
    text = new QGraphicsTextItem;
    QFont font = text->font();
    font.setPixelSize(BODY_FONT);
    text->setFont(font);
    addToGroup(text);

    setZValue(1);
}

void ChatSearchBubble::bounce()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "scale");
    animation->setDuration(250);
    animation->setStartValue(1);
    animation->setKeyValueAt(0.5, 1.5);
    animation->setEndValue(1);
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QRectF ChatSearchBubble::boundingRect() const
{
    return bubble->boundingRect();
}

void ChatSearchBubble::setSelection(const RectCursor &selection)
{
    m_selection = selection;

    setTransformOriginPoint(m_selection.first.center());

    QPointF textPos = m_selection.first.topLeft();
    textPos.setX(textPos.x() - text->document()->documentMargin());
    textPos.setY(textPos.y() - text->document()->documentMargin());
    text->setPos(textPos);
    text->setPlainText(m_selection.second.selectedText());

    QRectF glassRect;
    glassRect.setX(m_selection.first.x() - m_margin);
    glassRect.setY(m_selection.first.y() - m_margin);
    glassRect.setWidth(m_selection.first.width() + 2 * m_margin);
    glassRect.setHeight(m_selection.first.height() + 2 * m_margin);

    QPainterPath path;
    path.addRoundedRect(glassRect, m_radius, m_radius);
    bubble->setPath(path);

    QPainterPath shadowPath;
    glassRect.moveTop(glassRect.top() + 2);
    shadowPath.addRoundedRect(glassRect, m_radius, m_radius);
    shadow->setPath(shadowPath);
}
