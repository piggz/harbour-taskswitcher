TARGET = harbour-taskswitcher-user

CONFIG += sailfishapp link_pkgconfig
PKGCONFIG += sailfishapp mlite5 nemonotifications-qt5
PKGCONFIG += Qt5SystemInfo

QT += dbus network gui-private

DEFINES += "APPVERSION=\\\"$${SPECVERSION}\\\""

message($${DEFINES})

systemd_services.path = /usr/lib/systemd/user/
systemd_services.files = harbour-taskswitcher-user.service

INSTALLS += target \
            systemd_services

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

SOURCES += \
    src/tohkbd2user.cpp \
    src/userdaemon.cpp \
    src/viewhelper.cpp \
    src/applauncher.cpp \
    src/screenshot.cpp

HEADERS += \
    src/userdaemon.h \
    src/viewhelper.h \
    src/applauncher.h \
    src/screenshot.h

OTHER_FILES += \
    qml/taskswitcher.qml

DISTFILES += \
    harbour-taskswitcher-user.desktop \
    settings/TaskswitcherAppSettings.qml \
    settings/harbour-taskswitcher.json

# Settings
settings_json.files = $$PWD/settings/harbour-taskswitcher.json
settings_json.path = /usr/share/jolla-settings/entries/
INSTALLS += settings_json

settings_qml.files = $$PWD/settings/*.qml
settings_qml.path = /usr/share/$${TARGET}/settings/
INSTALLS += settings_qml
