import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.DBus 2.0
import QtQml 2.2
import Nemo.Configuration 1.0
import Sailfish.Pickers 1.0

Page {
    id: root

    property bool serviceRunning
    property bool userserviceRunning
    property bool taskswitcherAutostart
    property bool ready: true

    ConfigurationValue {
        id:cfgDeviceName
        key: "/uk/co/piggz/taskswitcher/deviceName"
        defaultValue: ""
    }

    ConfigurationValue {
        id:cfgLockOrientation
        key: "/uk/co/piggz/taskswitcher/lockOrientationOnConnect"
        defaultValue: false
    }

    DBusInterface {
        id: taskswitcherService

        bus: DBus.SessionBus
        service: "org.freedesktop.systemd1"
        iface: "org.freedesktop.systemd1.Unit"
        signalsEnabled: true

        function updateProperties() {
            var status = taskswitcherService.getProperty("ActiveState")
            taskswitcherSystemdStatus.status = status
            if (path !== "") {
                root.serviceRunning = status === "active"
            } else {
                root.serviceRunning = false
            }
        }
        onPathChanged: updateProperties()
    }

    DBusInterface {
        id: taskswitcherUserService

        bus: DBus.SessionBus
        service: "org.freedesktop.systemd1"
        iface: "org.freedesktop.systemd1.Unit"
        signalsEnabled: true

        function updateProperties() {
            var status = taskswitcherUserService.getProperty("ActiveState")
            taskswitcherSystemdStatus.status = status
            if (path !== "") {
                root.userserviceRunning = status === "active"
            } else {
                root.userserviceRunning = false
            }
        }
        onPathChanged: updateProperties()
    }

    DBusInterface {
        id: manager

        bus: DBus.SessionBus
        service: "org.freedesktop.systemd1"
        path: "/org/freedesktop/systemd1"
        iface: "org.freedesktop.systemd1.Manager"
        signalsEnabled: true

        signal unitNew(string name)
        onUnitNew: {
            if ((name == "harbour-taskswitcher.service") || (name == "harbour-taskswitcher-user.service")) {
                pathUpdateTimer.start()
            }
        }

        signal unitRemoved(string name)
        onUnitRemoved: {
            if ((name == "harbour-taskswitcher.service") || (name == "harbour-taskswitcher-user.service")) {
                taskswitcherService.path = ""
                taskswitcherUserService.path = ""
                pathUpdateTimer.stop()
            }
        }

        signal unitFilesChanged()
        onUnitFilesChanged: {
            updateAutostart()
        }

        Component.onCompleted: {
            updatePath()
            updateAutostart()
        }
        function updateAutostart() {
            manager.typedCall("GetUnitFileState", [{"type": "s", "value": "harbour-taskswitcher.service"}],
                              function(state) {
                                  console.log(state)
                                  if (state !== "disabled" && state !== "invalid") {
                                      root.taskswitcherAutostart = true
                                      quitOnCloseUi.value = false
                                  } else {
                                      root.taskswitcherAutostart = false
                                  }
                              },
                              function() {
                                  root.taskswitcherAutostart = false
                              })
            manager.typedCall("GetUnitFileState", [{"type": "s", "value": "harbour-taskswitcher-user.service"}],
                              function(state) {
                                  console.log(state)
                                  if (state !== "disabled" && state !== "invalid") {
                                      root.taskswitcherAutostart = true
                                      quitOnCloseUi.value = false
                                  } else {
                                      root.taskswitcherAutostart = false
                                  }
                              },
                              function() {
                                  root.taskswitcherAutostart = false
                              })
        }

        function setAutostart(isAutostart) {
            if(isAutostart)
                enableTaskswitcherUnits()
            else
                disableTaskswitcherUnits()
        }

        function enableTaskswitcherUnits() {
            manager.typedCall( "EnableUnitFiles",[{"type":"as","value":["harbour-taskswitcher.service"]},
                                                  {"type":"b","value":false},
                                                  {"type":"b","value":false}],
                              function(carries_install_info,changes){
                                  root.taskswitcherAutostart = true
                                  console.log(carries_install_info,changes)
                              },
                              function() {
                                  console.log("Enabling error")
                              }
                              )
            manager.typedCall( "EnableUnitFiles",[{"type":"as","value":["harbour-taskswitcher-user.service"]},
                                                  {"type":"b","value":false},
                                                  {"type":"b","value":false}],
                              function(carries_install_info,changes){
                                  root.taskswitcherAutostart = true
                                  console.log(carries_install_info,changes)
                              },
                              function() {
                                  console.log("Enabling error")
                              }
                              )
   cfgDeviceName     }

        function disableTaskswitcherUnits() {
            manager.typedCall( "DisableUnitFiles",[{"type":"as","value":["harbour-taskswitcher.service"]},
                                                   {"type":"b","value":false}],
                              function(changes){
                                  root.taskswitcherAutostart = false
                                  console.log(changes)
                              },
                              function() {
                                  console.log("Disabling error")
                              }
                              )
            manager.typedCall( "DisableUnitFiles",[{"type":"as","value":["harbour-taskswitcher-user.service"]},
                                                   {"type":"b","value":false}],
                              function(changes){
                                  root.taskswitcherAutostart = false
                                  console.log(changes)
                              },
                              function() {
                                  console.log("Disabling error")
                              }
                              )
        }

        function startTaskswitcherUnits() {
            manager.typedCall( "StartUnit",[{"type":"s","value":"harbour-taskswitcher.service"},
                                            {"type":"s","value":"fail"}],
                              function(job) {
                                  console.log("job started - ", job)
                                  taskswitcherService.updateProperties()
                                  runningUpdateTimer.start()
                              },
                              function() {
                                  console.log("job started failure")
                              })
            manager.typedCall( "StartUnit",[{"type":"s","value":"harbour-taskswitcher-user.service"},
                                            {"type":"s","value":"fail"}],
                              function(job) {
                                  console.log("job started - ", job)
                                  taskswitcherUserService.updateProperties()
                                  runningUpdateTimer.start()
                              },
                              function() {
                                  console.log("job started failure")
                              })
        }

        function stopTaskswitcherUnits() {
            manager.typedCall( "StopUnit",[{"type":"s","value":"harbour-taskswitcher.service"},
                                           {"type":"s","value":"replace"}],
                              function(job) {
                                  console.log("job stopped - ", job)
                                  taskswitcherService.updateProperties()
                              },
                              function() {
                                  console.log("job stopped failure")
                              })
            manager.typedCall( "StopUnit",[{"type":"s","value":"harbour-taskswitcher-user.service"},
                                           {"type":"s","value":"replace"}],
                              function(job) {
                                  console.log("job stopped - ", job)
                                  taskswitcherUserService.updateProperties()
                              },
                              function() {
                                  console.log("job stopped failure")
                              })
        }

        function updatePath() {
            manager.typedCall("GetUnit", [{ "type": "s", "value": "harbour-taskswitcher.service"}], function(unit) {
                taskswitcherService.path = unit
            }, function() {
                taskswitcherService.path = ""
            })

            manager.typedCall("GetUnit", [{ "type": "s", "value": "harbour-taskswitcher-user.service"}], function(unit) {
                taskswitcherUserService.path = unit
            }, function() {
                taskswitcherUserService.path = ""
            })
        }
    }

    Timer {
        // starting and stopping can result in lots of property changes
        id: runningUpdateTimer
        interval: 1000
        repeat: true
        onTriggered:{
            taskswitcherService.updateProperties()
            taskswitcherUserService.updateProperties()
        }
    }

    Timer {
        // stopping service can result in unit appearing and disappering, for some reason.
        id: pathUpdateTimer
        interval: 100
        onTriggered: manager.updatePath()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingMedium
        width: parent.width

        Column {
            id:content
            width:parent.width

            PageHeader {
                id: header
                title: qsTr("Taskswitcher settings")
            }

            TextSwitch {
                id: autostart
                //% "Start taskswitcher on bootup"
                text: qsTr("Start Taskswitcher on bootup")
                description: qsTr("When this is off, you won't get Taskswitcher on boot")
                enabled: root.ready
                automaticCheck: false
                checked: root.taskswitcherAutostart
                onClicked: {
                    manager.setAutostart(!checked)
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*Theme.horizontalPageMargin
                text: qsTr("Start/stop Taskswitcher daemons. Stopping Taskswitcher will stop alt-tab handling")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
            }

            Label {
                id: taskswitcherSystemdStatus
                property string status: "invalid"
                x: Theme.horizontalPageMargin
                width: parent.width - 2*Theme.horizontalPageMargin
                text: qsTr("Taskswitcher current status") + " - " + status
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingLarge

                Button {
                    enabled: root.ready && !root.serviceRunning
                    text: qsTr("Start daemons")
                    width: (content.width - 2*Theme.horizontalPageMargin - parent.spacing) / 2
                    onClicked: manager.startTaskswitcherUnits()
                }

                Button {
                    enabled: root.ready && root.serviceRunning
                    //% "Stop"
                    text: qsTr("Stop daemosn")
                    width: (content.width - 2*Theme.horizontalPageMargin - parent.spacing) / 2
                    onClicked: manager.stopTaskswitcherUnits()
                }
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }

            TextField {
                id: fldKeybord
                width: parent.width - 2 * x
                height: Theme.itemSizeMedium
                x:Theme.horizontalPageMargin
                label: qsTr("Keyboard Device")

                AddAnimation{}
                placeholderText: label
                text: cfgDeviceName.value
                onTextChanged: {
                    cfgDeviceName.value = text
                }
            }

            TextSwitch {
                id: fldOrientation
                text: qsTr("Lock orientation")
                description: qsTr("Lock orientation when keyboard connects.  Useful for BT keyboards.")
                automaticCheck: false
                checked: cfgLockOrientation.value
                onClicked: {
                    cfgLockOrientation.value = !checked
                }
            }
        }
    }
}
