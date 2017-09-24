#include "record_table.h"
#include "lv2validator.h"
#include "record_dialog.h"
#include "ui_polygoned.h"
#include "ui_arrayed.h"
#include "ui_fkeyed.h"
#include "ui_fkeyarrayed.h"
/* **************************************** cImageWindows ****************************************  */

cImageWidget::cImageWidget(QWidget *__par)
    : QScrollArea(__par)
    , image()
//  , draws()
{
    scaleStep = 0.50;
    scale = 0;
    pLabel = NULL;
}

cImageWidget::~cImageWidget()
{
    ;
}

bool cImageWidget::setImage(const QString& __fn, const QString& __t)
{
    hide();
    scale = 0;
    QString title = __t;
    if (title.isEmpty()) title = "IMAGE : " + __fn;
    setWindowTitle(title);
    if (!image.load(__fn)) return false;
    return resetImage();
}

bool cImageWidget::setImage(QSqlQuery __q, qlonglong __id, const QString& __t)
{
    cImage  im;
    im.setId(__id);
    if (1 != im.completion(__q)) return false;
    return setImage(im, __t);
}

bool cImageWidget::setImage(const cImage& __o, const QString& __t)
{
    hide();
    scale = 0;
    bool f = __o.dataIsPic();
    if (!f) return false;
    const char * _type = __o._getType();
    QByteArray   _data = __o.getImage();
    if (!image.loadFromData(_data, _type)) return false;
    QString title = __t;
    if (title.isEmpty()) title = __o.getNote();
    setWindowTitle(title);
    return resetImage();
}

bool cImageWidget::setText(const QString& _txt)
{
    pLabel = new QLabel();
    pLabel->setWordWrap(true);
    pLabel->setText(_txt);
    setWidget(pLabel);
    show();
    return true;
}

void cImageWidget::mousePressEvent(QMouseEvent * ev)
{
    mousePressed(ev->pos());
}

void cImageWidget::center(QPoint p)
{
    QSize s = size();
    ensureVisible(p.x(), p.y(), s.width() / 2, s.height() / 2);
}

bool cImageWidget::resetImage()
{
    if (scale < 0) image.setDevicePixelRatio(1.0 + (scale * scaleStep));
    else           image.setDevicePixelRatio(1.0/(1.0 + (scale * scaleStep)));
    draw();
    pLabel = new QLabel();
    pLabel->setPixmap(image);
    setWidget(pLabel);
    show();
    return true;
}

void cImageWidget::draw()
{
    if (draws.isEmpty()) return;
    QPainter painter(&image);
    painter.setBrush(brush);
    painter.setPen(pen);
    foreach (QVariant g, draws) {
        draw(painter, g);
    }
}

void cImageWidget::draw(QPainter& painter, QVariant& d)
{
    int t = d.userType();

    if (t == _UMTID_tPolygonF) {
        tPolygonF   tPol = d.value<tPolygonF>();
        QPolygonF   qPol = convertPolygon(tPol);
        d = QVariant::fromValue(qPol);
        t = QMetaType::QPolygonF;
    }

    switch (t) {
    case QMetaType::QPolygon:
        painter.drawPolygon(d.value<QPolygon>());
        break;
    case QMetaType::QPolygonF:
        painter.drawPolygon(d.value<QPolygonF>());
        break;
    default:
        EXCEPTION(ENOTSUPP, t, debVariantToString(d));
    }
}


void cImageWidget::zoomIn()
{
    ++scale;
    resetImage();
}
void cImageWidget::zoomOut()
{
    --scale;
    resetImage();
}

/* ********************************************************************************************
   ************************************** cFieldEditBase **************************************
   ******************************************************************************************** */

QString fieldWidgetType(int _t)
{
    switch (_t) {
    case FEW_UNKNOWN:       return _sUnknown;
    case FEW_SET:           return "cSetWidget";
    case FEW_ENUM_COMBO:    return "cEnumComboWidget";
    case FEW_ENUM_RADIO:    return "cEnumRadioWidget";
    case FEW_LINE:          return "cFieldLineWidget";
    case FEW_TEXT:          return "cFieldLineWidget/long";
    case FEW_ARRAY:         return "cArrayWidget";
    case FEW_FKEY_ARRAY:    return "cFKeyArrayWidget";
    case FEW_POLYGON:       return "cPolygonWidget";
    case FEW_FKEY:          return "cFKeyWidget";
    case FEW_DATE:          return "cDateWidget";
    case FEW_TIME:          return "cTimeWidget";
    case FEW_DATE_TIME:     return "cDateTimeWidget";
    case FEW_INTERVAL:      return "cIntervalWidget";
    case FEW_BINARY:        return "cBinaryWidget";
    case FEW_NULL:          return "cNullWidget";
    case FEW_COLOR:         return "cColorWidget";
    case FEW_FONT_FAMILY:   return "cFontFamilyWidget";
    case FEW_FONT_ATTR:     return "cFontAttrWidget";
    default:                return sInvalidEnum();
    }
}

inline bool tableIsReadOnly(const cTableShape &_tm, const cRecord& _r)
{
    if (!(_r.isUpdatable()))                             return true;   // A tábla nem modosítható
    if (_tm.getBool(_sTableShapeType, TS_READ_ONLY))     return true;   // Read only flag a tábla megj, leíróban
    if (!lanView::isAuthorized(_tm.getId(_sEditRights))) return true;   // Az aktuális user-nek nics joga modosítani a táblát
    return false;
}

inline bool fieldIsReadOnly(const cTableShape &_tm, const cTableShapeField &_tf, const cRecordFieldRef& _fr)
{
    if (tableIsReadOnly(_tm, _fr.record()))              return true;   // A táblát nem lehet/szabad modosítani
    if (!(_fr.descr().isUpdatable))                      return true;   // A mező nem modosítható
    if (_tf.getBool(_sFieldFlags,     FF_READ_ONLY))     return true;   // Read only flag a mező megj, leíróban
    if (!lanView::isAuthOrNull(_tf.getId(_sEditRights))) return true;   // Az aktuális user-nek nics joga modosítani a mezőt
    if (_fr.recDescr().autoIncrement()[_fr.index()])     return true;   // Az autoincrement mezőt nem modosítjuk.
    return false;
}

cFieldEditBase::cFieldEditBase(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par)
    : QObject(_par)
    , _pParentDialog(_par)
    , _colDescr(_fr.descr())
    , _tableShape(_tm)
    , _fieldShape(_tf)
    , _recDescr(_fr.record().descr())
    , _value()
{
    _wType      = FEW_UNKNOWN;
    _readOnly   = (_fl & FEB_READ_ONLY) || fieldIsReadOnly(_tm, _tf, _fr);
    _value      = _fr;
    _pWidget    = NULL;
    pq          = newQuery();
    _nullable   = _fr.isNullable();
    _hasDefault = _fr.descr().colDefault.isNull() == false;
    _hasAuto    = _fr.recDescr().autoIncrement()[_fr.index()];
    _isInsert   = (_fl & FEB_INSERT);      // ??!
    _dcNull     = DC_INVALID;
    if (_isInsert && _hasDefault) {
        _dcNull = DC_DEFAULT;       // Megadható NULL, mert van alapértelmezett érték
    }
    else if (_hasAuto) {
        _dcNull = DC_AUTO;          // Automatikus érték
    }
    else if (_nullable) {
        _dcNull = DC_NULL;          // Lehet NULL a mező
    }

    _DBGFNL() << VDEB(_nullable) << VDEB(_hasDefault) << VDEB(_isInsert) << " Index = " << fieldIndex() << " _value = " << debVariantToString(_value) << endl;
 }

cFieldEditBase::~cFieldEditBase()
{
    if (pq         != NULL) delete pq;
 // if (_pWidget   != NULL) delete _pWidget;
}

QVariant cFieldEditBase::get() const
{
    _DBGFN() << QChar('/') << _colDescr<< QChar(' ') <<  debVariantToString(_value) << endl;
    return _value;
}

QString cFieldEditBase::getName() const
{
    return _colDescr.toName(get());
}

qlonglong cFieldEditBase::getId() const
{
    return _colDescr.toId(get());
}

int cFieldEditBase::set(const QVariant& _v)
{
    _DBGFN() << QChar('/') << _colDescr<< QChar(' ') <<  debVariantToString(_v) << endl;
    qlonglong st = 0;
    QVariant v = _colDescr.set(_v, st);
    if (st & ES_DEFECTIVE) return -1;
    if (v == _value) return 0;
    _value = v;
    changedValue(this);
    return 1;
}

int cFieldEditBase::setName(const QString& v)
{
    return set(QVariant(v));
}
int cFieldEditBase::setId(qlonglong v)
{
    return set(QVariant(v));
}

void cFieldEditBase::setFromWidget(QVariant v)
{
    if (_value == v) {
        _DBGFN() << QChar('/') << _colDescr<< QChar(' ') <<  debVariantToString(v) << " dropped" << endl;
        return;
    }
    _DBGFN() << " { " << _colDescr << " } " <<  debVariantToString(v) << " != " <<  debVariantToString(_value) << endl;
    _value = v;
    changedValue(this);
}

cFieldEditBase * cFieldEditBase::anotherField(const QString& __fn, eEx __ex)
{
    if (_pParentDialog == NULL) {
        QString se = trUtf8("A keresett %1 nevű mező szerkesztő objektum, nem található, nincs parent dialugus.\nMező Leiró : %2")
                .arg(__fn, _fieldShape.identifying());
        if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, se);
        DERR() << se << endl;
        return NULL;
    }
    cFieldEditBase *p = (*_pParentDialog)[__fn];
    if (p == NULL) {
        QString se = trUtf8("A keresett %1 mező, nem található a '%2'.'%3'-ból.\nMező Leiró : %2")
                .arg(__fn, _pParentDialog->name, _colDescr.colName(), _fieldShape.identifying());
        if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, se);
        DERR() << se << endl;
    }
    return p;
}

/* / SLOT
void cFieldEditBase::modRec()
{
    _DBGFN() << QChar('/') << _recDescr << endl;
    if (_value != fieldValue()) {
        bool r = set(fieldValue());
        PDEB(VVERBOSE) << "Sync to widget field #" << _pFieldRef->index() << VDEB(_value) << " TO " << fieldToName() << (r ? " OK" : " FAILED") << endl;
    }
}

void cFieldEditBase::modField(int ix)
{
    _DBGFN() << QChar('/') << _recDescr << endl;
    if (ix == _pFieldRef->index()) modRec();
}
*/

