// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2014 Torsten Rahn <rahn@kde.org>
//

// Local
#include "StereographicProjection.h"

// Marble
#include "AzimuthalProjection_p.h"
#include "GeoDataCoordinates.h"
#include "GeoDataLineString.h"
#include "MarbleGlobal.h"
#include "ViewportParams.h"

#include <QIcon>
#include <qmath.h>

#define SAFE_DISTANCE

namespace Marble
{

class StereographicProjectionPrivate : public AzimuthalProjectionPrivate
{
public:
    explicit StereographicProjectionPrivate(StereographicProjection *parent);

    Q_DECLARE_PUBLIC(StereographicProjection)
};

StereographicProjection::StereographicProjection()
    : AzimuthalProjection(new StereographicProjectionPrivate(this))
{
    setMinLat(minValidLat());
    setMaxLat(maxValidLat());
}

StereographicProjection::StereographicProjection(StereographicProjectionPrivate *dd)
    : AzimuthalProjection(dd)
{
    setMinLat(minValidLat());
    setMaxLat(maxValidLat());
}

StereographicProjection::~StereographicProjection() = default;

StereographicProjectionPrivate::StereographicProjectionPrivate(StereographicProjection *parent)
    : AzimuthalProjectionPrivate(parent)
{
}

QString StereographicProjection::name() const
{
    return QObject::tr("Stereographic");
}

QString StereographicProjection::description() const
{
    return QObject::tr(
        "<p><b>Stereographic Projection</b> (\"orthogonal\")</p><p>Applications: Used for planetary cartography, geology and panorama photography.</p>");
}

QIcon StereographicProjection::icon() const
{
    return QIcon(QStringLiteral(":/icons/map-globe.png"));
}

qreal StereographicProjection::clippingRadius() const
{
    return 1;
}

bool StereographicProjection::screenCoordinates(const GeoDataCoordinates &coordinates,
                                                const ViewportParams *viewport,
                                                qreal &x,
                                                qreal &y,
                                                bool &globeHidesPoint) const
{
    const qreal lambda = coordinates.longitude();
    const qreal phi = coordinates.latitude();
    const qreal lambdaPrime = viewport->centerLongitude();
    const qreal phi1 = viewport->centerLatitude();

    qreal cosC = qSin(phi1) * qSin(phi) + qCos(phi1) * qCos(phi) * qCos(lambda - lambdaPrime);
    // Prevent division by zero
    if (cosC <= 0) {
        globeHidesPoint = true;
        return false;
    }

    qreal k = 1 / (1 + cosC);

    // Let (x, y) be the position on the screen of the placemark..
    x = (qCos(phi) * qSin(lambda - lambdaPrime)) * k;
    y = (qCos(phi1) * qSin(phi) - qSin(phi1) * qCos(phi) * qCos(lambda - lambdaPrime)) * k;

    x *= viewport->radius();
    y *= viewport->radius();

    const qint64 radius = clippingRadius() * viewport->radius();

    if (x * x + y * y > radius * radius) {
        globeHidesPoint = true;
        return false;
    }

    globeHidesPoint = false;

    x += viewport->width() / 2;
    y = viewport->height() / 2 - y;

    // Skip placemarks that are outside the screen area
    return !(x < 0 || x >= viewport->width() || y < 0 || y >= viewport->height());
}

bool StereographicProjection::screenCoordinates(const GeoDataCoordinates &coordinates,
                                                const ViewportParams *viewport,
                                                qreal *x,
                                                qreal &y,
                                                int &pointRepeatNum,
                                                const QSizeF &size,
                                                bool &globeHidesPoint) const
{
    pointRepeatNum = 0;
    globeHidesPoint = false;

    bool visible = screenCoordinates(coordinates, viewport, *x, y, globeHidesPoint);

    // Skip placemarks that are outside the screen area
    if (*x + size.width() / 2.0 < 0.0 || *x >= viewport->width() + size.width() / 2.0 || y + size.height() / 2.0 < 0.0
        || y >= viewport->height() + size.height() / 2.0) {
        return false;
    }

    // This projection doesn't have any repetitions,
    // so the number of screen points referring to the geopoint is one.
    pointRepeatNum = 1;
    return visible;
}

bool StereographicProjection::geoCoordinates(const int x, const int y, const ViewportParams *viewport, qreal &lon, qreal &lat, GeoDataCoordinates::Unit unit)
    const
{
    const qint64 radius = viewport->radius();
    // Calculate how many degrees are being represented per pixel.
    const qreal centerLon = viewport->centerLongitude();
    const qreal centerLat = viewport->centerLatitude();
    const qreal rx = (-viewport->width() / 2 + x);
    const qreal ry = (viewport->height() / 2 - y);
    const qreal p = qMax(qSqrt(rx * rx + ry * ry), qreal(0.0001)); // ensure we don't divide by zero
    const qreal c = 2 * qAtan2(p, radius);
    const qreal sinc = qSin(c);

    lon = centerLon + qAtan2(rx * sinc, (p * qCos(centerLat) * qCos(c) - ry * qSin(centerLat) * sinc));

    while (lon < -M_PI)
        lon += 2 * M_PI;
    while (lon > M_PI)
        lon -= 2 * M_PI;

    lat = qAsin(qCos(c) * qSin(centerLat) + (ry * sinc * qCos(centerLat)) / p);

    if (unit == GeoDataCoordinates::Degree) {
        lon *= RAD2DEG;
        lat *= RAD2DEG;
    }

    return true;
}

}
