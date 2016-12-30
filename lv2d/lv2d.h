#ifndef LV2D_H
#define LV2D_H

#include "QtCore"

#include "lanview.h"
#include "syscronthread.h"

#define APPNAME "lv2d"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2d;

class lv2d : public lanView {
    Q_OBJECT
public:
    lv2d();
    ~lv2d();
    virtual void setup(eTristate _tr = TS_NULL);
    /// A futó démonok száma
    int     runingCnt;
};


#endif // LV2D_H