cFieldEditBase *cFieldEditBase::createFieldWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par)
{
    _DBGFN() << QChar(' ') << _tm.tableName() << _sCommaSp << _fr.fullColumnName() << " ..." << endl;
    PDEB(VVERBOSE) << "Field value = " << debVariantToString(_fr) << endl;
    PDEB(VVERBOSE) << "Field descr = " << _fr.descr().allToString() << endl;
    if (!_tf.isNull(_sViewRights) && !lanView::isAuthorized(_tf.getId(_sViewRights))) {
        cNullWidget *p = new cNullWidget(_tm, _tf, _fr, _par);
        _DBGFNL() << " new cNullWidget" << endl;
        return p;
    }
    int et = _fr.descr().eColType;
    bool ro = (_fl & FEB_READ_ONLY) || fieldIsReadOnly(_tm, _tf, _fr);
    qlonglong fieldFlags = _tf.getId(_sFieldFlags);
    int fl = ro ? (_fl | FEB_READ_ONLY) : _fl;
    static const QString sRadioButtons = "radioButtons";
    if (ro) {       // Néhány widget-nek nincs read-only módja, azok helyett read-only esetén egy soros megj.
        switch (et) {
        // ReadOnly esetén a felsoroltak kivételével egysoros megjelenítés
        case cColStaticDescr::FT_SET:
        case cColStaticDescr::FT_POLYGON:
        case cColStaticDescr::FT_INTEGER_ARRAY:
        case cColStaticDescr::FT_REAL_ARRAY:
        case cColStaticDescr::FT_TEXT_ARRAY:
        case cColStaticDescr::FT_BINARY:
        case cColStaticDescr::FT_INTERVAL:
            break;
        case cColStaticDescr::FT_TEXT:  // Ha a text egy szín, akkor nem szövegként jelenítjük meg!
            if (fieldFlags & ENUM2SET2(FF_FG_COLOR, FF_BG_COLOR)) break;
            goto if_ro_cFieldLineWidget;
        case cColStaticDescr::FT_BOOLEAN:
        case cColStaticDescr::FT_ENUM:
            if (_tf.isFeature(sRadioButtons)) break;
            goto if_ro_cFieldLineWidget;
        if_ro_cFieldLineWidget:
        default: {
            cFieldLineWidget *p =  new cFieldLineWidget(_tm, _tf, _fr, fl, _par);
            _DBGFNL() << " new cFieldLineWidget" << endl;
            return  p;
          }
        }
    }
    switch (et) {
    // adattípus és a kivételek szerinti szerkesztő objektum típus kiválasztás.
    case cColStaticDescr::FT_INTEGER:
        if (_fr.descr().fKeyType != cColStaticDescr::FT_NONE) {     // Ha ez egy idegen kulcs
            cFKeyWidget *p = new cFKeyWidget(_tm, _tf, _fr, _par);
            _DBGFNL() << " new cFKeyWidget" << endl;
            return p;
        }
        goto case_FieldLineWidget;                                  // Egy soros text...
    case cColStaticDescr::FT_TEXT:
        if (fieldFlags & ENUM2SET2(FF_FG_COLOR, FF_BG_COLOR)) {    // Ez egy szín, mint text
            cColorWidget *p = new cColorWidget(_tm, _tf, _fr, fl, _par);
            _DBGFNL() << " new cColorWidget" << endl;
            return p;
        }
        if (fieldFlags & ENUM2SET(FF_FONT)) {                      // Font család neve
            if (ro) EXCEPTION(EPROGFAIL);     // nem lehet r.o.
            cFontFamilyWidget *p = new cFontFamilyWidget(_tm, _tf, _fr, _par);
            _DBGFNL() << " new cFontFamilyWidget" << endl;
            return p;
        }
        goto case_FieldLineWidget;                                  // Egy soros text...
    // Egy soros bevitel (LineEdit) kivételek vége
    case_FieldLineWidget:
    case cColStaticDescr::FT_REAL:
    case cColStaticDescr::FT_MAC:
    case cColStaticDescr::FT_INET:
    case cColStaticDescr::FT_CIDR: {
        cFieldLineWidget *p = new cFieldLineWidget(_tm, _tf, _fr, ro, _par);
        _DBGFNL() << " new cFieldLineWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_BOOLEAN:                               // Enumeráció (spec esete) -ként kezeljük
    case cColStaticDescr::FT_ENUM: {                                // Enumeráció mint radio-button-ok
        if (_tf.isFeature(sRadioButtons)) {
            cEnumRadioWidget *p = new cEnumRadioWidget(_tm, _tf, _fr, ro, _par);
            _DBGFNL() << " new cEnumRadioWidget" << endl;
            return p;
        }
        else {                                                      // Enumeráció : alapértelmezetten comb-box
            cEnumComboWidget *p = new cEnumComboWidget(_tm, _tf, _fr, _par);
            _DBGFNL() << " new cEnumComboWidget" << endl;
            return p;
        }
    }
    case cColStaticDescr::FT_SET: {
        if (fieldFlags & ENUM2SET(FF_FONT)) {                      // Font attributum
            // Csak ez a típus lehet!
            if (_fr.descr().enumType() == cFontAttrWidget::sEnumTypeName) {
                cFontAttrWidget *p = new cFontAttrWidget(_tm, _tf, _fr, ro, _par);
                _DBGFNL() << " new cFontAttrWidget" << endl;
                return p;
            }
        }
        cSetWidget *p = new cSetWidget(_tm, _tf, _fr, ro, _par);
        _DBGFNL() << " new cSetWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_POLYGON: {
        cPolygonWidget *p = new cPolygonWidget(_tm, _tf, _fr, ro, _par);
        _DBGFNL() << " new cPolygonWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_INTEGER_ARRAY:
        if (_fr.descr().fKeyType != cColStaticDescr::FT_NONE) {     // nem szám, hanem a hivatkozott rekordok kezelése
            cFKeyArrayWidget *p = new cFKeyArrayWidget(_tm, _tf, _fr, ro, _par);
            _DBGFNL() << " new cFKeyArrayWidget" << endl;
            return p;
        }
        // Nincs break; !!
    case cColStaticDescr::FT_REAL_ARRAY:
    case cColStaticDescr::FT_TEXT_ARRAY: {
         cArrayWidget *p = new cArrayWidget(_tm, _tf, _fr, ro, _par);
        _DBGFNL() << " new cArrayWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_BINARY: {
        cBinaryWidget *p = new cBinaryWidget(_tm, _tf, _fr, ro, _par);
        _DBGFNL() << " new cBinaryWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_TIME: {
        cTimeWidget *p = new cTimeWidget(_tm, _tf, _fr, _par);
        _DBGFNL() << " new cTimeWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_DATE: {
        cDateWidget *p = new cDateWidget(_tm, _tf, _fr, _par);
        _DBGFNL() << " new cDateWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_DATE_TIME: {
        cDateTimeWidget *p = new cDateTimeWidget(_tm, _tf, _fr, _par);
        _DBGFNL() << " new cDateTimeWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_INTERVAL: {
        cIntervalWidget *p = new cIntervalWidget(_tm, _tf, _fr, ro, _par);
        _DBGFNL() << " new cIntervalWidget" << endl;
        return p;
    }
    default:
        EXCEPTION(EDATA, et);
        return NULL;
    }
}

int cFieldEditBase::height()
{
    return 1;
}

/* **************************************** cNullWidget **************************************** */
cNullWidget::cNullWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase* _par)
    : cFieldEditBase(_tm, _tf, __fr, true, _par)
{
    _DBGOBJ() << _tf.identifying() << endl;
    _wType = FEW_NULL;
    QLineEdit *pL = new QLineEdit(_par == NULL ? NULL : _par->pWidget());
    _pWidget = pL;
    pL->setReadOnly(true);
    dcSetShort(pL, DC_NOT_PERMIT);
}

cNullWidget::~cNullWidget()
{
    DBGOBJ();
}

/* **************************************** cSetWidget **************************************** */

cSetWidget::cSetWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
{
    _DBGOBJ() << _tf.identifying() << endl;
    _wType = FEW_SET;
    _pWidget  = new QWidget(_par == NULL ? NULL : _par->pWidget());
    pButtons  = new QButtonGroup(pWidget());
    pLayout   = new QVBoxLayout;
    pButtons->setExclusive(false);      // SET, több opció is kiválasztható
    widget().setLayout(pLayout);
    _bits = _colDescr.toId(_value);
    int id;
    for (id = 0; id < _colDescr.enumType().enumValues.size(); ++id) {
        QCheckBox *pCheckBox = new QCheckBox(pWidget());
        enumSetShort(pCheckBox, _colDescr.enumType(), id, _colDescr.enumType().enum2str(id), _tf.getId(_sFieldFlags));
        pButtons->addButton(pCheckBox, id);
        pLayout->addWidget(pCheckBox);
        pCheckBox->setChecked(enum2set(id) & _bits);
        pCheckBox->setDisabled(_readOnly);
    }
    if (_dcNull != DC_INVALID) {
        QCheckBox *pCheckBox = new QCheckBox(pWidget());
        dcSetShort(pCheckBox, _dcNull);
        pButtons->addButton(pCheckBox, id);
        pLayout->addWidget(pCheckBox);
        pCheckBox->setChecked(__fr.isNull());
        pCheckBox->setDisabled(_readOnly);
    }
    connect(pButtons, SIGNAL(buttonClicked(int)), this, SLOT(setFromEdit(int)));
}

cSetWidget::~cSetWidget()
{
    ;
}

int cSetWidget::set(const QVariant& v)
{
    _DBGFN() << debVariantToString(v) << endl;
    int r = 1 == cFieldEditBase::set(v);
    if (r) {
        int nid = _colDescr.enumType().enumValues.size();
        QAbstractButton *pAB = pButtons->button(nid);
        if (pAB != NULL) pAB->setChecked(v.isNull());
        _bits = _colDescr.toId(_value);
        if (_bits < 0) _bits = 0;
        for (int id = 0; id < nid && NULL != (pAB = pButtons->button(id)) ; id++) {
            pAB->setChecked(enum2set(id) & _bits);
        }
    }
    return r;
}

int cSetWidget::height()
{
    return pButtons->buttons().size();
}

void cSetWidget::setFromEdit(int id)
{
    _DBGFNL() << id << endl;
    qlonglong dummy;
    int n =_colDescr.enumType().enumValues.size();
    if (id >= n) {
        for (int i = 0; i < n; ++i) {
            if (_bits & enum2set(i)) pButtons->button(i)->setChecked(false);
        }
        _bits = 0;
        setFromWidget(_colDescr.set(QVariant(), dummy));
    }
    else {
        _bits ^= enum2set(id);
        setFromWidget(_colDescr.set(QVariant(_bits), dummy));
        QAbstractButton *p = pButtons->button(n);
        if (p != NULL && p->isChecked()) p->setChecked(false);
    }
}


/* **************************************** cEnumRadioWidget ****************************************  */

cEnumRadioWidget::cEnumRadioWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
{
    _wType = FEW_ENUM_RADIO;
    _pWidget = new QWidget(_par == NULL ? NULL : _par->pWidget());
    pButtons  = new QButtonGroup(pWidget());
    pLayout   = new QVBoxLayout;
    pButtons->setExclusive(true);
    widget().setLayout(pLayout);
    int id = 0;
    eval = _colDescr.toId(_value);
    for (id = 0; id < _colDescr.enumType().enumValues.size(); ++id) {
        QRadioButton *pRadioButton = new QRadioButton(pWidget());
        enumSetShort(pRadioButton, _colDescr.enumType(), id, _colDescr.enumType().enum2str(id), _tf.getId(_sFieldFlags));
        pButtons->addButton(pRadioButton, id);
        pLayout->addWidget(pRadioButton);
        pRadioButton->setChecked(id == eval);
        pRadioButton->setDisabled(_readOnly);
    }
    if (_dcNull != DC_INVALID) {
        QRadioButton *pRadioButton = new QRadioButton(pWidget());
        dcSetShort(pRadioButton, _dcNull);
        pButtons->addButton(pRadioButton, id);
        pLayout->addWidget(pRadioButton);
        pRadioButton->setChecked(eval < 0);
        pRadioButton->setDisabled(_readOnly);
    }
    connect(pButtons, SIGNAL(buttonClicked(int)),  this, SLOT(setFromEdit(int)));
}

cEnumRadioWidget::~cEnumRadioWidget()
{
    ;
}

int cEnumRadioWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (1 == r) {
        pButtons->button(pButtons->checkedId())->setChecked(false);
        eval = _colDescr.toId(v);
        if (eval >= 0) pButtons->button(eval)->setChecked(true);
        else {
            QAbstractButton *pRB = pButtons->button(_colDescr.enumType().enumValues.size());
            if (pRB != NULL) pRB->setChecked(true);
        }
    }
    return r;
}

int cEnumRadioWidget::height()
{
    return pButtons->buttons().size();
}

void cEnumRadioWidget::setFromEdit(int id)
{
    QVariant v;
    if (eval == id) {
        if (pButtons->button(id)->isChecked() == false) pButtons->button(id)->setChecked(true);
        return;
    }
    if (id == _colDescr.enumType().enumValues.size()) {
        if (eval < 0) {
            if (pButtons->button(id)->isChecked() == false) pButtons->button(id)->setChecked(true);
            return;
        }
    }
    else {
        v = eval;
    }
    qlonglong dummy;
    setFromWidget(_colDescr.set(v, dummy));
}

