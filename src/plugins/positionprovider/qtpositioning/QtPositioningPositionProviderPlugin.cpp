// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2011 Daniel Marth <danielmarth@gmx.at>
// SPDX-FileCopyrightText: 2012 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
//

#include "QtPositioningPositionProviderPlugin.h"

#include "GeoDataAccuracy.h"
#include "GeoDataCoordinates.h"

#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QIcon>
#include <QTimer>

namespace Marble
{

class QtPositioningPositionProviderPluginPrivate
{
public:
    QtPositioningPositionProviderPluginPrivate();
    ~QtPositioningPositionProviderPluginPrivate();

    QGeoPositionInfoSource *m_source = nullptr;
    PositionProviderStatus m_status;
    QTimer *const m_updateChecker;

    QGeoPositionInfo m_lastKnownPosition;
};

QtPositioningPositionProviderPluginPrivate::QtPositioningPositionProviderPluginPrivate()
    : m_source(nullptr)
    , m_status(PositionProviderStatusUnavailable)
    , m_updateChecker(new QTimer)
{
}

QtPositioningPositionProviderPluginPrivate::~QtPositioningPositionProviderPluginPrivate()
{
    delete m_updateChecker;
    delete m_source;
}

QString QtPositioningPositionProviderPlugin::name() const
{
    return tr("Qt Positioning Position Provider Plugin");
}

QString QtPositioningPositionProviderPlugin::nameId() const
{
    return QStringLiteral("QtPositioning");
}

QString QtPositioningPositionProviderPlugin::guiString() const
{
    return tr("Qt Positioning Location");
}

QString QtPositioningPositionProviderPlugin::version() const
{
    return QStringLiteral("1.0");
}

QString QtPositioningPositionProviderPlugin::description() const
{
    return tr("Reports the GPS position of a QtPositioning compatible device.");
}

QString QtPositioningPositionProviderPlugin::copyrightYears() const
{
    return QStringLiteral("2011-2016");
}

QList<PluginAuthor> QtPositioningPositionProviderPlugin::pluginAuthors() const
{
    return QList<PluginAuthor>() << PluginAuthor(QStringLiteral("Daniel Marth"), QStringLiteral("danielmarth@gmx.at"));
}

QIcon QtPositioningPositionProviderPlugin::icon() const
{
    return {};
}

PositionProviderPlugin *QtPositioningPositionProviderPlugin::newInstance() const
{
    return new QtPositioningPositionProviderPlugin;
}

PositionProviderStatus QtPositioningPositionProviderPlugin::status() const
{
    return d->m_status;
}

GeoDataCoordinates QtPositioningPositionProviderPlugin::position() const
{
    if (d->m_source == nullptr) {
        return {};
    }

    const QGeoCoordinate p = d->m_lastKnownPosition.coordinate();
    if (!p.isValid()) {
        return {};
    }

    return {p.longitude(), p.latitude(), p.altitude(), GeoDataCoordinates::Degree};
}

GeoDataAccuracy QtPositioningPositionProviderPlugin::accuracy() const
{
    if (d->m_source == nullptr) {
        return GeoDataAccuracy();
    }

    const QGeoPositionInfo info = d->m_lastKnownPosition;
    const qreal horizontal = info.attribute(QGeoPositionInfo::HorizontalAccuracy);
    const qreal vertical = info.attribute(QGeoPositionInfo::VerticalAccuracy);
    GeoDataAccuracy::Level const level = horizontal > 0 ? GeoDataAccuracy::Detailed : GeoDataAccuracy::none;
    return GeoDataAccuracy(level, horizontal, vertical);
}

QtPositioningPositionProviderPlugin::QtPositioningPositionProviderPlugin()
    : d(new QtPositioningPositionProviderPluginPrivate)
{
}

QtPositioningPositionProviderPlugin::~QtPositioningPositionProviderPlugin()
{
    delete d;
}

void QtPositioningPositionProviderPlugin::initialize()
{
    d->m_source = QGeoPositionInfoSource::createDefaultSource(this);
    if (d->m_source) {
        d->m_status = PositionProviderStatusAcquiring;
        Q_EMIT statusChanged(d->m_status);
        connect(d->m_updateChecker, SIGNAL(timeout()), this, SLOT(update()));
        connect(d->m_source, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(update(QGeoPositionInfo)));
        d->m_source->setUpdateInterval(1000);
        d->m_source->startUpdates();
        d->m_updateChecker->start(5000);
    }
}

bool QtPositioningPositionProviderPlugin::isInitialized() const
{
    return d->m_source != nullptr;
}

qreal QtPositioningPositionProviderPlugin::speed() const
{
    if (d->m_source == nullptr) {
        return 0.0;
    }

    if (!d->m_lastKnownPosition.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
        return 0.0;
    }

    return d->m_lastKnownPosition.attribute(QGeoPositionInfo::GroundSpeed);
}

qreal QtPositioningPositionProviderPlugin::direction() const
{
    if (d->m_source == nullptr) {
        return 0.0;
    }

    if (!d->m_lastKnownPosition.hasAttribute(QGeoPositionInfo::Direction)) {
        return 0.0;
    }

    return d->m_lastKnownPosition.attribute(QGeoPositionInfo::Direction);
}

QDateTime QtPositioningPositionProviderPlugin::timestamp() const
{
    if (d->m_source == nullptr) {
        return {};
    }

    return d->m_lastKnownPosition.timestamp();
}

void QtPositioningPositionProviderPlugin::update()
{
    if (d->m_source) {
        update(d->m_source->lastKnownPosition());
    }
}

void QtPositioningPositionProviderPlugin::update(const QGeoPositionInfo &geoPositionInfo)
{
    PositionProviderStatus newStatus = geoPositionInfo.isValid() ? PositionProviderStatusAvailable : PositionProviderStatusError;
    if (geoPositionInfo.isValid()) {
        d->m_lastKnownPosition = geoPositionInfo;
    }

    if (newStatus != d->m_status) {
        d->m_status = newStatus;
        Q_EMIT statusChanged(d->m_status);
    }

    if (newStatus == PositionProviderStatusAvailable) {
        Q_EMIT positionChanged(position(), accuracy());
    }
}

} // namespace Marble

#include "moc_QtPositioningPositionProviderPlugin.cpp"
