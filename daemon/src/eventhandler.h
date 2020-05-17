#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <MGConfItem>

#include "worker.h"

class EventHandler : public QObject
{
    Q_OBJECT
    
public:
    EventHandler();
    ~EventHandler();
    
private:
    Q_SLOT void altTabPressed();
    Q_SLOT void altReleased();
    Q_SLOT void ctrlAltBackspacePressed();
    Q_SLOT void ctrlAltDeletePressed();
    Q_SLOT void keyboardOut();
    Q_SLOT void keyboardIn();
    
    Q_SLOT void workerFinished(); //probably the device disappeared
    Q_SLOT bool checkForDevice(const QString &deviceName, Worker *&worker, QThread* thread);
    Q_SLOT void checkForDevices();

    Q_SLOT void cleanupWorker(QObject *worker);
    Q_SIGNAL void start();

    QThread m_workerThread;
    QThread m_workerThread2;
    Worker *m_worker = nullptr;
    Worker *m_worker2 = nullptr;
    QTimer *m_timer = nullptr;
    MGConfItem *m_deviceName;
    MGConfItem *m_deviceNameSecondary;
    MGConfItem *m_lockOrientation;
    MGConfItem *m_lockOrientationSlide;
    MGConfItem *m_orientation;

    bool m_taskSwitcherVisible  = false;
    QString getDeviceFile(const QString &name);

};

#endif
