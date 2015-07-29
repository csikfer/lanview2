#include "imagedrv.h"

/// A GraphicsItem pointere alapján visszaadja a cImagePolygonWidget pointert (view).
static cImagePolygonWidget * pImagePolygonWidget(QGraphicsItem *p)
{
    QList<QGraphicsView *> views = p->scene()->views();
    if (views.size() != 1) EXCEPTION(ENOTSUPP);
    cImagePolygonWidget * pView = dynamic_cast<cImagePolygonWidget *>(views.first());
    return pView;
}

// ****

cGrPixmapItem::cGrPixmapItem(QPixmap _pm)
    : QGraphicsPixmapItem(_pm)
{
    ;
}

void  cGrPixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    cImagePolygonWidget *p = pImagePolygonWidget(this);
    if (p->pPolygonItem == NULL) {  // Ha még nincs polygonunk
        // Csinálunk egyet: háromszög a klikk helyén
        QPointF pos = event->scenePos();
        QPolygonF pol;
        qreal  s = cGrNodeItem::nodeSize *2;
        pol << pos + QPointF( 0, s);
        pol << pos + QPointF( s,-s);
        pol << pos + QPointF(-s,-s);
        p->setPolygon(pol);
        emit p->modifiedPolygon(pol);
    }
}

// ****

qreal cGrPolygonItem::polygonOpacity = 0.5;

cGrPolygonItem::cGrPolygonItem(const QPolygonF& _pol, QGraphicsItem * _par)
    : QGraphicsPolygonItem(_pol, _par)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    //setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
    setOpacity(polygonOpacity);
    editing = dise = false;
}

// ****

void cGrPolygonItem::editBegin()
{
    QList<QGraphicsItem *> childList = childItems();
    if (childList.size()) EXCEPTION(EPROGFAIL);
    editing = dise = true;
    QPointF   pt  = pos();
    QPolygonF pol = polygon();
    pol.translate(pt);
    setPolygon(pol);
    setPos(QPointF(0,0));
    setFlag(QGraphicsItem::ItemIsMovable, false);
    int index = 0;
    int maxix = pol.size() -1;
    cGrNodeItem::eNodeType nt = cGrNodeItem::NT_FIRST;
    cGrNodeItem *pItem;
    foreach (pt, pol) {
        pItem = new cGrNodeItem(pt, this, nt, index);
        if (index == 0) pFirstItem = pItem;
        index++;
        if (index == maxix) {       // Következő index az utolsó pontja a polygon-nak
            nt = cGrNodeItem::NT_LAST;
        }
        else {
            nt = cGrNodeItem::NT_OTHER;
        }
    }
    pLastItem = pItem;
    pt = (pol.first() + pol.last()) /2;
    pNewItem = new cGrNodeItem(pt, this, cGrNodeItem::NT_NEW, -1);
    dise = false;
}

void cGrPolygonItem::editEnd()
{
    if (!editing) return;
    QList<QGraphicsItem *> childList = childItems();
    if (childList.isEmpty()) EXCEPTION(EPROGFAIL);
    foreach (QGraphicsItem * pt, childList) {
        delete pt;
    }
    pLastItem = NULL;
    pFirstItem = NULL;
    pNewItem = NULL;
    setFlag(QGraphicsItem::ItemIsMovable, true);
    editing = false;
}

void cGrPolygonItem::modPolygon(QPolygonF _pol)
{
    editEnd();
    dise = true;
    setPolygon(_pol);
    setPos(QPointF(0,0));
    dise = false;
}

