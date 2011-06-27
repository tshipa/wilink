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

#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QDeclarativeItem>
#include <QFile>
#include <QHostInfo>
#include <QImage>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QTimer>

#include "QXmppCallManager.h"
#include "QXmppClient.h"
#include "QXmppJingleIq.h"
#include "QXmppRtpChannel.h"
#include "QXmppSrvInfo.h"
#include "QXmppUtils.h"

#include "QSoundMeter.h"
#include "QSoundPlayer.h"
#include "QVideoGrabber.h"

#include "application.h"
#include "calls.h"

static QAudioFormat formatFor(const QXmppJinglePayloadType &type)
{
    QAudioFormat format;
    format.setFrequency(type.clockrate());
    format.setChannels(type.channels());
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    return format;
}

void DeclarativePen::setColor(const QColor &c)
{
    _color = c;
    _valid = (_color.alpha() && _width >= 1) ? true : false;
    emit penChanged();
}

void DeclarativePen::setWidth(int w)
{
    if (_width == w && _valid)
        return;

    _width = w;
    _valid = (_color.alpha() && _width >= 1) ? true : false;
    emit penChanged();
}

CallAudioHelperPrivate::CallAudioHelperPrivate(CallAudioHelper *qq)
    : QObject(0),
    m_audioInput(0),
    m_audioInputMeter(0),
    m_audioOutput(0),
    m_audioOutputMeter(0),
    q(qq)
{
}

void CallAudioHelperPrivate::audioModeChanged(QIODevice::OpenMode mode)
{
    bool check;

    //qDebug("audio mode changed %i", (int)mode);
    Q_ASSERT(m_channel);

    QAudioFormat format = formatFor(m_channel->payloadType());

#ifdef Q_OS_MAC
    // 128ms at 8kHz
    const int bufferSize = 2048 * format.channels();
#else
    // 160ms at 8kHz
    const int bufferSize = 2560 * format.channels();
#endif

    // start or stop playback
    const bool canRead = (mode & QIODevice::ReadOnly);
    if (canRead && !m_audioOutput) {
        m_audioOutput = new QAudioOutput(wApp->audioOutputDevice(), format, this);
        check = connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)),
                        this, SLOT(audioStateChanged()));
        Q_ASSERT(check);

        m_audioOutputMeter = new QSoundMeter(format, m_channel, this);
        check = connect(m_audioOutputMeter, SIGNAL(valueChanged(int)),
                        q, SIGNAL(outputVolumeChanged(int)));
        Q_ASSERT(check);

        m_audioOutput->setBufferSize(bufferSize);
        m_audioOutput->start(m_audioOutputMeter);
    } else if (!canRead && m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = 0;
        delete m_audioOutputMeter;
        m_audioOutputMeter = 0;

        q->outputVolumeChanged(0);
    }

    // start or stop capture
    const bool canWrite = (mode & QIODevice::WriteOnly);
    if (canWrite && !m_audioInput) {
        m_audioInput = new QAudioInput(wApp->audioInputDevice(), format, this);
        check = connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)),
                        this, SLOT(audioStateChanged()));
        Q_ASSERT(check);

        m_audioInputMeter = new QSoundMeter(format, m_channel, this);
        check = connect(m_audioInputMeter, SIGNAL(valueChanged(int)),
                        q, SIGNAL(inputVolumeChanged(int)));
        Q_ASSERT(check);

        m_audioInput->setBufferSize(bufferSize);
        m_audioInput->start(m_audioInputMeter);
    } else if (!canWrite && m_audioInput) {
        m_audioInput->stop();
        delete m_audioInput;
        m_audioInput = 0;
        delete m_audioInputMeter;
        m_audioInputMeter = 0;

        q->inputVolumeChanged(0);
    }
}

void CallAudioHelperPrivate::audioStateChanged()
{
    QObject *audio = sender();
    if (!audio)
        return;
    else if (audio == m_audioInput) {
#if 0
        qDebug("Audio input state %i error %i",
            m_audioInput->state(),
            m_audioInput->error());
#endif

        // Restart audio input if we get an underrun.
        //
        // NOTE: seen on Mac OS X 10.6
        if (m_audioInput->state() == QAudio::IdleState &&
            m_audioInput->error() == QAudio::UnderrunError) {
            qWarning("Audio input needs restart due to buffer underrun");
            m_audioInput->start(m_audioInputMeter);
        }
    }
    else if (audio == m_audioOutput) {
#if 0
        qDebug("Audio output state %i error %i",
            m_audioOutput->state(),
            m_audioOutput->error());
#endif

        // Restart audio output if we get an underrun.
        //
        // NOTE: seen on Linux with pulseaudio
        if (m_audioOutput->state() == QAudio::IdleState &&
            m_audioOutput->error() == QAudio::UnderrunError) {
            qWarning("Audio output needs restart due to buffer underrun");
            m_audioOutput->start(m_audioOutputMeter);
        }
    }
}

