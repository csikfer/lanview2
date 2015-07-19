#ifndef IMAGEDRV_H
#define IMAGEDRV_H

#include "lv2g.h"
#include <QGraphicsView>
#include <QGraphicsScene>

class LV2GSHARED_EXPORT cImageDrawWidget : public QGraphicsView
{
public:
    cImageDrawWidget(QWidget *_par = NULL);

protected:
    QImage          image;
};

#endif // IMAGEDRV_H
