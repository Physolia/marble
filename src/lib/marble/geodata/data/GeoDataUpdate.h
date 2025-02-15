// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2013 Mohammed Nafees <nafees.technocool@gmail.com>
//

#ifndef GEODATAUPDATE_H
#define GEODATAUPDATE_H

#include "GeoDataObject.h"
#include "geodata_export.h"

namespace Marble
{

class GeoDataChange;
class GeoDataCreate;
class GeoDataDelete;
class GeoDataUpdatePrivate;

/**
 */
class GEODATA_EXPORT GeoDataUpdate : public GeoDataObject
{
public:
    GeoDataUpdate();

    GeoDataUpdate(const GeoDataUpdate &other);

    GeoDataUpdate &operator=(const GeoDataUpdate &other);
    bool operator==(const GeoDataUpdate &other) const;
    bool operator!=(const GeoDataUpdate &other) const;

    ~GeoDataUpdate() override;

    /** Provides type information for downcasting a GeoNode */
    const char *nodeType() const override;

    QString targetHref() const;
    void setTargetHref(const QString &targetHref);

    const GeoDataChange *change() const;
    GeoDataChange *change();
    void setChange(GeoDataChange *change);

    const GeoDataCreate *create() const;
    GeoDataCreate *create();
    void setCreate(GeoDataCreate *create);

    const GeoDataDelete *getDelete() const;
    GeoDataDelete *getDelete();
    void setDelete(GeoDataDelete *dataDelete);

private:
    GeoDataUpdatePrivate *const d;
};

}

#endif
