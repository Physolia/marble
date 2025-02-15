// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2011 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
//

#include "Speedometer.h"

#include <QIcon>

#include "MarbleGlobal.h"
#include "MarbleGraphicsGridLayout.h"
#include "MarbleLocale.h"
#include "MarbleModel.h"
#include "PositionTracking.h"
#include "WidgetGraphicsItem.h"

namespace Marble
{

Speedometer::Speedometer()
    : AbstractFloatItem(nullptr)
    , m_locale(nullptr)
    , m_widgetItem(nullptr)
{
}

Speedometer::Speedometer(const MarbleModel *marbleModel)
    : AbstractFloatItem(marbleModel, QPointF(10.5, 110), QSizeF(135.0, 80.0))
    , m_locale(nullptr)
    , m_widgetItem(nullptr)
{
    setVisible(false);

    const bool smallScreen = MarbleGlobal::getInstance()->profiles() & MarbleGlobal::SmallScreen;
    if (smallScreen) {
        setPosition(QPointF(10.5, 10.5));
    }
}

Speedometer::~Speedometer() = default;

QStringList Speedometer::backendTypes() const
{
    return QStringList(QStringLiteral("speedometer"));
}

QString Speedometer::name() const
{
    return tr("Speedometer");
}

QString Speedometer::guiString() const
{
    return tr("&Speedometer");
}

QString Speedometer::nameId() const
{
    return QStringLiteral("speedometer");
}

QString Speedometer::version() const
{
    return QStringLiteral("1.0");
}

QString Speedometer::description() const
{
    return tr("Display the current cruising speed.");
}

QString Speedometer::copyrightYears() const
{
    return QStringLiteral("2011");
}

QList<PluginAuthor> Speedometer::pluginAuthors() const
{
    return QList<PluginAuthor>() << PluginAuthor(QStringLiteral("Bernhard Beschow"), QStringLiteral("bbeschow@cs.tu-berlin.de"));
}

QIcon Speedometer::icon() const
{
    return QIcon(QStringLiteral(":/icons/speedometer.png"));
}

void Speedometer::initialize()
{
    if (!m_widgetItem) {
        auto widget = new QWidget;
        m_widget.setupUi(widget);
        m_widgetItem = new WidgetGraphicsItem(this);
        m_widgetItem->setWidget(widget);

        auto layout = new MarbleGraphicsGridLayout(1, 1);
        layout->addItem(m_widgetItem, 0, 0);
        setLayout(layout);
        setPadding(0);

        m_locale = MarbleGlobal::getInstance()->locale();
        connect(marbleModel()->positionTracking(), &PositionTracking::gpsLocation, this, &Speedometer::updateLocation);
    }
}

bool Speedometer::isInitialized() const
{
    return m_widgetItem;
}

void Speedometer::updateLocation(const GeoDataCoordinates &coordinates, qreal speed)
{
    Q_UNUSED(coordinates);

    speed *= METER2KM / SEC2HOUR;
    QString speedUnit;

    switch (m_locale->measurementSystem()) {
    case MarbleLocale::ImperialSystem:
        // miles per hour
        speedUnit = tr("mph");
        speed *= KM2MI;
        break;

    case MarbleLocale::MetricSystem:
        // kilometers per hour
        speedUnit = tr("km/h");
        break;

    case MarbleLocale::NauticalSystem:
        // nm per hour (kt)
        speedUnit = tr("kt");
        speed *= KM2NM;
        break;
    }

    m_widget.speed->display(speed);
    m_widget.speedUnit->setText(speedUnit);

    update();
    Q_EMIT repaintNeeded();
}

}

#include "moc_Speedometer.cpp"