/* **************************************** cEnumComboWidget ****************************************  */

cEnumComboWidget::cEnumComboWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, false, _par)
{
    if (_readOnly && _par != NULL) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + __fr.record().identifying());
    eval = getId();
    _wType = FEW_ENUM_COMBO;
    QComboBox *pCB = new QComboBox(_par == NULL ? NULL : _par->pWidget());
    _pWidget = pCB;
    nulltype = _dcNull == DC_INVALID ? NT_NOT_NULL : (eNullType)_dcNull;
    cEnumListModel *pModel = new cEnumListModel(&_colDescr.enumType(), nulltype);
    _setEnumListModel(pCB, pModel);
    pCB->setEditable(false);                  // Nem editálható, választás csak a listából
    setWidget();
    connect(pCB, SIGNAL(activated(int)), this, SLOT(setFromEdit(int)));
}

cEnumComboWidget::~cEnumComboWidget()
{
    ;
}

void cEnumComboWidget::setWidget()
{
    QComboBox *pComboBox = (QComboBox *)_pWidget;
    if (isContIx(_colDescr.enumType().enumValues, eval)) {
        int ix = eval;
        if (nulltype != NT_NOT_NULL) ++ix;
        pComboBox->setCurrentIndex(ix);
    }
    else {
        if (nulltype != NT_NOT_NULL) eval = 0;
        else                         eval = NULL_ID;
        pComboBox->setCurrentIndex(0);
    }
}

int cEnumComboWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (1 == r) {
        setFromWidget(v);
        eval = getId();
        setWidget();
    }
    return r;
}

void cEnumComboWidget::setFromEdit(int id)
{
    qlonglong newEval = id;
    if (nulltype != NT_NOT_NULL) {
        if (id == 0) newEval = NULL_ID;
        else         newEval--;
    }
    if (eval == newEval) return;
    qlonglong v = newEval;
    qlonglong dummy;
    setFromWidget(_colDescr.set(QVariant(v), dummy));
    //QComboBox *pCB = dynamic_cast<QComboBox *>(_pWidget);

}

/* **************************************** cFieldLineWidget ****************************************  */

cFieldLineWidget::cFieldLineWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr, _fl, _par)
{
    QLineEdit *pLE = NULL;
    QPlainTextEdit *pTE = NULL;
    isPwd = false;
    if (_colDescr.eColType == cColStaticDescr::FT_TEXT && _fieldShape.getBool(_sFieldFlags, FF_HUGE)) {
        _wType = FEW_TEXT;  // Widget típus azonosító
        pTE = new QPlainTextEdit(_par == NULL ? NULL : _par->pWidget());
        _pWidget = pTE;
    }
    else {
        _wType = FEW_LINE;  // Widget típus azonosító
        pLE = new QLineEdit(_par == NULL ? NULL : _par->pWidget());
        _pWidget = pLE;
        isPwd = _fieldShape.getBool(_sFieldFlags, FF_PASSWD);
    }
    bool nullable = _colDescr.isNullable;
    QString tx;
    if (_readOnly == false) {
        tx = _fr.toString();
        _value = QVariant(tx);
        switch (_colDescr.eColType) {
        case cColStaticDescr::FT_INTEGER:   pLE->setValidator(new cIntValidator( nullable, pLE));   break;
        case cColStaticDescr::FT_REAL:      pLE->setValidator(new cRealValidator(nullable, pLE));   break;
        case cColStaticDescr::FT_TEXT:                                                              break;
        case cColStaticDescr::FT_MAC:       pLE->setValidator(new cMacValidator( nullable, pLE));   break;
        case cColStaticDescr::FT_INET:      pLE->setValidator(new cINetValidator(nullable, pLE));   break;
        case cColStaticDescr::FT_CIDR:      pLE->setValidator(new cCidrValidator(nullable, pLE));   break;
        default:    EXCEPTION(ENOTSUPP);
        }
        if (isPwd) {
            pLE->setEchoMode(QLineEdit::Password);
            pLE->setText("");
            _value.clear();
            connect(pLE, SIGNAL(editingFinished()),  this, SLOT(setFromEdit()));
        }
        else if (pLE != NULL){
            pLE->setText(_fr);
            connect(pLE, SIGNAL(editingFinished()),  this, SLOT(setFromEdit()));
        }
        else {
            pTE->setPlainText(_fr);
            connect(pTE, SIGNAL(textChanged()),  this, SLOT(setFromEdit()));
        }
    }
    else {
        tx = _fr.view(*pq);
        if (isPwd) {
            tx = "********";
        }
        else if (_isInsert) {
            if (_wType == FEW_LINE) {
                if      (_hasAuto)    dcSetShort(pLE, DC_AUTO);
                else if (_hasDefault) dcSetShort(pLE, DC_DEFAULT);;
            }
        }
        if (_wType == FEW_LINE) {
            pLE->setText(tx);
            pLE->setReadOnly(true);
        }
        else {
            pTE->setPlainText(tx);
            pTE->setReadOnly(true);
        }
    }
}

cFieldLineWidget::~cFieldLineWidget()
{
    ;
}

int cFieldLineWidget::set(const QVariant& v)
{
    if (isPwd) {
        pLineEdit()->setText(_sNul);
        return 0;
    }
    int r = cFieldEditBase::set(v);
    if (1 == r) {
        QString txt;
        if (_readOnly == false) {
            txt = _colDescr.toName(_value);
        }
        else {
            txt = _colDescr.toView(*pq, _value);
        }
        if (_wType == FEW_LINE) pLineEdit()->setText(txt);
        else                    pTextEdit()->setPlainText(txt);
    }
    return r;
}

int cFieldLineWidget::height()
{
    if (_wType == FEW_LINE) return 1;
    else                    return 4;
}

void cFieldLineWidget::setFromEdit()
{
    QString  s;
    if (_wType == FEW_LINE) s = pLineEdit()->text();
    else                    s = pTextEdit()->toPlainText();
    QVariant v; // NULL
    if (!s.isEmpty() || !(isPwd || _nullable || (_isInsert && _hasDefault))) {
        v = QVariant(s);
    }
    setFromWidget(v);
    valid();
}


/* **************************************** cArrayWidget ****************************************  */


cArrayWidget::cArrayWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
    , last()
{
    _wType   = FEW_ARRAY;

    _pWidget = new QWidget(_par == NULL ? NULL : _par->pWidget());
    pUi      = new Ui_arrayEd;
    pUi->setupUi(_pWidget);

    selectedNum = 0;

    pUi->pushButtonAdd->setDisabled(_readOnly);
    pUi->pushButtonIns->setDisabled(_readOnly);
    pUi->pushButtonUp->setDisabled(_readOnly);
    pUi->pushButtonDown->setDisabled(_readOnly);
    pUi->pushButtonMod->setDisabled(_readOnly);
    pUi->pushButtonDel->setDisabled(_readOnly);
    pUi->pushButtonClr->setDisabled(_readOnly);

    pModel = new cStringListModel(pWidget());
    pModel->setStringList(_value.toStringList());
    pUi->listView->setModel(pModel);
    pUi->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (!_readOnly) {
        connect(pUi->pushButtonAdd, SIGNAL(pressed()), this, SLOT(addRow()));
        connect(pUi->pushButtonDel, SIGNAL(pressed()), this, SLOT(delRow()));
        connect(pUi->pushButtonClr, SIGNAL(pressed()), this, SLOT(clrRows()));
        connect(pUi->pushButtonMod, SIGNAL(pressed()), this, SLOT(modRow()));
        connect(pUi->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(changed(QString)));
        connect(pUi->listView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(selectionChanged(QModelIndex,QModelIndex)));
        connect(pUi->listView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(doubleClickRow(QModelIndex)));
    }
}

cArrayWidget::~cArrayWidget()
{
    ;
}

int cArrayWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (1 == r) {
        pModel->setStringList(_value.toStringList());
        setButtons();
    }
    return r;
}

int cArrayWidget::height()
{
    return 6;
}

void cArrayWidget::setButtons()
{
    bool eArr = pModel->isEmpty();
    bool eLin = pUi->lineEdit->text().isEmpty();
    bool sing = selectedNum == 1;
    bool any  = selectedNum > 0;

    pUi->pushButtonAdd ->setDisabled(eLin         || _readOnly);
    pUi->pushButtonIns ->setDisabled(eLin || sing || _readOnly);
    pUi->pushButtonUp  ->setDisabled(        any  || _readOnly);
    pUi->pushButtonDown->setDisabled(        any  || _readOnly);
    pUi->pushButtonMod ->setDisabled(eLin || sing || _readOnly);
    pUi->pushButtonDel ->setDisabled(eArr         || _readOnly);
    pUi->pushButtonClr ->setDisabled(eArr         || _readOnly);
}

// cArrayWidget SLOTS

void cArrayWidget::selectionChanged(QModelIndex cur, QModelIndex)
{
    DBGFN();
    if (cur.isValid()) {
        actIndex = cur;
        PDEB(INFO) << "Current row = " << actIndex.row() << endl;
    }
    else {
        actIndex = QModelIndex();
        PDEB(INFO) << "No current row." << endl;
    }
    setButtons();
}

void cArrayWidget::changed(QString _t)
{
    if (_t.isEmpty()) {
        last.clear();
        pUi->lineEdit->setText(last);
    }
    else {
        bool ok;
        switch (_colDescr.eColType) {
        case cColStaticDescr::FT_INTEGER_ARRAY:
            _t.toLongLong(&ok);
            break;
        case cColStaticDescr::FT_REAL_ARRAY:
            _t.toDouble(&ok);
            break;
        case cColStaticDescr::FT_TEXT_ARRAY:
            ok = true;
            break;
        default:
            EXCEPTION(ENOTSUPP, _colDescr.eColType);
        }
        if (ok) last = _t;
        else    pUi->lineEdit->setText(last);
    }
    setButtons();
}

void cArrayWidget::addRow()
{
    *pModel << last;
    setFromWidget(pModel->stringList());
    setButtons();
}

void cArrayWidget::insRow()
{
    int row = actIndex.row();
    pModel->insert(last, row);
    setFromWidget(pModel->stringList());
    setButtons();
}

void cArrayWidget::upRow()
{
    QModelIndexList mil = pUi->listView->selectionModel()->selectedRows();
    pModel->up(mil);
    setFromWidget(pModel->stringList());
    setButtons();
}

void cArrayWidget::downRow()
{
    QModelIndexList mil = pUi->listView->selectionModel()->selectedRows();
    pModel->down(mil);
    setFromWidget(pModel->stringList());
    setButtons();
}

void cArrayWidget::modRow()
{
    int row = actIndex.row();
    pModel->modify(last, row);
    setFromWidget(pModel->stringList());
}

void cArrayWidget::delRow()
{
    QModelIndexList mil = pUi->listView->selectionModel()->selectedIndexes();
    if (mil.size() > 0) {
        pModel->remove(mil);
    }
    else {
        pModel->pop_back();
    }
    setFromWidget(pModel->stringList());
    setButtons();
}

void cArrayWidget::clrRows()
{
    pModel->clear();
    setFromWidget(pModel->stringList());
    setButtons();
}

void cArrayWidget::doubleClickRow(const QModelIndex & index)
{
    const QStringList& sl = pModel->stringList();
    int row = index.row();
    if (isContIx(sl, row)) {
        pUi->lineEdit->setText(sl.at(row));
    }
}

/* **************************************** cPolygonWidget ****************************************  */
/**
  A 'features' mező:\n
  :<b>map</b>=<i>\<sql függvény\></i>: Ha megadtuk, és a rekordnak már van ID-je (nem új rekord felvitel), és a megadott SQL függvény a
                rekord ID alapján visszaadta egy kép (images.image_id) azonosítóját, akkor feltesz egy plussz gombot, ami megjeleníti
                a képet, és azon klikkelve is felvehetünk pontokat.
 */