CallAudioHelper::CallAudioHelper(QObject *parent)
    : QObject(parent),
    m_call(0)
{
    d = new CallAudioHelperPrivate(this);
    d->moveToThread(wApp->soundThread());
}

CallAudioHelper::~CallAudioHelper()
{
    if (m_call) {
        m_call->hangup();
        m_call->deleteLater();
    }
}

QXmppCall* CallAudioHelper::call() const
{
    return m_call;
}

void CallAudioHelper::setCall(QXmppCall *call)
{
    if (call != m_call) {
        // disconnect old signals
        if (m_call)
            m_call->disconnect(this);

        // connect new signals
        if (call) {
            bool check;

            d->m_channel = call->audioChannel();
            d->m_channel->setParent(0);
            d->m_channel->moveToThread(wApp->soundThread());

            check = connect(call, SIGNAL(audioModeChanged(QIODevice::OpenMode)),
                            d, SLOT(audioModeChanged(QIODevice::OpenMode)));
            Q_ASSERT(check);
            Q_UNUSED(check);
        }

        // change call
        m_call = call;
        emit callChanged(call);
    }
}

int CallAudioHelper::inputVolume() const
{
    return d->m_audioInputMeter ? d->m_audioInputMeter->value() : 0;
}

int CallAudioHelper::maximumVolume() const
{
    return QSoundMeter::maximum();
}

int CallAudioHelper::outputVolume() const
{
    return d->m_audioOutputMeter ? d->m_audioOutputMeter->value() : 0;
}

CallVideoHelper::CallVideoHelper(QObject *parent)
    : QObject(parent),
    m_call(0),
    m_videoConversion(0),
    m_videoGrabber(0),
    m_videoMonitor(0),
    m_videoOutput(0)
{
    bool check;
    m_videoTimer = new QTimer(this);
    check = connect(m_videoTimer, SIGNAL(timeout()),
                    this, SLOT(videoRefresh()));
    Q_ASSERT(check);
}

QXmppCall* CallVideoHelper::call() const
{
    return m_call;
}

void CallVideoHelper::setCall(QXmppCall *call)
{
    if (call != m_call) {
        m_call = call;

        bool check;
        check = connect(call, SIGNAL(videoModeChanged(QIODevice::OpenMode)),
                        this, SLOT(videoModeChanged(QIODevice::OpenMode)));
        Q_ASSERT(check);

        emit callChanged(call);
        emit openModeChanged();
    }
}

CallVideoHelper::OpenMode CallVideoHelper::openMode() const
{
    OpenMode mode = NotOpen;
    if (m_call && (m_call->videoMode() & QIODevice::ReadOnly))
        mode |= ReadOnly;
    if (m_call && (m_call->videoMode() & QIODevice::WriteOnly))
        mode |= WriteOnly;
    return mode;
}

CallVideoItem *CallVideoHelper::output() const
{
    return m_videoOutput;
}

void CallVideoHelper::setOutput(CallVideoItem *output)
{
    if (output != m_videoOutput) {
        m_videoOutput = output;
        emit outputChanged(output);
    }
}

CallVideoItem *CallVideoHelper::monitor() const
{
    return m_videoMonitor;
}

void CallVideoHelper::setMonitor(CallVideoItem *monitor)
{
    if (monitor != m_videoMonitor) {
        m_videoMonitor = monitor;
        emit monitorChanged(monitor);
    }
}

