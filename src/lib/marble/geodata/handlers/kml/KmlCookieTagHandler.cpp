// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2013 Mohammed Nafees <nafees.technocool@gmail.com>
//

#include "KmlCookieTagHandler.h"

#include "GeoDataNetworkLinkControl.h"
#include "KmlElementDictionary.h"

namespace Marble
{
namespace kml
{
KML_DEFINE_TAG_HANDLER(cookie)

GeoNode *KmlcookieTagHandler::parse(GeoParser &parser) const
{
    Q_ASSERT(parser.isStartElement() && parser.isValidElement(QLatin1StringView(kmlTag_cookie)));

    GeoStackItem parentItem = parser.parentElement();

    if (parentItem.represents(kmlTag_NetworkLinkControl)) {
        QString cookie = parser.readElementText();

        parentItem.nodeAs<GeoDataNetworkLinkControl>()->setCookie(cookie);
    }

    return nullptr;
}

}
}