cPolygonWidget::cPolygonWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
    , xPrev(), yPrev()
{
    _wType = FEW_POLYGON;

    pMapWin = NULL;
    pCImage = NULL;
    epic = NO_ANY_PIC;
    parentOrPlace_id = NULL_ID;
    selectedRowNum = 0;
    xOk = yOk = xyOk = false ;

    _pWidget = new QWidget(_par == NULL ? NULL : _par->pWidget());
    pUi = new Ui_polygonEd;
    pUi->setupUi(_pWidget);

    pUi->lineEditX->setDisabled(_readOnly);
    pUi->lineEditY->setDisabled(_readOnly);
    pUi->pushButtonAdd->setDisabled(true);
    pUi->pushButtonIns->setDisabled(true);
    pUi->pushButtonUp->setDisabled(true);
    pUi->pushButtonDown->setDisabled(true);
    pUi->pushButtonMod->setDisabled(true);
    pUi->pushButtonDel->setDisabled(true);
    pUi->pushButtonClr->setDisabled(_readOnly);
    pUi->pushButtonPic->setDisabled(true);
    pUi->doubleSpinBoxZoom->setDisabled(true);
    pUi->doubleSpinBoxZoom->setValue(stZoom);

    // Alaprajz előkészítése, ha van
    // Ha egy 'places' rekordot szerkesztünk, akkor van egy parent_id  mezőnk, amiből megvan az image_id
    if (_recDescr == cPlace().descr()) {
        cFieldEditBase *p = anotherField(_sParentId);
        connect(p, SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(changeId(cFieldEditBase*)));
        epic = IS_PLACE_REC;
    }
    // Másik lehetőség, a features-ben van egy függvénynevünk, ami a rekord id-alapján megadja az image id-t
    else {
        id2imageFun = _tableShape.feature("map");  // Meg van adva a image id-t visszaadó függvlny neve ?
        if (id2imageFun.isEmpty() == false) {
            cFieldEditBase *p = anotherField(_recDescr.idName());
            connect(p, SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(changeId(cFieldEditBase*)));
            epic = ID2IMAGE_FUN;
        }
    }
    if (epic != NO_ANY_PIC) {
        pCImage = new cImage;
        pCImage->setParent(this);
    }

    if (_value.isValid()) polygon = _value.value<tPolygonF>();
    pModel = new cPolygonTableModel(this);
    pModel->setPolygon(polygon);
    pUi->tableViewPolygon->setModel(pModel);
    if (!_readOnly) {
        connect(pUi->tableViewPolygon->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(tableSelectionChanged(QItemSelection,QItemSelection)));
        connect(pUi->tableViewPolygon, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(tableDoubleclicked(QModelIndex)));
        connect(pUi->lineEditX, SIGNAL(textChanged(QString)), this, SLOT(xChanged(QString)));
        connect(pUi->lineEditY, SIGNAL(textChanged(QString)), this, SLOT(yChanged(QString)));
        connect(pUi->pushButtonAdd, SIGNAL(pressed()), this, SLOT(addRow()));
        connect(pUi->pushButtonIns, SIGNAL(pressed()), this, SLOT(insRow()));
        connect(pUi->pushButtonUp , SIGNAL(pressed()), this, SLOT(upRow()));
        connect(pUi->pushButtonDown,SIGNAL(pressed()), this, SLOT(downRow()));
        connect(pUi->pushButtonMod, SIGNAL(pressed()), this, SLOT(modRow()));
        connect(pUi->pushButtonDel, SIGNAL(pressed()), this, SLOT(delRow()));
        connect(pUi->pushButtonClr,SIGNAL(pressed()), this, SLOT(clearRows()));
        connect(pUi->pushButtonPic,SIGNAL(pressed()), this, SLOT(imageOpen()));
        connect(pUi->doubleSpinBoxZoom, SIGNAL(valueChanged(double)), this, SLOT(zoom(double)));
        (void)getImage();
    }
    setButtons();
}

cPolygonWidget::~cPolygonWidget()
{
    ;
}

int cPolygonWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (r == 1) {
        polygon = _value.value<tPolygonF>();
        pModel->setPolygon(polygon);
        setButtons();
    }
    getImage();    // Feltételezi, hogy az rekord ID ha van elöbb lett megadva
    return r;
}

int cPolygonWidget::height()
{
    return 6;
}

/// Ha a feltételek teljesülnek, akkor kitölti a pCImage objektumot.
/// Ha epic == NO_ANY_PIC, akkor nem csinál semmit, és false-val tér vissza.
/// ...
bool cPolygonWidget::getImage(bool refresh)
{
    if (epic == NO_ANY_PIC) return false;
    if (pCImage == NULL) EXCEPTION(EPROGFAIL);
    qlonglong iid = NULL_ID;
    switch (epic) {
    case IS_PLACE_REC:
        if (parentOrPlace_id != NULL_ID) {
            bool ok;
            iid = execSqlIntFunction(*pq, &ok, "get_image", parentOrPlace_id);
            if (!ok || iid == 0) {  // Valamiért 0-val tér vissza, NULL helyett :-o
                iid = NULL_ID;
            }
        }
        break;
    case ID2IMAGE_FUN:
        if (parentOrPlace_id != NULL_ID) {
            bool ok;
            iid = execSqlIntFunction(*pq, &ok, id2imageFun, parentOrPlace_id);
            if (!ok || iid == 0) {  // Valamiért 0-val tér vissza, NULL helyett :-o
                iid = NULL_ID;
            }
        }
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    if (iid != NULL_ID) {
        if ((refresh || iid != pCImage->getId())) {
            pCImage->setById(*pq, iid);
            pUi->pushButtonPic->setEnabled(true);
        }
        if (pCImage->dataIsPic()) {
            if (pMapWin != NULL) pMapWin->setImage(*pCImage);
            return true;
        }
        else {
            pDelete(pMapWin);
            return false;
        }
    }
    else {
        pCImage->clear();
        pDelete(pMapWin);
        pUi->pushButtonPic->setDisabled(true);
        return false;
    }
}

void cPolygonWidget::drawPolygon()
{
    if (pMapWin == NULL) return;
    if (polygon.size() < 3)
        pMapWin->clearPolygon();
    else
        pMapWin->setPolygon(polygon);
    lastPos = QPointF(0,0);
}

void cPolygonWidget::modPostprod(QModelIndex select)
{
    if (select.isValid()) {
        pUi->tableViewPolygon->selectionModel()->select(select, QItemSelectionModel::ClearAndSelect);
    }
    else {
        setButtons();
    }
    setFromWidget(QVariant::fromValue(polygon));
    drawPolygon();
}

void cPolygonWidget::setButtons()
{
    bool ne  = polygon.size() > 0;
    bool one = selectedRowNum == 1;
    bool any = selectedRowNum  > 0;

    pUi->pushButtonAdd ->setEnabled(xyOk &&              !_readOnly);
    pUi->pushButtonIns ->setEnabled(xyOk && one &&       !_readOnly);
    pUi->pushButtonUp  ->setEnabled(        any &&       !_readOnly);
    pUi->pushButtonDown->setEnabled(        any &&       !_readOnly);
    pUi->pushButtonMod ->setEnabled(xyOk && one &&       !_readOnly);
    pUi->pushButtonDel ->setEnabled(               ne && !_readOnly);
    pUi->pushButtonClr ->setEnabled(               ne && !_readOnly);

    pUi->pushButtonPic->setEnabled(pCImage->dataIsPic());
    pUi->doubleSpinBoxZoom->setEnabled(pMapWin != NULL);
}

QModelIndex cPolygonWidget::actIndex(eEx __ex)
{
    if (__ex && !_actIndex.isValid()) EXCEPTION(EDATA);
    return _actIndex;
}

int cPolygonWidget::actRow(eEx __ex)
{
    if (__ex && _actRow < 0) EXCEPTION(EDATA);
    return _actRow;
}

// ***** cPolygonWidget SLOTS *****

void cPolygonWidget::tableSelectionChanged(const QItemSelection &, const QItemSelection &)
{
    QModelIndexList mil = pUi->tableViewPolygon->selectionModel()->selectedRows();
    selectedRowNum = mil.size();
    if (selectedRowNum == 1) {
        _actIndex = mil[0];
        _actRow   = _actIndex.row();
    }
    else {
        _actIndex = QModelIndex();
        _actRow   = -1;
    }
    setButtons();
}

void cPolygonWidget::tableDoubleclicked(const QModelIndex& mi)
{
    int row = mi.row();
    if (isContIx(polygon, row)) {
        QPointF p = polygon[row];
        pUi->lineEditX->setText(QString::number(p.x(), 'f', 0));
        pUi->lineEditY->setText(QString::number(p.y(), 'f', 0));
    }
}

void cPolygonWidget::xChanged(QString _t)
{
    if (_t.isEmpty()) {
        xPrev.clear();
        xOk = false;
    }
    else {
        bool ok;
        x = _t.toDouble(&ok);
        if (ok) {
            xPrev = _t;
            xOk = true;
        }
        else     pUi->lineEditX->setText(xPrev);
    }
    xyOk = xOk && yOk;
    setButtons();
}

void cPolygonWidget::yChanged(QString _t)
{
    if (_t.isEmpty()) {
        yPrev.clear();
        xOk = false;
    }
    else {
        bool ok;
        y = _t.toDouble(&ok);
        if (ok) {
            yPrev = _t;
            yOk = true;
        }
        else     pUi->lineEditY->setText(yPrev);
    }
    xyOk = xOk && yOk;
    setButtons();
}

void cPolygonWidget::addRow()
{
    QPointF pt(x,y);
    *pModel << pt;
    polygon = pModel->polygon();
    int row = polygon.size() -1;
    QModelIndex mi = index(row);
    modPostprod(mi);
}

void cPolygonWidget::insRow()
{
    QPointF pt(x,y);
    QModelIndex mi = actIndex();
    int row = mi.row();
    pModel->insert(pt, row);
    polygon = pModel->polygon();
    modPostprod(mi);
}

void cPolygonWidget::upRow()
{
    QModelIndexList mil = pUi->tableViewPolygon->selectionModel()->selectedRows();
    pModel->up(mil);
    modPostprod();
}

void cPolygonWidget::downRow()
{
    QModelIndexList mil = pUi->tableViewPolygon->selectionModel()->selectedRows();
    pModel->down(mil);
    modPostprod();
}

void cPolygonWidget::modRow()
{
    int row = actRow();
    QPointF p(x, y);
    pModel->modify(row, p);
    modPostprod();
}

void cPolygonWidget::delRow()
{
    QModelIndexList mil = pUi->tableViewPolygon->selectionModel()->selectedRows();
    if (mil.size()) {
        polygon = pModel->remove(mil).polygon();
    }
    else {
        pModel->pop_back();
        polygon = pModel->polygon();
    }
    modPostprod();
}

void cPolygonWidget::clearRows()
{
    polygon.clear();
    pModel->clear();
    modPostprod();
}

void cPolygonWidget::imageOpen()
{
    if (pCImage == NULL) EXCEPTION(EPROGFAIL);
    if (pMapWin != NULL) {
        pMapWin->show();
        return;
    }
    pMapWin = new cImagePolygonWidget(!isReadOnly(), _pParentDialog == NULL ? NULL : _pParentDialog->pWidget());
    pMapWin->setWindowFlags(Qt::Window);
    pMapWin->setImage(*pCImage);
    pMapWin->show();
    pMapWin->setBrush(QBrush(QColor(Qt::red)));
    drawPolygon();
    pUi->doubleSpinBoxZoom->setEnabled(true);
    if (!isReadOnly()) {
        // connect(pMapWin, SIGNAL(mousePressed(QPoint)), this, SLOT(imagePoint(QPoint)));
        connect(pMapWin, SIGNAL(modifiedPolygon(QPolygonF)),   this, SLOT(setted(QPolygonF)));
    }
    pMapWin->setScale(stZoom);
}

qreal cPolygonWidget::stZoom = 1.0;

void cPolygonWidget::zoom(double z)
{
    stZoom = z;
    if (pMapWin == NULL) EXCEPTION(EPROGFAIL);
    pMapWin->setScale(z);
}

void cPolygonWidget::destroyedImage(QObject *p)
{
    (void)p;
    pMapWin = NULL;
    pUi->doubleSpinBoxZoom->setDisabled(true);
}

void cPolygonWidget::changeId(cFieldEditBase *p)
{
    (void)p;
    parentOrPlace_id = p->getId();
    getImage();
}

void cPolygonWidget::setted(QPolygonF pol)
{
    polygon.clear();
    foreach (QPointF pt, pol) {
        polygon << pt;
    }
    pModel->setPolygon(polygon);
    setFromWidget(QVariant::fromValue(polygon));
}

/* **************************************** cFKeyWidget ****************************************  */

cFKeyWidget::cFKeyWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, false, _par)
{
    if (_readOnly && _par != NULL) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + __fr.record().identifying());
    _wType = FEW_FKEY;
    _pWidget = new QWidget(_par == NULL ? NULL : _par->pWidget());
    pUi = new Ui_fKeyEd;
    pUi->setupUi(_pWidget);

    pRDescr = cRecStaticDescr::get(_colDescr.fKeyTable, _colDescr.fKeySchema);
    pModel = new cRecordListModel(*pRDescr, pWidget());
    pModel->nullable = _colDescr.isNullable;
    pModel->setToNameF(_colDescr.fnToName);
    QString owner = _fieldShape.feature(_sOwner);
    if (!owner.isEmpty()) {
        if (0 == owner.compare(_sSelf, Qt::CaseInsensitive)) {  // TREE
            if (_pParentDialog == NULL) {
                QAPPMEMO(trUtf8("Invalid feature %1.%2 'owner=self', invalid context.").arg(_tableShape.getName(), _fieldShape.getName()), RS_CRITICAL | RS_BREAK);
            }
            owner_ix = __fr.record().descr().ixToOwner(EX_IGNORE);
            if (owner_ix < 0) {
                QAPPMEMO(trUtf8("Invalid feature %1.%2 'owner=self', owner id index not found.").arg(_tableShape.getName(), _fieldShape.getName()), RS_CRITICAL | RS_BREAK);
            }
            ownerId = NULL_ID;
            if (_pParentDialog->_pOwnerTable != NULL) {
                ownerId = _pParentDialog->_pOwnerTable->owner_id;
            }
            else if (_pParentDialog->_pOwnerDialog != NULL) {
                EXCEPTION(ENOTSUPP);
            }
            if (ownerId == NULL_ID) {
                EXCEPTION(EDATA);
            }
            QString sql = __fr.record().descr().columnName(owner_ix) + " = " + QString::number(ownerId);
            pModel->setConstFilter(sql, FT_SQL_WHERE);
            pModel->setFilter(_sNul, OT_ASC, FT_NO);
        }
        else {
            // Ha nincs parent dialog, akkor ez nem fog menni
            if (_pParentDialog == NULL) EXCEPTION(EDATA, -1, _tableShape.identifying(false));
            cRecordDialog *pDialog = dynamic_cast<cRecordDialog *>(_pParentDialog);
            if (pDialog == NULL) EXCEPTION(EDATA, -1, _tableShape.identifying(false));
            QList<cFieldEditBase *>::iterator it = pDialog->fields.begin(); // A hivatkozott mező a jelenlegi elött!!
            for (;it < pDialog->fields.end(); ++it) {
                if (0 == (*it)->_fieldShape.getName().compare(owner, Qt::CaseInsensitive)) {
                    cFieldEditBase *pfeb = *it;
                    ownerId = pfeb->getId();
                    connect(pfeb, SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(modifyOwnerId(cFieldEditBase*)));
                    pModel->setFilter(ownerId, OT_ASC, FT_FKEY_ID);
                    break;
                }
            }
            if (it >= pDialog->fields.end()) EXCEPTION(EDATA);
        }
    }
    else {
        pModel->setFilter(_sNul, OT_ASC, FT_NO);
    }
    _setRecordListModel(pUi->comboBox, pModel);
    pUi->pushButtonEdit->setDisabled(true);
    _value = pModel->atId(0);
    pTableShape = new cTableShape();
    // Dialógus leíró neve a feature mezőben
    QString tsn = _fieldShape.feature(_sDialog);
    // ha ott nincs megadva, akkor a megjelenítő neve azonos a tulajdonság rekord nevével
    if (tsn.isEmpty()) tsn = _colDescr.fKeyTable;
    if (pTableShape->fetchByName(tsn)) {   // Ha meg tudjuk jeleníteni
        // Nem lehet öröklés !!
        qlonglong tit = pTableShape->getId(_sTableInheritType);
        if (tit != TIT_NO && tit != TIT_ONLY) EXCEPTION(EDATA);
        pTableShape->fetchFields(*pq);
        pUi->pushButtonNew->setEnabled(true);
        connect(pUi->pushButtonEdit, SIGNAL(pressed()), this, SLOT(modifyF()));
        connect(pUi->pushButtonNew,  SIGNAL(pressed()), this, SLOT(insertF()));
    }
    else {
        pDelete(pTableShape);
        pUi->pushButtonNew->setDisabled(true);
    }

    setWidget();

    connect(pUi->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setFromEdit(int)));
    //connect(pUi->comboBox, SIGNAL(editTextChanged(QString)), this, SLOT(_edited(QString)));
}

