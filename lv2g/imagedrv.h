#ifndef IMAGEDRV_H
#define IMAGEDRV_H

#include "lv2g.h"
#include <QGraphicsView>
#include <QGraphicsScene>

#define LITLEREAL 0.0001
static inline bool isNegligible(qreal v)    { return LITLEREAL > v && -LITLEREAL < v; }
static inline bool isNegligible(QPointF v)  { return isNegligible(v.x()) && isNegligible(v.y()); }
static inline void noNegligible(QPointF& v) { if (isNegligible(v)) v.setX(LITLEREAL * 1.1);
}

class cGrNodeItem;

class LV2GSHARED_EXPORT cGrPixmapItem  : public QGraphicsPixmapItem  {
public:
    cGrPixmapItem(QPixmap _pm);
protected:
    virtual void        mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
};

class LV2GSHARED_EXPORT cGrPolygonItem  : public QGraphicsPolygonItem  {
    friend class cGrNodeItem;
public:
    cGrPolygonItem(const QPolygonF& _pol, QGraphicsItem * _par = nullptr);
    void editBegin();
    void editEnd();
    bool isEditing() { return editing; }
    void modPolygon(QPolygonF _pol);
    static qreal polygonOpacity;
protected:
    virtual QVariant	itemChange(GraphicsItemChange change, const QVariant & value);
    virtual void        mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    bool                editing;
    bool                dise;
    cGrNodeItem *       pFirstItem;
    cGrNodeItem *       pLastItem;
    cGrNodeItem *       pNewItem;
};

class LV2GSHARED_EXPORT cGrNodeItem  : public QGraphicsEllipseItem  {
public:
    static const qreal nodeSize;
    enum eNodeType { NT_FIRST, NT_OTHER, NT_LAST, NT_NEW };
    cGrNodeItem(const QPointF& p, QGraphicsItem * _par, enum eNodeType _t, int _ix);
protected:
    virtual QVariant	itemChange(GraphicsItemChange change, const QVariant & value);
    static void static_init();
    int     pointIx;
    static QList<QBrush>    brushList;
    static QList<QPen>      penList;
    bool                    dise;
};

class LV2GSHARED_EXPORT cImagePolygonWidget : public QGraphicsView
{
    friend class cGrPixmapItem;
    Q_OBJECT
public:
    cImagePolygonWidget(bool _e, QWidget *_par = nullptr);

    /// Az ablak tartaémának a betöltése egy cImage objektumból (háttér)
    /// Ha a képet nem sikerül betülteni, akkor kizárást dob.
    /// @par __o A kép et tartalmazó feltöltött cImage objektum referenciája
    /// @par __t Opcionális paraméter, az ablak címe, nincs megadva, akkor az image_note vagy image_name mező lessz a cím
    /// @return Ha a kép betöltése megtörtént, akkor true, ha ugyanazt a képet akartuk betölteni, mint ami már be lett töltve
    ///         (a hash érték azonos) akkor nincs változás, és fase értékkel tér vissza.
    bool setImage(const cImage& __o, const QString& __t = QString());
    cImagePolygonWidget& setText(const QString& _txt);
    cImagePolygonWidget& setPolygon(const QPolygonF& pol);
    cImagePolygonWidget& setPolygon(const tPolygonF& _pol);
    cImagePolygonWidget& clearPolygon() { pDelete(pPolygonItem); return *this; }
    cImagePolygonWidget& setBrush(const QBrush& _b) { brush = _b; return *this; }
    cImagePolygonWidget& setPen(const QPen& _pen) { pen = _pen; return *this; }
protected:
    QByteArray      imageHash;
    QPixmap         picture;
    QPen            pen;
    QBrush          brush;
    cGrPixmapItem      *pImageItem;
    QGraphicsTextItem  *pTextItem;
    cGrPolygonItem     *pPolygonItem;
    const bool      editable;
    QList<QGraphicsRectItem *> pointList;
public slots:
    void setScale(qreal s);
signals:
    void modifiedPolygon(QPolygonF polygon);
};

#endif // IMAGEDRV_H
