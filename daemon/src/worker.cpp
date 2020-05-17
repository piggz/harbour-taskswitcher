#include "worker.h"

#include <sys/types.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <QDebug>

#define KEY_RELEASE 0
#define KEY_PRESS 1
//#define SW_KEYPAD_SLIDE		0x0a

typedef struct input_event input_event;

/*
*
 * Opens the keyboard device file
 *
 * @param  deviceFile   the path to the keyboard device file
 * @return              the file descriptor on success, error code on failure
 */
static int openKeyboardDeviceFile(char *deviceFile) {
    int kbd_fd = open(deviceFile, O_RDONLY);
    if (kbd_fd == -1) {
        qDebug() <<  strerror(errno);
    }

    return kbd_fd;
}

/**
 * Converts a key code to an ascii character. See linux/input.h for more
 * information
 *
 * @param  code the key code to convert
 * @return      the corresponding ascii character
 */
char *getKeyText(uint16_t code, uint8_t shift_pressed) {
    char **arr;
    if (shift_pressed != 0) {
        arr = shift_key_names;
    } else {
        arr = key_names;
    }

    if (code < ARRAY_SIZE(key_names)) {
        return arr[code];
    } else {
        return UNKNOWN_KEY;
    }
}

Worker::Worker(const QString &device) : m_device(device)
{
}

void Worker::start()
{
    qDebug() << "opening device:" << m_device;
    int kbd_fd = openKeyboardDeviceFile(m_device.toLocal8Bit().data());
    qDebug() << "fd:" << kbd_fd;

    if (kbd_fd <= 0) {
        qDebug() << "Unable to open device";
        emit finished();
        return;
    }

    uint8_t shift_pressed = 0;
    input_event event;
    bool alt_pressed = false;
    bool ctrl_pressed = false;
    
    while (read(kbd_fd, &event, sizeof(input_event)) > 0) {
        if (event.type == EV_KEY) {
            if (event.value == KEY_PRESS) {
                char *name = getKeyText(event.code, shift_pressed);
                //qDebug() << "Pressed" << name;
                if (strcmp(name, "<LAlt>") == 0 || strcmp(name, "<RAlt>") == 0) {
                    alt_pressed = true;
                } else if (strcmp(name, "<LCtrl>") == 0 || strcmp(name, "<RCtrl>") == 0) {
                    ctrl_pressed = true;
                } else if (strcmp(name, "<Tab>") == 0) {
                    if (alt_pressed) {
                        qDebug() << "alt-tab pressed";
                        emit altTabPressed();
                    }
                } else if (strcmp(name, "<Backspace>") == 0) {
                    if (alt_pressed && ctrl_pressed) {
                        qDebug() << "ctrl-alt-backspace pressed";
                        emit ctrlAltBackspacePressed();
                    }
                } else if (strcmp(name, "<Delete>") == 0) {
                    if (alt_pressed && ctrl_pressed) {
                        qDebug() << "ctrl-alt-delete pressed";
                        emit ctrlAltDeletePressed();
                    }
                }
            } else if (event.value == KEY_RELEASE) {
                char *name = getKeyText(event.code, shift_pressed);
                //qDebug() << "Released" << name;
                if (strcmp(name, "<LAlt>") == 0 || strcmp(name, "<RAlt>") == 0) {
                    alt_pressed = false;
                    emit altReleased();
                } if (strcmp(name, "<LCtrl>") == 0 || strcmp(name, "<RCtrl>") == 0) {
                    ctrl_pressed = false	;
                }
            }
        } else if (event.type == EV_SW) {
            //qDebug() << "Switch:" << event.code << event.value;
            if (event.code == SW_KEYPAD_SLIDE) {
                if (event.value == KEY_PRESS) {
                    emit keyboardIn();
                } else if (event.value == KEY_RELEASE) {
                    emit keyboardOut();
                }
            }
        }
    }

    close(kbd_fd);
    emit finished();
}
