#include "viewhelper.h"

#include <stdio.h>

#include <QGuiApplication>
#include <QDir>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <QFileInfo>
#include "../daemon/src/defaultSettings.h"

ViewHelper::ViewHelper(QQuickView *parent) :
    QObject(parent),
    view(parent)
{
    m_currentApp = 0;
    m_numberOfApps = 0;
    m_visible = false;

    mruList.clear();

    apps.clear();
    appsDesktopFiles.clear();

    emit currentAppChanged();
    emit numberOfAppsChanged();
    emit visibleChanged();
}

void ViewHelper::detachWindow()
{
    view->close();
    view->create();

    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    native->setWindowProperty(view->handle(), QLatin1String("CATEGORY"), "notification");
    setDefaultRegion();
}

void ViewHelper::setMouseRegion(const QRegion &region)
{
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    native->setWindowProperty(view->handle(), QLatin1String("MOUSE_REGION"), region);
}

void ViewHelper::setTouchRegion(const QRect &rect)
{
    setMouseRegion(QRegion(rect));
}

void ViewHelper::setDefaultRegion()
{
    setMouseRegion(QRegion( (540-240)/2, (960-480)/2, 240, 480));
}

void ViewHelper::hideWindow()
{
    qDebug() << "hide window";
    m_visible = false;
    emit visibleChanged();
    launchApplication(m_currentApp);
}

void ViewHelper::showWindow()
{
    qDebug() << "taskswitcher:showWindow";

    //parse all desktop files
    QVariantMap map;

    QFileInfoList list;
    QDir dir;
    QStringList desktops;
    QStringList runningApps;

    dir.setPath("/usr/share/applications/");
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    dir.setNameFilters(QStringList() << "*.desktop");
    dir.setSorting(QDir::Name);

    list = dir.entryInfoList();

    for (int i=0 ; i<list.size() ; i++)
    {
        desktops << list.at(i).absoluteFilePath();
    }
    QVariantList desktopFiles;

    //1. get a list of all desktop files that have runnable apps
    for (int i = 0 ; i < desktops.count() ; i++)
    {
        MDesktopEntry app(desktops.at(i));

        if (app.isValid() && !app.hidden() && !app.noDisplay())
        {
            map.clear();
            map.insert("name", app.name());
            if (app.icon().startsWith("icon-launcher-") || app.icon().startsWith("icon-l-") || app.icon().startsWith("icons-Applications"))
                map.insert("iconId", QString("image://theme/%1").arg(app.icon()));
            else if (app.icon().startsWith("/"))
                map.insert("iconId", QString("%1").arg(app.icon()));
            else
                map.insert("iconId", QString("/usr/share/icons/hicolor/86x86/apps/%1.png").arg(app.icon()));

            map.insert("exec", app.exec());
            map.insert("desktop", desktops.at(i));			
            
            desktopFiles.prepend(map);
        }
    }
    
    //2. get a list of running processes
    QProcess ps;
    ps.start("ps", QStringList() << "ax" << "-o" << "cmd=");
    ps.waitForFinished();
    QStringList pr = QString(ps.readAllStandardOutput()).split("\n");

    QStringList cmds;
    for (int i=0 ; i<pr.count() ; i++)
    {
        QString cmd;
        if (!(pr.at(i).trimmed().startsWith("[") || pr.at(i).trimmed().contains("invoker"))  && (pr.at(i).trimmed().startsWith("/") || pr.at(i).trimmed().contains("."))) { //filter out kernel and invoker proceses and include android processes
            if (pr.at(i).trimmed().length() > 1) {
                cmd = pr.at(i).trimmed().split(" ")[0];
                if (cmd.contains("/")) {
                    cmd = cmd.mid(cmd.lastIndexOf("/") + 1);
                }
                cmds << cmd;
                //qDebug() << "Processes:" << cmd;
            }
        }
    }
    cmds.removeDuplicates();
    
    QStringList runningDesktopFiles;
    
    //3. loop over processes and compare against desktop files
    foreach(QString cmd, cmds) {
        //qDebug() << "Looking for " << cmd;
        for (int i = 0; i < desktopFiles.count(); i++) {
            //Check if the command is in the desktop with a param, or at the end or is an android style exec
            if (desktopFiles.at(i).toMap()["exec"].toString().split(" ").contains(cmd)  ||
                    desktopFiles.at(i).toMap()["exec"].toString().split(" ").contains("/usr/bin/" + cmd)  ||
                    desktopFiles.at(i).toMap()["exec"].toString().contains(cmd + "/")) { //Android application
                qDebug() << "Found a running app:" << cmd << desktopFiles.at(i).toMap()["exec"].toString() << desktopFiles.at(i).toMap()["desktop"].toString();
                runningDesktopFiles << desktopFiles.at(i).toMap()["desktop"].toString();
                if (!appsDesktopFiles.contains(desktopFiles.at(i).toMap()["desktop"].toString())) {
                    apps.prepend(desktopFiles.at(i));
                    appsDesktopFiles.prepend(desktopFiles.at(i).toMap()["desktop"].toString());
                }
            }
        }
    }

    //4. Remove apps not in cmds
    for (int i = 0 ; i < appsDesktopFiles.count() ; i++)
    {
        if (!runningDesktopFiles.contains(appsDesktopFiles.at(i)))
        {
            qDebug() << "removing closed app " << appsDesktopFiles.at(i);
            appsDesktopFiles.removeAt(i);
            apps.removeAt(i);
        }
    }

    //5. tell QML
    /* Force updating the model in QML */
    m_numberOfApps = 0;
    emit numberOfAppsChanged();

    m_numberOfApps = apps.count();
    emit numberOfAppsChanged();

    if (m_numberOfApps > 1)
    {
        m_currentApp = 1;
        emit currentAppChanged();

        view->showFullScreen();
        m_visible = true;
        emit visibleChanged();
    }
    
    qDebug() << "no apps" << m_numberOfApps << m_currentApp;
}

