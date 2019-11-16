#include <stdio.h>
#include <sailfishapp.h>
#include <QHostAddress>
#include <QtSystemInfo/QDeviceInfo>

#include "userdaemon.h"

static const char *SERVICE = SERVICE_NAME;
static const char *PATH = "/";

UserDaemon::UserDaemon(QObject *parent) :
    QObject(parent)
{
    m_dbusRegistered = false;
    m_launchPending = false;
}

UserDaemon::~UserDaemon()
{
    if (m_dbusRegistered)
    {
        QDBusConnection connection = QDBusConnection::sessionBus();
        connection.unregisterObject(PATH);
        connection.unregisterService(SERVICE);

        printf("taskswitcher-user: unregistered from dbus sessionBus\n");
    }
}

void UserDaemon::registerDBus()
{
    if (!m_dbusRegistered)
    {
        // DBus
        QDBusConnection connection = QDBusConnection::sessionBus();
        if (!connection.registerService(SERVICE))
        {
            QCoreApplication::quit();
            return;
        }

        if (!connection.registerObject(PATH, this, QDBusConnection::ExportAllSlots))
        {
            QCoreApplication::quit();
            return;
        }
         m_dbusRegistered = true;

        printf("taskswitcher-user: succesfully registered to dbus sessionBus \"%s\"\n", SERVICE);
    }
}

void UserDaemon::quit()
{
    printf("taskswitcher-user: quit requested from dbus\n");
    QCoreApplication::quit();
}

void UserDaemon::setOrientationLock(const QString &value)
{
    if (value == "dynamic" || value == "landscape" || value == "portrait" || value == "landscape-inverted" || value == "portrait-inverted")
    {
        printf("taskswitcher-user: setting orientation lock to \"%s\"\n", qPrintable(value));

        MGConfItem ci("/lipstick/orientationLock");
        ci.set(value);
    }
    else
    {
        printf("taskswitcher-user: error: orientation lock can be set only to dynamic, landscape or portrait (or the inverted variants).\n");
    }
}

QString UserDaemon::getOrientationLock()
{
    MGConfItem ci("/lipstick/orientationLock");

    QString orientation = ci.value().toString();

    /* Assume orientation to be "dynamic" if returns empty */

    if (orientation.isEmpty())
        orientation = "dynamic";

    printf("taskswitcher-user: orientation lock is \"%s\"\n", qPrintable(orientation));

    return orientation;
}

void UserDaemon::launchApplication(const QString &desktopFilename)
{
    m_launchPending = true;
    emit _lauchApplication(desktopFilename);
}

void UserDaemon::showKeyboardConnectionNotification(const bool &connected)
{
    if (connected)
    {
        //: Notification shown when keyboard is connected
        //% "Keyboard connected"
        showNotification(tr("Keyboard Connected"));
    }
    else
    {
        //: Notification shown when keyboard is removed
        //% "Keyboard removed"
        showNotification(tr("Keyboard Disconnected"));
    }
}

QString UserDaemon::getVersion()
{
    return QString(APPVERSION);
}

void UserDaemon::launchSuccess(const QString &appName)
{
    if (m_launchPending)
    {
        //: Notification shown when application is started by pressing shortcut key
        //% "Starting %1..."
        showNotification(qtTrId("starting-app").arg(appName));
    }

    m_launchPending = false;
}

void UserDaemon::launchFailed()
{
    m_launchPending = false;
}

/* show notification
 */
void UserDaemon::showNotification(const QString &text)
{
    Notification notif;

    notif.setPreviewBody(text);
    notif.setCategory("x-harbour.taskswitcher");
    notif.publish();
}

void UserDaemon::actionWithRemorse(const QString &action)
{
    printf("taskswitcher-user: requested %s.\n", qPrintable(action));

    emit _requestActionWithRemorse(action);
}

QString UserDaemon::getPathTo(const QString &filename)
{
    return SailfishApp::pathTo(filename).toLocalFile();
}

/*
 * Check that Sailfish version is at least required version
 */
bool UserDaemon::checkSailfishVersion(QString versionToCompare)
{
    QDeviceInfo deviceInfo;
    QString sailfishVersion = deviceInfo.version(QDeviceInfo::Os);

    return (QHostAddress(sailfishVersion).toIPv4Address()
            >= QHostAddress(versionToCompare).toIPv4Address());
}