cFKeyWidget::~cFKeyWidget()
{
    delete pTableShape;
}

bool cFKeyWidget::setWidget()
{
    qlonglong id = _colDescr.toId(_value);
    pUi->pushButtonEdit->setDisabled(pTableShape == NULL || id == NULL_ID);
    int ix = pModel->indexOf(id);
    if (ix < 0) return false;
    pUi->comboBox->setCurrentIndex(ix);
    return true;
}

 int cFKeyWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (1 == r) {
        if (!setWidget()) r = -1;
    }
    return r;
}

void cFKeyWidget::setFromEdit(int i)
{
    qlonglong id = pModel->atId(i);
    setFromWidget(id);
    // pUi->comboBox()->setCurrentIndex(i);
}

/*
void cFKeyWidget::_edited(QString _txt)
{
    while (!pModel->setFilter(_txt, OT_ASC, FT_BEGIN)) {
        if (_txt.size() == 0) break;
        _txt.chop(1);
        pComboBox()->
    }

}*/

/// Egy tulajdosnság kulcs mezőben vagyunk.
/// Be szertnénk szúrni egy tulajdonság rekordot
void cFKeyWidget::insertF()
{
    cRecordDialog *pDialog = new cRecordDialog(*pTableShape, ENUM2SET2(DBT_OK, DBT_CANCEL), true, _pParentDialog);
    while (1) {
        int keyId = pDialog->exec(false);
        if (keyId == DBT_CANCEL) break;
        if (!pDialog->accept()) continue;
        if (!cErrorMessageBox::condMsgBox(pDialog->record().tryInsert(*pq))) continue;
        pModel->setFilter(_sNul, OT_ASC, FT_NO);    // Refresh combo box
        pUi->comboBox->setCurrentIndex(pModel->indexOf(pDialog->record().getId()));
        break;
    }
    pDialog->close();
    delete pDialog;
    return;
}

void cFKeyWidget::modifyF()
{
    cRecordAny rec(pRDescr);
    cRecordDialog *pDialog = new cRecordDialog(*pTableShape, ENUM2SET2(DBT_OK, DBT_CANCEL), true, _pParentDialog);
    int cix = pUi->comboBox->currentIndex();
    qlonglong id = pModel->atId(cix);
    if (!rec.fetchById(*pq, id)) return;
    pDialog->restore(&rec);
    while (1) {
        int keyId = pDialog->exec(false);
        if (keyId == DBT_CANCEL) break;
        if (!pDialog->accept()) continue;
        if (!cErrorMessageBox::condMsgBox(pDialog->record().tryUpdate(*pq, true))) continue;
        pModel->setFilter(_sNul, OT_ASC, FT_NO);    // Refresh combo box
        pUi->comboBox->setCurrentIndex(pModel->indexOf(rec.getId()));
        break;
    }
    pDialog->close();
    delete pDialog;
    return;
}

// Megváltozott az owner id
void cFKeyWidget::modifyOwnerId(cFieldEditBase* pof)
{
    ownerId = pof->getId();
    pModel->setFilter(ownerId, OT_ASC, FT_FKEY_ID);
    pUi->comboBox->setCurrentIndex(0);
    setFromEdit(0);
}

/* **************************************** cDateWidget ****************************************  */


cDateWidget::cDateWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, false, _par)
{
    _wType = FEW_DATE;
    QDateEdit * pDE = new QDateEdit(_par == NULL ? NULL : _par->pWidget());
    _pWidget = pDE;
    connect(pDE, SIGNAL(dateChanged(QDate)),  this, SLOT(setFromEdit(QDate)));
}

cDateWidget::~cDateWidget()
{
    ;
}

int cDateWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1) {
        QDateEdit * pDE = (QDateEdit *)_pWidget;
        pDE->setDate(_value.toDate());
    }
    return r;
}

void cDateWidget::setFromEdit(QDate d)
{
    setFromWidget(QVariant(d));
}

/* **************************************** cTimeWidget ****************************************  */

cTimeWidget::cTimeWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, false, _par)
{
    if (_readOnly && _par != NULL) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + __fr.record().identifying());
    _wType = FEW_TIME;
    QTimeEdit * pTE = new QTimeEdit(_par == NULL ? NULL : _par->pWidget());
    _pWidget = pTE;
    connect(pTE, SIGNAL(TimeChanged(QTime)),  this, SLOT(setFromEdit(QTime)));
}

cTimeWidget::~cTimeWidget()
{
    ;
}

int cTimeWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1) {
        QTimeEdit * pTE = (QTimeEdit *)_pWidget;
        pTE->setTime(_value.toTime());
    }
    return r;
}

void cTimeWidget::setFromEdit(QTime d)
{
    setFromWidget(QVariant(d));
}

/* **************************************** cDateTimeWidget ****************************************  */

cDateTimeWidget::cDateTimeWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr,false, _par)
{
    if (_readOnly && _par != NULL) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + __fr.record().identifying());
    _wType = FEW_DATE_TIME;
    QDateTimeEdit * pDTE = new QDateTimeEdit(_par == NULL ? NULL : _par->pWidget());
    _pWidget = pDTE;
    connect(pDTE, SIGNAL(dateTimeChanged(QDateTime)),  this, SLOT(setFromEdit(QDateTime)));
}

cDateTimeWidget::~cDateTimeWidget()
{
    ;
}

int cDateTimeWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1) {
        QDateTimeEdit * pDTE = (QDateTimeEdit *)_pWidget;
        pDTE->setDateTime(_value.toDateTime());
    }
    return r;
}

void cDateTimeWidget::setFromEdit(QDateTime d)
{
   setFromWidget(QVariant(d));
}

/* **************************************** cIntervalWidget ****************************************  */


cIntervalWidget::cIntervalWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
{
    _wType = FEW_INTERVAL;
    _pWidget = new QWidget(_par == NULL ? NULL : _par->pWidget());
    pLayout = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    pLineEdDay = new QLineEdit(_pWidget);
    pLayout->addWidget(pLineEdDay);
    pLabelDay = new QLabel(tr(" Nap "), _pWidget);
    pLayout->addWidget(pLabelDay);
    pTimeEd = new QTimeEdit(_pWidget);
    pLayout->addWidget(pTimeEd);
    pTimeEd->setDisplayFormat("HH:mm:ss.zzz");
    if (_readOnly) {
        pValidatorDay = NULL;
        view();
        pLineEdDay->setReadOnly(true);
        pTimeEd->setReadOnly(true);
    }
    else {
        pValidatorDay = new QIntValidator(0, 9999, _pWidget);
        pLineEdDay->setValidator(pValidatorDay);
        view();
        connect(pLineEdDay, SIGNAL(editingFinished()),  this, SLOT(setFromEdit()));
        connect(pTimeEd,    SIGNAL(editingFinished()),  this, SLOT(setFromEdit()));
    }
}

