/*
    SPDX-FileCopyrightText: 2009 Thibaut GRIDEL <tgridel@free.fr>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "GPXwptTagHandler.h"

#include <QStringView>

#include "GPXElementDictionary.h"
#include "GeoDataDocument.h"
#include "GeoDataPlacemark.h"
#include "GeoParser.h"

namespace Marble
{
namespace gpx
{
GPX_DEFINE_TAG_HANDLER(wpt)

GeoNode *GPXwptTagHandler::parse(GeoParser &parser) const
{
    Q_ASSERT(parser.isStartElement() && parser.isValidElement(QLatin1StringView(gpxTag_wpt)));

    GeoStackItem parentItem = parser.parentElement();
    if (parentItem.represents(gpxTag_gpx)) {
        auto doc = parentItem.nodeAs<GeoDataDocument>();
        auto placemark = new GeoDataPlacemark;

        QXmlStreamAttributes attributes = parser.attributes();
        QStringView tmp;
        qreal lat = 0;
        qreal lon = 0;
        tmp = attributes.value(QLatin1StringView(gpxTag_lat));
        if (!tmp.isEmpty()) {
            lat = tmp.toString().toFloat();
        }
        tmp = attributes.value(QLatin1StringView(gpxTag_lon));
        if (!tmp.isEmpty()) {
            lon = tmp.toString().toFloat();
        }
        placemark->setCoordinate(lon, lat, 0, GeoDataCoordinates::Degree);
        placemark->setRole(QStringLiteral("Waypoint"));

        placemark->setStyle(doc->style(QStringLiteral("waypoint")));

        doc->append(placemark);
        return placemark;
    }
    return nullptr;
}

} // namespace gpx

} // namespace Marble
