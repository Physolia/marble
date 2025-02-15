// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2014 Sanjiban Bairagya <sanjiban22393@gmail.com>
//

#include "SerialTrack.h"
#include "GeoDataCamera.h"
#include "GeoDataLookAt.h"
#include "PlaybackFlyToItem.h"
#include "PlaybackTourControlItem.h"
#include "PlaybackWaitItem.h"
#include "TourPlayback.h"

namespace Marble
{

SerialTrack::SerialTrack()
    : QObject()
{
    m_currentIndex = 0;
    m_finishedPosition = 0;
    m_currentPosition = 0;
    m_paused = true;
}

SerialTrack::~SerialTrack()
{
    clear();
}

void SerialTrack::append(PlaybackItem *item)
{
    connect(item, &PlaybackItem::progressChanged, this, &SerialTrack::changeProgress);
    connect(item, &PlaybackItem::centerOn, this, &SerialTrack::centerOn);
    connect(item, &PlaybackItem::finished, this, &SerialTrack::handleFinishedItem);
    connect(item, &PlaybackItem::paused, this, &SerialTrack::pause);
    m_items.append(item);
    if (m_items.size() == 1) {
        auto flyTo = dynamic_cast<PlaybackFlyToItem *>(item);
        if (flyTo != nullptr) {
            flyTo->setFirst(true);
        }
    }
}

void SerialTrack::play()
{
    m_paused = false;
    m_items[m_currentIndex]->play();
}

void SerialTrack::pause()
{
    m_paused = true;
    m_items[m_currentIndex]->pause();
}

void SerialTrack::stop()
{
    m_paused = true;
    if (m_items.size() != 0 && m_currentIndex >= 0 && m_currentIndex <= m_items.size() - 1) {
        m_items[m_currentIndex]->stop();
    }
    m_finishedPosition = 0;
    Q_EMIT progressChanged(m_finishedPosition);
    m_currentIndex = 0;
}

void SerialTrack::seek(double offset)
{
    m_currentPosition = offset;
    int index = -1;
    for (int i = 0; i < m_items.size(); i++) {
        if (offset < m_items[i]->duration()) {
            index = i;
            break;
        } else {
            m_items[i]->stop();
            offset -= m_items[i]->duration();
        }
    }

    if (index == -1) {
        index = m_items.size() - 1;
    }

    if (index < m_items.size() - 1) {
        for (int i = index + 1; i < m_items.size(); i++) {
            m_items[i]->stop();
        }
    }

    if (index > m_currentIndex) {
        for (int i = m_currentIndex; i < index; i++) {
            m_finishedPosition += m_items[i]->duration();
        }
    } else {
        for (int i = m_currentIndex - 1; i >= index && i >= 0; i--) {
            m_finishedPosition -= m_items[i]->duration();
        }
    }

    if (m_currentIndex != index && !m_paused) {
        m_items[index]->play();
    }

    m_currentIndex = index;
    if (m_currentIndex != -1) {
        double t = offset / m_items[m_currentIndex]->duration();
        Q_ASSERT(t >= 0 && t <= 1);
        m_items[m_currentIndex]->seek(t);
    }
}

double SerialTrack::duration() const
{
    double duration = 0.0;
    for (PlaybackItem *item : m_items) {
        duration += item->duration();
    }
    return duration;
}

void SerialTrack::clear()
{
    qDeleteAll(m_items);
    m_items.clear();
    m_currentIndex = 0;
    m_finishedPosition = 0;
    m_currentPosition = 0;
    m_paused = true;
}

void SerialTrack::handleFinishedItem()
{
    if (m_paused) {
        return;
    }
    if (m_currentIndex + 1 < m_items.size()) {
        m_finishedPosition += m_items[m_currentIndex]->duration();
        m_currentIndex++;
        m_items[m_currentIndex]->play();
        Q_EMIT itemFinished(m_currentIndex + 1);

    } else {
        Q_EMIT finished();
    }
}

void SerialTrack::changeProgress(double progress)
{
    m_currentPosition = m_finishedPosition + progress;
    Q_EMIT progressChanged(m_currentPosition);
}

int SerialTrack::size() const
{
    return m_items.size();
}

PlaybackItem *SerialTrack::at(int i)
{
    return m_items.at(i);
}

double SerialTrack::currentPosition()
{
    return m_currentPosition;
}

}

#include "moc_SerialTrack.cpp"