void CallVideoHelper::videoModeChanged(QIODevice::OpenMode mode)
{
    Q_ASSERT(m_call);

    //qDebug("video mode changed %i", (int)mode);
    QXmppRtpVideoChannel *channel = m_call->videoChannel();
    if (!channel)
        mode = QIODevice::NotOpen;

    // start or stop playback
    const bool canRead = (mode & QIODevice::ReadOnly);
    if (canRead && !m_videoTimer->isActive()) {
        if (m_videoOutput) {
            QXmppVideoFormat format = channel->decoderFormat();
            m_videoOutput->setFormat(format);
            m_videoTimer->start(1000 / format.frameRate());
        }
    } else if (!canRead && m_videoTimer->isActive()) {
        m_videoTimer->stop();
    }

    // start or stop capture
    const bool canWrite = (mode & QIODevice::WriteOnly);
    if (canWrite && !m_videoGrabber) {
        const QXmppVideoFormat format = channel->encoderFormat();

        // check we have a video input
        QList<QVideoGrabberInfo> grabbers = QVideoGrabberInfo::availableGrabbers();
        if (grabbers.isEmpty())
            return;

        // determine if we need a conversion
        QList<QXmppVideoFrame::PixelFormat> pixelFormats = grabbers.first().supportedPixelFormats();
        if (!pixelFormats.contains(format.pixelFormat())) {
            qWarning("we need a format conversion");
            QXmppVideoFormat auxFormat = format;
            auxFormat.setPixelFormat(pixelFormats.first());
            m_videoGrabber = new QVideoGrabber(auxFormat);

            QPair<int, int> metrics = QVideoGrabber::byteMetrics(format.pixelFormat(), format.frameSize());
            m_videoConversion = new QXmppVideoFrame(metrics.second, format.frameSize(), metrics.first, format.pixelFormat());
        } else {
            m_videoGrabber = new QVideoGrabber(format);
        }

        connect(m_videoGrabber, SIGNAL(frameAvailable(QXmppVideoFrame)),
                this, SLOT(videoCapture(QXmppVideoFrame)));
        m_videoGrabber->start();

        if (m_videoMonitor)
            m_videoMonitor->setFormat(format);
    } else if (!canWrite && m_videoGrabber) {
        m_videoGrabber->stop();
        delete m_videoGrabber;
        m_videoGrabber = 0;
        if (m_videoConversion) {
            delete m_videoConversion;
            m_videoConversion = 0;
        }
    }

    emit openModeChanged();
}

void CallVideoHelper::videoCapture(const QXmppVideoFrame &frame)
{
    Q_ASSERT(m_call);

    QXmppRtpVideoChannel *channel = m_call->videoChannel();
    if (!channel)
        return;

    if (frame.isValid()) {
        if (m_videoConversion) {
            //m_videoConversion.setStartTime(frame.startTime());
            QVideoGrabber::convert(frame.size(),
                                   frame.pixelFormat(), frame.bytesPerLine(), frame.bits(),
                                   m_videoConversion->pixelFormat(), m_videoConversion->bytesPerLine(), m_videoConversion->bits());
            channel->writeFrame(*m_videoConversion);
        } else {
            channel->writeFrame(frame);
        }
        if (m_videoMonitor)
            m_videoMonitor->present(frame);
    }
}

void CallVideoHelper::videoRefresh()
{
    Q_ASSERT(m_call);

    QXmppRtpVideoChannel *channel = m_call->videoChannel();
    if (!channel)
        return;

    QList<QXmppVideoFrame> frames = channel->readFrames();
    if (!frames.isEmpty() && m_videoOutput)
        m_videoOutput->present(frames.last());
}

CallVideoItem::CallVideoItem(QDeclarativeItem *parent)
    : QDeclarativeItem(parent),
    m_radius(8)
{
    m_border = new DeclarativePen(this);
    setFlag(QGraphicsItem::ItemHasNoContents, false);
}

QRectF CallVideoItem::boundingRect() const
{
    return QRectF(0, 0, width(), height());
}

void CallVideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    const QRectF m_boundingRect(0, 0, width(), height());
    if (m_boundingRect.isEmpty())
        return;
    painter->setPen(QPen(m_border->color(), m_border->width()));
    if (!m_image.size().isEmpty()) {
        QBrush brush(m_image);
        brush.setTransform(brush.transform().scale(m_boundingRect.width() / m_image.width(), m_boundingRect.height() / m_image.height()));
        painter->setBrush(brush);
    } else {
        painter->setBrush(Qt::black);
    }
    painter->drawRoundedRect(m_boundingRect.adjusted(0.5, 0.5, -0.5, -0.5), m_radius, m_radius);
}

void CallVideoItem::present(const QXmppVideoFrame &frame)
{
    if (!frame.isValid() || frame.size() != m_image.size())
        return;
    QVideoGrabber::frameToImage(&frame, &m_image);
    update();
}

void CallVideoItem::setFormat(const QXmppVideoFormat &format)
{
    const QSize size = format.frameSize();
    if (size != m_image.size()) {
        m_image = QImage(size, QImage::Format_RGB32);
        m_image.fill(0);
    }
}

DeclarativePen* CallVideoItem::border()
{
    return m_border;
}

qreal CallVideoItem::radius() const
{
    return m_radius;
}

void CallVideoItem::setRadius(qreal radius)
{
    if (radius != m_radius) {
        m_radius = radius;
        emit radiusChanged(radius);
    }
}

