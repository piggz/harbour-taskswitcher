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
    property bool taskswitcherUserAutostart
    property bool ready: false

    ConfigurationValue {
        id:cfgDeviceName
        key: "/uk/co/piggz/taskswitcher/deviceName"
        defaultValue: ""
    }

    ConfigurationValue {
        id:cfgDeviceNameSecondary
        key: "/uk/co/piggz/taskswitcher/deviceNameSecondary"
        defaultValue: ""
    }

    ConfigurationValue {
        id:cfgLockOrientation
        key: "/uk/co/piggz/taskswitcher/lockOrientationOnConnect"
        defaultValue: false
    }

    ConfigurationValue {
        id:cfgLockOrientationSlide
        key: "/uk/co/piggz/taskswitcher/lockOrientationOnSlide"
        defaultValue: false
    }

    ConfigurationValue {
        id:cfgOrientation
        key: "/uk/co/piggz/taskswitcher/lockOrientation"
        defaultValue: "dynamic"
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
                                      root.taskswitcherUserAutostart = true
                                  } else {
                                      root.taskswitcherUserAutostart = false
                                  }
                              },
                              function() {
                                  root.taskswitcherUserAutostart = false
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
                                  root.taskswitcherUserAutostart = true
                                  console.log(carries_install_info,changes)
                              },
                              function() {
                                  console.log("Enabling user error")
                              }
                              )
        }

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
                                  root.taskswitcherUserAutostart = false
                                  console.log(changes)
                              },
                              function() {
                                  console.log("Disabling user error")
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
                                  console.log("job started user failure")
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
                                  console.log("job stopped user failure")
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
        interval: 200
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
                checked: root.taskswitcherAutostart && root.taskswitcherUserAutostart
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
                    enabled: root.ready && (!root.serviceRunning || !root.userserviceRunning)
                    text: qsTr("Start daemons")
                    width: (content.width - 2*Theme.horizontalPageMargin - parent.spacing) / 2
                    onClicked: manager.startTaskswitcherUnits()
                }

                Button {
                    enabled: root.ready && (root.serviceRunning || root.userserviceRunning)
                    //% "Stop"
                    text: qsTr("Stop daemons")
                    width: (content.width - 2*Theme.horizontalPageMargin - parent.spacing) / 2
                    onClicked: manager.stopTaskswitcherUnits()
                }
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }

            ComboBox {
                id: cboKeyboard
                label: qsTr("Primary Keyboard Device")

                menu: ContextMenu {
                    id: mnuKeyboard
                }

                onCurrentIndexChanged: {
                    value = menu.children[currentIndex].text
                    cfgDeviceName.value = value;
                }
            }

            ComboBox {
                id: cboKeyboardSecondary
                label: qsTr("Secondary Keyboard Device")

                menu: ContextMenu {
                    id: mnuKeyboardSecondary
                }

                onCurrentIndexChanged: {
                    value = menu.children[currentIndex].text
                    cfgDeviceNameSecondary.value = value;
                }
            }

            Component {
            id: menuItemComp
            MenuItem {}
            }

            TextSwitch {
                id: fldOrientation
                text: qsTr("Lock Orientation")
                description: qsTr("Lock orientation when keyboard connects.  Useful for BT keyboards.")
                automaticCheck: false
                checked: cfgLockOrientation.value
                onClicked: {
                    cfgLockOrientation.value = !checked
                }
            }

            TextSwitch {
                id: fldOrientationSlide
                text: qsTr("Lock Orientation on Slide")
                description: qsTr("Lock orientation when keyboard slides out.")
                automaticCheck: false
                checked: cfgLockOrientationSlide.value
                onClicked: {
                    cfgLockOrientationSlide.value = !checked
                }
            }

            ComboBox {
                id: cboOrientation
                label: qsTr("Orientation")

                menu: ContextMenu {
                    id: mnu
                    MenuItem { text: "dynamic" }
                    MenuItem { text: "landscape" }
                    MenuItem { text: "landscape-inverted" }
                    MenuItem { text: "portrait" }
                    MenuItem { text: "portrait-inverted" }
                }

                onValueChanged: {
                    if (ready){
                        cfgOrientation.value = value;
                    }
                }

                function setIndexOf(val) {
                    console.log("Looking for", val);
                    for (var i = 0; i < mnu.children.length; i++) {
                        console.log(i, mnu.children[i].text);
                        if (mnu.children[i].text == val) {
                            currentIndex = i;
                        }
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        readDevices()
        cboOrientation.setIndexOf(cfgOrientation.value)
        cboKeyboard.value = cfgDeviceName.value
        cboKeyboardSecondary.value = cfgDeviceNameSecondary.value
        ready = true;
    }

    function readDevices()
    {
        menuItemComp.createObject(mnuKeyboard._contentColumn, {"text" : "None"})
        menuItemComp.createObject(mnuKeyboardSecondary._contentColumn, {"text" : "None"})
        var xhr = new XMLHttpRequest;
        xhr.open("GET", "/proc/bus/input/devices");
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                var response = xhr.responseText;
                var lines = response.split('\n')

                for (var i = 0; i < lines.length; i++){
                    if (lines[i].substr(0, 8) === "N: Name=") {
                        var devName = lines[i].substring(lines[i].indexOf("\"") + 1, lines[i].lastIndexOf("\""))
                        var newMenuItem = menuItemComp.createObject(mnuKeyboard._contentColumn, {"text" : devName})
                        var newMenuItemS = menuItemComp.createObject(mnuKeyboardSecondary._contentColumn, {"text" : devName})
                    }
                }
            }
        };
        xhr.send();

    }
}
