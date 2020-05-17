#include "eventhandler.h"
#include "defaultSettings.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDebug>
#include <QFile>

#define SERVICE_NAME "uk.co.piggz.taskswitcher"

EventHandler::EventHandler()
{
    m_deviceName =  new MGConfItem("/uk/co/piggz/taskswitcher/deviceName", this);
    m_deviceNameSecondary =  new MGConfItem("/uk/co/piggz/taskswitcher/deviceNameSecondary", this);
    m_lockOrientation =  new MGConfItem("/uk/co/piggz/taskswitcher/lockOrientationOnConnect", this);
    m_lockOrientationSlide =  new MGConfItem("/uk/co/piggz/taskswitcher/lockOrientationOnSlide", this);
    m_orientation =  new MGConfItem("/uk/co/piggz/taskswitcher/lockOrientation", this);

    //Start a timer to check for BT keyboard
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &EventHandler::checkForDevices);
    m_timer->start(10000);
}

EventHandler::~EventHandler() {
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_deviceName;
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

void EventHandler::keyboardOut()
{
    qDebug() << "Keyboard out" << m_orientation->value("dynamic").toString();
    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
    iface.call("setOrientationLock", m_orientation->value("dynamic").toString());
}

void EventHandler::keyboardIn()
{
    qDebug() << "Keyboard in";
    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
    iface.call("setOrientationLock", "dynamic");
}

void EventHandler::workerFinished()
{
    qDebug() << "Worker has finished";

    QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
    iface.call("showKeyboardConnectionNotification", false);
    iface.call("setOrientationLock", "dynamic");

    m_timer->start();
}

bool EventHandler::checkForDevice(const QString &deviceName, Worker *&worker, QThread* thread)
{
    QString deviceFile = getDeviceFile(deviceName);
    qDebug() << "Looking for " << deviceName << "found:" << deviceFile;

    if (deviceFile.startsWith("/dev/input") && worker == nullptr) {
        worker = new Worker(deviceFile);
        worker->moveToThread(thread);
        connect(thread, &QThread::finished, worker, &QObject::deleteLater);
        connect(worker, &Worker::destroyed, this, &EventHandler::cleanupWorker);

        connect(this, &EventHandler::start, worker, &Worker::start);
        connect(worker, &Worker::altTabPressed, this, &EventHandler::altTabPressed);
        connect(worker, &Worker::altReleased, this, &EventHandler::altReleased);
        connect(worker, &Worker::ctrlAltBackspacePressed, this, &EventHandler::ctrlAltBackspacePressed);
        connect(worker, &Worker::ctrlAltDeletePressed, this, &EventHandler::ctrlAltDeletePressed);
        connect(worker, &Worker::keyboardIn, this, &EventHandler::keyboardIn);
        connect(worker, &Worker::keyboardOut, this, &EventHandler::keyboardOut);
        connect(worker, &Worker::finished, this, &EventHandler::workerFinished);

        thread->start();
        start();
        return true;
    }
    return false;
}

void EventHandler::checkForDevices()
{
    QString deviceName = m_deviceName->value().toString();
    QString deviceNameSecondary = m_deviceNameSecondary->value().toString();

    if ((!deviceName.isEmpty() || !(deviceName == "None")) && m_worker == nullptr) {
        if (checkForDevice(deviceName, m_worker, &m_workerThread)) {
            QDBusInterface iface(SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
            iface.call("showKeyboardConnectionNotification", true);

            if(m_lockOrientation->value().toBool()){
                iface.call("setOrientationLock", m_orientation->value("dynamic").toString());
            }
        }
    }

    //Secondary device
    if ((!deviceNameSecondary.isEmpty() || !(deviceNameSecondary == "None")) && m_worker2 == nullptr) {
        checkForDevice(deviceNameSecondary, m_worker2, &m_workerThread2);
    }
}

void EventHandler::cleanupWorker(QObject *worker)
{
    worker = nullptr;
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
