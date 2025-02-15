// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2009 Bastian Holst <bastianholst@gmx.de>
//

// Self
#include "GeonamesParser.h"

// Marble
#include "MarbleGlobal.h"
#include "WikipediaItem.h"

// Qt
#include <QByteArray>

using namespace Marble;

GeonamesParser::GeonamesParser(MarbleWidget *widget, QList<WikipediaItem *> *list, QObject *parent)
    : m_marbleWidget(widget)
    , m_list(list)
    , m_parent(parent)
{
}

bool GeonamesParser::read(const QByteArray &data)
{
    addData(data);

    while (!atEnd()) {
        readNext();

        if (isStartElement()) {
            if (name() == QLatin1StringView("geonames"))
                readGeonames();
            else
                raiseError(QObject::tr("The file is not a valid Geonames answer."));
        }
    }

    return !error();
}

void GeonamesParser::readUnknownElement()
{
    Q_ASSERT(isStartElement());

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            readUnknownElement();
    }
}

void GeonamesParser::readGeonames()
{
    Q_ASSERT(isStartElement() && name() == QLatin1StringView("geonames"));

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement()) {
            if (name() == QLatin1StringView("entry"))
                readEntry();
            else
                readUnknownElement();
        }
    }
}

void GeonamesParser::readEntry()
{
    Q_ASSERT(isStartElement() && name() == QLatin1StringView("entry"));

    auto item = new WikipediaItem(m_marbleWidget, m_parent);
    m_list->append(item);

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement()) {
            if (name() == QLatin1StringView("title"))
                readTitle(item);
            else if (name() == QLatin1StringView("lng"))
                readLongitude(item);
            else if (name() == QLatin1StringView("lat"))
                readLatitude(item);
            else if (name() == QLatin1StringView("wikipediaUrl"))
                readUrl(item);
            else if (name() == QLatin1StringView("summary"))
                readSummary(item);
            else if (name() == QLatin1StringView("thumbnailImg"))
                readThumbnailImage(item);
            else if (name() == QLatin1StringView("rank"))
                readRank(item);
            else
                readUnknownElement();
        }
    }
}

void GeonamesParser::readTitle(WikipediaItem *item)
{
    Q_ASSERT(isStartElement() && name() == QLatin1StringView("title"));

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isCharacters()) {
            item->setName(text().toString());
        }
    }
}

void GeonamesParser::readLongitude(WikipediaItem *item)
{
    Q_ASSERT(isStartElement() && name() == QLatin1StringView("lng"));

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isCharacters()) {
            item->setLongitude(text().toString().toDouble() * DEG2RAD);
        }
    }
}

void GeonamesParser::readLatitude(WikipediaItem *item)
{
    Q_ASSERT(isStartElement() && name() == QLatin1StringView("lat"));

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isCharacters()) {
            item->setLatitude(text().toString().toDouble() * DEG2RAD);
        }
    }
}

void GeonamesParser::readUrl(WikipediaItem *item)
{
    Q_ASSERT(isStartElement() && name() == QLatin1StringView("wikipediaUrl"));

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isCharacters()) {
            // Try to switch to the mobile version, geonames
            // lacks API for that unfortunately
            QString url = text().toString();
            if (!url.contains(QLatin1StringView("m.wikipedia.org"))) {
                url.replace(QStringLiteral("wikipedia.org"), QStringLiteral("m.wikipedia.org"));
            }
            item->setUrl(QUrl::fromEncoded(url.toUtf8()));
        }
    }
}

void GeonamesParser::readSummary(WikipediaItem *item)
{
    Q_ASSERT(isStartElement() && name() == QLatin1StringView("summary"));

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isCharacters()) {
            item->setSummary(text().toString());
        }
    }
}

void GeonamesParser::readThumbnailImage(WikipediaItem *item)
{
    Q_ASSERT(isStartElement() && name() == QLatin1StringView("thumbnailImg"));

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isCharacters()) {
            item->setThumbnailImageUrl(QUrl(text().toString()));
        }
    }
}

void GeonamesParser::readRank(WikipediaItem *item)
{
    Q_ASSERT(isStartElement() && name() == QLatin1StringView("rank"));

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isCharacters()) {
            item->setRank(text().toString().toDouble());
        }
    }
}