void ViewHelper::nextApp()
{
    if (m_numberOfApps > 0)
        m_currentApp = (m_currentApp+1) % m_numberOfApps;

    emit currentAppChanged();
}

int ViewHelper::getCurrentApp()
{
    return m_currentApp;
}

int ViewHelper::getNumberOfApps()
{
    return m_numberOfApps;
}

bool ViewHelper::getVisible()
{
    return m_visible;
}

/* Testing */
void ViewHelper::setNumberOfApps(int n)
{
    m_numberOfApps = n;

    if (m_currentApp > m_numberOfApps)
    {
        m_currentApp = m_numberOfApps;
        emit currentAppChanged();
    }

    emit numberOfAppsChanged();
}

void ViewHelper::setCurrentApp(int n)
{
    m_currentApp = n;
    emit currentAppChanged();
}

void ViewHelper::launchApplication(int n)
{
    if (appsDesktopFiles.count() > 0 && n < appsDesktopFiles.count() ) {
        QString desktopFile = appsDesktopFiles.at(n);

        printf("taskswitcher-user: Starting %s\n", qPrintable(desktopFile));

        /* Put this launched app to first of the list */
        appsDesktopFiles.prepend(appsDesktopFiles.takeAt(n));
        apps.prepend(apps.takeAt(n));

        emit _launchApplication(desktopFile);
    }
    
    view->hide();

}

QVariantList ViewHelper::getCurrentApps()
{
    return apps;
}

/*
 * Reboot related stuff
 */
void ViewHelper::requestActionWithRemorse(const QString &action)
{
    m_remorseAction = action;

    if (m_remorseAction == ACTION_REBOOT_REMORSE)
    {
        view->showFullScreen();

        //: Remorse timer text, "Rebooting" in 5 seconds
        //% "Rebooting"
        m_remorseText = qtTrId("reboot-remorse");
        emit remorseTextChanged();

        emit startRemorse();
    }
    else if (m_remorseAction == ACTION_RESTART_LIPSTICK_REMORSE)
    {
        view->showFullScreen();

        //: Remorse timer text, "Restarting Lipstick" in 5 seconds
        //% "Restarting Lipstick"
        m_remorseText = qtTrId("restart-lipstick-remorse");
        emit remorseTextChanged();

        emit startRemorse();
    }
}


void ViewHelper::remorseCancelled()
{
    printf("taskswitcher-user: %s cancelled\n", qPrintable(m_remorseAction));

    view->hide();
}

void ViewHelper::remorseTriggered()
{
    printf("taskswitcher-user: executing %s.\n", qPrintable(m_remorseAction));

    view->hide();

    QProcess proc;

    if (m_remorseAction == ACTION_REBOOT_REMORSE)
    {
        proc.startDetached("/usr/sbin/dsmetool" , QStringList() << QString("--reboot"));
    }
    else if (m_remorseAction == ACTION_RESTART_LIPSTICK_REMORSE)
    {
        proc.startDetached("systemctl" , QStringList() << QString("--user") << QString("restart") << QString("lipstick"));
    }

    QThread::msleep(100);
}