cIntervalWidget::~cIntervalWidget()
{
    ;
}

qlonglong cIntervalWidget::getFromWideget() const
{
    QTime t = pTimeEd->time();
    qlonglong r;
    r = pLineEdDay->text().toInt();
    r = (r *   24) + t.hour();
    r = (r *   60) + t.minute();
    r = (r *   60) + t.second();
    r = (r * 1000) + t.msec();
    return r;
}

int cIntervalWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1) view();
    return r;
}

void cIntervalWidget::view()
{
    if (_value.isValid()) {
        qlonglong v = _colDescr.toId(_value);
        int msec   = v % 1000; v /= 1000;
        int sec    = v %   60; v /=   60;
        int minute = v %   60; v /=   60;
        int hour   = v %   24; v /=   24;
        QTime t(hour, minute, sec, msec);
        pTimeEd->setTime(t);
        pLineEdDay->setText(QString::number(v));
    }
    else {
        pTimeEd->clear();
        pLineEdDay->clear();
    }
}

void cIntervalWidget::setFromEdit()
{
    setFromWidget(getFromWideget());
}

/* **************************************** cBinaryWidget ****************************************  */

cBinaryWidget::cBinaryWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
    , data()
{
    _init();
    bool z = _value.isNull();
    if (!z) data = _value.toByteArray();
    pRadioButtonNULL->setChecked(z);
    setViewButtons();
}

cBinaryWidget::~cBinaryWidget()
{
    pDelete(pCImage);
    pDelete(pImageWidget);
}

void cBinaryWidget::_init()
{
    _wType = FEW_BINARY;
    pLayout         = NULL;
    pRadioButtonNULL    = NULL;
    pLoadButton     = NULL;
    pViewButton     = NULL;
    pZoomInButton   = NULL;
    pZoomOutButton  = NULL;
    pImageWidget    = NULL;
    pCImage         = NULL;
    const cRecStaticDescr& cidescr = cImage().descr();
    isCImage   = _recDescr == cidescr;     // éppen egy cImage objektumot szerkesztünk

    _pWidget        = new QWidget(_pParentDialog == NULL ? NULL :_pParentDialog->pWidget());
    pLayout         = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    pRadioButtonNULL    = new QRadioButton(_sNULL, _pWidget);
    pRadioButtonNULL->setDisabled(_readOnly);
    pLayout->addWidget(pRadioButtonNULL);
    if (!_readOnly) {
        pLoadButton  = new QPushButton(trUtf8("Betölt"), _pWidget);
        pLayout->addWidget(pLoadButton);
        connect(pRadioButtonNULL, SIGNAL(clicked(bool)), this, SLOT(nullChecked(bool)));
        connect(pLoadButton, SIGNAL(pressed()), this, SLOT(loadDataFromFile()));
    }
    if (isCImage) {     // Ha egy cImage objektum része, akkor meg tudjuk jeleníteni.
        pViewButton     = new QPushButton(trUtf8("Megjelenít"));
        pViewButton->setDefault(true);
        pLayout->addWidget(pViewButton);
        pZoomInButton   = new QPushButton(trUtf8("+"));
        pZoomInButton->setDisabled(true);
        pLayout->addWidget(pZoomInButton);
        pZoomOutButton  = new QPushButton(trUtf8("-"));
        pZoomOutButton->setDisabled(true);
        pLayout->addWidget(pZoomOutButton);
        connect(pViewButton, SIGNAL(pressed()), this, SLOT(viewPic()));
        // Ha jó a mező sorrend, akkor ezek a mezők már megvannak.
        connect(anotherField(_sImageType), SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(changedAnyField(cFieldEditBase*)));
    }
}


bool cBinaryWidget::setCImage()
{
    setNull();
    if (!isCImage) EXCEPTION(EPROGFAIL);            // Ha nem cImage a rekord, akkor hogy kerültünk ide ?!
    if (pCImage == NULL) pCImage = new cImage;
    pCImage->clear();
    if (data.size() > 0 && !pRadioButtonNULL->isChecked()) {
        QString type;
        pCImage->setName(_sImageName, anotherField(_sImageName)->getName());
        pCImage->setType(anotherField(_sImageType)->getName());
        pCImage->setImage(data);
        if (pCImage->dataIsPic()) {
            pViewButton->setEnabled(true);
            if (pImageWidget) {
                closePic();
                openPic();
            }
            return true;
        }
    }
    if (pImageWidget) {
        closePic();
    }
    return false;
}

void cBinaryWidget::setViewButtons()
{
    if (pViewButton == NULL) return;
    bool z = data.isEmpty() || !setCImage();
    if (z) {
        pViewButton->setDisabled(true);
    }
}

int cBinaryWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1) {
        bool z = v.isNull();
        pRadioButtonNULL->setChecked(z);
        if (z) {
            data.clear();
        }
        else {
            data = _value.toByteArray();
        }
    }
    if (r != 0) setCImage();
    return r;
}

void cBinaryWidget::loadDataFromFile()
{
    QString fn = QFileDialog::getOpenFileName(pWidget());
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (f.open(QIODevice::ReadOnly) == false && f.isReadable() == false) {
        QMessageBox::warning(pWidget(), dcViewShort(DC_ERROR), trUtf8("A megadott %1 nevű fájl nem olvasható").arg(fn));
        return;
    }
    data = f.readAll();
    pRadioButtonNULL->setChecked(false);
    pRadioButtonNULL->setCheckable(true);
    setFromWidget(QVariant(data));
    setCImage();
}

void cBinaryWidget::setNull()
{
    if (data.isEmpty()) {
        pRadioButtonNULL->setChecked(true);
        pRadioButtonNULL->setDisabled(true);
    }
    else {
        pRadioButtonNULL->setEnabled(true);
    }
    bool f = pRadioButtonNULL->isChecked();
    if (f) {
        _value.clear();
    }
}

void cBinaryWidget::nullChecked(bool checked)
{
    (void)checked;
    setCImage();
}

void cBinaryWidget::closePic()
{
    if (pImageWidget == NULL) EXCEPTION(EPROGFAIL);
    pZoomInButton->setDisabled(true);
    pZoomOutButton->setDisabled(true);
    pImageWidget->close();
    pDelete(pImageWidget);
}

void cBinaryWidget::openPic()
{
    if (pImageWidget) EXCEPTION(EPROGFAIL);
    pImageWidget = new cImageWidget;
    pImageWidget->setImage(*pCImage);
    pZoomInButton->setEnabled(true);
    pZoomOutButton->setEnabled(true);
    connect(pImageWidget,   SIGNAL(destroyed(QObject*)), this, SLOT(destroyedImage(QObject*)));
    connect(pZoomInButton,  SIGNAL(pressed()), pImageWidget, SLOT(zoomIn()));
    connect(pZoomOutButton, SIGNAL(pressed()), pImageWidget, SLOT(zoomOut()));
}

void cBinaryWidget::viewPic()
{
    if (pImageWidget != NULL) {
        pImageWidget->show();
        return;
    }
    if (pCImage == NULL) EXCEPTION(EPROGFAIL);
    if (pCImage->dataIsPic()) openPic();
}

void cBinaryWidget::changedAnyField(cFieldEditBase *p)
{
    (void)p;
    setCImage();
}

void cBinaryWidget::destroyedImage(QObject *p)
{
    (void)p;
    pImageWidget = NULL;
}

/* **************************************** cFKeyArrayWidget ****************************************  */


cFKeyArrayWidget::cFKeyArrayWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
    , last()
{
    _wType   = FEW_FKEY_ARRAY;

    pFRecModel = NULL;
    _pWidget = new QWidget(_par == NULL ? NULL : _par->pWidget());
    pUi      = new Ui_fKeyArrayEd;
    pUi->setupUi(_pWidget);

    selectedNum = 0;

    pUi->pushButtonAdd->setDisabled(_readOnly);
    pUi->pushButtonIns->setDisabled(_readOnly);
    pUi->pushButtonUp->setDisabled(_readOnly);
    pUi->pushButtonDown->setDisabled(_readOnly);
    pUi->pushButtonDel->setDisabled(_readOnly);
    pUi->pushButtonClr->setDisabled(_readOnly);

    pArrayModel = new cStringListModel(pWidget());
    pRDescr = cRecStaticDescr::get(_colDescr.fKeyTable, _colDescr.fKeySchema);
    cRecordAny  r(pRDescr);
    foreach (QVariant vId, _value.toList()) {
        valueView << r.getNameById(*pq, vId.toLongLong());
    }
    pArrayModel->setStringList(valueView);
    pUi->listView->setModel(pArrayModel);
    pUi->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (!_readOnly) {
        pFRecModel  = new cRecordListModel(*pRDescr, pWidget());
        pUi->comboBox->setModel(pFRecModel);
        pFRecModel->setFilter();
        pUi->comboBox->setCurrentIndex(0);

        connect(pUi->pushButtonAdd, SIGNAL(pressed()), this, SLOT(addRow()));
        connect(pUi->pushButtonDel, SIGNAL(pressed()), this, SLOT(delRow()));
        connect(pUi->pushButtonClr, SIGNAL(pressed()), this, SLOT(clrRows()));
        connect(pUi->listView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(selectionChanged(QModelIndex,QModelIndex)));
        connect(pUi->listView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(doubleClickRow(QModelIndex)));
    }
}

cFKeyArrayWidget::~cFKeyArrayWidget()
{
    ;
}

int cFKeyArrayWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (1 == r) {
        valueView.clear();
        cRecordAny  r(pRDescr);
        foreach (QVariant vId, _value.toList()) {
            valueView << r.getNameById(*pq, vId.toLongLong());
        }
        pArrayModel->setStringList(valueView);
        setButtons();
    }
    return r;
}

void cFKeyArrayWidget::setButtons()
{
    bool eArr = pArrayModel->isEmpty();
    bool sing = selectedNum == 1;
    bool any  = selectedNum > 0;

    pUi->pushButtonAdd ->setDisabled(                _readOnly);
    pUi->pushButtonIns ->setDisabled(        sing || _readOnly);
    pUi->pushButtonUp  ->setDisabled(        any  || _readOnly);
    pUi->pushButtonDown->setDisabled(        any  || _readOnly);
    pUi->pushButtonDel ->setDisabled(eArr         || _readOnly);
    pUi->pushButtonClr ->setDisabled(eArr         || _readOnly);
}

// cFKeyArrayWidget SLOTS

void cFKeyArrayWidget::selectionChanged(QModelIndex cur, QModelIndex)
{
    DBGFN();
    if (cur.isValid()) {
        actIndex = cur;
        PDEB(INFO) << "Current row = " << actIndex.row() << endl;
    }
    else {
        actIndex = QModelIndex();
        PDEB(INFO) << "No current row." << endl;
    }
    setButtons();
}

void cFKeyArrayWidget::addRow()
{
    int ix = pUi->comboBox->currentIndex();
    qlonglong id = pFRecModel->atId(ix);
    QString   nm = pFRecModel->at(ix);
    *pArrayModel << nm;
    QVariantList nv = _value.toList();
    nv << id;
    setFromWidget(nv);
    setButtons();
}

void cFKeyArrayWidget::insRow()
{
    int ix = pUi->comboBox->currentIndex();
    qlonglong id = pFRecModel->atId(ix);
    QString   nm = pFRecModel->at(ix);
    int row = actIndex.row();
    pArrayModel->insert(nm, row);
    QVariantList nv = _value.toList();
    nv.insert(row, id);
    setFromWidget(nv);
    setButtons();
}

