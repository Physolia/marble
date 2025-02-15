// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2010 Utku Aydın <utkuaydin34@gmail.com>
// SPDX-FileCopyrightText: 2012 Illya Kovalevskyy <illya.kovalevskyy@gmail.com>
//

#include "OpenDesktopModel.h"
#include "GeoDataCoordinates.h"
#include "GeoDataLatLonAltBox.h"
#include "MarbleGlobal.h"
#include "MarbleModel.h"
#include "OpenDesktopItem.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QUrl>

using namespace Marble;

OpenDesktopModel::OpenDesktopModel(const MarbleModel *marbleModel, QObject *parent)
    : AbstractDataPluginModel(QStringLiteral("opendesktop"), marbleModel, parent)
{
    // Nothing to do...
}

OpenDesktopModel::~OpenDesktopModel()
{
    // Nothing to do...
}

void OpenDesktopModel::setMarbleWidget(MarbleWidget *widget)
{
    m_marbleWidget = widget;
}

void OpenDesktopModel::getAdditionalItems(const GeoDataLatLonAltBox &box, qint32 number)
{
    Q_UNUSED(number)

    if (marbleModel()->planetId() != QLatin1StringView("earth")) {
        return;
    }

    GeoDataCoordinates coords = box.center();

    const QString openDesktopUrl(QLatin1StringView("http://api.opendesktop.org/v1/person/data") + QLatin1StringView("?latitude=")
                                 + QString::number(coords.latitude() * RAD2DEG) + QLatin1StringView("&longitude=")
                                 + QString::number(coords.longitude() * RAD2DEG) + QLatin1StringView("&format=json"));

    downloadDescriptionFile(QUrl(openDesktopUrl));
}

void OpenDesktopModel::parseFile(const QByteArray &file)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(file);
    QJsonValue dataValue = jsonDoc.object().value(QStringLiteral("data"));

    // Parse if any result exists
    if (dataValue.isArray()) {
        // Add items to the list
        QList<AbstractDataPluginItem *> items;

        QJsonArray dataArray = dataValue.toArray();
        for (int index = 0; index < dataArray.size(); ++index) {
            QJsonObject dataObject = dataArray[index].toObject();

            // Convert profile's properties from JSON to appropriate types
            const QString personid = dataObject.value(QStringLiteral("personid")).toString();
            const QString firstName = dataObject.value(QStringLiteral("firstname")).toString();
            const QString lastName = dataObject.value(QStringLiteral("lastname")).toString();
            const QString city = dataObject.value(QStringLiteral("city")).toString();
            const QString country = dataObject.value(QStringLiteral("country")).toString();
            const QString role = dataObject.value(QStringLiteral("communityrole")).toString();
            const double longitude = dataObject.value(QStringLiteral("longitude")).toDouble();
            const double latitude = dataObject.value(QStringLiteral("latitude")).toDouble();
            const QUrl avatarUrl(dataObject.value(QStringLiteral("avatarpic")).toString());

            if (!itemExists(personid)) {
                // If it does not exists, create it
                GeoDataCoordinates coor(longitude * DEG2RAD, latitude * DEG2RAD);
                auto item = new OpenDesktopItem(this);
                item->setMarbleWidget(m_marbleWidget);
                item->setId(personid);
                item->setCoordinate(coor);
                item->setFullName(firstName + QLatin1Char(' ') + lastName);
                item->setLocation(city + QLatin1StringView(", ") + country);
                item->setRole(!role.isEmpty() ? role : QStringLiteral("nothing"));
                downloadItem(avatarUrl, QStringLiteral("avatar"), item);
                items << item;
            }
        }

        addItemsToList(items);
    }
}

#include "moc_OpenDesktopModel.cpp"
