// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2008 Henry de Valence <hdevalence@gmail.com>

#include "LatLonRunner.h"

#include "GeoDataCoordinates.h"
#include "GeoDataPlacemark.h"

#include <QList>

namespace Marble
{

LatLonRunner::LatLonRunner(QObject *parent)
    : SearchRunner(parent)
{
}

LatLonRunner::~LatLonRunner() = default;

void LatLonRunner::search(const QString &searchTerm, const GeoDataLatLonBox &)
{
    QList<GeoDataPlacemark *> vector;

    bool successful = false;
    const GeoDataCoordinates coord = GeoDataCoordinates::fromString(searchTerm, successful);

    if (successful) {
        auto placemark = new GeoDataPlacemark;
        placemark->setName(searchTerm);
        qreal lon, lat;
        coord.geoCoordinates(lon, lat);
        placemark->setCoordinate(lon, lat);
        placemark->setVisualCategory(GeoDataPlacemark::Coordinate);
        placemark->setPopularity(1000000000);
        placemark->setZoomLevel(1);
        vector.append(placemark);
    }

    Q_EMIT searchFinished(vector);
}

}

#include "moc_LatLonRunner.cpp"