void cFKeyArrayWidget::upRow()
{ /*
    QModelIndexList mil = pUi->listView->selectionModel()->selectedRows();
    pModel->up(mil);
    setFromWidget(pModel->stringList());
    setButtons(); */
}

void cFKeyArrayWidget::downRow()
{ /*
    QModelIndexList mil = pUi->listView->selectionModel()->selectedRows();
    pModel->down(mil);
    setFromWidget(pModel->stringList());
    setButtons(); */
}

void cFKeyArrayWidget::delRow()
{
    QModelIndexList mil = pUi->listView->selectionModel()->selectedIndexes();
    QVariantList nv = _value.toList();
    if (mil.size() > 0) {
        pArrayModel->remove(mil);
        QVector<int> rows = mil2rowsDesc(mil);
        foreach (int ix, rows) {
            nv.removeAt(ix);
        }
    }
    else {
        pArrayModel->pop_back();
        nv.pop_back();
    }
    setFromWidget(nv);
    setButtons();
}

void cFKeyArrayWidget::clrRows()
{
    pArrayModel->clear();
    setFromWidget(QVariantList());
    setButtons();
}

void cFKeyArrayWidget::doubleClickRow(const QModelIndex & index)
{
    (void)index;
    /*
    const QStringList& sl = pModel->stringList();
    int row = index.row();
    if (isContIx(sl, row)) {
        pUi->lineEdit->setText(sl.at(row));
    }*/
}

/* **************************************** cColorWidget ****************************************  */


cColorWidget::cColorWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par), pixmap(24, 24)
{
    _wType = FEW_COLOR;
    _pWidget = new QWidget(_par == NULL ? NULL : _par->pWidget());
    QHBoxLayout *pLayout = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    pLineEdit = new QLineEdit(_value.toString());
    pLineEdit->setReadOnly(_fl);
    pLayout->addWidget(pLineEdit, 1);
    pLabel = new QLabel;
    pLayout->addWidget(pLabel);
    if (!_readOnly) {
        QToolButton *pButton = new QToolButton;
        pButton->setIcon(QIcon("://colorize.ico"));
        pLayout->addWidget(pButton, 0);
        connect(pLineEdit, SIGNAL(textChanged(QString)),  this, SLOT(setFromEdit(QString)));
        connect(pButton,   SIGNAL(pressed()),             this, SLOT(colorDialog()));
        sTitle = trUtf8("Szín kiválasztása");
    }
    setColor(_value.toString());
    pLayout->addStretch();
}

cColorWidget::~cColorWidget()
{
    ;
}

void cColorWidget::setColor(const QString& text)
{
    color.setNamedColor(text);
    if (color.isValid()) {
        pixmap.fill(color);
        pLabel->setPixmap(pixmap);
    }
    else {
        QIcon icon("://dialog-no.ico");
        pLabel->setPixmap(icon.pixmap(pixmap.size()));
    }
}

int cColorWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1) {
        QString cn = v.toString();
        pLineEdit->setText(cn);
    }
    return r;
}

void cColorWidget::setFromEdit(const QString& text)
{
    if (text.isEmpty()) setFromWidget(QVariant());
    else                setFromWidget(QVariant(text));
    setColor(text);
}

void cColorWidget::colorDialog()
{
    QColor c = QColorDialog::getColor(color, pWidget(), sTitle);
    if (c.isValid()) {
        pLineEdit->setText(c.name());
    }
}

/* **************************************** cFontFamilyWidget ****************************************  */


cFontFamilyWidget::cFontFamilyWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, false, _par)
    , iconNull("://dialog-no.ico"), iconNotNull("://dialog-no-off.png")
{
    if (_readOnly && _par != NULL) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + __fr.record().identifying());
    _wType = FEW_FONT_FAMILY;
    _pWidget = new QWidget(_par == NULL ? NULL : _par->pWidget());
    QHBoxLayout *pLayout = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    bool isNull = __fr.isNull();
    pToolButtonNull = new QToolButton;
    pToolButtonNull->setIcon(isNull ? iconNull : iconNotNull);
    pToolButtonNull->setIcon(iconNull);
    pToolButtonNull->setCheckable(true);
    pToolButtonNull->setChecked(isNull);
    pLayout->addWidget(pToolButtonNull);

    pFontComboBox = new QFontComboBox;
    pFontComboBox->setDisabled(isNull);
    if (!isNull) pFontComboBox->setCurrentFont(QFont((QString)__fr));
    pLayout->addWidget(pFontComboBox);
    pLayout->addStretch(0);
    connect(pToolButtonNull, SIGNAL(toggled(bool)),             this, SLOT(togleNull(bool)));
    connect(pFontComboBox,   SIGNAL(currentFontChanged(QFont)), this, SLOT(changeFont(QFont)));
}

cFontFamilyWidget::~cFontFamilyWidget()
{
    ;
}


int cFontFamilyWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1) {
        bool isNull = v.isNull();
        pToolButtonNull->setChecked(isNull);
        pFontComboBox->setDisabled(isNull);
        if (!isNull) pFontComboBox->setCurrentFont(QFont(v.toString()));
    }
    return r;
}

void cFontFamilyWidget::togleNull(bool f)
{
    pFontComboBox->setDisabled(f);
    setFromWidget(f ? QVariant(pFontComboBox->currentText()) : QVariant());
    pToolButtonNull->setIcon(f ? iconNull : iconNotNull);
}

void cFontFamilyWidget::changeFont(const QFont&)
{
    setFromWidget(QVariant(pFontComboBox->currentText()));
}


/* **************************************** cFontAttrWidget ****************************************  */

const QString cFontAttrWidget::sEnumTypeName = "fontattr";

cFontAttrWidget::cFontAttrWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
    , iconNull("://dialog-no.ico"), iconNotNull("://dialog-no-off.png")
    , iconBold("://icons/format-text-bold.ico"), iconBoldNo("://icons/format-text-bold-no.png")
    , iconItalic("://icons/format-text-italic.ico"), iconItalicNo("://icons/format-text-italic-no.png")
    , iconUnderline("://icons/format-text-underline.ico"), iconUnderlineNo("://icons/format-text-underline-no.png")
    , iconStrikeout("://icons/format-text-strikethrough.ico"), iconStrikeoutNo("://icons/format-text-strikethrough-no.png")
    , iconSize(22,22)
{
    _wType = FEW_FONT_ATTR;
    bool isNull = __fr.isNull();
    m = isNull ? 0 : (qlonglong)__fr;
    _pWidget = new QWidget(_par == NULL ? NULL : _par->pWidget());
    QHBoxLayout *pLayout = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    QWidget *pW;
    QIcon icon = isNull ? iconNull : iconNotNull;
    if (_readOnly) {
        pW = pLabelNull = new QLabel;
        pLabelNull->setPixmap(icon.pixmap(iconSize));
        pToolButtonNull = NULL;
    }
    else {
        pW = pToolButtonNull = new QToolButton();
        pToolButtonNull->setIcon(icon);
        pToolButtonNull->setCheckable(true);
        pToolButtonNull->setChecked(isNull);
        pLabelNull = NULL;
    }
    pLayout->addWidget(pW);

    bool f = m & ENUM2SET(FA_BOOLD);
    icon = f ? iconBold : iconBoldNo;
    if (_readOnly) {
        pW = pLabelBold = new QLabel;
        pLabelBold->setPixmap(icon.pixmap(iconSize));
        pToolButtonBold = NULL;
    }
    else {
        pW = pToolButtonBold = new QToolButton();
        pToolButtonBold->setIcon(icon);
        pToolButtonBold->setCheckable(true);
        pToolButtonBold->setChecked(f);
        pToolButtonBold->setDisabled(isNull);
        pLabelBold = NULL;
    }
    pLayout->addWidget(pW);

    f = m & ENUM2SET(FA_ITALIC);
    icon = f ? iconItalic : iconItalicNo;
    if (_readOnly) {
        pW = pLabelItalic = new QLabel;
        pLabelItalic->setPixmap(icon.pixmap(iconSize));
        pToolButtonItalic = NULL;
    }
    else {
        pW = pToolButtonItalic = new QToolButton();
        pToolButtonItalic->setIcon(icon);
        pToolButtonItalic->setCheckable(true);
        pToolButtonItalic->setChecked(f);
        pToolButtonItalic->setDisabled(isNull);
        pLabelItalic = NULL;
    }
    pLayout->addWidget(pW);

    f = m & ENUM2SET(FA_UNDERLINE);
    icon = f ? iconUnderline : iconUnderlineNo;
    if (_readOnly) {
        pW = pLabelUnderline = new QLabel;
        pLabelUnderline->setPixmap(icon.pixmap(iconSize));
        pToolButtonUnderline = NULL;
    }
    else {
        pW = pToolButtonUnderline  = new QToolButton();
        pToolButtonUnderline->setIcon(icon);
        pToolButtonUnderline->setCheckable(true);
        pToolButtonUnderline->setChecked(f);
        pToolButtonUnderline->setDisabled(isNull);
        pLabelUnderline = NULL;
    }
    pLayout->addWidget(pW);

    f = m & ENUM2SET(FA_STRIKEOUT);
    icon = f ? iconStrikeout : iconStrikeoutNo;
    if (_readOnly) {
        pW = pLabelStrikeout = new QLabel;
        pLabelStrikeout->setPixmap(icon.pixmap(iconSize));
        pToolButtonStrikeout = NULL;
    }
    else {
        pW = pToolButtonStrikeout  = new QToolButton();
        pToolButtonStrikeout->setIcon(icon);
        pToolButtonStrikeout->setCheckable(true);
        pToolButtonStrikeout->setChecked(f);
        pToolButtonStrikeout->setDisabled(isNull);
        pLabelStrikeout = NULL;
    }
    pLayout->addWidget(pW);
    pLayout->addStretch(0);

    QSqlQuery q = getQuery();
    pEnumType = cColEnumType::fetchOrGet(q, sEnumTypeName);
    if (!_readOnly) {
        connect(pToolButtonNull,      SIGNAL(toggled(bool)), this, SLOT(togleNull(bool)));
        connect(pToolButtonBold,      SIGNAL(toggled(bool)), this, SLOT(togleBoold(bool)));
        connect(pToolButtonItalic,    SIGNAL(toggled(bool)), this, SLOT(togleItelic(bool)));
        connect(pToolButtonUnderline, SIGNAL(toggled(bool)), this, SLOT(togleUnderline(bool)));
        connect(pToolButtonStrikeout, SIGNAL(toggled(bool)), this, SLOT(togleStrikeout(bool)));
    }
}

cFontAttrWidget::~cFontAttrWidget()
{
    ;
}


int cFontAttrWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1) {
        bool isNull = v.isNull();
        if (_readOnly) {
            m = pEnumType->lst2set(v.toStringList());
            pLabelNull->     setPixmap((isNull                     ? iconNull      : iconNotNull    ).pixmap(iconSize));
            pLabelBold->     setPixmap((m & ENUM2SET(FA_BOOLD)     ? iconBold      : iconBoldNo     ).pixmap(iconSize));
            pLabelItalic->   setPixmap((m & ENUM2SET(FA_ITALIC)    ? iconItalic    : iconItalicNo   ).pixmap(iconSize));
            pLabelUnderline->setPixmap((m & ENUM2SET(FA_UNDERLINE) ? iconUnderline : iconUnderlineNo).pixmap(iconSize));
            pLabelStrikeout->setPixmap((m & ENUM2SET(FA_STRIKEOUT) ? iconStrikeout : iconStrikeoutNo).pixmap(iconSize));
        }
        else {
            if (!isNull) {
                m = pEnumType->lst2set(v.toStringList());
                pToolButtonBold->setChecked(m & ENUM2SET(FA_BOOLD));
                pToolButtonItalic->setChecked(m & ENUM2SET(FA_ITALIC));
                pToolButtonUnderline->setChecked(m & ENUM2SET(FA_UNDERLINE));
                pToolButtonStrikeout->setChecked(m & ENUM2SET(FA_STRIKEOUT));
            }
            pToolButtonBold->setDisabled(isNull);
            pToolButtonItalic->setDisabled(isNull);
            pToolButtonUnderline->setDisabled(isNull);
            pToolButtonStrikeout->setDisabled(isNull);
        }
    }
    return r;
}

