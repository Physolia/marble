// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2008 Shashan Singh <shashank.personal@gmail.com>
// SPDX-FileCopyrightText: 2009 Bastian Holst <bastianholst@gmx.de>
//

// Self
#include "PanoramioModel.h"
#include "PanoramioItem.h"
#include "PanoramioParser.h"

// Marble
#include "GeoDataLatLonAltBox.h"
#include "MarbleModel.h"

// Qt
#include <QString>
#include <QUrl>

using namespace Marble;

PanoramioModel::PanoramioModel(const MarbleModel *marbleModel, QObject *parent)
    : AbstractDataPluginModel("panoramio", marbleModel, parent)
    , m_marbleWidget(0)
{
}

void PanoramioModel::setMarbleWidget(MarbleWidget *widget)
{
    m_marbleWidget = widget;
}

void PanoramioModel::getAdditionalItems(const GeoDataLatLonAltBox &box, qint32 number)
{
    if (marbleModel()->planetId() != QLatin1StringView("earth")) {
        return;
    }

    // FIXME: Download a list of constant number, because the parser doesn't support
    // loading a file of an unknown length.
    const QUrl jsonUrl(QLatin1StringView("http://www.panoramio.com/map/get_panoramas.php?from=") + QString::number(0) + QLatin1StringView("&order=upload_date")
                       + QLatin1StringView("&set=public") + QLatin1StringView("&to=")
                       + QString::number(number)
                       //                        + QLatin1StringView("&to=") + QString::number( number )
                       + QLatin1StringView("&minx=") + QString::number(box.west() * RAD2DEG) + QLatin1StringView("&miny=")
                       + QString::number(box.south() * RAD2DEG) + QLatin1StringView("&maxx=") + QString::number(box.east() * RAD2DEG)
                       + QLatin1StringView("&maxy=") + QString::number(box.north() * RAD2DEG) + QLatin1StringView("&size=small"));

    downloadDescriptionFile(jsonUrl);
}

void PanoramioModel::parseFile(const QByteArray &file)
{
    PanoramioParser panoramioJsonParser;
    QList<panoramioDataStructure> list = panoramioJsonParser.parseAllObjects(file, numberOfImagesPerFetch);

    QList<panoramioDataStructure>::iterator it;
    for (it = list.begin(); it != list.end(); ++it) {
        // Setting the meta information of the current image
        GeoDataCoordinates coordinates((*it).longitude, (*it).latitude, 0, GeoDataCoordinates::Degree);

        if (itemExists(QString::number((*it).photo_id))) {
            continue;
        }

        PanoramioItem *item = new PanoramioItem(m_marbleWidget, this);
        item->setCoordinate(coordinates);
        item->setId(QString::number((*it).photo_id));
        item->setPhotoUrl((*it).photo_url);
        item->setUploadDate((*it).upload_date);

        downloadItem(QUrl((*it).photo_file_url), standardImageSize, item);

        addItemToList(item);
    }
}

#include "moc_PanoramioModel.cpp"
