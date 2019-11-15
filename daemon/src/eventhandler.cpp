#include "eventhandler.h"
#include "defaultSettings.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDebug>
#include <QFile>

#define SERVICE_NAME "uk.co.piggz.taskswitcher"

EventHandler::EventHandler() {
    m_worker = new Worker;
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &EventHandler::start, m_worker, &Worker::readKeyboard);
    connect(m_worker, &Worker::altTabPressed, this, &EventHandler::altTabPressed);
    connect(m_worker, &Worker::altReleased, this, &EventHandler::altReleased);
    connect(m_worker, &Worker::ctrlAltBackspacePressed, this, &EventHandler::ctrlAltBackspacePressed);
    connect(m_worker, &Worker::ctrlAltDeletePressed, this, &EventHandler::ctrlAltDeletePressed);
    connect(m_worker, &Worker::finished, this, &EventHandler::workerFinished);
    m_workerThread.start();

    m_deviceName =  new MGConfItem("/uk/co/piggz/taskswitcher/deviceName", this);
    m_lockOrientation =  new MGConfItem("/uk/co/piggz/taskswitcher/lockOrientationOnConnect", this);

    //Start a timer to check for BT keyboard
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &EventHandler::checkForDevice);
    m_timer->start(10000);
}

EventHandler::~EventHandler() {
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_deviceName;
}

void EventHandler::startWorker(const QString &device)
{
    m_timer->stop();

    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
    iface.call("showKeyboardConnectionNotification", true);

    if(m_lockOrientation->value().toBool()){
        iface.call("setOrientationLock", "landscape");
    }

    start(device);
}

void EventHandler::altTabPressed()
{
    qDebug() << "Eventhandler::altTabPressed";

    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());

    if (!m_taskSwitcherVisible)
    {
        /* show taskswitcher and advance one app */
        m_taskSwitcherVisible = true;
        iface.call("nextAppTaskSwitcher");
        iface.call("showTaskSwitcher");
    }
    else
    {
        /* Toggle to next app */
        iface.call("nextAppTaskSwitcher");
    }
}


void EventHandler::altReleased()
{
    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());

    if (m_taskSwitcherVisible)
    {
        m_taskSwitcherVisible = false;
        iface.call("hideTaskSwitcher");
    }
    
}

void EventHandler::ctrlAltBackspacePressed()
{
    qDebug() << "Eventhandler::ctrlAltBackspacePressed";

    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
    iface.call("actionWithRemorse", ACTION_REBOOT_REMORSE);   
}

void EventHandler::ctrlAltDeletePressed()
{
    qDebug() << "Eventhandler::ctrlAltDeletePressed";

    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
    iface.call("actionWithRemorse", ACTION_RESTART_LIPSTICK_REMORSE);   
}

void EventHandler::workerFinished()
{
    qDebug() << "Worker has finished";

    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
    iface.call("showKeyboardConnectionNotification", false);
    iface.call("setOrientationLock", "dynamic");


    m_timer->start();
}

void EventHandler::checkForDevice()
{
    QString deviceName = m_deviceName->value().toString(); //"KBMAG7BK";

    if (deviceName.isEmpty()) {
        deviceName = "Integrated keyboard";
    }
    
    QString deviceFile = getDeviceFile(deviceName);

    qDebug() << "Looking for " << deviceName << "found:" << deviceFile;

    if (deviceFile.startsWith("/dev/input")) {
        startWorker(deviceFile);
    }
}

QString EventHandler::getDeviceFile(const QString &name)
{
    bool name_found = false;
    QString eventstring;
    QString devicepath;

    QFile file("/proc/bus/input/devices");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Unable to open devices list";
        return QString();    
    }
    
    QByteArray contents = file.readAll();
    QTextStream in(&contents);
    while (!in.atEnd()) {
        QString line = in.readLine();

        if (!name_found) {
            if (line.contains(name)) {
                name_found = true;
                qDebug() << "found name";
            }
        } else {
            if (line.contains("event")) {
                eventstring = line.mid(line.indexOf("event"));
                eventstring = eventstring.mid(0, eventstring.indexOf(' '));
                break;
            }
        }
    }

    if (eventstring.startsWith("event")) {
        devicepath = "/dev/input/" + eventstring;
    }

    return devicepath;
}
