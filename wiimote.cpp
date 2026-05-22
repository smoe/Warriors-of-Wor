#include "wiimote.h"

WiiMote::WiiMote(cwiid_wiimote_t * h, int number) : rumbleTimer(this)
{
#ifdef HAVE_CWIID
    Q_ASSERT(h);
    handle = h;
    rumbleTimer.setSingleShot(true);
    if(cwiid_command(handle, CWIID_CMD_RPT_MODE, CWIID_RPT_BTN)) {
        qDebug() << Q_FUNC_INFO << "Failed to set flags";
        close();
    } else {
        qDebug() << "Wiimote connected";
        setNumber(number);
    }
    currentButtons = 0;
#else
    Q_UNUSED(h);
    handle = 0;
    rumbleTimer.setSingleShot(true);
    setNumber(number);
    currentButtons = 0;
#endif
}

WiiMote::~WiiMote() {
    close();
}

int WiiMote::defaultFlags() {
#ifdef HAVE_CWIID
    return CWIID_FLAG_MESG_IFC | CWIID_FLAG_NONBLOCK;
#else
    return 0;
#endif
}

void WiiMote::setNumber(int num) {
    myNumber = num;
#ifdef HAVE_CWIID
    int flags = 0;
    if(num==0) flags = CWIID_LED1_ON;
    if(num==1) flags = CWIID_LED2_ON;
    if(num==2) flags = CWIID_LED3_ON;
    if(num==3) flags = CWIID_LED4_ON;
    if(num==4) flags = CWIID_LED2_ON | CWIID_LED3_ON | CWIID_LED4_ON;
    if(num==5) flags = CWIID_LED1_ON | CWIID_LED3_ON | CWIID_LED4_ON;
    if(num==6) flags = CWIID_LED1_ON | CWIID_LED2_ON | CWIID_LED4_ON;
    if(num==7) flags = CWIID_LED1_ON | CWIID_LED2_ON | CWIID_LED3_ON;

    if(cwiid_command(handle, CWIID_CMD_LED, flags)) {
        qDebug() << Q_FUNC_INFO << "Unable to set flags";
        close();
        return;
    }
    shake();
#endif
}

int WiiMote::id() {
#ifdef HAVE_CWIID
    return cwiid_get_id(handle);
#else
    return -1;
#endif
}

int WiiMote::number() {
    return myNumber;
}
void WiiMote::pollMote() {
#ifdef HAVE_CWIID
    if(!handle) return;
    int mesg_count=0;
    union cwiid_mesg *mesg;
    struct timespec t;
    QPoint newDir;
    int newButtons = 0;
    cwiid_get_mesg(handle, &mesg_count, &mesg, &t);
    for (int i=0; i < mesg_count; i++) {
        switch (mesg[i].type) {
        case CWIID_MESG_BTN:
            if(mesg[i].btn_mesg.buttons & CWIID_BTN_LEFT) {
                newDir.setY(1);
            }
            if(mesg[i].btn_mesg.buttons & CWIID_BTN_RIGHT) {
                newDir.setY(-1);
            }
            if(mesg[i].btn_mesg.buttons & CWIID_BTN_UP) {
                newDir.setX(-1);
            }
            if(mesg[i].btn_mesg.buttons & CWIID_BTN_DOWN) {
                newDir.setX(1);
            }
            if(mesg[i].btn_mesg.buttons & CWIID_BTN_2) {
                newButtons |= 1;
            }
            if(mesg[i].btn_mesg.buttons & CWIID_BTN_1) {
                newButtons |= 2;
                qDebug() << Q_FUNC_INFO << "1 pressed";
            }
            if(mesg[i].btn_mesg.buttons & CWIID_BTN_PLUS) {
                newButtons |= 4;
            }
            if(mesg[i].btn_mesg.buttons & CWIID_BTN_MINUS) {
                newButtons |= 8;
            }
            if(newDir != currentDir) {
                currentDir = newDir;
                emit controlInput(currentDir);
            }
            if(newButtons != currentButtons) {
                currentButtons = newButtons;
                emit buttonPressed(currentButtons);
            }
            break;
        case CWIID_MESG_ERROR:
            close();
            break;
        default:
            Q_ASSERT(0);
            break;
        }
    }
#endif
}

void WiiMote::close() {
    qDebug() << Q_FUNC_INFO;
#ifdef HAVE_CWIID
    if(handle)
        cwiid_close(handle);
#endif
    handle = 0;
}

void WiiMote::shake() {
    qDebug() << Q_FUNC_INFO;
#ifdef HAVE_CWIID
    if(cwiid_command(handle, CWIID_CMD_RUMBLE, 1)) {
        qDebug() << Q_FUNC_INFO << "Unable to set flags";
        close();
    } else {
        connect(&rumbleTimer, SIGNAL(timeout()), this, SLOT(stopShaking()), Qt::QueuedConnection);
        rumbleTimer.start(150);
    }
#endif
}

void WiiMote::stopShaking() {
#ifdef HAVE_CWIID
    if(!handle) return;
    if(cwiid_command(handle, CWIID_CMD_RUMBLE, 0)) {
        qDebug() << Q_FUNC_INFO << "Unable to set flags";
        close();
    } else {

    }
#endif
}
