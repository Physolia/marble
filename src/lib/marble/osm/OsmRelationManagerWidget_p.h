// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2015 Stanciu Marius-Valeriu <stanciumarius94@gmail.com>
//

#ifndef MARBLE_OSMRELATIONMANAGERWIDGETPRIVATE_H
#define MARBLE_OSMRELATIONMANAGERWIDGETPRIVATE_H

#include "ui_OsmRelationManagerWidget.h"

namespace Marble
{

class OsmRelationManagerWidget;
class GeoDataPlacemark;
class OsmPlacemarkData;

class OsmRelationManagerWidgetPrivate : public Ui::OsmRelationManagerWidgetPrivate
{
public:
    OsmRelationManagerWidgetPrivate();
    ~OsmRelationManagerWidgetPrivate();
    void populateRelationsList();
    void populateDropMenu();

private:
    friend class OsmRelationManagerWidget;
    GeoDataPlacemark *m_placemark;
    const QHash<qint64, OsmPlacemarkData> *m_allRelations;
    QMenu *m_relationDropMenu;
};

}

#endif
