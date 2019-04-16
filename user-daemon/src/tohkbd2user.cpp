/*
 * (C) 2014 Kimmo Lindholm <kimmo.lindholm@gmail.com> Kimmoli
 *
 * Main, Daemon functions
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <QtGui/QGuiApplication>
#include <QtQml>
#include <QQuickView>
#include <QQmlEngine>
#include <QQmlContext>
#include <QScopedPointer>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QResource>

#include <sailfishapp.h>

#include "userdaemon.h"
//#include "../dbus/src/userdaemonAdaptor.h"
#include "viewhelper.h"
#include "applauncher.h"
#include "screenshot.h"

int main(int argc, char **argv)
{
    /* To make remorse timer run without steroids */
    setenv("QSG_RENDER_LOOP", "basic", 1);

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());
    QObject::connect(view->engine(), SIGNAL(quit()), app.data(), SLOT(quit()));
    QScopedPointer<ViewHelper> helper(new ViewHelper(view.data()));
    view->rootContext()->setContextProperty("viewHelper", helper.data());

    QColor color;
    color.setRedF(0.0);
    color.setGreenF(0.0);
    color.setBlueF(0.0);
    color.setAlphaF(0.0);
    view->setColor(color);

    view->setClearBeforeRendering(true);

    QTranslator engineeringTranslator;
    engineeringTranslator.load("engineering_en", SailfishApp::pathTo("i18n").toLocalFile());
    app->installTranslator(&engineeringTranslator);

    QTranslator translator;
    translator.load(QLocale::system().name(), SailfishApp::pathTo("i18n").toLocalFile());
    app->installTranslator(&translator);

    view->setSource(SailfishApp::pathTo("qml/taskswitcher.qml"));

    QTimer::singleShot(1, helper.data(), SLOT(detachWindow()));

    setlinebuf(stdout);
    setlinebuf(stderr);

    printf("taskswitcher-user: starting daemon version %s\n", APPVERSION);

    printf("taskswitcher-user: Locale is %s\n", qPrintable(QLocale::system().name()));

    UserDaemon rw;

    rw.registerDBus();

    QObject::connect(&rw, SIGNAL(_showTaskSwitcher()), helper.data(), SLOT(showWindow()));
    QObject::connect(&rw, SIGNAL(_hideTaskSwitcher()), helper.data(), SLOT(hideWindow()));
    QObject::connect(&rw, SIGNAL(_nextAppTaskSwitcher()), helper.data(), SLOT(nextApp()));
    QObject::connect(&rw, SIGNAL(_requestActionWithRemorse(QString)), helper.data(), SLOT(requestActionWithRemorse(QString)));

    AppLauncher al;

    QObject::connect(&al, SIGNAL(launchSuccess(QString)), &rw, SLOT(launchSuccess(QString)));
    QObject::connect(&al, SIGNAL(launchFailed()), &rw, SLOT(launchFailed()));
    QObject::connect(&rw, SIGNAL(_lauchApplication(QString)), &al, SLOT(launchApplication(QString)));
    QObject::connect(helper.data(), SIGNAL(_launchApplication(QString)), &al, SLOT(launchApplication(QString)));

    ScreenShot ss;

    QObject::connect(&rw, SIGNAL(_takeScreenShot()), &ss, SLOT(takeScreenShot()));

    QDBusConnection::sessionBus().connect("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "ActionInvoked",
                                            &ss, SLOT(handleNotificationActionInvoked(const QDBusMessage&)));

    return app->exec();
}
