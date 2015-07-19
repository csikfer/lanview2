#include "imagedrv.h"

cImageDrawWidget::cImageDrawWidget(QWidget * _par) : QGraphicsView()
{
    setScene(new QGraphicsScene(this));
}
