// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2006-2007 Torsten Rahn <tackat@kde.org>
// SPDX-FileCopyrightText: 2007 Inge Wallin <ingwa@kde.org>
//

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QStandardPaths>
#include <QTranslator>

#include "QtMainWindow.h"

#include "GeoUriParser.h"
#include "MapThemeManager.h"
#include "MarbleDebug.h"
#include "MarbleDirs.h"
#include "MarbleLocale.h"
#include "MarbleTest.h"
#include "MarbleWidgetInputHandler.h"

#ifdef STATIC_BUILD
#include <QtPlugin>
Q_IMPORT_PLUGIN(qjpeg)
Q_IMPORT_PLUGIN(qsvg)
#endif

#ifdef Q_OS_MACX
// for getting app bundle path
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace Marble;

// load translation file from normal "KDE Applications" packaging installation
static bool loadTranslation(const QString &localeDirName, QApplication &app)
{
    const QString subPath = QLatin1StringView("locale/") + localeDirName + QLatin1StringView("/LC_MESSAGES/marble_qt.qm");
    const QString fullPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, subPath);
    if (fullPath.isEmpty()) {
        return false;
    }

    QTranslator *translator = new QTranslator(&app);
    if (!translator->load(fullPath)) {
        delete translator;
        return false;
    }

    app.installTranslator(translator);

    return true;
}

