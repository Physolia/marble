// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2013 Mihail Ivchenko <ematirov@gmail.com>
// SPDX-FileCopyrightText: 2017 Sergey Popov <sergobot@protonmail.com>
//

#include "CycleStreetsRunner.h"

#include "GeoDataData.h"
#include "GeoDataDocument.h"
#include "GeoDataExtendedData.h"
#include "GeoDataLineString.h"
#include "GeoDataPlacemark.h"
#include "HttpDownloadManager.h"
#include "MarbleDebug.h"
#include "routing/Maneuver.h"
#include "routing/RouteRequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>

#include <QUrlQuery>

namespace Marble
{

CycleStreetsRunner::CycleStreetsRunner(QObject *parent)
    : RoutingRunner(parent)
    , m_networkAccessManager()
    , m_request()
{
    connect(&m_networkAccessManager, &QNetworkAccessManager::finished, this, &CycleStreetsRunner::retrieveData);

    turns.insert({}, Maneuver::Continue);
    turns.insert(QStringLiteral("straight on"), Maneuver::Straight);
    turns.insert(QStringLiteral("bear right"), Maneuver::SlightRight);
    turns.insert(QStringLiteral("bear left"), Maneuver::SlightLeft);
    turns.insert(QStringLiteral("sharp right"), Maneuver::SharpRight);
    turns.insert(QStringLiteral("sharp left"), Maneuver::SharpLeft);
    turns.insert(QStringLiteral("turn right"), Maneuver::Right);
    turns.insert(QStringLiteral("turn left"), Maneuver::Left);
    turns.insert(QStringLiteral("double-back"), Maneuver::TurnAround);
    turns.insert(QStringLiteral("first exit"), Maneuver::RoundaboutFirstExit);
    turns.insert(QStringLiteral("second exit"), Maneuver::RoundaboutSecondExit);
    turns.insert(QStringLiteral("third exit"), Maneuver::RoundaboutThirdExit);
    turns.insert(QStringLiteral("fourth exit"), Maneuver::RoundaboutExit);
    turns.insert(QStringLiteral("fifth exit"), Maneuver::RoundaboutExit);
    turns.insert(QStringLiteral("sixth exit"), Maneuver::RoundaboutExit);
    turns.insert(QStringLiteral("seventh or more exit"), Maneuver::RoundaboutExit);
}

CycleStreetsRunner::~CycleStreetsRunner()
{
    // nothing to do
}

void CycleStreetsRunner::retrieveRoute(const RouteRequest *route)
{
    if (route->size() < 2 || route->size() > 12) {
        return;
    }

    QHash<QString, QVariant> settings = route->routingProfile().pluginSettings()[QStringLiteral("cyclestreets")];

    QUrl url(QStringLiteral("https://www.cyclestreets.net/api/journey.json"));
    QMap<QString, QString> queryStrings;
    queryStrings[QStringLiteral("key")] = QStringLiteral("cdccf13997d59e70");
    queryStrings[QStringLiteral("reporterrors")] = QLatin1Char('1');
    queryStrings[QStringLiteral("plan")] = settings[QStringLiteral("plan")].toString();
    if (queryStrings[QStringLiteral("plan")].isEmpty()) {
        mDebug() << "Missing a value for 'plan' in the settings, falling back to 'balanced'";
        queryStrings[QStringLiteral("plan")] = QStringLiteral("balanced");
    }
    queryStrings[QStringLiteral("speed")] = settings[QStringLiteral("speed")].toString();
    if (queryStrings[QStringLiteral("speed")].isEmpty()) {
        mDebug() << "Missing a value for 'speed' in the settings, falling back to '20'";
        queryStrings[QStringLiteral("speed")] = QStringLiteral("20");
    }
    GeoDataCoordinates::Unit const degree = GeoDataCoordinates::Degree;
    QString itinerarypoints;
    itinerarypoints.append(QString::number(route->source().longitude(degree), 'f', 6) + QLatin1Char(',')
                           + QString::number(route->source().latitude(degree), 'f', 6));
    for (int i = 1; i < route->size(); ++i) {
        itinerarypoints.append(QLatin1Char('|') + QString::number(route->at(i).longitude(degree), 'f', 6) + QLatin1Char(',')
                               + QString::number(route->at(i).latitude(degree), 'f', 6));
    }
    queryStrings[QStringLiteral("itinerarypoints")] = itinerarypoints;

    QUrlQuery urlQuery;
    for (const QString &key : queryStrings.keys()) {
        urlQuery.addQueryItem(key, queryStrings.value(key));
    }
    url.setQuery(urlQuery);

    m_request.setUrl(url);
    m_request.setRawHeader("User-Agent", HttpDownloadManager::userAgent(QStringLiteral("Browser"), QStringLiteral("CycleStreetsRunner")));

    QEventLoop eventLoop;

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(15000);

    connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    connect(this, &RoutingRunner::routeCalculated, &eventLoop, &QEventLoop::quit);

    // @todo FIXME Must currently be done in the main thread, see bug 257376
    QTimer::singleShot(0, this, SLOT(get()));
    timer.start();

    eventLoop.exec();
}

void CycleStreetsRunner::get()
{
    QNetworkReply *reply = m_networkAccessManager.get(m_request);
    connect(reply, &QNetworkReply::errorOccurred, this, &CycleStreetsRunner::handleError, Qt::DirectConnection);
}

void CycleStreetsRunner::retrieveData(QNetworkReply *reply)
{
    if (reply->isFinished()) {
        QByteArray data = reply->readAll();
        reply->deleteLater();
        // mDebug() << "Download completed: " << data;
        GeoDataDocument *document = parse(data);

        if (!document) {
            mDebug() << "Failed to parse the downloaded route data" << data;
        }

        Q_EMIT routeCalculated(document);
    }
}

int CycleStreetsRunner::maneuverType(const QString &cycleStreetsName) const
{
    if (turns.contains(cycleStreetsName)) {
        return turns[cycleStreetsName];
    }
    return Maneuver::Unknown;
}

GeoDataDocument *CycleStreetsRunner::parse(const QByteArray &content) const
{
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(content, &error);

    if (json.isEmpty()) {
        mDebug() << "Cannot parse json file with routing instructions: " << error.errorString();
        return nullptr;
    }

    // Check if CycleStreets has found any error
    if (!json.object()[QStringLiteral("error")].isNull()) {
        mDebug() << "CycleStreets reported an error: " << json.object()[QStringLiteral("error")].toString();
        return nullptr;
    }

    auto result = new GeoDataDocument();
    result->setName(QStringLiteral("CycleStreets"));
    auto routePlacemark = new GeoDataPlacemark;
    routePlacemark->setName(QStringLiteral("Route"));

    auto routeWaypoints = new GeoDataLineString;
    QJsonArray features = json.object()[QStringLiteral("marker")].toArray();

    if (features.isEmpty()) {
        return nullptr;
    }
    QJsonObject route = features.first().toObject()[QStringLiteral("@attributes")].toObject();
    QJsonValue coordinates = route[QStringLiteral("coordinates")];
    QStringList coordinatesList = coordinates.toString().split(QLatin1Char(' '));

    QStringList::iterator iter = coordinatesList.begin();
    QStringList::iterator end = coordinatesList.end();

    for (; iter != end; ++iter) {
        const QStringList coordinate = iter->split(QLatin1Char(','));
        if (coordinate.size() == 2) {
            double const lon = coordinate.at(0).toDouble();
            double const lat = coordinate.at(1).toDouble();
            GeoDataCoordinates const position(lon, lat, 0.0, GeoDataCoordinates::Degree);
            routeWaypoints->append(position);
        }
    }
    routePlacemark->setGeometry(routeWaypoints);

    QTime duration;
    duration = duration.addSecs(route[QStringLiteral("time")].toInt());
    qreal length = routeWaypoints->length(EARTH_RADIUS);

    const QString name = nameString(QStringLiteral("CS"), length, duration);
    const GeoDataExtendedData data = routeData(length, duration);
    routePlacemark->setExtendedData(data);
    result->setName(name);
    result->append(routePlacemark);

    for (int i = 1; i < features.count(); ++i) {
        QJsonObject segment = features.at(i).toObject()[QStringLiteral("@attributes")].toObject();

        QString name = segment[QStringLiteral("name")].toString();
        QString maneuver = segment[QStringLiteral("turn")].toString();
        QStringList points = segment[QStringLiteral("points")].toString().split(QLatin1Char(' '));
        QStringList const elevation = segment[QStringLiteral("elevations")].toString().split(QLatin1Char(','));

        auto instructions = new GeoDataPlacemark;
        QString instructionName;
        if (!maneuver.isEmpty()) {
            instructionName = maneuver.left(1).toUpper() + maneuver.mid(1);
        } else {
            instructionName = QStringLiteral("Straight");
        }
        if (name != QLatin1StringView("Short un-named link") && name != QLatin1StringView("Un-named link")) {
            instructionName.append(QLatin1StringView(" into ") + name);
        }
        instructions->setName(instructionName);

        GeoDataExtendedData extendedData;
        GeoDataData turnType;
        turnType.setName(QStringLiteral("turnType"));
        turnType.setValue(maneuverType(maneuver));
        extendedData.addValue(turnType);

        instructions->setExtendedData(extendedData);
        auto lineString = new GeoDataLineString;
        QStringList::iterator iter = points.begin();
        QStringList::iterator end = points.end();
        for (int j = 0; iter != end; ++iter, ++j) {
            const QStringList coordinate = iter->split(QLatin1Char(','));
            if (coordinate.size() == 2) {
                double const lon = coordinate.at(0).toDouble();
                double const lat = coordinate.at(1).toDouble();
                double const alt = j < elevation.size() ? elevation[j].toDouble() : 0.0;
                lineString->append(GeoDataCoordinates(lon, lat, alt, GeoDataCoordinates::Degree));
            }
        }
        instructions->setGeometry(lineString);
        result->append(instructions);
    }
    return result;
}

void CycleStreetsRunner::handleError(QNetworkReply::NetworkError error)
{
    mDebug() << " Error when retrieving cyclestreets.net route: " << error;
}

} // namespace Marble

#include "moc_CycleStreetsRunner.cpp"