void cFontAttrWidget::togleNull(bool f)
{
    setFromWidget(f ? QVariant() : QVariant(m));
    pToolButtonNull->setIcon(f ? iconNull : iconNotNull);
    pToolButtonBold->setDisabled(f);
    pToolButtonItalic->setDisabled(f);
    pToolButtonUnderline->setDisabled(f);
    pToolButtonStrikeout->setDisabled(f);
}

void cFontAttrWidget::togleBoold(bool f)
{
    if (f == ((bool)(m & ENUM2SET(FA_BOOLD)))) return;
    if (f) m |=  ENUM2SET(FA_BOOLD);
    else   m &= ~ENUM2SET(FA_BOOLD);
    setFromWidget(QVariant(m));
    pToolButtonBold->setIcon(f ? iconBold : iconBoldNo);
}

void cFontAttrWidget::togleItelic(bool f)
{
    if (f == ((bool)(m & ENUM2SET(FA_ITALIC)))) return;
    if (f) m |=  ENUM2SET(FA_ITALIC);
    else   m &= ~ENUM2SET(FA_ITALIC);
    setFromWidget(QVariant(m));
    pToolButtonItalic->setIcon(f ? iconItalic : iconItalicNo);
}

void cFontAttrWidget::togleUnderline(bool f)
{
    if (f == ((bool)(m & ENUM2SET(FA_UNDERLINE)))) return;
    if (f) m |=  ENUM2SET(FA_UNDERLINE);
    else   m &= ~ENUM2SET(FA_UNDERLINE);
    setFromWidget(QVariant(m));
    pToolButtonUnderline->setIcon(f ? iconUnderline : iconUnderlineNo);
}

void cFontAttrWidget::togleStrikeout(bool f)
{
    if (f == ((bool)(m & ENUM2SET(FA_STRIKEOUT)))) return;
    if (f) m |=  ENUM2SET(FA_STRIKEOUT);
    else   m &= ~ENUM2SET(FA_STRIKEOUT);
    setFromWidget(QVariant(m));
    pToolButtonStrikeout->setIcon(f ? iconStrikeout : iconStrikeoutNo);
}

/* **** **** */

cSelectPlace::cSelectPlace(QComboBox *_pZone, QComboBox *_pPLace, QLineEdit *_pFilt, const QString& _constFilt)
    : QObject()
    , pComboBoxZone(_pZone)
    , pComboBoxPLace(_pPLace)
    , pLineEditPlaceFilt(_pFilt)
    , constFilterPlace(_constFilt)
{
    blockPlaceSignal = false;
    pZoneModel = new cZoneListModel(this);
    pComboBoxZone->setModel(pZoneModel);
    pPlaceModel = new cPlacesInZoneModel(this);
    pComboBoxPLace->setModel(pPlaceModel);
    pComboBoxPLace->setCurrentIndex(0);
    if (!constFilterPlace.isEmpty()) {
        pPlaceModel->setConstFilter(constFilterPlace, FT_SQL_WHERE);
    }
    connect(pComboBoxZone,  SIGNAL(currentIndexChanged(int)),   this, SLOT(_zoneChanged(int)));
    connect(pComboBoxPLace, SIGNAL(currentIndexChanged(int)),   this, SLOT(_placeChanged(int)));
    if (_pFilt != NULL) {
        connect(pLineEditPlaceFilt, SIGNAL(textChanged(QString)),    this, SLOT(_placePatternChanged(QString)));
    }
}

void cSelectPlace::_zoneChanged(int ix)
{
    qlonglong id = pZoneModel->atId(ix);
    pPlaceModel->setZone(id);
}

void cSelectPlace::_placeChanged(int ix)
{
    if (blockPlaceSignal) return;
    QString   s  = pPlaceModel->at(ix);
    placeNameChanged(s);
    qlonglong id = pPlaceModel->atId(ix);
    placeIdChanged(id);
}

void cSelectPlace::_placePatternChanged(const QString& s)
{
    qlonglong pid = pPlaceModel->atId(pComboBoxPLace->currentIndex());
    blockPlaceSignal = true;
    if (s.isEmpty()) {
        pPlaceModel->setFilter(QVariant(), OT_DEFAULT, FT_NO);
    }
    else {
        pPlaceModel->setFilter(s, OT_DEFAULT, FT_LIKE);
    }
    int pix = pPlaceModel->indexOf(pid);
    if (pix < 0) {  // Nincs már megfelelő place -> NULL
        pComboBoxPLace->setCurrentIndex(0);
        blockPlaceSignal = false;
        _placeChanged(0);
    }
    else {          // A node változatlan (csak az indexe változhatott)
        pComboBoxPLace->setCurrentIndex(pix);
        blockPlaceSignal = false;
    }
}

/* **** **** */

cSelectNode::cSelectNode(QComboBox *_pZone, QComboBox *_pPlace, QComboBox *_pNode,
            QLineEdit *_pPlaceFilt, QLineEdit *_pNodeFilt,
            const QString& _placeConstFilt, const QString& _nodeConstFilt)
    : cSelectPlace(_pZone, _pPlace, _pPlaceFilt, _placeConstFilt)
    , pComboBoxNode(_pNode)
    , pLineEditNodeFilt(_pNodeFilt)
    , constFilterNode(_nodeConstFilt)
{
    blockNodeSignal = false;
    pNodeModel = NULL;
    setNodeModel(new cRecordListModel(cPatch().descr(), this), TS_TRUE);
    connect(this, SIGNAL(placeIdChanged(qlonglong)), this, SLOT(_placeIdChanged(qlonglong)));
    if (pLineEditNodeFilt != NULL) {
        connect(pLineEditNodeFilt, SIGNAL(textChanged(QString)),    this, SLOT(_nodePatternChanged(QString)));
    }
    connect(pComboBoxNode,  SIGNAL(currentIndexChanged(int)),   this, SLOT(_nodeChanged(int)));
}

void cSelectNode::setNodeModel(cRecordListModel *  _pNodeModel, eTristate _nullable)
{
    setBool(_pNodeModel->nullable, _nullable);
    pComboBoxNode->setModel(_pNodeModel);
    if (!constFilterNode.isEmpty()) {
        _pNodeModel->setConstFilter(constFilterNode, FT_SQL_WHERE);
    }
    pDelete(pNodeModel);
    pNodeModel = _pNodeModel;
}

void cSelectNode::reset()
{
    blockNodeSignal = true;
    pComboBoxZone->setCurrentIndex(0);
    pComboBoxPLace->setCurrentIndex(0);
    pComboBoxNode->setCurrentIndex(0);
    blockNodeSignal = false;
    if (pPlaceModel->rowCount() > 0) {
        placeIdChanged(pPlaceModel->atId(0));
    }
    else {
        placeIdChanged(NULL_ID);
    }
}

void cSelectNode::nodeSetNull(bool _sig)
{
    blockNodeSignal = _sig;
    pComboBoxNode->setCurrentIndex(0);
    blockNodeSignal = false;
}

void cSelectNode::_placeIdChanged(qlonglong pid)
{
    qlonglong nid = pNodeModel->atId(pComboBoxNode->currentIndex());
    blockNodeSignal = true;
    QString sql;
    if (pid == NULL_ID) {
        if (!constFilterNode.isEmpty()) sql = constFilterNode;
        else                            sql = _sTrue;
    }
    else {
        sql = QString("place_id = %1").arg(pid);
        if (!constFilterNode.isEmpty()) sql = "(" + sql + " AND " + constFilterNode + ")";
    }
    pNodeModel->setConstFilter(sql, FT_SQL_WHERE);
    pNodeModel->setFilter();
    int nix = pNodeModel->indexOf(nid);
    if (nix < 0) {  // Nincs már megfelelő node -> NULL
        pComboBoxNode->setCurrentIndex(0);
        blockNodeSignal = false;
        _nodeChanged(0);
    }
    else {          // A node változatlan (csak az indexe változhatott)
        pComboBoxNode->setCurrentIndex(nix);
        blockNodeSignal = false;
    }
}

void cSelectNode::_nodePatternChanged(const QString& s)
{
    qlonglong nid = pNodeModel->atId(pComboBoxNode->currentIndex());
    blockNodeSignal = true;
    if (s.isEmpty()) {
        pNodeModel->setFilter(QVariant(), OT_DEFAULT, FT_NO);
    }
    else {
        pNodeModel->setFilter(s, OT_DEFAULT, FT_LIKE);
    }
    int nix = pNodeModel->indexOf(nid);
    if (nix < 0) {  // Nincs már megfelelő node -> NULL
        pComboBoxNode->setCurrentIndex(0);
        blockNodeSignal = false;
        _nodeChanged(0);
    }
    else {          // A node változatlan (csak az indexe változhatott)
        pComboBoxNode->setCurrentIndex(nix);
        blockNodeSignal = false;
    }
}

void cSelectNode::_nodeChanged(int ix)
{
    if (blockNodeSignal) return;
    QString   s  = pNodeModel->at(ix);
    nodeNameChanged(s);
    qlonglong id = pNodeModel->atId(ix);
    nodeIdChanged(id);
}

/* *** */

cStringMapEdit::cStringMapEdit(bool _isDialog, tStringMap& _map, QWidget *par)
    : QObject(par), isDialog(_isDialog), map(_map)
{
    pButtons = NULL;
    pTableWidget = new QTableWidget(par);
    if (isDialog) {
        _pWidget = new QDialog(par);
        _pWidget->setWindowTitle(trUtf8("Paraméterek"));
        QVBoxLayout *layout = new QVBoxLayout;
        _pWidget->setLayout(layout);
        layout->addWidget(pTableWidget, 1);
        pButtons = new cDialogButtons(ENUM2SET(DBT_OK));
        layout->addWidget(pButtons->pWidget());
        connect(pButtons, SIGNAL(buttonClicked(int)), this, SLOT(clicked(int)));
    }
    else {
        _pWidget = pTableWidget;
    }
    int rows = 0;
    pTableWidget->setColumnCount(2);
    Qt::ItemFlags flagConst = Qt::ItemIsEnabled;
    Qt::ItemFlags flagEdit  = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    QTableWidgetItem *pi;
    pi = new QTableWidgetItem(trUtf8("Név"));
    pi->setFlags(flagConst);
    pTableWidget->setItem(0,0,pi);
    pi = new QTableWidgetItem(trUtf8("Érték"));
    pi->setFlags(flagConst);
    pTableWidget->setItem(0,1,pi);
    foreach (QString n, map.keys()) {
        rows++;
        pTableWidget->setRowCount(rows);
        pi = new QTableWidgetItem(n);
        pi->setFlags(flagConst);
        pTableWidget->setItem(rows -1,0,pi);
        pi = new QTableWidgetItem(map[n]);
        pi->setFlags(flagEdit);
        pTableWidget->setItem(rows -1,1,pi);
    }
    connect(pTableWidget, SIGNAL(cellChanged(int,int)), this, SLOT(changed(int,int)));
}

cStringMapEdit::~cStringMapEdit()
{
    delete _pWidget;
}

QDialog& cStringMapEdit::dialog()
{
    if (!isDialog) EXCEPTION(EPROGFAIL);
    return *(QDialog *)_pWidget;
}

void cStringMapEdit::clicked(int id)
{
    switch (id) {
    case DBT_OK:    dialog().accept();  break;
    case DBT_CANCEL:dialog().reject();  break;
    default:        EXCEPTION(EPROGFAIL);
    }
}

void cStringMapEdit::changed(int row, int column)
{
    if (column != 1) return;
    QString n = pTableWidget->takeItem(row, 0)->text();
    map[n] = pTableWidget->takeItem(row, 1)->text();
}