// load KDE translators system based translations
// TODO: document other possible supported translation systems, if any, and where their catalog files are
static void loadTranslations(QApplication &app)
{
    // Quote from ecm_create_qm_loader created code:
    // The way Qt translation system handles plural forms makes it necessary to
    // have a translation file which contains only plural forms for `en`.
    // That's why we load the `en` translation unconditionally, then load the
    // translation for the current locale to overload it.
    const QString en(QStringLiteral("en"));

    loadTranslation(en, app);

    QLocale locale = QLocale::system();
    if (locale.name() != en) {
        if (!loadTranslation(locale.name(), app)) {
            loadTranslation(locale.bcp47Name(), app);
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Marble Virtual Globe");
    app.setOrganizationName("KDE");
    app.setOrganizationDomain("kde.org");
    app.setDesktopFileName(QStringLiteral("org.kde.marble-qt"));

    // Load Qt translation system catalog for libmarblewidget, the plugins and this app
    loadTranslations(app);

    app.setApplicationDisplayName(MainWindow::tr("Marble - Virtual Globe"));

    // For non static builds on mac and win
    // we need to be sure we can find the qt image
    // plugins. In mac be sure to look in the
    // application bundle...

#ifdef Q_WS_WIN
    QApplication::addLibraryPath(QApplication::applicationDirPath() + QDir::separator() + QLatin1StringView("plugins"));
#endif
#ifdef Q_OS_MACX
    QApplication::instance()->setAttribute(Qt::AA_DontShowIconsInMenus);
    qDebug("Adding qt image plugins to plugin search path...");
    CFURLRef myBundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFStringRef myMacPath = CFURLCopyFileSystemPath(myBundleRef, kCFURLPOSIXPathStyle);
    const char *mypPathPtr = CFStringGetCStringPtr(myMacPath, CFStringGetSystemEncoding());
    CFRelease(myBundleRef);
    CFRelease(myMacPath);
    QString myPath(mypPathPtr);
    // if we are not in a bundle assume that the app is built
    // as a non bundle app and that image plugins will be
    // in system Qt frameworks. If the app is a bundle
    // lets try to set the qt plugin search path...
    if (myPath.contains(".app")) {
        myPath += QLatin1StringView("/Contents/plugins");
        QApplication::addLibraryPath(myPath);
        qDebug("Added %s to plugin search path", qPrintable(myPath));
    }
#endif

    QString marbleDataPath;
    int dataPathIndex = 0;
    QString mapThemeId;
    QString tour;
    QString coordinatesString;
    QString distanceString;
    QString geoUriString;
    MarbleGlobal::Profiles profiles = MarbleGlobal::getInstance()->profiles();

    QStringList args = QApplication::arguments();

    if (args.contains("-h") || args.contains("--help")) {
        qWarning() << "Usage: marble [options] [files]";
        qWarning();
        qWarning() << "[files] can be zero, one or more .kml and/or .gpx files to load and show.";
        qWarning();
        qWarning() << "general options:";
        qWarning() << "  --marbledatapath=<path> .... Overwrite the compile-time path to map themes and other data";
        qWarning() << "  --geo-uri=<uri> ............ Show map at given geo uri";
        qWarning() << "  --latlon=<coordinates> ..... Show map at given lat lon coordinates";
        qWarning() << "  --distance=<value> ......... Set the distance of the observer to the globe (in km)";
        qWarning() << "  --map=<id> ................. Use map id (e.g. \"earth/openstreetmap/openstreetmap.dgml\")";
        qWarning() << "  --tour=<file> .............. Load a KML tour from the given file and play it";
        qWarning();
        qWarning() << "debug options:";
        qWarning() << "  --debug-info ............... write (more) debugging information to the console";
        qWarning() << "  --fps ...................... Show the paint performance (paint rate) in the top left corner";
        qWarning() << "  --runtimeTrace.............. Show the time spent and other debug info of each layer";
        qWarning() << "  --tile-id................... Write the identifier of texture tiles on top of them";
        qWarning() << "  --timedemo ................. Measure the paint performance while moving the map and quit";
        qWarning() << "  --debug-polygons ........... Display the polygon nodes and their index for debugging";
        qWarning() << "  --debug-levels ............. Display OSM placemarks according to the level selected";
        qWarning();
        qWarning() << "profile options (note that marble should automatically detect which profile to use. Override that with the options below):";
        qWarning() << "  --highresolution ........... Enforce the profile for devices with high resolution (e.g. desktop computers)";
        qWarning() << "  --nohighresolution ......... Deactivate the profile for devices with high resolution (e.g. desktop computers)";

        return 0;
    }

    for (int i = 1; i < args.count(); ++i) {
        const QString arg = args.at(i);

        if (arg == QLatin1StringView("--debug-info")) {
            MarbleDebug::setEnabled(true);
        } else if (arg.startsWith(QLatin1StringView("--marbledatapath="), Qt::CaseInsensitive)) {
            marbleDataPath = args.at(i).mid(17);
        } else if (arg.compare(QLatin1StringView("--marbledatapath"), Qt::CaseInsensitive) == 0 && i + 1 < args.size()) {
            dataPathIndex = i + 1;
            marbleDataPath = args.value(dataPathIndex);
            ++i;
        } else if (arg == QLatin1StringView("--highresolution")) {
            profiles |= MarbleGlobal::HighResolution;
        } else if (arg == QLatin1StringView("--nohighresolution")) {
            profiles &= ~MarbleGlobal::HighResolution;
        } else if (arg.startsWith(QLatin1StringView("--latlon="), Qt::CaseInsensitive)) {
            coordinatesString = arg.mid(9);
        } else if (arg.compare(QLatin1StringView("--latlon"), Qt::CaseInsensitive) == 0 && i + 1 < args.size()) {
            ++i;
            coordinatesString = args.value(i);
        } else if (arg.compare(QLatin1StringView("--geo-uri="), Qt::CaseInsensitive) == 0) {
            geoUriString = arg.mid(10);
        } else if (arg.compare(QLatin1StringView("--geo-uri"), Qt::CaseInsensitive) == 0 && i + 1 < args.size()) {
            ++i;
            geoUriString = args.value(i);
        } else if (arg.startsWith(QLatin1StringView("--distance="), Qt::CaseInsensitive)) {
            distanceString = arg.mid(11);
        } else if (arg.compare(QLatin1StringView("--distance"), Qt::CaseInsensitive) == 0 && i + 1 < args.size()) {
            ++i;
            distanceString = args.value(i);
        } else if (arg.startsWith(QLatin1StringView("--map="), Qt::CaseInsensitive)) {
            mapThemeId = arg.mid(6);
        } else if (arg.compare(QLatin1StringView("--map"), Qt::CaseInsensitive) == 0 && i + 1 < args.size()) {
            ++i;
            mapThemeId = args.value(i);
        } else if (arg.startsWith(QLatin1StringView("--tour="), Qt::CaseInsensitive)) {
            tour = arg.mid(7);
        } else if (arg.compare(QLatin1StringView("--tour"), Qt::CaseInsensitive) == 0 && i + 1 < args.size()) {
            ++i;
            tour = args.value(i);
        }
    }
    MarbleGlobal::getInstance()->setProfiles(profiles);

    MarbleLocale::MeasurementSystem const measurement = (MarbleLocale::MeasurementSystem)QLocale::system().measurementSystem();
    MarbleGlobal::getInstance()->locale()->setMeasurementSystem(measurement);

    QVariantMap cmdLineSettings;
    if (!mapThemeId.isEmpty()) {
        cmdLineSettings.insert(QLatin1StringView("mapTheme"), QVariant(mapThemeId));
    }

    if (!coordinatesString.isEmpty()) {
        bool success = false;
        const GeoDataCoordinates coordinates = GeoDataCoordinates::fromString(coordinatesString, success);
        if (success) {
            QVariantList lonLat;
            lonLat << QVariant(coordinates.longitude(GeoDataCoordinates::Degree)) << QVariant(coordinates.latitude(GeoDataCoordinates::Degree));
            cmdLineSettings.insert(QLatin1StringView("lonlat"), QVariant(lonLat));
        }
    }
    if (!distanceString.isEmpty()) {
        bool success = false;
        const qreal distance = distanceString.toDouble(&success);
        if (success) {
            cmdLineSettings.insert(QLatin1StringView("distance"), QVariant(distance));
        }
    }
    if (!tour.isEmpty()) {
        cmdLineSettings.insert(QLatin1StringView("tour"), QVariant(tour));
    }

    cmdLineSettings.insert(QLatin1StringView("geo-uri"), QVariant(geoUriString));

    MainWindow window(marbleDataPath, cmdLineSettings);

    //    window.marbleWidget()->rotateTo( 0, 0, -90 );
    //    window.show();

    for (int i = 1; i < args.count(); ++i) {
        const QString arg = args.at(i);
        if (arg == QLatin1StringView("--timedemo")) {
            window.resize(900, 640);
            MarbleTest marbleTest(window.marbleWidget());
            marbleTest.timeDemo();
            return 0;
        }

        if (arg == QLatin1StringView("--fps")) {
            window.marbleControl()->marbleWidget()->setShowFrameRate(true);
        } else if (arg == QLatin1StringView("--tile-id")) {
            window.marbleControl()->marbleWidget()->setShowTileId(true);
        } else if (arg == QLatin1StringView("--runtimeTrace")) {
            window.marbleControl()->marbleWidget()->setShowRuntimeTrace(true);
        } else if (arg == QLatin1StringView("--debug-polygons")) {
            window.marbleControl()->marbleWidget()->setShowDebugPolygons(true);
        } else if (i != dataPathIndex && QFile::exists(arg)) {
            window.addGeoDataFile(arg);
        } else if (arg == QLatin1StringView("--debug-levels")) {
            window.marbleWidget()->setDebugLevelTags(true);
        }
    }

    auto const marbleWidget = window.marbleControl()->marbleWidget();
    bool const debugModeEnabled =
        marbleWidget->showRuntimeTrace() || marbleWidget->showDebugPolygons() || marbleWidget->debugLevelTags() || MarbleDebug::isEnabled();
    marbleWidget->inputHandler()->setDebugModeEnabled(debugModeEnabled);

    return app.exec();
}
