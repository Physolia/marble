// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2013 Mihail Ivchenko <ematirov@gmail.com>
//

#ifndef MARBLE_CYCLESTREETSRUNNER_H
#define MARBLE_CYCLESTREETSRUNNER_H

#include "RoutingRunner.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Marble
{

class CycleStreetsRunner : public RoutingRunner
{
    Q_OBJECT

public:
    explicit CycleStreetsRunner(QObject *parent = nullptr);

    ~CycleStreetsRunner() override;

    // Overriding MarbleAbstractRunner
    void retrieveRoute(const RouteRequest *request) override;

private Q_SLOTS:
    void get();

    /** Route data was retrieved via http */
    void retrieveData(QNetworkReply *reply);

    /** A network error occurred */
    void handleError(QNetworkReply::NetworkError);

private:
    GeoDataDocument *parse(const QByteArray &content) const;

    int maneuverType(const QString &cycleStreetsName) const;

    QNetworkAccessManager m_networkAccessManager;

    QNetworkRequest m_request;

    QMap<QString, int> turns;
};

}

#endif
