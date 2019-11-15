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
    void startWorker(const QString &device);

    Q_SLOT void altTabPressed();
    Q_SLOT void altReleased();
    Q_SLOT void ctrlAltBackspacePressed();
    Q_SLOT void ctrlAltDeletePressed();        
    
    Q_SLOT void workerFinished(); //probably the device disappeared
    Q_SLOT void checkForDevice();

    Q_SIGNAL void start(const QString &device);

    QThread m_workerThread;
    Worker *m_worker;
    QTimer *m_timer;
    MGConfItem *m_deviceName;
    MGConfItem *m_lockOrientation;

    bool m_taskSwitcherVisible  = false;
    QString getDeviceFile(const QString &name);

};

#endif