QVariant cGrPolygonItem::itemChange(GraphicsItemChange change, const QVariant & value)
{
    if (change == ItemPositionChange) {
        // value is the new position.
        QPointF newPos = value.toPointF();
        if (editing) {
            if (!dise) {    // Hacsak nem mi nulláztuk a pozíciót.
                EXCEPTION(ENOTSUPP);   // Edit módban nincs mozgatás.
            }
        }
        else {
            emit pImagePolygonWidget(this)->modifiedPolygon(polygon().translated(newPos));
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void cGrPolygonItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    (void)event;
    if (!editing) {  // Editor módba váltunk
        editBegin();
    }
    else {
        editEnd();
    }
}

// ****

const qreal cGrNodeItem::nodeSize = 4;
QList<QBrush>    cGrNodeItem::brushList;
QList<QPen>      cGrNodeItem::penList;

void cGrNodeItem::static_init()
{
    if (brushList.isEmpty()) {
        QPen    pen;
        pen.setWidth(1);
        // NT_FIRST
        brushList << QBrush(Qt::black);
        pen.setColor(Qt::white);
        penList << pen;
        // NT_OTHER
        brushList << QBrush(Qt::gray);
        pen.setColor(Qt::gray);
        penList << pen;
        // NT_LAST
        brushList << QBrush(Qt::white);
        pen.setColor(Qt::black);
        penList << pen;
        // NT_NEW
        brushList << QBrush(Qt::lightGray);
        pen.setColor(Qt::gray);
        penList << pen;
    }
}

cGrNodeItem::cGrNodeItem(const QPointF& p, QGraphicsItem * _par, enum eNodeType _t, int _ix)
    : QGraphicsEllipseItem( - cGrNodeItem::nodeSize / 2, - cGrNodeItem::nodeSize / 2, cGrNodeItem::nodeSize, cGrNodeItem::nodeSize, _par)
{
    dise = true;
    static_init();
    pointIx  = _ix;
    setPos(p);
    setFlag(QGraphicsItem::ItemIsMovable);
    // setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
    setBrush(brushList[_t]);
    setPen(penList[_t]);
    dise = false;
}

QVariant cGrNodeItem::itemChange(GraphicsItemChange change, const QVariant & value)
{
    if (change == ItemPositionChange && !dise) {
        QPointF newPos = value.toPointF();
        cGrPolygonItem * par = dynamic_cast<cGrPolygonItem *>(parentItem());
        QPolygonF polygon = par->polygon();
        if (pointIx < 0) {      // New point
            // last item: NT_LAST -> NT_OTHER
            par->pLastItem->setBrush(brushList[NT_OTHER]);
            par->pLastItem->setPen(penList[NT_OTHER]);
            // Item: NT_NEW -> NT_Last
            pointIx = polygon.size();
            polygon << newPos;  // Add to polygon
            setBrush(brushList[NT_LAST]);
            setPen(penList[NT_LAST]);
            par->pLastItem = this;
            // (new) new item
            QPointF newPoint = (polygon.first() + polygon.last()) /2;
            par->pNewItem = new cGrNodeItem(newPoint, par, cGrNodeItem::NT_NEW, -1);
        }
        else if (pointIx < polygon.size()) {
            polygon[pointIx] = newPos;
            if (pointIx == 0 || pointIx == (polygon.size() -1)) {
                QPointF newItemNewPos = (polygon.first() + polygon.last()) /2;
                par->pNewItem->dise = true;
                par->pNewItem->setPos(newItemNewPos);
                par->pNewItem->dise = false;
            }
        }
        else {
            EXCEPTION(EPROGFAIL);
        }
        par->setPolygon(polygon);
        emit pImagePolygonWidget(this)->modifiedPolygon(polygon);
    }
    return QGraphicsItem::itemChange(change, value);
}

// ****

cImagePolygonWidget::cImagePolygonWidget(bool _e, QWidget * _par)
    : QGraphicsView(_par)
    , editable(_e)
{
    pImageItem   = NULL;
    pTextItem    = NULL;
    pPolygonItem = NULL;
    pen.setColor(Qt::blue);
    pen.setWidth(1);
    brush = QBrush(Qt::red);
    setScene(new QGraphicsScene(this));
}

bool cImagePolygonWidget::setImage(const cImage& __o, const QString& __t)
{
    QString name = __o.getName();
    if (!__o.dataIsPic()) EXCEPTION(EDATA, -1, trUtf8("Image %1 is not picture.").arg(name));
    if (__o.hashIsNull()) EXCEPTION(EDATA, -1, trUtf8("Image %1 hash is NULL.").arg(name));
    QByteArray hash = __o.getHash();
    if (imageHash == hash) return false;        // Nincs változás
    const char * _type = __o._getType();
    QByteArray   _data = __o.getImage();
    if (!picture.loadFromData(_data, _type)) EXCEPTION(EDATA, -1, trUtf8("Image %1 convert to QPixmap failed.").arg(name));
    QString title = __t;
    if (title.isEmpty()) title = __o.getNoteOrName();
    setWindowTitle(title);
    imageHash = hash;
    pDelete(pTextItem);
    if (pImageItem == NULL) {
        pImageItem = new cGrPixmapItem(picture);
        scene()->addItem(pImageItem);
    }
    else {
        pImageItem->setPixmap(picture);
    }
    return true;
}

cImagePolygonWidget &cImagePolygonWidget::setText(const QString& _txt)
{
    pDelete(pImageItem);
    pDelete(pPolygonItem);
    if (pTextItem == NULL) {
        pTextItem = scene()->addText(_txt);
    }
    else {
        pTextItem->setPlainText(_txt);
    }
    return *this;
}

cImagePolygonWidget& cImagePolygonWidget::setPolygon(const QPolygonF& pol)
{
    if (pPolygonItem == NULL) {
        pPolygonItem = new cGrPolygonItem(pol);
        pPolygonItem->setBrush(brush);
        pPolygonItem->setPen(pen);
        scene()->addItem(pPolygonItem);
    }
    else {
        pPolygonItem->modPolygon(pol);
    }
    return *this;
}

cImagePolygonWidget& cImagePolygonWidget::setPolygon(const tPolygonF& _pol)
{
    QPolygonF pol;
    foreach (QPointF p, _pol) {
        pol << p;
    }
    return setPolygon(pol);
}

// SLOTS

/// A nagyítás mértékének a beállítása
void cImagePolygonWidget::setScale(qreal s)
{
    // _DBGFN() << QString(" s = %1, act factor (dX) = %2 ******").arg(s).arg(transform().m11()) << endl;
    qreal f  = transform().m11();   // Jelenlegi nagyítás
    qreal rf = s / f;               // relatív nagyítás
    scale(rf, rf);
}

