// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2010 Dennis Nienhüser <nienhueser@kde.org>
//

#include "routing/instructions/InstructionTransformation.h"
#include "routing/instructions/RoutingInstruction.h"
#include "routing/instructions/WaypointParser.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QStringList>
#include <QTextStream>
#include <QTranslator>

using namespace Marble;

QString adjustGosmoreVersion(QTextStream &stream, WaypointParser &parser)
{
    QString content = stream.readAll();
    if (!QCoreApplication::instance()->arguments().contains("--routino")) {
        QStringList lines = content.split(QLatin1Char('\r'));
        if (lines.size() > 2) {
            QStringList fields = lines.at(lines.size() - 2).split(QLatin1Char(','));
            parser.setFieldIndex(WaypointParser::RoadType, fields.size() - 3);
            parser.setFieldIndex(WaypointParser::TotalSecondsRemaining, fields.size() - 2);
            parser.setFieldIndex(WaypointParser::RoadName, fields.size() - 1);
        }
    }
    return content;
}

void loadTranslations(QCoreApplication &app, QTranslator &translator)
{
    const QString lang = QLocale::system().name();
    QString code;

    int index = lang.indexOf(QLatin1Char('_'));
    if (lang == QLatin1StringView("C")) {
        code = "en";
    } else if (index != -1) {
        code = lang.left(index);
    } else {
        index = lang.indexOf(QLatin1Char('@'));
        if (index != -1)
            code = lang.left(index);
        else
            code = lang;
    }

    QString const i18nDir = "/usr/share/marble/translations";
    QString const relativeDir = app.applicationDirPath() + QLatin1StringView("/translations");
    const QStringList paths = {i18nDir, relativeDir, QDir::currentPath()};
    for (const QString &path : paths) {
        for (const QString &lang : {lang, code}) {
            QFileInfo translations = QFileInfo(path + QLatin1StringView("/routing-instructions_") + lang + QLatin1StringView(".qm"));
            if (translations.exists() && translator.load(translations.absoluteFilePath())) {
                app.installTranslator(&translator);
                return;
            }
        }
    }
}

void usage()
{
    QTextStream console(stderr);
    console << "Usage: routing-instructions [options] [file]\n";
    console << '\n' << "file should be a text file with gosmore or routino output.";
    console << " If file is not given, stdin is read.";
    console << "\nOptions:\n";
    console << "\t--routino\t\tParse routino output. When not given, gosmore format is assumed\n";
    console << "\t--dense\t\t\tReplicate the gosmore output format and only exchange road names with driving instructions\n";
    console << "\t--csv\t\t\tUse csv output format\n";
    console << "\t--remaining-duration\tInclude the remaining duration in the output\n";
    console << "\nTranslations:\n";
    console << "The system locale is examined to load translation files.";
    console << " Translation files must be named routing-instructions_$lang.qm,";
    console << " where $lang is a two-letter ISO 639 language code, optionally suffixed by an underscore";
    console << " and an uppercase two-letter ISO 3166 country code, e.g. nl or de_DE. Such files are searched for";
    console << " in /usr/share/marble/translations, the translations subdir of the applications installation";
    console << " directory and the current working directory.\n";
    console << "\nExamples:\n";
    console << "export QUERY_STRING=\"flat=49.0&flon=8.3&tlat=49.0&tlon=8.35&fastest=1&v=motorcar\"\n";
    console << "gosmore gosmore.pak | routing-instructions\n";
    console << "LC_ALL=\"zh_TW\" gosmore gosmore.pak | routing-instructions --dense\n";
    console << "LC_ALL=\"nl.UTF-8\" routing-instructions gosmore.txt\n";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTranslator translator;
    loadTranslations(app, translator);

    QStringList const arguments = QCoreApplication::instance()->arguments();
    if (arguments.contains("--help") || arguments.contains("-h")) {
        usage();
        return 0;
    }

    RoutingInstructions directions;
    WaypointParser parser;
    if (arguments.contains("--routino")) {
        parser.setLineSeparator("\n");
        parser.setFieldSeparator(QLatin1Char('\t'));
        parser.setFieldIndex(WaypointParser::RoadName, 10);
    } else {
        parser.addJunctionTypeMapping("Jr", RoutingWaypoint::Roundabout);
    }

    if (argc > 1 && !(QString(argv[argc - 1]).startsWith("--"))) {
        QString filename(argv[argc - 1]);
        QFile input(filename);
        input.open(QIODevice::ReadOnly);
        QTextStream fileStream(&input);
        QString content = adjustGosmoreVersion(fileStream, parser);
        QTextStream stream(&content);
        directions = InstructionTransformation::process(parser.parse(stream));
    } else {
        QTextStream console(stdin);
        console.setAutoDetectUnicode(true);
        QString content = adjustGosmoreVersion(console, parser);
        QTextStream stream(&content);
        directions = InstructionTransformation::process(parser.parse(stream));
    }

    QTextStream console(stdout);

    if (arguments.contains("--dense")) {
        console << "Content-Type: text/plain\n\n";
    }

    for (int i = 0; i < directions.size(); ++i) {
        console << directions[i] << '\n';
    }

    return 0;
}
