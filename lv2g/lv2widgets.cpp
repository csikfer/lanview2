#include "record_table.h"
#include "lv2validator.h"
#include "lv2link.h"
#include "record_dialog.h"
#include "object_dialog.h"
#include "ui_polygoned.h"
#include "ui_arrayed.h"
#include "ui_fkeyed.h"
#include "ui_place_ed.h"
#include "ui_rplace_ed.h"
#include "ui_port_ed.h"
#include "ui_fkeyarrayed.h"
#include "mainwindow.h"

#include "popupreport.h"

bool pixmap(const cImage& o, QPixmap &_pixmap)
{
    bool f;
    f = o.dataIsPic();
    if (f) {
        const char * _type = o._getType();
        QByteArray   _data = o.getImage();
        if (!_pixmap.loadFromData(_data, _type)) return false;
        return true;
    }
    return false;
}

bool setPixmap(const cImage& im, QLabel *pw)
{
    QPixmap pm;
    bool r = pixmap(im, pm);
    pw->setPixmap(pm);
    return r;
}

bool setPixmap(QSqlQuery& q, qlonglong iid, QLabel *pw)
{
    if (iid == NULL_ID) {
        pw->setPixmap(QPixmap());
        return false;
    }
    cImage im;
    im.setById(q, iid);
    return setPixmap(im, pw);
}

/* ***************************************** cSelectLanguage ***************************************** */

cSelectLanguage::cSelectLanguage(QComboBox *_pComboBox, QLabel * _pFlag, bool _nullable, QObject *_pPar)
    : QObject(_pPar)
{
    pComboBox = _pComboBox;
    pFlag     = _pFlag;
    pModel = new cRecordListModel(_sLanguages);
    pModel->joinWith(pComboBox);
    pModel->nullable = _nullable;
    pModel->setFilter();
    int ix;
    if (_nullable) {
        ix = 0;
    }
    else {
        QSqlQuery q = getQuery();
        int id = getLanguageId(q);
        ix = pModel->indexOf(id);
        if (ix < 0) ix = 0;
    }
    pComboBox->setCurrentIndex(ix);
    connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(_languageChanged(int)));
    _languageChanged(ix);
}

void cSelectLanguage::_languageChanged(int ix)
{
    languageIdChanged(int(pModel->atId(ix)));
    if (pFlag != nullptr) {
        QSqlQuery q = getQuery();
        cLanguage language;
        language.fetchById(q, pModel->currendId());
        setPixmap(q, language.getId(_sFlagImage), pFlag);
    }
}

/* ***************************************** cLineWidget ***************************************** */

bool setFormEditWidget(QFormLayout *_fl, QWidget *_lw, QWidget *_ew, eEx __ex)
{
    int row;
    QFormLayout::ItemRole role;
    _fl->getWidgetPosition(_lw, &row, &role);
    if (row < 0 || role != QFormLayout::LabelRole) {
        if (__ex != EX_IGNORE) EXCEPTION(EDATA, row);
        return false;
    }
    _fl->setWidget(row, QFormLayout::FieldRole, _ew);
    return true;
}


cLineWidget::cLineWidget(QWidget *par, bool _ro, bool _horizontal)
    : QWidget(par)
    , pLayout(_horizontal ? static_cast<QLayout *>(new QHBoxLayout) : static_cast<QLayout *>(new QVBoxLayout))
    , pLineEdit(new QLineEdit)
    , pNullButton(static_cast<QToolButton *>(_ro ? new cROToolButton : new QToolButton))
{
    pLineEdit->setReadOnly(_ro);
    pNullButton->setIcon(lv2g::iconNull);
    pNullButton->setCheckable(true);
    pLayout->setMargin(0);
    setLayout(pLayout);
    pLayout->addWidget(pLineEdit);
    pLayout->addWidget(pNullButton);
    if (!_ro) {
        connect(pLineEdit, SIGNAL(textChanged(QString)), this, SLOT(on_LineEdit_textChanged(QString)));
        connect(pNullButton, SIGNAL(toggled(bool)),      this, SLOT(on_NullButton_togled(bool)));
    }
}

void cLineWidget::set(const QVariant& _val)
{
    val = _val;
    bool _isNull = !_val.isValid();
    pNullButton->setChecked(_isNull);
    pLineEdit->setDisabled(_isNull);
    pLineEdit->setText(_val.toString());
}

void cLineWidget::setDisabled(bool _f)
{
    pLineEdit->setDisabled(_f || isNull());
    pNullButton->setDisabled(_f);
}

void cLineWidget::on_NullButton_togled(bool f)
{
    pLineEdit->setDisabled(f);
    QVariant newVal = get();
    if (val == newVal) return;
    val = newVal;
    changed(val);
}

void cLineWidget::on_LineEdit_textChanged(const QString& s)
{
    (void)s;
    QVariant newVal = get();
    if (val == newVal) return;
    val = newVal;
    changed(val);
}

/* **************************************** cComboLineWidget **************************************** */

cComboLineWidget::cComboLineWidget(const cRecordFieldRef& _cfr, const QString& sqlWhere, bool _horizontal, QWidget *par)
    : QWidget(par)
    , pLayout(_horizontal ? static_cast<QLayout *>(new QHBoxLayout) : static_cast<QLayout *>(new QVBoxLayout))
    , pComboBox(new QComboBox)
    , pNullButton(new QToolButton)
    , pModel(new cRecFieldSetOfValueModel(_cfr, QStringList(sqlWhere)))
{
    pComboBox->setEditable(true);
    pModel->joinWith(pComboBox);
    pNullButton->setIcon(lv2g::iconNull);
    pNullButton->setCheckable(true);
    pLayout->setMargin(0);
    setLayout(pLayout);
    pLayout->addWidget(pComboBox);
    pLayout->addWidget(pNullButton);
    connect(pComboBox,   SIGNAL(currentTextChanged(QString)), this, SLOT(on_ComboBox_textChanged(QString)));
    connect(pNullButton, SIGNAL(toggled(bool)),               this, SLOT(on_NullButton_togled(bool)));
}

void cComboLineWidget::set(const QVariant& _val, Qt::MatchFlags flags)
{
    val = _val;
    bool _isNull = !_val.isValid();
    pNullButton->setChecked(_isNull);
    pComboBox->setDisabled(_isNull);
    if (!_isNull) {
        QString s = val.toString();
        pModel->setCurrent(s, flags);
    }
}

void cComboLineWidget::setDisabled(bool _f)
{
    pComboBox->setDisabled(_f || isNull());
    pNullButton->setDisabled(_f);
}

void cComboLineWidget::on_NullButton_togled(bool f)
{
    pComboBox->setDisabled(f);
    QVariant newVal = get();
    if (val == newVal) return;
    val = newVal;
    emit changed(val);
}

void cComboLineWidget::on_ComboBox_textChanged(const QString& s)
{
    (void)s;
    QVariant newVal = get();
    if (val == newVal) return;
    val = newVal;
    emit changed(val);
}

/* **************************************** cImageWidget **************************************** */

double cImageWidget::scale = 1.0;

cImageWidget::cImageWidget(QWidget *__par)
    : QScrollArea(__par)
    , image()
{
    pLabel = nullptr;
}

cImageWidget::~cImageWidget()
{
    ;
}

bool cImageWidget::setImage(const QString& __fn, const QString& __t)
{
    hide();
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

bool cImageWidget::setImage(const cImage& __o, const QString &__t)
{
    hide();
    if (!pixmap(__o, image)) return false;
    setWindowTitle(__t.isEmpty() ? __o.getName() : __t);
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
    image.setDevicePixelRatio(scale);
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


void cImageWidget::zoom(double z)
{
    if (z > 0.1) scale = 1/z;
    else         scale = 10;
    resetImage();
}

/* ********************************************************************************************
   ******************************************************************************************** */

void cROToolButton::mousePressEvent(QMouseEvent *e)
{
    (void)e;
}

void cROToolButton::mouseReleaseEvent(QMouseEvent *e)
{
    (void)e;
}

/* ********************************************************************************************
   ************************************** cFieldEditBase **************************************
   ******************************************************************************************** */

static const QString _sRadioButtons = "radioButtons";
static const QString _sSpinBox      = "spinBox";
static const QString _sHide         = "hide";
static const QString _sAutoset      = "autoset";
static const QString _sCollision    = "collision";
static const QString _sColumn       = "column";
static const QString _sWidgetFilter = "widget_filter";
// _sRefine

QString fieldWidgetType(int _t)
{
    switch (_t) {
    case FEW_UNKNOWN:       return _sUnknown;
    case FEW_SET:           return "cSetWidget";
    case FEW_ENUM_COMBO:    return "cEnumComboWidget";
    case FEW_ENUM_RADIO:    return "cEnumRadioWidget";
    case FEW_LINE:          return "cFieldLineWidget";
    case FEW_LINES:         return "cFieldLineWidget/long";
    case FEW_SPIN_BOX:      return "cFieldSpinBoxWidget";
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
    case FEW_LTEXT:         return "cLTextWidget";
    case FEW_LTEXT_LONG:    return "cLTextWidget/long";
    case FEW_FEATURES:      return "cFeatureWidget";
    case FEW_PARAM_VALUE:   return "cParamValueWidget";
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
    static const QString _sNotRO = "!" + _sReadOnly;
    if (tableIsReadOnly(_tm, _fr.record()))              return true;   // A táblát nem lehet/szabad modosítani
    if (_tf.feature(_sFieldFlags).contains(_sNotRO))     return false;
    if (!(_fr.descr().isUpdatable))                      return true;   // A mező nem modosítható
    if (_tf.getBool(_sFieldFlags,     FF_READ_ONLY))     return true;   // Read only flag a mező megj, leíróban
    if (!lanView::isAuthOrNull(_tf.getId(_sEditRights))) return true;   // Az aktuális user-nek nics joga modosítani a mezőt
    if (_fr.recDescr().autoIncrement()[_fr.index()])     return true;   // Az autoincrement mezőt nem modosítjuk.
    return false;
}

cFieldEditBase::cFieldEditBase(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par)
    : QWidget(_par == nullptr ? nullptr : _par->pWidget())
    , _pParentDialog(_par)
    , _colDescr(_fr.descr())
    , _tableShape(_tm)
    , _fieldShape(_tf)
    , _recDescr(_fr.record().descr())
    , _value()
    , iconSize(20,20)
{
    pLayout     = nullptr;
    pNullButton = nullptr;
    pEditWidget = nullptr;
    _wType      = FEW_UNKNOWN;
    _readOnly   = (0 == (_fl & FEB_YET_EDIT)) && ((_fl & FEB_READ_ONLY) || fieldIsReadOnly(_tm, _tf, _fr));
    _value      = _fr;
    _actValueIsNULL = _fr.isNull();
    pq          = newQuery();
    _nullable   = _fr.isNullable();
    _hasDefault = _fr.descr().colDefault.isNull() == false;
    _hasAuto    = _fr.recDescr().autoIncrement()[_fr.index()];
    _isInsert   = (_fl & FEB_INSERT);      // ??!
    _dcNull     = DC_INVALID;
    _height     = 1;
    if (_hasDefault) {
        _dcNull = DC_DEFAULT;       // can be NULL because it has a default value
    }
    else if (_hasAuto) {
        _dcNull = DC_AUTO;          // Auto (read only)
        _hasDefault = true;
    }
    else if (_nullable) {
        _dcNull = DC_NULL;          // can be NULL
    }
    actNullIcon = _hasDefault ? lv2g::iconDefault : lv2g::iconNull;
    _DBGFNL() << VDEB(_nullable) << VDEB(_hasDefault) << VDEB(_isInsert) << " Index = " << fieldIndex() << " _value = " << debVariantToString(_value) << endl;
}

cFieldEditBase::~cFieldEditBase()
{
    if (pq != nullptr) delete pq;
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
    _actValueIsNULL = v.isNull();
    if (pNullButton != nullptr) {
        pNullButton->setChecked(_actValueIsNULL);
        disableEditWidget(_actValueIsNULL);
    }
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

void cFieldEditBase::disableEditWidget(eTristate tsf)
{
    setBool(_actValueIsNULL, tsf);
    if (pEditWidget != nullptr) {
        pEditWidget->setDisabled(_actValueIsNULL);
    }
}

QWidget *cFieldEditBase::setupNullButton(bool isNull, QAbstractButton * p)
{
    if (_readOnly) {
        if (p != nullptr) delete p;    // nem kell, mert csak egy cimke lessz
        pNullButton = new cROToolButton;
    }
    else {
        pNullButton = p == nullptr ? new QToolButton : p;
    }
    pNullButton->setIcon(actNullIcon);
    pNullButton->setCheckable(true);
    pNullButton->setChecked(isNull);
    if (p == nullptr) {
        pLayout->addWidget(pNullButton);
    }
    if (!_readOnly) {
        connect(pNullButton, SIGNAL(clicked(bool)), this, SLOT(togleNull(bool)));
    }
    return pNullButton;
}

cFieldEditBase * cFieldEditBase::anotherField(const QString& __fn, eEx __ex)
{
    if (_pParentDialog == nullptr) {
        QString se = tr("A keresett %1 nevű mező szerkesztő objektum, nem található, nincs parent dialugus.\nMező Leiró : %2")
                .arg(__fn, _fieldShape.identifying());
        if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, se);
        DERR() << se << endl;
        return nullptr;
    }
    cFieldEditBase *p = (*_pParentDialog)[__fn];
    if (p == nullptr) {
        QString se = tr("A keresett %1 mező, nem található a '%2'.'%3'-ból.\nMező Leiró : %2")
                .arg(__fn, _pParentDialog->name, _colDescr.colName(), _fieldShape.identifying());
        if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, se);
        DERR() << se << endl;
    }
    return p;
}

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
    if (ro) {       // Néhány widget-nek nincs read-only módja, azok helyett read-only esetén egy soros megj. Ez hűlyeség! Legyen Read-only. Javítani!!!
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
            if (_tf.isFeature(_sRadioButtons)) break;
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
        if (_tf.getBool(_sFieldFlags, FF_RAW) == false && _fr.descr().fKeyType != cColStaticDescr::FT_NONE) {     // Ha ez egy idegen kulcs
            cFKeyWidget *p = new cFKeyWidget(_tm, _tf, _fr, _par);
            _DBGFNL() << " new cFKeyWidget" << endl;
            return p;
        }
        if (_tf.isFeature(_sSpinBox)) {
            cFieldSpinBoxWidget *p = new cFieldSpinBoxWidget(_tm, _tf, _fr, fl, _par);
            _DBGFNL() << " new cFieldSpinBoxWidget" << endl;
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
        if (_fr.columnName() == _sFeatures && 0 == (fieldFlags & ENUM2SET(FF_RAW))) {   // features
            cFeatureWidget *p = new cFeatureWidget(_tm, _tf, _fr, fl, _par);
            return p;
        }
        goto case_FieldLineWidget;                                  // Egy soros text...
    // Egy soros bevitel (LineEdit) kivételek vége
    case cColStaticDescr::FT_REAL:
        if (_tf.isFeature(_sSpinBox)) {
            cFieldSpinBoxWidget *p = new cFieldSpinBoxWidget(_tm, _tf, _fr, fl, _par);
            _DBGFNL() << " new cFieldSpinBoxWidget/double" << endl;
            return p;
        }
    LV2_FALLTHROUGH
    case cColStaticDescr::FT_MAC:
    case cColStaticDescr::FT_INET:
    case cColStaticDescr::FT_CIDR: {
      case_FieldLineWidget:
        cFieldLineWidget *p = new cFieldLineWidget(_tm, _tf, _fr, ro, _par);
        _DBGFNL() << " new cFieldLineWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_BOOLEAN:                               // Enumeráció (spec esete) -ként kezeljük
    case cColStaticDescr::FT_ENUM: {                                // Enumeráció mint radio-button-ok
        if (_tf.isFeature(_sRadioButtons)) {
            cEnumRadioWidget *p = new cEnumRadioWidget(_tm, _tf, _fr, ro, _par);
            _DBGFNL() << " new cEnumRadioWidget" << endl;
            return p;
        }
        else {                                                      // Enumeráció : alapértelmezetten comboBox
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
        if (_tf.getBool(_sFieldFlags, FF_RAW) == false && _fr.descr().fKeyType != cColStaticDescr::FT_NONE) {     // nem szám, hanem a hivatkozott rekordok kezelése
            cFKeyArrayWidget *p = new cFKeyArrayWidget(_tm, _tf, _fr, ro, _par);
            _DBGFNL() << " new cFKeyArrayWidget" << endl;
            return p;
        }
        LV2_FALLTHROUGH
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
    }
}

void cFieldEditBase::togleNull(bool f)
{
    disableEditWidget(f);
    _setFromEdit();
}

// Alap változat, ha az edit widget QLineEdit, vagy pPlainTextEdit, ha nem kizárást dob
void cFieldEditBase::_setFromEdit()
{
    QVariant v;
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        ; // NULL/Default
    }
    else {
        if (pEditWidget == nullptr) EXCEPTION(EPROGFAIL);
        QString s;
        QLineEdit *pLineEdit = qobject_cast<QLineEdit *>(pEditWidget);
        if (pLineEdit != nullptr) {
            s = pLineEdit->text();
        }
        else {
            QPlainTextEdit *pPlainTextEdit = qobject_cast<QPlainTextEdit *>(pEditWidget);
            if (pPlainTextEdit != nullptr) {
                s = pPlainTextEdit->toPlainText();
            }
            else {
                QTextEdit *pTextEdit = qobject_cast<QTextEdit *>(pEditWidget);
                if (pTextEdit != nullptr) {
                    s = pTextEdit->toHtml();
                }
                else {
                    EXCEPTION(EPROGFAIL);
                }
            }
        }
        v = s;
    }
    setFromWidget(v);
}

/* **************************************** cNullWidget **************************************** */
cNullWidget::cNullWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase* _par)
    : cFieldEditBase(_tm, _tf, __fr, true, _par)
{
    _DBGOBJ() << _tf.identifying() << endl;
    _wType = FEW_NULL;
    QLineEdit *pLineEdit = new QLineEdit;
    pLineEdit->setReadOnly(true);
    dcSetShort(pLineEdit, DC_NOT_PERMIT);
    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    pLayout->addWidget(pLineEdit);
}

cNullWidget::~cNullWidget()
{
    DBGOBJ();
}

/* **************************************** .......... **************************************** */

bool setWidgetAutoset(qlonglong& on, const QMap<int, qlonglong> autosets)
{
    foreach (int e, autosets.keys()) {
        if (0 == (autosets[e] & on)) {
            on |= ENUM2SET(e);
            return true;
        }
    }
    return false;
}

void nextColLayout(QHBoxLayout *pHLayout, QVBoxLayout *& pVLayout, int rows, int n)
{
    if (pHLayout != nullptr && n != 0 && (n % rows) == 0) {
        pVLayout   = new QVBoxLayout;
        pHLayout->addLayout(pVLayout);
    }
}

/* **************************************** cSetWidget **************************************** */

cSetWidget::cSetWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
{
    _DBGOBJ() << _tf.identifying() << endl;
    _wType = FEW_SET;
    _hiddens    = _fieldShape.features().eValue(_sHide, _colDescr.enumType().enumValues);
    _autosets   = _fieldShape.features().eMapValue(_sAutoset,   _colDescr.enumType().enumValues);
    _collisions = _fieldShape.features().eMapValue(_sCollision, _colDescr.enumType().enumValues);
    if (_fieldShape.isFeature(_sDefault)) {
        _defaults = _fieldShape.features().eValue(_sDefault, _colDescr.enumType().enumValues);
        _dcNull = DC_DEFAULT;
    }
    else if (_hasDefault) {
        QString s = _colDescr.colDefault;
        s = unTypeQuoted(s);
        _defaults = _colDescr.toId(_colDescr.fromSql(s));
        _dcNull = DC_DEFAULT;
    }
    else {
        _defaults = 0;
    }
    _nId = _colDescr.enumType().enumValues.size();
    int cols, rows;
    cols = int(_fieldShape.feature(_sColumn, 1));
    rows = _nId - onCount(_hiddens);
    if (_dcNull != DC_INVALID) ++rows;
    rows = (rows + cols -1) / cols;
    _height = rows;
    pButtons  = new QButtonGroup(this);
    QVBoxLayout *pVLayout   = new QVBoxLayout;
    QHBoxLayout *pHLayout   = nullptr;
    pButtons->setExclusive(false);      // SET, több opció is kiválasztható
    if (cols > 1) {
        pLayout = pHLayout = new QHBoxLayout;
        setLayout(pHLayout);
        pHLayout->addLayout(pVLayout);
    }
    else {
        pLayout = pVLayout;
        setLayout(pVLayout);
    }
    _bits = _colDescr.toId(_value);
    _isNull = __fr.isNull();
    int i,      // sequence number
        id;     // enum value
    for (i = id = 0; id < _nId; ++id) {
        if (ENUM2SET(id) & _hiddens) continue;
        nextColLayout(pHLayout, pVLayout, rows, i);
        QCheckBox *pCheckBox = new QCheckBox(this);
        enumSetShort(pCheckBox, _colDescr.enumType(), id, _colDescr.enumType().enum2str(id), _tf.getId(_sFieldFlags));
        pButtons->addButton(pCheckBox, id);
        pVLayout->addWidget(pCheckBox);
        pCheckBox->setChecked(enum2set(id) & _bits);
        pCheckBox->setDisabled(_readOnly);
        ++i;
    }
    if (_dcNull != DC_INVALID) {
        nextColLayout(pHLayout, pVLayout, rows, i);
        QCheckBox *pCheckBox = new QCheckBox(this);
        dcSetShort(pCheckBox, _dcNull);
        pButtons->addButton(pCheckBox, id);
        pVLayout->addWidget(pCheckBox);
        pCheckBox->setChecked(_isNull);
        pCheckBox->setDisabled(_readOnly);
    }
    connect(pButtons, SIGNAL(buttonClicked(int)), this, SLOT(setFromEdit(int)));
}

cSetWidget::~cSetWidget()
{
    ;
}

void cSetWidget::setChecked()
{
    if (_bits < 0) {
        _bits   = 0;
        _isNull = true;
    }
    if (_isNull && _defaults != 0) {
        _bits = _defaults;
    }
    QAbstractButton * pAbstractButton;
    int id;
    for (id = 0; id < _nId ; id++) {
        pAbstractButton = pButtons->button(id);
        if (nullptr != pAbstractButton) {
            pAbstractButton->setChecked(enum2set(id) & _bits);
        }
    }
    pAbstractButton = pButtons->button(id);
    if (nullptr != pAbstractButton) {
        pAbstractButton->setChecked(_isNull);
    }
}

int cSetWidget::set(const QVariant& v)
{
    _DBGFN() << debVariantToString(v) << endl;
    int r = 1 == cFieldEditBase::set(v);
    if (r) {
        _isNull = _value.isNull();
        _bits = _colDescr.toId(_value);
        setChecked();
    }
    return r;
}

void cSetWidget::setFromEdit(int id)
{
    _DBGFNL() << id << endl;

    QAbstractButton *pButton =  pButtons->button(id);
    if (pButton == nullptr) EXCEPTION(EPROGFAIL, id);
    if (id == _nId) { // NULL
        _isNull = pButton->isChecked();
        setChecked();
    }
    else {
        qlonglong m = enum2set(id);
        if (pButton->isChecked()) {
            _bits |=  m;
            if (_collisions.find(id) != _collisions.end()) {
                _bits &= ~_collisions[id];
                setWidgetAutoset(_bits, _autosets);
            }
            else {
                bool corr = false;
                foreach (int e, _collisions.keys()) {
                    if (m & _collisions[e]) {    // rule?
                        if (_bits & enum2set(e)) { // collision (reverse)
                            _bits &= ~enum2set(e);
                            corr = true;
                        }
                    }
                }
                if (corr) {
                    setWidgetAutoset(_bits, _autosets);
                }
            }
        }
        else {
            _bits &= ~m;
            setWidgetAutoset(_bits, _autosets);
        }
        // _isNull = _bits == 0;    // Lehet üres tömb is!
        if (_isNull && _defaults != 0) {
            _bits = _defaults;
        }
        else {
            pButton = pButtons->button(_nId);
            if (pButton != nullptr) pButton->setChecked(_isNull);
        }
    }
    setChecked();
    qlonglong dummy;
    set(_colDescr.set(QVariant(_bits), dummy));
}


/* **************************************** cEnumRadioWidget ****************************************  */

cEnumRadioWidget::cEnumRadioWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
{
    _wType = FEW_ENUM_RADIO;
    bool vert = _fieldShape.isFeature("horizontal");
    pButtons  = new QButtonGroup(this);
    if (vert) {
        pLayout = new QHBoxLayout;
        _height = _colDescr.enumType().enumValues.size();
        if (_dcNull != DC_INVALID) ++_height;
    }
    else {
        pLayout = new QVBoxLayout;
    }
    pButtons->setExclusive(true);
    setLayout(pLayout);
    int id = 0;
    eval = _colDescr.toId(_value);
    for (id = 0; id < _colDescr.enumType().enumValues.size(); ++id) {
        QRadioButton *pRadioButton = new QRadioButton(this);
        enumSetShort(pRadioButton, _colDescr.enumType(), id, _colDescr.enumType().enum2str(id), _tf.getId(_sFieldFlags));
        pButtons->addButton(pRadioButton, id);
        pLayout->addWidget(pRadioButton);
        pRadioButton->setChecked(id == eval);
        pRadioButton->setDisabled(_readOnly);
    }
    if (_dcNull != DC_INVALID) {
        QRadioButton *pRadioButton = new QRadioButton(this);
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
        if (eval >= 0) pButtons->button(int(eval))->setChecked(true);
        else {
            QAbstractButton *pRB = pButtons->button(_colDescr.enumType().enumValues.size());
            if (pRB != nullptr) pRB->setChecked(true);
        }
    }
    return r;
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
    if (_readOnly && _par != nullptr) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + __fr.record().identifying());
    eval = getId();
    _wType = FEW_ENUM_COMBO;
    pEditWidget = pComboBox = new QComboBox;
    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    pLayout->addWidget(pComboBox);
    bool isNull = eval == NULL_ID;
    evalDef = ENUM_INVALID;
    if (!_hasDefault && !_nullable) {   // Nem lehet NULL, nincs default, akkor a default az első érték.
        _value = eval = evalDef = 0;
        isNull = false;
    }
    else if (_hasDefault) {
        evalDef = _colDescr.enumType().str2enum(_colDescr.colDefault);
        if (isNull) {
            _value = eval = evalDef;
            isNull = false;
        }
    }
    if (_nullable || _hasDefault) {
        setupNullButton(isNull || eval == evalDef);
        cFieldEditBase::disableEditWidget(isNull);
    }
    pModel = new cEnumListModel(&_colDescr.enumType());
    pModel->joinWith(pComboBox);
    pComboBox->setEditable(false);
    setWidget();
    connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setFromEdit(int)));
    if (!isNull) setFromEdit(pComboBox->currentIndex());
}

cEnumComboWidget::~cEnumComboWidget()
{
    ;
}

void cEnumComboWidget::setWidget()
{
    if (eval < 0 && evalDef >= 0) eval = evalDef;
    if (eval < 0 && pNullButton != nullptr) {
        pNullButton->setChecked(true);
    }
    else {
        if (pNullButton != nullptr) {
            pNullButton->setChecked(eval == evalDef);
        }
        int ix = pModel->indexOf(int(eval));
        if (ix < 0) ix = 0;
        pComboBox->setCurrentIndex(ix);
    }
}

void cEnumComboWidget::togleNull(bool f)
{
    // _DBGFN() << _colDescr << VDEB(f) << endl;
    if (evalDef > ENUM_INVALID) {
        if (f) {
            int ix = pModel->indexOf(int(evalDef));
            pComboBox->setCurrentIndex(ix);
            // PDEB(INFO) << VDEB(eval) << VDEB(evalDef) << endl;
        }
        else {
            if (eval == evalDef) pNullButton->setChecked(true);
        }
        return;
    }
    disableEditWidget(f);
    if (f) {
        eval = NULL_ID;
        setFromWidget(QVariant());
    }
    else {
        int ix = pComboBox->currentIndex();
        setFromEdit(ix);
    }
}

int cEnumComboWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (1 == r) {           // Change
        setFromWidget(v);
        eval = getId();
        setWidget();
    }
    return r;
}

void cEnumComboWidget::setFromEdit(int index)
{
    //_DBGFN() << VDEB(index);
    qlonglong newEval = pModel->atInt(index);
    if (eval == newEval) {
        //PDEB(INFO) << "no change: " << VDEB(eval) << endl;
        return;
    }
    //PDEB(INFO) << VDEB(eval) << " := " << newEval << endl;
    eval = newEval;
    if (pNullButton != nullptr && evalDef > ENUM_INVALID) pNullButton->setChecked(eval == evalDef);
    qlonglong v = newEval;
    qlonglong dummy;
    setFromWidget(_colDescr.set(QVariant(v), dummy));
}

/* **************************************** cFieldLineWidget ****************************************  */

cFieldLineWidget::cFieldLineWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr, _fl, _par)
{
    pLineEdit      = nullptr;
    pPlainTextEdit = nullptr;
    pTextEdit      = nullptr;
    pComboBox      = nullptr;
    pModel         = nullptr;
    modeltype      = NO_MODEL;
    isPwd          = false;
    const QString sSetOfValue = "setOfValue";
    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    bool isText = _colDescr.eColType == cColStaticDescr::FT_TEXT;
    if (isText && _fieldShape.getBool(_sFieldFlags, FF_HTML_TEXT)) {
        _wType = FEW_HTML_LINES;  // Widget type
        pEditWidget = pTextEdit = new QTextEdit;
        pLayout->addWidget(pTextEdit);
        _height = 4;
    }
    else if (isText && _fieldShape.getBool(_sFieldFlags, FF_HUGE)) {
        _wType = FEW_LINES;  // Widget type
        pEditWidget = pPlainTextEdit = new QPlainTextEdit;
//        QSizePolicy spol = pPlainTextEdit->sizePolicy();
//        spol.setVerticalPolicy(QSizePolicy::MinimumExpanding);
//        pPlainTextEdit->setSizePolicy(spol);
        pLayout->addWidget(pPlainTextEdit);
        _height = 4;
    }
    else if (!_readOnly && isText && _fieldShape.isFeature(sSetOfValue)) {
        _wType = FEW_COMBO_BOX;  // Widget type
        pEditWidget = pComboBox = new QComboBox;
        pComboBox->setEditable(true);
        pLayout->addWidget(pComboBox);
        cRecFieldSetOfValueModel *pm = new cRecFieldSetOfValueModel(_fr, _fieldShape.features().slValue(sSetOfValue));
        pModel = pm;
        pm->joinWith(pComboBox);
        modeltype = SETOF_MODEL;
    }
    else if (!_readOnly && isText && _fieldShape.getBool(_sFieldFlags, FF_IMAGE)) {   // Icon name from resource
        _wType = FEW_COMBO_BOX;  // Widget type
        pEditWidget = pComboBox = new QComboBox;
        pComboBox->setEditable(false);
        pLayout->addWidget(pComboBox);
        cResourceIconsModel *pm = new cResourceIconsModel();
        pModel = pm;
        pm->joinWith(pComboBox);
        modeltype = ICON_MODEL;
        _nullable = false;
    }
    else {
        _wType = FEW_LINE;  // Widget type
        pEditWidget = pLineEdit = new QLineEdit;
        pLayout->addWidget(pLineEdit);
        isPwd = _fieldShape.getBool(_sFieldFlags, FF_PASSWD);   // Password?
    }
//    if (_colDescr.eColType == cColStaticDescr::FT_TEXT) {
        if (_nullable || _hasDefault) {
            bool isNull = _fr.isNull();
            setupNullButton(isNull);
            cFieldEditBase::disableEditWidget(isNull);
        }
//    }

    QString tx;
    if (_readOnly == false) {
        tx = _fr.toString();
        _value = QVariant(tx);
        switch (_colDescr.eColType) {
        case cColStaticDescr::FT_INTEGER:   pLineEdit->setValidator(new cIntValidator( _nullable, pLineEdit));   break;
        case cColStaticDescr::FT_REAL:      pLineEdit->setValidator(new cRealValidator(_nullable, pLineEdit));   break;
        case cColStaticDescr::FT_TEXT:      /* no validator */                                                  break;
        case cColStaticDescr::FT_MAC:       pLineEdit->setValidator(new cMacValidator( _nullable, pLineEdit));   break;
        case cColStaticDescr::FT_INET:      pLineEdit->setValidator(new cINetValidator(_nullable, pLineEdit));   break;
        case cColStaticDescr::FT_CIDR:      pLineEdit->setValidator(new cCidrValidator(_nullable, pLineEdit));   break;
        default:    EXCEPTION(ENOTSUPP);
        }
        switch(_wType) {
        case FEW_LINE:
            if (isPwd) {
                pLineEdit->setEchoMode(QLineEdit::Password);
                pLineEdit->setText("");
                _value.clear();
                connect(pLineEdit, SIGNAL(editingFinished()),  this, SLOT(_setFromEdit()));
                break;
            }
            pLineEdit->setText(_fr);
            connect(pLineEdit, SIGNAL(editingFinished()),  this, SLOT(_setFromEdit()));
            break;
        case FEW_LINES:
            pPlainTextEdit->setPlainText(_fr);
            connect(pPlainTextEdit, SIGNAL(textChanged()),  this, SLOT(_setFromEdit()));
            break;
        case FEW_HTML_LINES:
            pTextEdit->setHtml(_fr);
            connect(pTextEdit, SIGNAL(textChanged()),  this, SLOT(_setFromEdit()));
            break;
        case FEW_COMBO_BOX:
            switch (modeltype) {
            case SETOF_MODEL:
                static_cast<cRecFieldSetOfValueModel *>(pModel)->setCurrent(_fr);
                break;
            case ICON_MODEL:
                static_cast<cResourceIconsModel *>(pModel)->setCurrent(_fr);
                break;
            case NO_MODEL:
                EXCEPTION(EPROGFAIL, 0);

            }
            connect(pComboBox, SIGNAL(currentTextChanged(QString)),  this, SLOT(_setFromEdit()));
            break;
        default:
            EXCEPTION(EPROGFAIL);
        }
    }
    else {
        tx = _fr.view(*pq);
        if (isPwd) {
            tx = "********";
        }
        else if (_isInsert) {
            if (_wType == FEW_LINE) {
                if      (_hasAuto)    dcSetShort(pLineEdit, DC_AUTO);
                else if (_hasDefault) dcSetShort(pLineEdit, DC_DEFAULT);;
            }
        }
        switch (_wType) {
        case FEW_LINE:
            pLineEdit->setText(tx);
            pLineEdit->setReadOnly(true);
            break;
        case FEW_LINES:
            pPlainTextEdit->setPlainText(tx);
            pPlainTextEdit->setReadOnly(true);
            break;
        case FEW_HTML_LINES:
            pTextEdit->setHtml(tx);
            pTextEdit->setReadOnly(true);
            break;
        default:
            EXCEPTION(EPROGFAIL);
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
        pLineEdit->setText(_sNul);
        return 0;
    }
    int r = cFieldEditBase::set(v);
    if (1 == r && !_actValueIsNULL) {
        QString txt;
        if (_readOnly == false) {
            txt = _colDescr.toName(_value);
        }
        else {
            txt = _colDescr.toView(*pq, _value);
        }
        switch (_wType) {
        case FEW_LINE:
            pLineEdit->setText(txt);
            break;
        case FEW_LINES:
            pPlainTextEdit->setPlainText(txt);
            break;
        case FEW_HTML_LINES:
            pTextEdit->setHtml(txt);
            break;
        case FEW_COMBO_BOX:
            switch (modeltype) {
            case SETOF_MODEL:
                static_cast<cRecFieldSetOfValueModel *>(pModel)->setCurrent(txt);
                break;
            case ICON_MODEL:
                static_cast<cResourceIconsModel *>(pModel)->setCurrent(txt);
                break;
            case NO_MODEL:
                EXCEPTION(EPROGFAIL, 0);

            }
            break;
        default:
            EXCEPTION(EPROGFAIL, _wType);
        }
    }
    return r;
}

void cFieldLineWidget::_setFromEdit()
{
    if ((pNullButton != nullptr && pNullButton->isChecked()) || _wType != FEW_COMBO_BOX) {
        cFieldEditBase::_setFromEdit();
    }
    else {
        QString s = pComboBox->currentText();
        setFromWidget(QVariant(s));
    }
}


/* **************************************** cFieldSpinBoxWidget ****************************************  */

cFieldSpinBoxWidget::cFieldSpinBoxWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase* _par)
: cFieldEditBase(_tm, _tf, _fr, _fl, _par)
{
    _wType = FEW_SPIN_BOX;
    pSpinBox = nullptr;
    pDoubleSpinBox = nullptr;
    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    QStringList minmax = _tf.features().slValue(_sSpinBox);
    switch (_colDescr.eColType) {
    case cColStaticDescr::FT_INTEGER:
        pEditWidget = pSpinBox = new QSpinBox;
        if (minmax.size()) {
            bool ok;
            int i = minmax.first().toInt(&ok);
            if (ok) {                           // Set minimum
                pSpinBox->setMinimum(i);
            }
            if (minmax.size() > 1) {
                i = minmax.at(1).toInt(&ok);
                if (ok) {                       // Set maximum
                    pSpinBox->setMaximum(i);
                }
            }
        }
        break;
    case cColStaticDescr::FT_REAL:
        pEditWidget = pDoubleSpinBox = new QDoubleSpinBox;
        if (minmax.size()) {
            bool ok;
            double d = minmax.first().toDouble(&ok);
            if (ok) {                           // Set minimum
                pDoubleSpinBox->setMinimum(d);
            }
            if (minmax.size() > 1) {
                d = minmax.at(1).toDouble(&ok);
                if (ok) {                       // Set maximum
                    pDoubleSpinBox->setMaximum(d);
                }
                if (minmax.size() > 2) {
                    int i = minmax.at(2).toInt(&ok);
                    if (ok) {                   // Set Decimal
                        pDoubleSpinBox->setDecimals(i);
                    }
                }
            }
        }
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }

    pLayout->addWidget(pSpinBox);
    if (_nullable || _hasDefault) {
        bool isNull = _fr.isNull();
        setupNullButton(isNull);
        cFieldEditBase::disableEditWidget(isNull);
    }
    if (pSpinBox != nullptr) connect(pSpinBox, SIGNAL(valueChanged(int)),    this, SLOT(setFromEdit(int)));
    else            connect(pDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setFromEdit(double)));
}

cFieldSpinBoxWidget::~cFieldSpinBoxWidget()
{

}

int cFieldSpinBoxWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1 && !_actValueIsNULL) {
        bool ok = false;
        if (pSpinBox != nullptr) pSpinBox->setValue(_value.toInt(&ok));
        else            pDoubleSpinBox->setValue(_value.toDouble(&ok));
        if (!ok) return -1;
    }
    return r;
}

void cFieldSpinBoxWidget::_setFromEdit()
{
    QVariant v;
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        ;
    }
    else {
        if (pSpinBox != nullptr) v = pSpinBox->value();
        else            v = pDoubleSpinBox->value();
    }
    setFromWidget(v);
}

void cFieldSpinBoxWidget::setFromEdit(int i)
{
    setFromWidget(QVariant(i));
}

void cFieldSpinBoxWidget::setFromEdit(double d)
{
    setFromWidget(QVariant(d));
}

/* **************************************** cArrayWidget ****************************************  */


cArrayWidget::cArrayWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr, _fl, _par)
    , last()
{
    _wType   = FEW_ARRAY;
    _height  = 4;

    pUi      = new Ui_arrayEd;
    pUi->setupUi(this);
    pEditWidget = pUi->listView;

    selectedNum = 0;

    pUi->pushButtonAdd->setDisabled(_readOnly);
    pUi->pushButtonIns->setDisabled(_readOnly);
    pUi->pushButtonUp->setDisabled(_readOnly);
    pUi->pushButtonDown->setDisabled(_readOnly);
    pUi->pushButtonMod->setDisabled(_readOnly);
    pUi->pushButtonDel->setDisabled(_readOnly);
    pUi->pushButtonClr->setDisabled(_readOnly);

    pModel = new cStringListModel(this);
    pModel->setStringList(_value.toStringList());
    pUi->listView->setModel(pModel);
    pUi->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (_nullable || _hasDefault) {
        bool isNull = _fr.isNull();
        setupNullButton(isNull, new QPushButton);
        cArrayWidget::disableEditWidget(bool2ts(isNull));
        pUi->gridLayout->addWidget(pNullButton, 3, 1);
    }

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
    if (1 == r && !_actValueIsNULL) {
        pModel->setStringList(_value.toStringList());
    }
    setButtons();
    return r;
}

void cArrayWidget::setButtons()
{
    bool eArr = _readOnly || _actValueIsNULL || pModel->isEmpty();
    bool eLin = _readOnly || _actValueIsNULL || pUi->lineEdit->text().isEmpty();
    bool sing = _readOnly || _actValueIsNULL || selectedNum != 1;
    bool any  = _readOnly || _actValueIsNULL || selectedNum == 0;

    pUi->pushButtonAdd ->setDisabled(eLin        );
    pUi->pushButtonIns ->setDisabled(eLin || sing);
    pUi->pushButtonUp  ->setDisabled(        any );
    pUi->pushButtonDown->setDisabled(        any );
    pUi->pushButtonMod ->setDisabled(eLin || sing);
    pUi->pushButtonDel ->setDisabled(eArr        );
    pUi->pushButtonClr ->setDisabled(eArr        );
}

void cArrayWidget::disableEditWidget(eTristate tsf)
{
    cFieldEditBase::disableEditWidget(tsf);
    setButtons();
    pUi->lineEdit->setDisabled(_actValueIsNULL || _readOnly);
}

// cArrayWidget SLOTS

void cArrayWidget::_setFromEdit()
{
    QVariant v;
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        ; // NULL/Default
    }
    else {
        qlonglong dummy;
        v = _colDescr.set(pModel->stringList(), dummy);
    }
    setFromWidget(v);
}
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
    _height = 5;

    pMapWin = nullptr;
    pCImage = nullptr;
    epic = NO_ANY_PIC;
    parentOrPlace_id = NULL_ID;
    selectedRowNum = 0;
    xOk = yOk = xyOk = false ;

    pUi = new Ui_polygonEd;
    pUi->setupUi(this);

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
        id2imageFun = _tableShape.feature(_sMap);  // Meg van adva a image id-t visszaadó függvlny neve ?
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

/// Ha a feltételek teljesülnek, akkor kitölti a pCImage objektumot.
/// Ha epic == NO_ANY_PIC, akkor nem csinál semmit, és false-val tér vissza.
/// ...
bool cPolygonWidget::getImage(bool refresh)
{
    if (epic == NO_ANY_PIC) return false;
    if (pCImage == nullptr) EXCEPTION(EPROGFAIL);
    qlonglong iid = NULL_ID;
    switch (epic) {
    case IS_PLACE_REC:
        if (parentOrPlace_id != NULL_ID) {
            bool ok;
            iid = execSqlIntFunction(*pq, &ok, "get_image_id_by_place_id", parentOrPlace_id);
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
            if (pMapWin != nullptr) pMapWin->setImage(*pCImage);
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
    if (pMapWin == nullptr) return;
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
    pUi->doubleSpinBoxZoom->setEnabled(pMapWin != nullptr);
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
    if (pCImage == nullptr) EXCEPTION(EPROGFAIL);
    if (pMapWin != nullptr) {
        pMapWin->show();
        return;
    }
    pMapWin = new cImagePolygonWidget(!isReadOnly(), _pParentDialog == nullptr ? nullptr : _pParentDialog->pWidget());
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
    if (pMapWin == nullptr) EXCEPTION(EPROGFAIL);
    pMapWin->setScale(z);
}

void cPolygonWidget::destroyedImage(QObject *p)
{
    (void)p;
    pMapWin = nullptr;
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
    if (_readOnly && _par != nullptr) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + __fr.record().identifying());
    _wType = FEW_FKEY;
    _filter = F_NO;
    _height = 1;
    pUi       = nullptr;
    pUiPlace  = nullptr;
    pUiRPlace = nullptr;
    pUiPort   = nullptr;
    pModel    = nullptr;
    pRDescr   = nullptr;
    pSelectPlace= nullptr;
    pSelectNode=nullptr;
    pFKeyTableShape = nullptr;
    owner_ix = NULL_IX;
    ownerId  = NULL_ID;
    ixRPlaceId= NULL_IX;

    // Tábla név, amire az aktuális mező, mint kulcs mutat.
    QString fkTable = _colDescr.fKeyTable;
    // A tábla leíró objektuma.
    pRDescr = cRecStaticDescr::get(fkTable);
    // A "filter" feature változó alapján az altípus (_filter) kiválasztása
    if (_tf.isFeature(_sWidgetFilter)) {
        QString sFilter = _tf.feature(_sWidgetFilter);
        _height = 2;
        if (sFilter.isEmpty()) {        // SIMPLE: Az ui-ban definiált egyszerű szűrő az objektum neve alapján
            _filter = F_SIMPLE;
        }
        else if (0 == sFilter.compare(_sPlaces, Qt::CaseInsensitive)) {     // Szűrés a hely alapján
            if (_sPlaces == fkTable) {  // PLACE: A tábla a places, zóna, és minta szerinti szűrés
                _filter = F_PLACE;
            }
            else {                      // RPLACE: A tábla egy mezője távoli kulcs (kell legyen) a places-re, hely és zóna szerinti szűrés.
                ixRPlaceId = pRDescr->toIndex(_sPlaceId, EX_IGNORE);
                if (ixRPlaceId == NULL_IX) EXCEPTION(EDATA, 0, tr("A filter=palces feature ebben az esetben nem támogatott : %1").arg(_tf.identifying(false)));
                _filter = F_RPLACE;
                _height = 3;
            }
        }
                                        // PORT: Portok: zóna, hely, node alapján szűrés (a node rekordok típusát meg kell adni)
        else if (sFilter.startsWith(_sPort + ".")) {
            QString pt = sFilter.mid(_sPort.size() + 1);
            if      (0 == pt.compare(_sAll,  Qt::CaseInsensitive)) _pt = P_ALL;
            else if (0 == pt.compare(_sNode, Qt::CaseInsensitive)) _pt = P_NODE;
            else if (0 == pt.compare(_sSnmp, Qt::CaseInsensitive)) _pt = P_SNMP;
            else if (0 == pt.compare(_sPatch,Qt::CaseInsensitive)) _pt = P_PATCH;
            else {
                EXCEPTION(EDATA, 0, tr("Invalid filter sub type : %1; Field shape : %2").arg(sFilter, _tf.identifying(false)));
            }
            _filter = F_PORT;
            _height = 4;
        }
        else {
            EXCEPTION(EDATA, 0, tr("Invalid filter type : %1; Field shape : %2").arg(sFilter, _tf.identifying(false)));
        }
    }
    // Altípus (_flter) szerinti egyedi initek, form-ok..
    switch (_filter) {
    case F_NO:
        {
            pLayout = new QHBoxLayout;
            pEditWidget = pComboBox = new QComboBox;
            pLayout->addWidget(pEditWidget);
            pButtonEdit = new QToolButton;
            pButtonEdit->setIcon(cDialogButtons::icons.at(DBT_MODIFY));
            pLayout->addWidget(pButtonEdit);
            pButtonAdd  = new QToolButton;
            pButtonAdd->setIcon(cDialogButtons::icons.at(DBT_PUT_IN));
            pLayout->addWidget(pButtonAdd);
            pButtonRefresh= new QToolButton;
            pButtonRefresh->setIcon(cDialogButtons::icons.at(DBT_REFRESH));
            pLayout->addWidget(pButtonRefresh);
            pButtonInfo = new QToolButton;
            pButtonInfo->setIcon(cDialogButtons::icons.at(DBT_REPORT));
            pLayout->addWidget(pButtonInfo);
            setLayout(pLayout);
        }
        break;
    case F_SIMPLE:
        {
            pUi = new Ui_fKeyEd;
            pUi->setupUi(this);
            pLayout = pUi->horizontalLayout2;       // A NULL gombot ebbe rakjuk
            pEditWidget = pComboBox = pUi->comboBox;
            pButtonEdit = pUi->toolButtonEdit;
            pButtonAdd  = pUi->toolButtonAdd;
            pButtonRefresh= pUi->toolButtonRefresh;
            pButtonInfo = pUi->toolButtonInfo;
            pUi->toolButtonEdit->setDisabled(true);
        }
        break;
    case F_PLACE:
        {
            pUiPlace = new Ui_placeEd;
            pUiPlace->setupUi(this);
            pLayout = pUiPlace->horizontalLayout2;  // A NULL gombot ebbe rakjuk
            pSelectPlace = new cSelectPlace(pUiPlace->comboBoxZone, pUiPlace->comboBoxPlace, pUiPlace->lineEditPlacePattern, _sNul, this);
            pSelectPlace->setPlaceRefreshButton(pUiPlace->toolButtonRefresh);
            pSelectPlace->setPlaceInsertButton(pUiPlace->toolButtonPlaceAdd);
            pSelectPlace->setPlaceEditButton(pUiPlace->toolButtonPlaceEdit);
            pSelectPlace->setPlaceInfoButton(pUiPlace->toolButtonPlaceInfo);
            pEditWidget = pComboBox = pUiPlace->comboBoxPlace;
            pButtonEdit = pUiPlace->toolButtonPlaceEdit;
            pButtonAdd  = pUiPlace->toolButtonPlaceAdd;
            pButtonRefresh= pUiPlace->toolButtonRefresh;
            pButtonInfo = nullptr;
            // nodeName -> place
            bool n2p = !cSysParam::getTextSysParam(*pq, _sNode2Place).isEmpty() && _tf.isFeature(_sNode2Place);
            if (n2p) {
                connect(pUiPlace->toolButtonNode2Place, SIGNAL(clicked()), this, SLOT(node2place()));
            }
            else {
                pUiPlace->toolButtonNode2Place->hide();
            }
        }
        break;
    case F_RPLACE:
        {
            pUiRPlace = new Ui_fKeyPlaceEd;
            pUiRPlace->setupUi(this);
            pLayout = pUiRPlace->horizontalLayout2;    // A NULL gombot ebbe rakjuk
            pSelectPlace = new cSelectPlace(pUiRPlace->comboBoxZone, pUiRPlace->comboBoxPlace, pUiRPlace->lineEditPlacePattern, _sNul, this);
            pSelectPlace->setPlaceRefreshButton(pUiRPlace->toolButtonPlaceRefresh);
            pSelectPlace->setPlaceInsertButton(pUiRPlace->toolButtonPlaceAdd);
            pSelectPlace->setPlaceEditButton(pUiRPlace->toolButtonPlaceEdit);
            pSelectPlace->setPlaceInfoButton(pUiRPlace->toolButtonPlaceInfo);

            pEditWidget = pComboBox = pUiRPlace->comboBox;
            pButtonEdit = pUiRPlace->toolButtonEdit;
            pButtonAdd  = pUiRPlace->toolButtonAdd;
            pButtonRefresh= pUiRPlace->toolButtonRefresh;
            pButtonInfo = pUiRPlace->toolButtonInfo;
        }
        break;
    case F_PORT:
        {
            pUiPort = new Ui_fKeyPortEd;
            pUiPort->setupUi(this);
            pLayout = pUiPort->horizontalLayout2;    // A NULL gombot ebbe rakjuk
            pSelectNode = new cSelectNode(pUiPort->comboBoxZone, pUiPort->comboBoxPlace, pUiPort->comboBoxNode,
                                          pUiPort->lineEditPlacePattern, pUiPort->lineEditNodePattern, _sNul, _sNul, this);
            pSelectNode->setPlaceRefreshButton(pUiPort->toolButtonRefresh);
            pSelectNode->setPlaceInsertButton(pUiPort->toolButtonPlaceAdd);
            pSelectNode->setPlaceEditButton(pUiPort->toolButtonPlaceEdit);
            pSelectNode->setPlaceInfoButton(pUiPort->toolButtonPlaceInfo);

            pEditWidget = pComboBox = pUiPort->comboBox;
            pButtonEdit = pUiPort->toolButtonEdit;
            pButtonAdd  = pUiPort->toolButtonAdd;
            pButtonRefresh= pUiPort->toolButtonRefresh;
            pButtonInfo = pUiPort->toolButtonInfo;
        }
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }

    if (_filter != F_PLACE) {
        connect(pButtonInfo, SIGNAL(clicked()), this, SLOT(info()));
        pModel = new cRecordListModel(*pRDescr, this);
        if (_filter == F_PORT) {
            switch (_pt) {
            case P_ALL:
                break;
            case P_PATCH:
                pSelectNode->setNodeModel(new cRecordListModel(_sPatchs), TS_FALSE);
                break;
            case P_NODE:
                pSelectNode->setNodeModel(new cRecordListModel(_sNodes), TS_FALSE);
                break;
            case P_SNMP:
                pSelectNode->setNodeModel(new cRecordListModel(_sSnmpDevices), TS_FALSE);
                break;
            default:
                EXCEPTION(EPROGFAIL);
            }
        }
        // Ha nincs név mező...
        if (pRDescr->nameIndex(EX_IGNORE) < 0) pModel->setToNameF(_colDescr.fnToName);
        pModel->joinWith(pComboBox);;

        pFKeyTableShape = new cTableShape();
        // Dialógus leíró neve a feature mezőben
        QString tsn = _fieldShape.feature(_sDialog);
        // ha ott nincs megadva, akkor a megjelenítő neve azonos a tulajdonság rekord nevével
        if (tsn.isEmpty()) tsn = _colDescr.fKeyTable;
        if (pFKeyTableShape->fetchByName(tsn)) {   // Ha meg tudjuk jeleníteni
            // Nem lehet öröklés !!
            qlonglong tit = pFKeyTableShape->getId(_sTableInheritType);
            if (tit != TIT_NO && tit != TIT_ONLY) EXCEPTION(EDATA);
            pFKeyTableShape->fetchFields(*pq);
            pButtonAdd->setEnabled(true);
            connect(pButtonEdit, SIGNAL(pressed()), this, SLOT(modifyF()));
            connect(pButtonAdd,  SIGNAL(pressed()), this, SLOT(insertF()));
        }
        else {
            pDelete(pFKeyTableShape);
            pButtonEdit->setDisabled(true);
            pButtonAdd->setDisabled(true);
        }
        connect(pButtonRefresh, SIGNAL(clicked()), this, SLOT(refresh()));
    }

    QString owner = _fieldShape.feature(_sOwner);   //
    if (!owner.isEmpty()) {
        if (_filter != F_NO) EXCEPTION(EDATA, 0, tr("A filter és az owner feature együttes használata nem támogatott : %1").arg(_tf.identifying(false)));
        if (0 == owner.compare(_sSelf, Qt::CaseInsensitive)) {  // TREE
            if (_pParentDialog == nullptr) {
                QAPPMEMO(tr("Invalid feature %1.%2 'owner=self', invalid context.").arg(_tableShape.getName(), _fieldShape.getName()), RS_CRITICAL | RS_BREAK);
            }
            owner_ix = __fr.record().descr().ixToOwner(EX_IGNORE); // ??????!!!!!
            if (owner_ix < 0) {
                QAPPMEMO(tr("Invalid feature %1.%2 'owner=self', owner id index not found.").arg(_tableShape.getName(), _fieldShape.getName()), RS_CRITICAL | RS_BREAK);
            }
            ownerId = NULL_ID;
            if (_pParentDialog->_pOwnerTable != nullptr) {
                ownerId = _pParentDialog->_pOwnerTable->owner_id;
            }
            else if (_pParentDialog->_pOwnerDialog != nullptr) {
                EXCEPTION(ENOTSUPP);
            }
            if (ownerId == NULL_ID) {
                EXCEPTION(EDATA);
            }
            pModel->_setOwnerId(ownerId, __fr.record().descr().columnName(owner_ix));
        }
        else {
            // Ha nincs parent dialog, akkor ez nem fog menni
            if (_pParentDialog == nullptr) EXCEPTION(EDATA, -1, _tableShape.identifying(false));
            cRecordDialog *pDialog = dynamic_cast<cRecordDialog *>(_pParentDialog);
            if (pDialog == nullptr) EXCEPTION(EDATA, -1, _tableShape.identifying(false));
            QList<cFieldEditBase *>::iterator it = pDialog->fields.begin(); // A hivatkozott mező a jelenlegi elött kell legyen!!
            for (;it < pDialog->fields.end(); ++it) {
                if (0 == (*it)->_fieldShape.getName().compare(owner, Qt::CaseInsensitive)) {
                    cFieldEditBase *pfeb = *it;
                    ownerId = pfeb->getId();
                    connect(pfeb, SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(modifyOwnerId(cFieldEditBase*)));
                    pModel->_setOwnerId(ownerId, owner);
                    break;
                }
            }
            if (it >= pDialog->fields.end()) EXCEPTION(EDATA);
        }
        if (_tf.isFeature(_sWidgetFilter)) {
            _filter = F_SIMPLE;
            _height = 2;
        }
    }
    setConstFilter();   // 'refine'

    actId = qlonglong(__fr);
    if (_nullable || _hasDefault) {
        bool isNull = __fr.isNull();
        setupNullButton(isNull);
        cFieldEditBase::disableEditWidget(isNull);
    }

    switch (_filter) {
    case F_NO:          // Nincs szűrés
        connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setFromEdit(int)));
        pModel->setFilter(_sNul, OT_ASC, FT_NO);
        break;
    case F_SIMPLE:      // Egyszerű szúrés minta alapján
        connect(pUi->lineEditFilter, SIGNAL(textChanged(QString)), this, SLOT(setFilter(QString)));
        connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setFromEdit(int)));
        pModel->setFilter(_sNul, OT_ASC, FT_LIKE);
        break;
    case F_PLACE:       // Hely
        connect(pSelectPlace, SIGNAL(placeIdChanged(qlonglong)), this, SLOT(setFromEdit(qlonglong)));
        break;
    case F_RPLACE:      // Szűrés: helyre
        connect(pSelectPlace, SIGNAL(placeIdChanged(qlonglong)), this, SLOT(setPlace(qlonglong)));
        connect(pUiRPlace->lineEditPattern, SIGNAL(textChanged(QString)), this, SLOT(setFilter(QString)));
        connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setFromEdit(int)));
        pModel->_setOwnerId(pSelectPlace->currentPlaceId(), _sPlaceId, TS_TRUE, "is_parent_place");
        pModel->setFilter(_sNul, OT_ASC, FT_LIKE);
        break;
    case F_PORT:        // Port
        connect(pSelectNode, SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(setNode(qlonglong)));
        connect(pUiPort->lineEditPattern, SIGNAL(textChanged(QString)), this, SLOT(setFilter(QString)));
        connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setFromEdit(int)));
        pModel->setFilter(_sNul, OT_ASC, FT_LIKE);
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    // Az aktuális érték megjelenítése
    setWidget();
}

cFKeyWidget::~cFKeyWidget()
{
    pDelete(pFKeyTableShape);
}

bool cFKeyWidget::setWidget()
{
    setButtons();
    if (pNullButton != nullptr) pNullButton->setChecked(_actValueIsNULL);
    if (!_actValueIsNULL) {
        if (pModel != nullptr) {
            int ix;
            if (_filter == F_PORT) {
                QSqlQuery q = getQuery();
                cNPort po;
                po.setById(q, actId);
                qlonglong nid = po.getId(_sNodeId);
                pSelectNode->setCurrentNode(nid);
                pModel->setOwnerId(nid, _sNodeId, TS_TRUE);
                ix = pModel->indexOf(actId);
            }
            else {
                ix = pModel->indexOf(actId);
                if (ix < 0) {
                    if (pSelectPlace != nullptr) {
                        pSelectPlace->setCurrentZone(ALL_PLACE_GROUP_ID);
                        pSelectPlace->setCurrentZone(NULL_ID);
                    }
                    else if (pSelectNode) {
                        cPatch n;
                        if (!n.fetchById(actId)) return false;
                        pSelectNode->setCurrentNode(n.getId(_sNodeId));
                    }
                    else return false;
                    ix = pModel->indexOf(actId);
                }
            }
            if (ix < 0) return false;
            pComboBox->setCurrentIndex(ix);
        }
        else if (pSelectPlace != nullptr) {
            pSelectPlace->setCurrentPlace(actId);
        }
        else {
            EXCEPTION(EPROGFAIL);
        }
    }
    return true;
}

int cFKeyWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    actId = _colDescr.toId(_value);
    if (1 == r) {
        // ? _refresh();
        if (pSelectPlace != nullptr) {
            pSelectPlace->refresh();
        }
        if (!setWidget()) r = -1;
    }
    return r;
}

QString cFKeyWidget::getName() const
{
    QString r;
    if (pModel != nullptr) {
        r = pModel->currendName();
    }
    else if (pSelectPlace != nullptr) {
        r = pSelectPlace->currentPlaceName();
    }
    else {
        EXCEPTION(EPROGFAIL);
    }
    return r;
}

void cFKeyWidget::setFromEdit(int i)
{
    if (pModel == nullptr) EXCEPTION(EPROGFAIL);
    qlonglong id = pModel->atId(i);
    setFromWidget(id);
    if (pNullButton != nullptr && _actValueIsNULL) {
        pNullButton->setChecked(false);
        disableEditWidget(TS_FALSE);
    }
}

void cFKeyWidget::setFromEdit(qlonglong id)
{
    setFromWidget(id);
}

void cFKeyWidget::setButtons()
{
    bool null = pFKeyTableShape == nullptr;
    bool disa = _readOnly || null;
    switch (_filter) {
    case F_NO:
    case F_SIMPLE:
        pButtonEdit->setDisabled(_actValueIsNULL || disa || !lanView::isAuthorized(pFKeyTableShape->getId(_sViewRights)));
        pButtonAdd->setDisabled(                    disa || !lanView::isAuthorized(pFKeyTableShape->getId(_sInsertRights)));
        pButtonInfo->setDisabled(_actValueIsNULL || null || !lanView::isAuthorized(pFKeyTableShape->getId(_sViewRights)));
        break;
    case F_PLACE:           // Rights ???!!!
        pUiPlace->toolButtonPlaceEdit->setDisabled(_actValueIsNULL || disa);
        pUiPlace->toolButtonPlaceAdd->setDisabled(disa);
        break;
    case F_RPLACE:
        pUiRPlace->toolButtonPlaceEdit->setDisabled(_actValueIsNULL || disa);
        pUiRPlace->toolButtonPlaceAdd->setDisabled(disa);
        pButtonAdd->setDisabled(                    disa || !lanView::isAuthorized(pFKeyTableShape->getId(_sInsertRights)));
        pButtonInfo->setDisabled(_actValueIsNULL || null || !lanView::isAuthorized(pFKeyTableShape->getId(_sViewRights)));
        break;
    case F_PORT:
        pButtonEdit->setDisabled(_actValueIsNULL || disa || !lanView::isAuthorized(pFKeyTableShape->getId(_sViewRights)));
        pButtonAdd->setDisabled(                    disa || !lanView::isAuthorized(pFKeyTableShape->getId(_sEditRights)));
        pButtonInfo->setDisabled(_actValueIsNULL || null || !lanView::isAuthorized(pFKeyTableShape->getId(_sViewRights)));
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
}

void cFKeyWidget::disableEditWidget(eTristate tsf)
{
    cFieldEditBase::disableEditWidget(tsf);
    switch (_filter) {
    case F_NO:
        break;
    case F_SIMPLE:
        pUi->label->setVisible(!_actValueIsNULL);
        // pUi->lineEditFilter->setVisible(!_actValueIsNULL);
        pUi->lineEditFilter->setDisabled(_actValueIsNULL);
        break;
    case F_PLACE:
        if (pSelectPlace == nullptr) EXCEPTION(EPROGFAIL);
        pSelectPlace->setDisabled(_actValueIsNULL);
        break;
    case F_RPLACE:
        if (pSelectPlace == nullptr) EXCEPTION(EPROGFAIL);
        pSelectPlace->setDisabled(_actValueIsNULL);
        pUiRPlace->lineEditPattern->setDisabled(_actValueIsNULL);
        break;
    case F_PORT:
        if (pSelectNode == nullptr) EXCEPTION(EPROGFAIL);
        pSelectNode->setDisabled(_actValueIsNULL);
        pUiPort->lineEditPattern->setDisabled(_actValueIsNULL);
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    setButtons();
}

bool cFKeyWidget::setConstFilter()
{
    if (_pParentDialog != nullptr) {
        QStringList constFilter = _fieldShape.features().slValue(_sRefine);
        if (!constFilter.isEmpty() && !constFilter.first().isEmpty()) {
            const cRecord *pr = _pParentDialog->_pRecord;
            QString sql = constFilter.first();
            constFilter.pop_front();
            foreach (QString s, constFilter) {
                if (s.isEmpty()) EXCEPTION(EDATA);
                switch (s[0].toLatin1()) {
                case '#':   // int
                    s = s.mid(1);
                    s = pr->isNull(s) ? _sNULL : QString::number(pr->getId(s));
                    break;
                case '&':   // string
                    s = s.mid(1);
                    s = pr->isNull(s) ? _sNULL : quoted(pr->getName(s));
                    break;
                default:
                    s = pr->getName();
                    break;
                }
                sql = sql.arg(s);
            }
            if (pModel == nullptr) EXCEPTION(EPROGFAIL);
            pModel->setConstFilter(sql, FT_SQL_WHERE);
            return true;
        }
    }
    return false;
}

void cFKeyWidget::_setFromEdit()
{
    QVariant v;
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        ; // NULL/Default
    }
    else {
        if (pModel != nullptr) {
            int ix = pComboBox->currentIndex();
            v = pModel->atId(ix);
        }
        else if (pSelectPlace != nullptr) {
            v = pSelectPlace->currentPlaceId();
        }
        else {
            EXCEPTION(EPROGFAIL);
        }
    }
    setFromWidget(v);
}

void cFKeyWidget::setFilter(const QString& _s)
{
    if(pModel == nullptr) EXCEPTION(EPROGFAIL);
    qlonglong id;
    int ix = pComboBox->currentIndex();
    id = pModel->atId(ix);
    if (_s.isNull()) {
        pModel->setFilter(QVariant(), OT_DEFAULT, FT_NO);
    }
    else {
        QString s = _s;
        if (!s.contains('?') && !s.contains('%')) s = '%' + s + '%';
        pModel->setFilter(s, OT_DEFAULT, FT_LIKE);
    }
    ix = pModel->indexOf(id);
    if (ix >= 0) pComboBox->setCurrentIndex(ix);
    _setFromEdit();
}

void cFKeyWidget::setPlace(qlonglong _pid)
{
    if(pModel == nullptr) EXCEPTION(EPROGFAIL);
    qlonglong id;
    int ix = pComboBox->currentIndex();
    id = pModel->atId(ix);
    pModel->setOwnerId(_pid, _sPlaceId, TS_TRUE);
    ix = pModel->indexOf(id);
    if (ix >= 0) pComboBox->setCurrentIndex(ix);
    _setFromEdit();
}

void cFKeyWidget::setNode(qlonglong _nid)
{
    if(pModel == nullptr) EXCEPTION(EPROGFAIL);
    qlonglong id;
    int ix = pComboBox->currentIndex();
    id = pModel->atId(ix);
    pModel->setOwnerId(_nid, _sNodeId, TS_TRUE);
    ix = pModel->indexOf(id);
    if (ix >= 0) pComboBox->setCurrentIndex(ix);
    _setFromEdit();
}

/// Egy tulajdosnság kulcs mezőben vagyunk.
/// Be szertnénk szúrni egy tulajdonság rekordot
void cFKeyWidget::insertF()
{
    if (pModel != nullptr && pFKeyTableShape != nullptr && lanView::isAuthorized(pFKeyTableShape->getId(_sInsertRights))) {
        cRecordDialog *pDialog = new cRecordDialog(*pFKeyTableShape, ENUM2SET2(DBT_OK, DBT_CANCEL), true, _pParentDialog);
        while (1) {
            int keyId = pDialog->exec(false);
            if (keyId == DBT_CANCEL) break;
            if (!pDialog->accept()) continue;
            if (!cErrorMessageBox::condMsgBox(pDialog->record().tryInsert(*pq, TS_NULL, true))) continue;
            pModel->setFilter(_sNul, OT_ASC, FT_NO);    // Refresh combo box
            cRecord& r = pDialog->record();
            qlonglong id = r.getId();
            int ix = pModel->indexOf(id);
            pUi->comboBox->setCurrentIndex(ix);
            break;
        }
        pDialog->close();
        delete pDialog;
    }
//    else {
//        EXCEPTION(EPROGFAIL);
//    }
}

void cFKeyWidget::modifyF()
{
    if (pModel != nullptr && pFKeyTableShape != nullptr && lanView::isAuthorized(pFKeyTableShape->getId(_sViewRights))) {
        cRecordAny rec(pRDescr);
        cRecordDialog *pDialog = new cRecordDialog(*pFKeyTableShape, ENUM2SET2(DBT_OK, DBT_CANCEL), true, _pParentDialog);
        int cix = pComboBox->currentIndex();
        qlonglong id = pModel->atId(cix);
        if (!rec.fetchById(*pq, id)) return;
        pDialog->restore(&rec);
        while (1) {
            int keyId = pDialog->exec(false);
            if (keyId == DBT_CANCEL) break;
            if (!pDialog->accept()) continue;
            if (!cErrorMessageBox::condMsgBox(pDialog->record().tryUpdateById(*pq, TS_NULL, true))) continue;
            pModel->setFilter(_sNul, OT_ASC, FT_NO);    // Refresh combo box
            pComboBox->setCurrentIndex(pModel->indexOf(rec.getId()));
            break;
        }
        pDialog->close();
        delete pDialog;
    }
//    else {
//        EXCEPTION(EPROGFAIL);
//    }
}

// Megváltozott az owner id
void cFKeyWidget::modifyOwnerId(cFieldEditBase* pof)
{
    if (pModel == nullptr) EXCEPTION(EPROGFAIL);
    ownerId = pof->getId();
    pModel->setOwnerId(ownerId);
    pComboBox->setCurrentIndex(0);
    setFromEdit(0);
}

void cFKeyWidget::_refresh()
{
    if (_pParentDialog != nullptr) {
        QStringList constFilter = _fieldShape.features().slValue(_sRefine);
        if (!constFilter.isEmpty() && !constFilter.first().isEmpty()) {
            // ? if (pUi == nullptr) EXCEPTION(EPROGFAIL);
            QString sql = constFilter.first();
            constFilter.pop_front();
            foreach (QString s, constFilter) {
                if (s.isEmpty()) EXCEPTION(EDATA);
                qlonglong id;
                switch (s[0].toLatin1()) {
                case '#':   // int
                    s = s.mid(1);
                    id = (*_pParentDialog)[s]->getId();
                    s = id == NULL_ID ? _sNULL : QString::number(id);
                    break;
                case '&':   // string
                    s = s.mid(1);
                    if ((*_pParentDialog)[s]->get().isNull()) {
                        s = _sNULL;
                    }
                    else {
                        s = (*_pParentDialog)[s]->getName();
                    }
                    break;
                default:
                    s = (*_pParentDialog)[s]->getName();
                    break;
                }
                sql = sql.arg(s);
            }
            if (pModel == nullptr) EXCEPTION(EPROGFAIL);
            pModel->setConstFilter(sql, FT_SQL_WHERE);
        }
    }
    pComboBox->blockSignals(true);
    pModel->setFilter();
    pComboBox->blockSignals(false);
}

void cFKeyWidget::refresh()
{
    if (pModel == nullptr) EXCEPTION(EPROGFAIL);
    qlonglong id = pModel->currendId();
    _refresh();
    int ix = pModel->indexOf(id);
    if (ix < 0) ix = 0;
    pComboBox->setCurrentIndex(ix);
}

void cFKeyWidget::node2place()
{
    if (_pParentDialog == nullptr) {
        if (pParentBatchEdit != nullptr) {
            pParentBatchEdit->done(FKEY_BATCHEDIT);
        }
    }
    else {
        QString nodeName =_pParentDialog->_pRecord->getName();
        cPlace place;
        place.nodeName2place(*pq, nodeName);
        qlonglong pid = place.getId();
        if (pid != NULL_ID) pSelectPlace->setCurrentPlace(pid);
    }
}

void cFKeyWidget::info()
{
    qlonglong   id = getId();
    if (id != NULL_ID) {
        cRecord *po = new cRecordAny(pRDescr);
        po->setById(*pq, id);
        QString name = pFKeyTableShape->feature(_sReport);
        if (name.isEmpty()) name = pRDescr->tableName();
        tStringPair sp = htmlReport(*pq, *po, name, pFKeyTableShape);
        delete po;
        popupReportWindow(_pParentDialog->pWidget(), sp.second, sp.first);
    }
}

/* **************************************** cDateWidget ****************************************  */


cDateWidget::cDateWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef _fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr, 0, _par)
{
    _wType = FEW_DATE;
    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    pEditWidget = pDateEdit = new QDateEdit;
    pLayout->addWidget(pDateEdit);
    if (_nullable || _hasDefault) {
        bool isNull = _fr.isNull();
        setupNullButton(isNull);
        cFieldEditBase::disableEditWidget(isNull);
    }
    connect(pDateEdit, SIGNAL(dateChanged(QDate)),  this, SLOT(setFromEdit(QDate)));
}

cDateWidget::~cDateWidget()
{
    ;
}

int cDateWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1 && !_actValueIsNULL) {
        pDateEdit->setDate(_value.toDate());
    }
    return r;
}

void cDateWidget::_setFromEdit()
{
    QVariant v;
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        ;
    }
    else {
        v = pDateEdit->date();
    }
    setFromWidget(v);
}

void cDateWidget::setFromEdit(QDate d)
{
    setFromWidget(QVariant(d));
}

/* **************************************** cTimeWidget ****************************************  */

cTimeWidget::cTimeWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef _fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr, false, _par)
{
    if (_readOnly && _par != nullptr) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + _fr.record().identifying());
    _wType = FEW_TIME;
    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    pEditWidget = pTimeEdit = new QTimeEdit;
    pFirstButton = new QToolButton();
    pLastButton  = new QToolButton();
    pTimeEdit->setDisplayFormat("hh:mm:ss.zzz");
    pFirstButton->setIcon(QIcon("://icons/first.ico"));
    pLastButton ->setIcon(QIcon("://icons/last.ico"));
    pLayout->addWidget(pTimeEdit);
    pLayout->addWidget(pFirstButton);
    pLayout->addWidget(pLastButton);
    if (_nullable || _hasDefault) {
        bool isNull = _fr.isNull();
        setupNullButton(isNull);
        cFieldEditBase::disableEditWidget(isNull);
    }
    connect(pTimeEdit, SIGNAL(timeChanged(QTime)),  this, SLOT(setFromEdit(QTime)));
    connect(pFirstButton, SIGNAL(clicked()), this, SLOT(setFirst()));
    connect(pLastButton, SIGNAL(clicked()), this, SLOT(setLast()));
}

cTimeWidget::~cTimeWidget()
{
    ;
}

int cTimeWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1 && !_actValueIsNULL) {
        pTimeEdit->setTime(_value.toTime());
    }
    return r;
}

void cTimeWidget::disableEditWidget(eTristate tsf)
{
    cFieldEditBase::disableEditWidget(tsf);
    pFirstButton->setDisabled(_actValueIsNULL);
    pLastButton ->setDisabled(_actValueIsNULL);
}


void cTimeWidget::_setFromEdit()
{
    QVariant v;
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        ;
    }
    else {
        v = pTimeEdit->time();
    }
    setFromWidget(v);
}

void cTimeWidget::setFromEdit(QTime d)
{
    setFromWidget(QVariant(d));
}

void cTimeWidget::setFirst()
{
    setName("00:00");
}
void cTimeWidget::setLast()
{
    setName("23:59:59.999");
}

/* **************************************** cDateTimeWidget ****************************************  */

cDateTimeWidget::cDateTimeWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef _fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr,false, _par)
{
    if (_readOnly && _par != nullptr) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + _fr.record().identifying());
    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    _wType = FEW_DATE_TIME;
    pEditWidget = pDateTimeEdit = new QDateTimeEdit;
    pLayout->addWidget(pEditWidget);
    if (_nullable || _hasDefault) {
        bool isNull = _fr.isNull();
        setupNullButton(isNull);
        cFieldEditBase::disableEditWidget(isNull);
    }
    connect(pDateTimeEdit, SIGNAL(dateTimeChanged(QDateTime)),  this, SLOT(setFromEdit(QDateTime)));
}

cDateTimeWidget::~cDateTimeWidget()
{
    ;
}

int cDateTimeWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1 && !_actValueIsNULL) {
        pDateTimeEdit->setDateTime(_value.toDateTime());
    }
    return r;
}

void cDateTimeWidget::_setFromEdit()
{
    QVariant v;
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        ;
    }
    else {
        v = pDateTimeEdit->dateTime();
    }
    setFromWidget(v);
}

void cDateTimeWidget::setFromEdit(QDateTime d)
{
   setFromWidget(QVariant(d));
}

/* **************************************** cIntervalWidget ****************************************  */


cIntervalWidget::cIntervalWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr, _fl, _par)
{
    _wType = FEW_INTERVAL;
    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    pLineEditDay = new QLineEdit;
    pLayout->addWidget(pLineEditDay);
    pLabelDay = new QLabel(tr(" Nap "));
    pLayout->addWidget(pLabelDay);
    pTimeEdit = new QTimeEdit;
    pLayout->addWidget(pTimeEdit);
    pTimeEdit->setDisplayFormat("HH:mm:ss.zzz");
    if (_readOnly) {
        pValidatorDay = nullptr;
        view();
        pLineEditDay->setReadOnly(true);
        pTimeEdit->setReadOnly(true);
    }
    else {
        pValidatorDay = new QIntValidator(0, 9999, this);
        pLineEditDay->setValidator(pValidatorDay);
        view();
        connect(pLineEditDay, SIGNAL(editingFinished()),  this, SLOT(_setFromEdit()));
        connect(pTimeEdit,    SIGNAL(editingFinished()),  this, SLOT(_setFromEdit()));
    }
    if (_nullable || _hasDefault) {
        bool isNull = _fr.isNull();
        setupNullButton(isNull);
        cIntervalWidget::disableEditWidget(bool2ts(isNull));
    }
}

cIntervalWidget::~cIntervalWidget()
{
    ;
}

void cIntervalWidget::disableEditWidget(eTristate tsf)
{
    setBool(_actValueIsNULL, tsf);
    pLineEditDay->setDisabled(_actValueIsNULL);
    pLabelDay->setDisabled(_actValueIsNULL);
    pTimeEdit->setDisabled(_actValueIsNULL);
}

qlonglong cIntervalWidget::getFromWideget() const
{
    qlonglong r = NULL_ID;
    if (!_actValueIsNULL) {
        QTime t = pTimeEdit->time();
        r = pLineEditDay->text().toInt();
        r = (r *   24) + t.hour();
        r = (r *   60) + t.minute();
        r = (r *   60) + t.second();
        r = (r * 1000) + t.msec();
    }
    return r;
}

int cIntervalWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1 && !_actValueIsNULL) view();
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
        pTimeEdit->setTime(t);
        pLineEditDay->setText(QString::number(v));
    }
    else {
        pTimeEdit->clear();
        pLineEditDay->clear();
    }
}

void cIntervalWidget::_setFromEdit()
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
    if (!pCheckBoxDetach->isChecked()) {
        pDelete(pImageWidget);
    }
}

void cBinaryWidget::_init()
{
    _wType = FEW_BINARY;
    pLayout         = nullptr;
    pRadioButtonNULL= nullptr;
    pUpLoadButton   = nullptr;
    pDownLoadButton = nullptr;
    pViewButton     = nullptr;
    pDoubleSpinBoxZoom= nullptr;
    pImageWidget    = nullptr;
    pCImage         = nullptr;
    const cRecStaticDescr& cidescr = cImage().descr();
    isCImage   = _recDescr == cidescr;     // éppen egy cImage objektumot szerkesztünk

    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    pRadioButtonNULL = new QRadioButton(_sNULL, this);
    pRadioButtonNULL->setDisabled(_readOnly);
    pLayout->addWidget(pRadioButtonNULL);
    if (!_readOnly) {
        pUpLoadButton = new QPushButton(QIcon("://icons/db_comit.ico"), tr("Feltölt"));
        pUpLoadButton->setToolTip(tr("A bináris adat ill. kép betöltése egy fájlból."));
        pLayout->addWidget(pUpLoadButton);
        connect(pRadioButtonNULL, SIGNAL(clicked(bool)), this, SLOT(nullChecked(bool)));
        connect(pUpLoadButton, SIGNAL(pressed()), this, SLOT(loadDataFromFile()));
    }
    pDownLoadButton = new QPushButton(QIcon("://icons/db_update.ico"), tr("Letölt"));
    pUpLoadButton->setToolTip(tr("A bináris adat ill. kép letöltése egy fájlba."));
    pLayout->addWidget(pDownLoadButton);
    connect(pDownLoadButton, SIGNAL(pressed()), this, SLOT(saveDataToFile()));
    if (isCImage) {     // Ha egy cImage objektum része, akkor meg tudjuk jeleníteni.
        pViewButton     = new QPushButton(QIcon("://icons/view-preview.ico"), tr("Megjelenít"));
        pViewButton->setDefault(true);
        pLayout->addWidget(pViewButton);
        pLayout->addWidget(new QLabel(tr("Nagyít :")));
        pDoubleSpinBoxZoom  = new QDoubleSpinBox;
        pDoubleSpinBoxZoom->setValue(cImageWidget::scale);
        pDoubleSpinBoxZoom->setMinimum(0.1);
        pDoubleSpinBoxZoom->setMaximum(10);
        pDoubleSpinBoxZoom->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
        pLayout->addWidget(pDoubleSpinBoxZoom);
        connect(pViewButton, SIGNAL(pressed()), this, SLOT(viewPic()));
        // Ha jó a mező sorrend, akkor ezek a mezők már megvannak.
        connect(anotherField(_sImageType), SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(changedAnyField(cFieldEditBase*)));
        pCheckBoxDetach = new QCheckBox(tr("Leválaszt"));
        pCheckBoxDetach->setToolTip(tr("A megjelenített kép leválasztása a dialógusról. A kép a dialógus bezárása után is nyitva marad."));
        pCheckBoxDetach->setChecked(false);
        pCheckBoxDetach->setDisabled(true);
        pLayout->addWidget(pCheckBoxDetach);
    }
}


bool cBinaryWidget::setCImage()
{
    setNull();
    if (!isCImage) EXCEPTION(EPROGFAIL);            // Ha nem cImage a rekord, akkor hogy kerültünk ide ?!
    if (pCImage == nullptr) pCImage = new cImage;
    pCImage->clear();
    if (data.size() > 0 && !pRadioButtonNULL->isChecked()) {
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
    if (pViewButton == nullptr) return;
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
    QString fn = QFileDialog::getOpenFileName(this);
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!cMsgBox::tryOpenRead(f, this)) return;
    data = f.readAll();
    pRadioButtonNULL->setChecked(false);
    pRadioButtonNULL->setCheckable(true);
    setFromWidget(QVariant(data));
    setCImage();
    f.close();
}

void cBinaryWidget::saveDataToFile()
{
    QString fn = QFileDialog::getSaveFileName(this);
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!cMsgBox::tryOpenWrite(f, this)) return;
    f.write(data);
    f.close();
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
    if (pImageWidget == nullptr) EXCEPTION(EPROGFAIL);
    pDoubleSpinBoxZoom->setDisabled(true);
    pImageWidget->close();
    pDelete(pImageWidget);
    pCheckBoxDetach->setChecked(false);
    pCheckBoxDetach->setDisabled(true);
}

void cBinaryWidget::openPic()
{
    if (pImageWidget) EXCEPTION(EPROGFAIL);
    cImageWidget::scale = pDoubleSpinBoxZoom->value();
    pImageWidget = new cImageWidget();
    pImageWidget->setImage(*pCImage);
    connect(pDoubleSpinBoxZoom, SIGNAL(valueChanged(double)), pImageWidget, SLOT(zoom(double)));
    // connect(pImageWidget,       SIGNAL(destroyed(QObject*)),  this,         SLOT(destroyedImage(QObject*)));
    pCheckBoxDetach->setDisabled(false);
}

void cBinaryWidget::viewPic()
{
    if (pImageWidget != nullptr) {
        pImageWidget->show();
        return;
    }
    if (pCImage == nullptr) EXCEPTION(EPROGFAIL);
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
    pImageWidget = nullptr;
}

/* **************************************** cFKeyArrayWidget ****************************************  */


cFKeyArrayWidget::cFKeyArrayWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
    , pRDescr(cRecStaticDescr::get(_colDescr.fKeyTable, _colDescr.fKeySchema))
    , last()
    , ra(pRDescr)
{
    _wType  = FEW_FKEY_ARRAY;
    _height = 4;
    unique  = true;
    if (str2tristate(_tf.feature(_sUnique), EX_IGNORE) == TS_FALSE) unique = false;

    pFRecModel  = nullptr;
    pTableShape = nullptr;
    pUi         = new Ui_fKeyArrayEd;
    pUi->setupUi(this);
    pEditWidget = pUi->listView;
    pLayout     = pUi->horizontalLayout;

    selectedNum = 0;
    actRow      = NULL_IX;

    pUi->pushButtonAdd->setDisabled(_readOnly);
    pUi->pushButtonIns->setDisabled(_readOnly);
    pUi->pushButtonUp->setDisabled(_readOnly);
    pUi->pushButtonDown->setDisabled(_readOnly);
    pUi->pushButtonDel->setDisabled(_readOnly);
    pUi->pushButtonClr->setDisabled(_readOnly);

    pArrayModel = new cStringListModel(this);
    foreach (QVariant vId, _value.toList()) {
        qlonglong id = vId.toLongLong();
        valueView << ra.getNameById(*pq, id);
        ids       << id;
    }
    pArrayModel->setStringList(valueView);
    pUi->listView->setModel(pArrayModel);
    pUi->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);


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
    }
    else {
        pDelete(pTableShape);
    }

    if (_nullable || _hasDefault) {
        bool isNull = __fr.isNull();
        setupNullButton(isNull);
        cFKeyArrayWidget::disableEditWidget(bool2ts(isNull));
    }

    if (!_readOnly) {
        pFRecModel  = new cRecordListModel(*pRDescr, this);
        pFRecModel->joinWith(pUi->comboBox);
        pFRecModel->setFilter();
        pUi->comboBox->setCurrentIndex(0);

        connect(pUi->listView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    }
}

cFKeyArrayWidget::~cFKeyArrayWidget()
{
    pDelete(pTableShape);
}

int cFKeyArrayWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (1 == r && !_actValueIsNULL) {
        valueView.clear();
        foreach (QVariant vId, _value.toList()) {
            qlonglong id = vId.toLongLong();
            QString name = ra.getNameById(*pq, id, EX_IGNORE);
            if (name.isNull()) name = dcViewShort(DC_HAVE_NO) + QString(" [%1]").arg(id);   // Error ...
            valueView << name;
            ids       << id;
        }
        pArrayModel->setStringList(valueView);
    }
    setButtons();
    return r;
}

void cFKeyArrayWidget::setButtons()
{
    bool disa = _readOnly || _actValueIsNULL;
    bool add  = !disa;
    if (add) {
        qlonglong id = pFRecModel->currendId();
        add = id != NULL_ID;
        add = add && !(unique && ids.contains(id));
    }
    bool up = !disa && selectedNum == 1;
    bool down = up;
    if (up) {
        up   = actRow > 0;
        down = actRow < (ids.size() -1);
    }
    pUi->pushButtonAdd ->setEnabled(add);
    pUi->pushButtonIns ->setEnabled(add && selectedNum == 1);
    pUi->pushButtonUp  ->setEnabled(up);
    pUi->pushButtonDown->setEnabled(down);
    pUi->pushButtonDel ->setDisabled(disa || selectedNum == 0);
    pUi->pushButtonClr ->setDisabled(disa || ids.isEmpty());
    pUi->pushButtonNew ->setDisabled(disa || pTableShape == nullptr || !lanView::isAuthorized(pTableShape->getId(_sInsertRights)));
    pUi->pushButtonEdit->setDisabled(disa || pTableShape == nullptr || !lanView::isAuthorized(pTableShape->getId(_sViewRights)));
}

void cFKeyArrayWidget::disableEditWidget(eTristate tsf)
{
    cFieldEditBase::disableEditWidget(tsf);
    setButtons();
    pUi->comboBox->setDisabled(_actValueIsNULL || _readOnly);
}


// cFKeyArrayWidget SLOTS

void cFKeyArrayWidget::selectionChanged(QItemSelection, QItemSelection)
{
    QModelIndexList mil = pUi->listView->selectionModel()->selectedRows();
    selectedNum = mil.size();
    if (selectedNum == 1) {
        actRow = mil.first().row();
    }
    else {
        actRow = NULL_IX;
    }
    setButtons();
}

void cFKeyArrayWidget::on_comboBox_currentIndexChanged(int)
{
    setButtons();
}


void cFKeyArrayWidget::on_pushButtonAdd_pressed()
{
    int ix = pUi->comboBox->currentIndex();
    qlonglong id = pFRecModel->atId(ix);
    QString   nm = pFRecModel->at(ix);
    *pArrayModel << nm;
    ids          << id;
    valueView    << ra.getNameById(*pq, id);
    _setFromEdit();
    setButtons();
}

void cFKeyArrayWidget::on_pushButtonIns_pressed()
{
    if (actRow < 0) return;
    int ix = pUi->comboBox->currentIndex();
    qlonglong id = pFRecModel->atId(ix);
    QString   nm = pFRecModel->at(ix);
    pArrayModel->insert(nm, actRow);
    ids.insert(actRow, id);
    valueView.insert(actRow, ra.getNameById(*pq, id));
    _setFromEdit();
    setButtons();
}

void cFKeyArrayWidget::on_pushButtonUp_pressed()
{
    if (actRow < 1) return;
    std::swap(ids[actRow],       ids[actRow -1]);
    std::swap(valueView[actRow], valueView[actRow -1]);
    pArrayModel->setStringList(valueView);
    QModelIndex mi = pArrayModel->index(actRow, 0);
    pUi->listView->selectionModel()->select(mi, QItemSelectionModel::Deselect);
    mi = pArrayModel->index(actRow -1, 0);
    pUi->listView->selectionModel()->select(mi, QItemSelectionModel::Select);
}

void cFKeyArrayWidget::on_pushButtonDown_pressed()
{
    if (actRow < 0) return;
    int n = ids.size();
    if (actRow >= (n -1)) return;
    std::swap(ids[actRow],       ids[actRow +1]);
    std::swap(valueView[actRow], valueView[actRow +1]);
    pArrayModel->setStringList(valueView);
    QModelIndex mi = pArrayModel->index(actRow, 0);
    pUi->listView->selectionModel()->select(mi, QItemSelectionModel::Deselect);
    mi = pArrayModel->index(actRow +1, 0);
    pUi->listView->selectionModel()->select(mi, QItemSelectionModel::Select);
}

void cFKeyArrayWidget::on_pushButtonDel_pressed()
{
    QModelIndexList mil = pUi->listView->selectionModel()->selectedIndexes();
    if (mil.size() > 0) {
        pArrayModel->remove(mil);
        QVector<int> rows = mil2rowsDesc(mil);
        foreach (int ix, rows) {
            ids.removeAt(ix);
            valueView.removeAt(ix);
        }
    }
    else {
        pArrayModel->pop_back();
        ids.pop_back();
        valueView.pop_back();
    }
    _setFromEdit();
    setButtons();
}

void cFKeyArrayWidget::on_pushButtonClr_pressed()
{
    pArrayModel->clear();
    ids.clear();
    valueView.clear();
    setFromWidget(QVariantList());
    setButtons();
}

void cFKeyArrayWidget::on_pushButtonNew_pressed()
{
    if (pTableShape != nullptr && lanView::isAuthorized(pTableShape->getId(_sInsertRights))) {
        cRecordDialog *pDialog = new cRecordDialog(*pTableShape, ENUM2SET2(DBT_OK, DBT_CANCEL), true, _pParentDialog);
        while (1) {
            int keyId = pDialog->exec(false);
            if (keyId == DBT_CANCEL) break;
            if (!pDialog->accept()) continue;
            if (!cErrorMessageBox::condMsgBox(pDialog->record().tryInsert(*pq, TS_NULL, true))) continue;
            pFRecModel->setFilter(_sNul, OT_ASC, FT_NO);    // Refresh combo box
            pUi->comboBox->setCurrentIndex(pFRecModel->indexOf(pDialog->record().getId()));
            break;
        }
        pDialog->close();
        delete pDialog;
    }
}

void cFKeyArrayWidget::on_pushButtonEdit_pressed()
{
    if (pTableShape != nullptr && lanView::isAuthorized(pTableShape->getId(_sInsertRights))) {
        cRecordDialog *pDialog = new cRecordDialog(*pTableShape, ENUM2SET2(DBT_OK, DBT_CANCEL), true, _pParentDialog);
        cRecordAny rec(pRDescr);
        int cix = pUi->comboBox->currentIndex();
        qlonglong id = pFRecModel->atId(cix);
        if (!rec.fetchById(*pq, id)) return;
        rec.fetchText(*pq);
        pDialog->restore(&rec);
        while (1) {
            int keyId = pDialog->exec(false);
            if (keyId == DBT_CANCEL) break;
            if (!pDialog->accept()) continue;
            if (!cErrorMessageBox::condMsgBox(pDialog->record().tryUpdateById(*pq, TS_NULL, true))) continue;
            pFRecModel->setFilter(_sNul, OT_ASC, FT_NO);    // Refresh combo box
            pUi->comboBox->setCurrentIndex(pFRecModel->indexOf(pDialog->record().getId()));
            break;
        }
        pDialog->close();
        delete pDialog;
    }

}

void cFKeyArrayWidget::on_listView_doubleClicked(const QModelIndex & index)
{
    int row = index.row();
    int ix;
    if (isContIx(ids, row) && 0 <= (ix = pFRecModel->indexOf(ids[row]))) {
        pUi->comboBox->setCurrentIndex(ix);
        on_pushButtonEdit_pressed();
    }
}

void cFKeyArrayWidget::_setFromEdit()
{
    setFromWidget(list_longlong2variant(ids));
}

/* **************************************** cColorWidget ****************************************  */


// EGYEDI!!!! Javítandó!!!!
cColorWidget::cColorWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par), pixmap(24, 24)
{
    _wType = FEW_COLOR;
    QHBoxLayout *pHBLayout = new QHBoxLayout;
    pLayout = pHBLayout;
    setLayout(pHBLayout);
    pLineEdit = new QLineEdit(_value.toString());
    pLineEdit->setReadOnly(_fl);
    pHBLayout->addWidget(pLineEdit, 1);
    pLabel = new QLabel;
    pHBLayout->addWidget(pLabel);
    if (!_readOnly) {
        QToolButton *pButton = new QToolButton;
        pButton->setIcon(QIcon(":/icons/colorize.ico"));
        pHBLayout->addWidget(pButton, 0);
        connect(pLineEdit, SIGNAL(textChanged(QString)),  this, SLOT(setFromEdit(QString)));
        connect(pButton,   SIGNAL(pressed()),             this, SLOT(colorDialog()));
        sTitle = tr("Szín kiválasztása");
    }
    setColor(_value.toString());
    pHBLayout->addStretch();
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
        QIcon icon(":/icons/dialog-no.ico");
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
    QColor c = QColorDialog::getColor(color, this, sTitle);
    if (c.isValid()) {
        pLineEdit->setText(c.name());
    }
}

/* **************************************** cFontFamilyWidget ****************************************  */


cFontFamilyWidget::cFontFamilyWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, false, _par)
{
    if (_readOnly && _par != nullptr) EXCEPTION(EPROGFAIL, 0, _tf.identifying() + "\n" + __fr.record().identifying());
    _wType = FEW_FONT_FAMILY;
    QHBoxLayout *pHBLayout = new QHBoxLayout;
    pLayout = pHBLayout;
    setLayout(pHBLayout);
    pEditWidget = pFontComboBox = new QFontComboBox;
    pLayout->addWidget(pFontComboBox);
    bool isNull = __fr.isNull();
    if (_nullable || _hasDefault) {
        setupNullButton(isNull);
        cFieldEditBase::disableEditWidget(isNull);
    }
    if (!isNull) pFontComboBox->setCurrentFont(QFont(QString(__fr)));
    pHBLayout->addStretch(0);
    connect(pFontComboBox,   SIGNAL(currentFontChanged(QFont)), this, SLOT(changeFont(QFont)));
}

cFontFamilyWidget::~cFontFamilyWidget()
{
    ;
}


int cFontFamilyWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1 && _actValueIsNULL) {
        pFontComboBox->setCurrentFont(QFont(v.toString()));
    }
    return r;
}

void cFontFamilyWidget::changeFont(const QFont&)
{
    setFromWidget(QVariant(pFontComboBox->currentText()));
}

void cFontFamilyWidget::_setFromEdit()
{
    QVariant v;
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        ; // NULL/Default
    }
    else {
        v = QVariant(pFontComboBox->currentText());
    }
    setFromWidget(v);
}


/* **************************************** cFontAttrWidget ****************************************  */

const QString cFontAttrWidget::sEnumTypeName = "fontattr";
QIcon         cFontAttrWidget::iconBold;
QIcon         cFontAttrWidget::iconItalic;
QIcon         cFontAttrWidget::iconUnderline;
QIcon         cFontAttrWidget::iconStrikeout;

cFontAttrWidget::cFontAttrWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _fl, _par)
{
    if (iconBold.isNull()) {
        iconBold.     addFile("://icons/format-text-bold.ico",             QSize(), QIcon::Normal, QIcon::On);
        iconBold.     addFile("://icons/format-text-bold-no.png",          QSize(), QIcon::Normal, QIcon::Off);
        iconItalic.   addFile("://icons/format-text-italic.ico",           QSize(), QIcon::Normal, QIcon::On);
        iconItalic.   addFile("://icons/format-text-italic-no.png",        QSize(), QIcon::Normal, QIcon::Off);
        iconUnderline.addFile("://icons/format-text-underline.ico",        QSize(), QIcon::Normal, QIcon::On);
        iconUnderline.addFile("://icons/format-text-underline-no.png",     QSize(), QIcon::Normal, QIcon::Off);
        iconStrikeout.addFile("://icons/format-text-strikethrough.ico",    QSize(), QIcon::Normal, QIcon::On);
        iconStrikeout.addFile("://icons/format-text-strikethrough-no.png", QSize(), QIcon::Normal, QIcon::Off);
    }
    _wType = FEW_FONT_ATTR;
    bool isNull = __fr.isNull();
    m = isNull ? 0 : qlonglong(__fr);
    QHBoxLayout *pHBLayout = new QHBoxLayout;
    pLayout = pHBLayout;
    setLayout(pLayout);

    bool f;
    f = 0 != (m & ENUM2SET(FA_BOOLD));
    setupFlagWidget(f, iconBold, pToolButtonBold);

    f = 0 != (m & ENUM2SET(FA_ITALIC));
    setupFlagWidget(f, iconItalic, pToolButtonItalic);

    f = 0 != (m & ENUM2SET(FA_UNDERLINE));
    setupFlagWidget(f, iconUnderline, pToolButtonUnderline);

    f = 0 != (m & ENUM2SET(FA_STRIKEOUT));
    setupFlagWidget(f, iconStrikeout, pToolButtonStrikeout);
    pHBLayout->addStretch(0);

    setupNullButton(isNull);

    cFontAttrWidget::disableEditWidget(bool2ts(isNull));

    QSqlQuery q = getQuery();
    pEnumType = cColEnumType::fetchOrGet(q, sEnumTypeName);
    if (!_readOnly) {
        connect(pToolButtonBold,      SIGNAL(toggled(bool)), this, SLOT(togleBoold(bool)));
        connect(pToolButtonItalic,    SIGNAL(toggled(bool)), this, SLOT(togleItalic(bool)));
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
    if (r == 1 && !_actValueIsNULL) {
        m = pEnumType->lst2set(v.toStringList());
        pToolButtonBold     ->setChecked(m & ENUM2SET(FA_BOOLD));
        pToolButtonItalic   ->setChecked(m & ENUM2SET(FA_ITALIC));
        pToolButtonUnderline->setChecked(m & ENUM2SET(FA_UNDERLINE));
        pToolButtonStrikeout->setChecked(m & ENUM2SET(FA_STRIKEOUT));
    }
    return r;
}

void cFontAttrWidget::setupFlagWidget(bool f, const QIcon& icon, QToolButton *& pButton)
{
    pButton = _readOnly ? new cROToolButton() : new QToolButton();
    pButton->setIcon(icon);
    pButton->setCheckable(true);
    pButton->setChecked(f);
    pLayout->addWidget(pButton);
}

void cFontAttrWidget::disableEditWidget(eTristate tsf)
{
    setBool(_actValueIsNULL, tsf);
    bool f = _readOnly || _actValueIsNULL;
    pToolButtonBold->setDisabled(f);
    pToolButtonItalic->setDisabled(f);
    pToolButtonUnderline->setDisabled(f);
    pToolButtonStrikeout->setDisabled(f);
}

void cFontAttrWidget::_setFromEdit()
{
    QVariant v;
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        ; // NULL/Default
    }
    else {
        qlonglong m = 0;
        m |= bitByButton(pToolButtonBold,      ENUM2SET(FA_BOOLD));
        m |= bitByButton(pToolButtonItalic,    ENUM2SET(FA_ITALIC));
        m |= bitByButton(pToolButtonUnderline, ENUM2SET(FA_UNDERLINE));
        m |= bitByButton(pToolButtonStrikeout, ENUM2SET(FA_STRIKEOUT));
        v = m;
    }
    setFromWidget(v);
}


void cFontAttrWidget::togleBoold(bool f)
{
    if (f == (bool(m & ENUM2SET(FA_BOOLD)))) return;
    if (f) m |=  ENUM2SET(FA_BOOLD);
    else   m &= ~ENUM2SET(FA_BOOLD);
    setFromWidget(QVariant(m));
}

void cFontAttrWidget::togleItalic(bool f)
{
    if (f == (bool(m & ENUM2SET(FA_ITALIC)))) return;
    if (f) m |=  ENUM2SET(FA_ITALIC);
    else   m &= ~ENUM2SET(FA_ITALIC);
    setFromWidget(QVariant(m));
}

void cFontAttrWidget::togleUnderline(bool f)
{
    if (f == (bool(m & ENUM2SET(FA_UNDERLINE)))) return;
    if (f) m |=  ENUM2SET(FA_UNDERLINE);
    else   m &= ~ENUM2SET(FA_UNDERLINE);
    setFromWidget(QVariant(m));
}

void cFontAttrWidget::togleStrikeout(bool f)
{
    if (f == (bool(m & ENUM2SET(FA_STRIKEOUT)))) return;
    if (f) m |=  ENUM2SET(FA_STRIKEOUT);
    else   m &= ~ENUM2SET(FA_STRIKEOUT);
    setFromWidget(QVariant(m));
}

/* **** **** */

cLTextWidget::cLTextWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _ti, int _fl, cRecordDialogBase* _par)
    : cFieldEditBase(_tm, _tf, _fr, _fl, _par)
{
    _readOnly   = (_fl & FEB_READ_ONLY) || tableIsReadOnly(_tm, _fr.record())
               || !(_fr.descr().isUpdatable)
               || _tf.getBool(_sFieldFlags,     FF_READ_ONLY)
               || !lanView::isAuthOrNull(_tf.getId(_sEditRights));
    _value      = _fr.record().getText(_ti);
    _nullable   = false;
    _hasDefault = false;
    _hasAuto    = false;
    _dcNull     = DC_INVALID;


    pLineEdit = nullptr;
    pPlainTextEdit = nullptr;
    pTextEdit = nullptr;
    pLayout = new QHBoxLayout;
    setLayout(pLayout);
    if (_fieldShape.getBool(_sFieldFlags, FF_HTML_TEXT)) {
        _wType = FEW_LTEXT_HTML;  // Widget típus azonosító
        pTextEdit = new QTextEdit;
        pLayout->addWidget(pTextEdit);
        pTextEdit->setText(_value.toString());
        pTextEdit->setReadOnly(_readOnly);
        connect(pTextEdit, SIGNAL(textChanged()),  this, SLOT(_setFromEdit()));
    }
    else if (_fieldShape.getBool(_sFieldFlags, FF_HUGE)) {
        _wType = FEW_LTEXT_LONG;  // Widget típus azonosító
        pPlainTextEdit = new QPlainTextEdit;
        pLayout->addWidget(pPlainTextEdit);
        pPlainTextEdit->setPlainText(_value.toString());
        pPlainTextEdit->setReadOnly(_readOnly);
        connect(pPlainTextEdit, SIGNAL(textChanged()),  this, SLOT(_setFromEdit()));
    }
    else {
        _wType = FEW_LTEXT;  // Widget típus azonosító
        pLineEdit = new QLineEdit;
        pLayout->addWidget(pLineEdit);
        pLineEdit->setText(_value.toString());
        pLineEdit->setReadOnly(_readOnly);
        connect(pLineEdit, SIGNAL(editingFinished()),  this, SLOT(_setFromEdit()));
    }
}

cLTextWidget::~cLTextWidget()
{

}

int cLTextWidget::set(const QVariant& v)
{
    QString t = v.toString();
    if (t == _value.toString()) return 0;
    _value = t;
    emit changedValue(this);
    switch (_wType) {
    case FEW_LTEXT:         pLineEdit->setText(t);              break;
    case FEW_LTEXT_LONG:    pPlainTextEdit->setPlainText(t);    break;
    case FEW_LTEXT_HTML:    pTextEdit->setHtml(t);              break;
    default:                EXCEPTION(EPROGFAIL);
    }
    return 1;
}

void cLTextWidget::_setFromEdit()
{
    QString  s;
    switch (_wType) {
    case FEW_LTEXT:         s = pLineEdit->text();              break;
    case FEW_LTEXT_LONG:    s = pPlainTextEdit->toPlainText();  break;
    case FEW_LTEXT_HTML:    s = pTextEdit->toHtml();            break;
    default:                EXCEPTION(EPROGFAIL);
    }
    QVariant v; // NULL
    if (!s.isEmpty()) {
        v = QVariant(s);
    }
    setFromWidget(v);
}

/* **** **** */

cFeatureWidgetRow::cFeatureWidgetRow(cFeatureWidget *par, int row, const QString& key, const QString& val)
    : QObject(par)
{
    pDialog = nullptr;
    pListWidget = nullptr;
    pTableWidget = nullptr;
    _pParentWidget = par;
    pItemKey = new QTableWidgetItem(key);
    par->pTable->setItem(row, COL_KEY, pItemKey);
    pItemVal = new QTableWidgetItem(val);
    par->pTable->setItem(row, COL_VAL, pItemVal);
    pListButton = new QToolButton;
    pListButton->setIcon(QIcon(QString(":/icons/view-list-details.ico")));
    par->pTable->setCellWidget(row, COL_B_LIST, pListButton);
    connect(pListButton, SIGNAL(clicked()), this, SLOT(listDialog()));
    pMapButton  = new QToolButton;
    pMapButton->setIcon(QIcon(QString(":/icons/view-list-icon.ico")));
    par->pTable->setCellWidget(row, COL_B_MAP, pMapButton);
    connect(pMapButton, SIGNAL(clicked()), this, SLOT(mapDialog()));
}

cFeatureWidgetRow::~cFeatureWidgetRow()
{
    ;
}

void cFeatureWidgetRow::clickButton(int id)
{
    if (pDialog == nullptr) return;
    switch (id) {
    case DBT_OK:
    case DBT_CANCEL:
        pDialog->done(id);
        break;
    case DBT_INSERT:
        if (pListWidget != nullptr) {
            QListWidgetItem *pItem = new QListWidgetItem;
            pItem->setFlags(pItem->flags() | Qt::ItemIsEditable);
            QModelIndexList mil = pListWidget->selectionModel()->selectedRows();
            if (mil.isEmpty()) pListWidget->addItem(pItem);
            else               pListWidget->insertItem(mil.first().row(), pItem);
        }
        else if (pTableWidget != nullptr) {
            int rows = pTableWidget->rowCount();
            pTableWidget->setRowCount(rows +1);
            pTableWidget->setItem(rows, 0, new QTableWidgetItem);
            pTableWidget->setItem(rows, 1, new QTableWidgetItem);
        }
        break;
    case DBT_DELETE:
        if (pListWidget != nullptr) {
            QModelIndexList mil = pListWidget->selectionModel()->selectedRows();
            if (!mil.isEmpty()) delete pListWidget->takeItem(mil.first().row());
        }
        else if (pTableWidget != nullptr) {
            QModelIndexList mil = pTableWidget->selectionModel()->selectedRows();
            if (!mil.isEmpty()) {
                pTableWidget->removeRow(mil.first().row());
            }
        }
        break;
    case DBT_REPORT:
        if (pTableWidget != nullptr) {
            int rows = pTableWidget->rowCount();
            if (rows > 0) {
                QList<QStringList> matrix;
                for (int row = 0; row < rows; ++row) {
                    QStringList v;
                    v << pTableWidget->item(row, 0)->text();
                    v << pTableWidget->item(row, 1)->text();
                    matrix << v;
                }
                QStringList head;
                head << tr("Név");
                head << tr("Érték");
                QString name = pItemKey->text();
                QString title = tr("A features %1 nevű értékébe ágyazott nevesített érték lista.")
                        .arg(name);
                QWidget *par = lv2g::pMainWindow;
                popupReportWindow(par, htmlTable(head, matrix), title);
            }
        }
    }
}

void cFeatureWidgetRow::listDialog()
{
    pDialog     = new QDialog;
    pListWidget = new QListWidget;
    QVBoxLayout *   pVLayout    = new QVBoxLayout;
    cDialogButtons *pButtons    = new cDialogButtons(ENUM2SET4(DBT_OK, DBT_INSERT, DBT_DELETE, DBT_CANCEL));

    pDialog->setWindowTitle(tr("Lista szerkesztése"));
    pDialog->setLayout(pVLayout);
    pVLayout->addWidget(pListWidget);
    pVLayout->addWidget(pButtons->pWidget());
    QString val = pItemVal->text();
    QStringList list = cFeatures::value2list(val);
    foreach (QString s, list) {
        QListWidgetItem *pItem = new QListWidgetItem(s);
        pItem->setFlags(pItem->flags() | Qt::ItemIsEditable);
        pListWidget->addItem(pItem);
    }
    connect(pButtons, SIGNAL(buttonClicked(int)), this, SLOT(clickButton(int)));
    int id = pDialog->exec();
    if (id == DBT_OK) {
        list.clear();
        int i, n = pListWidget->count();
        for (i = 0; i < n; ++i) {
            list << pListWidget->item(i)->text();
        }
        val = cFeatures::list2value(list);
        pItemVal->setText(val);
    }
    pDelete(pDialog);
    pListWidget = nullptr;
}

void cFeatureWidgetRow::mapDialog()
{
    pDialog     = new QDialog;
    pTableWidget = new QTableWidget;
    QVBoxLayout *   pVLayout    = new QVBoxLayout;
    cDialogButtons *pButtons    = new cDialogButtons(ENUM2SET5(DBT_OK, DBT_REPORT, DBT_INSERT, DBT_DELETE, DBT_CANCEL));

    pDialog->setWindowTitle(tr("Map szerkesztése"));
    pDialog->setLayout(pVLayout);
    pVLayout->addWidget(pTableWidget);
    pVLayout->addWidget(pButtons->pWidget());
    QString val = pItemVal->text();
    tStringMap map = cFeatures::value2map(val);
    QStringList keys = map.keys();
    pTableWidget->setRowCount(keys.size());
    pTableWidget->setColumnCount(2);
    pTableWidget->horizontalHeader()->setStretchLastSection(true);
    QStringList labels;
    labels << tr("Név") << tr("Érték");
    pTableWidget->setHorizontalHeaderLabels(labels);
    QTableWidgetItem *pItem;
    int _row = 0;
    foreach (QString key, keys) {
        QString val = map[key];
        pItem = new QTableWidgetItem(key);
        pTableWidget->setItem(_row, 0, pItem);
        pItem = new QTableWidgetItem(val);
        pTableWidget->setItem(_row, 1, pItem);
        ++_row;
    }
    connect(pButtons, SIGNAL(buttonClicked(int)), this, SLOT(clickButton(int)));
    int id = pDialog->exec();
    if (id == DBT_OK) {
        map.clear();
        int i, n = pTableWidget->rowCount();
        for (i = 0; i < n; ++i) {
            QString key = pTableWidget->item(i, 0)->text();
            QString kvl = pTableWidget->item(i, 1)->text();
            map[key] = kvl;
        }
        val = cFeatures::map2value(map);
        pItemVal->setText(val);
    }
    pDelete(pDialog);
    pTableWidget = nullptr;

}

/* ---- ---- */
cFeatureWidget::cFeatureWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr, _fl, _par)
{
    _wType   = FEW_FEATURES;
    _height  = 6;
    busy = true;

    pHLayout = new QHBoxLayout;
    setLayout(pHLayout);
    pEditWidget = pTable  = new QTableWidget;
    pTable->horizontalHeader()->setStretchLastSection(true);
    QStringList headLabels;
    headLabels << _sNul << _sNul;
    headLabels << tr("Név");
    headLabels << tr("Érték");
    pTable->setColumnCount(cFeatureWidgetRow::COL_NUMBER);
    pTable->horizontalHeader()->setMinimumSectionSize(24);
    pTable->setRowCount(0);     // empty
    pTable->setHorizontalHeaderLabels(headLabels);
    pHLayout->addWidget(pTable, 1);
    pVLayout = new QVBoxLayout;
    pHLayout->addLayout(pVLayout);
    pDelRow = new QPushButton(tr("Töröl"));
    pInsRow = new QPushButton(tr("Beszúr"));
    pVLayout->addWidget(pInsRow);
    pVLayout->addWidget(pDelRow);
    pVLayout->addStretch();
    pLayout = new QHBoxLayout;
    pVLayout->addLayout(pLayout);
    pLayout->addStretch();

    if (_nullable || _hasDefault) {
        bool isNull = _fr.isNull();
        setupNullButton(isNull);
        cFieldEditBase::disableEditWidget(isNull);
    }
    connect(pTable, SIGNAL(cellChanged(int, int)), this, SLOT(onChangedCell(int, int)));
    connect(pInsRow, SIGNAL(clicked()), this, SLOT(onInsClicked()));
    connect(pDelRow, SIGNAL(clicked()), this, SLOT(onDelClicked()));
    busy = false;
}

cFeatureWidget::~cFeatureWidget()
{
    busy = true;
    clearRows();
}

int cFeatureWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (1 == r) {
        busy = true;
        clearRows();
        QString s = v.toString();
        if (!s.isEmpty() && s != ":") {
            if (!features.split(s, false, EX_IGNORE)) {
                cMsgBox::error(tr("Hibás features érték: '%1'").arg(s));
            }
        }
        QStringList keys = features.keys();
        pTable->setRowCount(keys.size());
        int row = 0 ;
        foreach (QString key, keys) {
            QString val = features.value(key);
            rowList << new cFeatureWidgetRow(this, row, key, val);
            ++row;
        }
        pTable->resizeColumnsToContents();
        busy = false;
    }
    return r;
}

void cFeatureWidget::clearRows()
{
    features.clear();
    pTable->setRowCount(0);
    while (!rowList.isEmpty()) delete rowList.takeLast();
}

void cFeatureWidget::onChangedCell(int, int)
{
    if (!busy) _setFromEdit();
}

void cFeatureWidget::_setFromEdit()
{
    if (pNullButton != nullptr && pNullButton->isChecked()) {
        if (!_value.isNull()) setFromWidget(QVariant());
    }
    else {
        features.clear();
        int rows = pTable->rowCount();
        for (int i = 0; i < rows; ++i) {
            QTableWidgetItem *p = pTable->item(i, cFeatureWidgetRow::COL_KEY);
            if (p == nullptr) {
                continue;
            }
            QString key  = p->text();
            if (!key.isEmpty()) {
                p = pTable->item(i, cFeatureWidgetRow::COL_VAL);
                QString value;
                if (p != nullptr) {
                    value = p->text();
                }
                features[key] = value;
            }
        }
        QString s = features.join();
        if (s != _value.toString()) setFromWidget(s);
    }
}

void cFeatureWidget::onInsClicked()
{
    busy = true;
    int rows = pTable->rowCount();
    pTable->setRowCount(rows +1);
    rowList << new cFeatureWidgetRow(this, rows, _sNul, _sNul);
    busy = false;
}

void cFeatureWidget::onDelClicked()
{
    QModelIndexList mil = pTable->selectionModel()->selectedRows();
    if (mil.isEmpty()) return;
    busy = true;
    QList<int> rowsIndexList;
    foreach (QModelIndex mi, mil) { rowsIndexList << mi.row(); }
    std::sort(rowsIndexList.begin(), rowsIndexList.end());
    while (rowsIndexList.size() > 0) {
        int row = rowsIndexList.takeLast();
        pTable->removeRow(row);
        delete rowList.takeAt(row);
    }
    busy = false;
    onChangedCell(0,0);
}

/* **** **** */

cParamValueWidget::cParamValueWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr, _fl, _par)
{
    if (_colDescr.eColType != cColStaticDescr::FT_TEXT) EXCEPTION(ECONTEXT, _colDescr.eColType, _colDescr);
    lastType = NULL_ID;
    typeIdIndex = NULL_IX;
    rawValue = false;
    pParamType = new cParamType;
    pSVarType  = nullptr;
    pLayout = nullptr;
    pPlainTextEdit = nullptr;
    pLineEdit = nullptr;
    pComboBox = nullptr;

    cFieldEditBase *pWType = nullptr;
    typeIdIndex = _pParentDialog->rDescr.toIndex(_sParamTypeId, EX_IGNORE);     // sys_params, node_params, port_params
    if (typeIdIndex < 0) {                                                      // service_vars, service_rrd_vars
        pSVarType = new cServiceVarType;
        if (_colDescr == _sServiceVarValue) {
            typeIdIndex = pSVarType->toIndex(_sParamTypeId, EX_IGNORE);
        }
        else if (_colDescr == _sRawValue) {
            typeIdIndex = pSVarType->toIndex(_sRawParamTypeId, EX_IGNORE);
        }
        else {
            EXCEPTION(ECONTEXT);
        }
        pWType = _pParentDialog->fieldByTableFieldName(_sServiceVarTypeId);
    }
    else {
        pWType = _pParentDialog->fieldByTableFieldName(_sParamTypeId);
    }
    if (pWType == nullptr) EXCEPTION(EPROGFAIL);
    connect(pWType, SIGNAL(changedValue(cFieldEditBase *)), this, SLOT(_changeType(cFieldEditBase *)));
    _changeType(pWType);
}

cParamValueWidget::~cParamValueWidget()
{
    pDelete(pParamType);
    pDelete(pSVarType);
}

int cParamValueWidget::set(const QVariant& _v)
{
    int r = cFieldEditBase::set(_v);
    if (r) refreshWidget();
    return r;
}

void cParamValueWidget::refreshWidget()
{
    if (lastType < 0) {
        if (pLineEdit != nullptr) pLineEdit->setText(_sNul);
        return;
    }
    if (pPlainTextEdit != nullptr) pPlainTextEdit->setPlainText(getName());
    else if (pLineEdit != nullptr) pLineEdit->setText(getName());
    else if (pComboBox != nullptr) pComboBox->setCurrentIndex(get().toBool() ? 1 : 0);
    // ...
}


void cParamValueWidget::_changeType(cFieldEditBase *pTW)
{
    qlonglong paramTypeId = NULL_ID;
    if (pSVarType == nullptr) {     // Changed param_type_id
        paramTypeId = pTW->getId();
    }
    else {                          // Changed service_var_type_id
        qlonglong varTypeId = pTW->getId();
        pSVarType->fetchById(*pq, varTypeId);
        paramTypeId = pSVarType->getId(typeIdIndex);
    }
    if (paramTypeId < 0 || !pParamType->fetchById(*pq, paramTypeId)) {  // type is NULL
        pParamType->clear();
        lastType = NULL_ID;
        return;
    }
    qlonglong type = pParamType->getId(_sParamTypeType);
    if (lastType == type) return;
    lastType = type;
    pDelete(pLayout);
    pPlainTextEdit = nullptr;
    pLineEdit = nullptr;
    pComboBox = nullptr;
    pLayout = new QHBoxLayout;

    switch (lastType) {
    case PT_TEXT:
        pEditWidget = pPlainTextEdit = new QPlainTextEdit;
        pLayout->addWidget(pPlainTextEdit);
        break;
    case PT_BOOLEAN:
        pEditWidget = pComboBox = new QComboBox;
        pComboBox->addItem(langBool(false));
        pComboBox->addItem(langBool(true));
        pLayout->addWidget(pComboBox);
        break;
    case PT_INTEGER:
    case PT_REAL:
    case PT_MAC:
    case PT_INET:
    case PT_CIDR:
        pLineEdit = new QLineEdit;
        if (_readOnly) {
            pLineEdit->setReadOnly(true);
        }
        else switch (lastType) {
        case PT_INTEGER:
            pLineEdit->setValidator(new cIntValidator);
            break;
        case PT_REAL:
            pLineEdit->setValidator(new cRealValidator);
            break;
        case PT_MAC:
            pLineEdit->setValidator(new cMacValidator);
            break;
        case PT_INET:
            pLineEdit->setValidator(new cINetValidator);
            break;
        case PT_CIDR:
            pLineEdit->setValidator(new cCidrValidator);
            break;
        default:
            EXCEPTION(EPROGFAIL);
        }
        pLayout->addWidget(pLineEdit);
        break;
    case PT_DATE:
    case PT_TIME:
    case PT_DATETIME:
    case PT_INTERVAL:
    case PT_POINT:
    case PT_BYTEA:
    default:
        pLineEdit = new QLineEdit;
        pLineEdit->setDisabled(true);
        dcSetShort(pLineEdit, DC_NOT_PERMIT);
        pLayout->addWidget(pLineEdit);
        break;
    }
    setLayout(pLayout);
    set(_value);
}

void cParamValueWidget::_setFromEdit()
{

}

/* **** **** */

cSelectPlace::cSelectPlace(QComboBox *_pZone, QComboBox *_pPLace, QLineEdit *_pFilt, const QString& _constFilt, QWidget *_par)
    : QObject(_par)
    , bbPlace(this, &cSelectPlace::emitChangePlace)
    , pComboBoxZone(_pZone)
    , pComboBoxPLace(_pPLace)
    , pLineEditPlaceFilt(_pFilt)
    , constFilterPlace(_constFilt)
{
    pButtonPlaceInsert = pButtonPlaceRefresh = pButtonPlaceInfo = pButtonPlaceEdit = nullptr;
    pSlave = nullptr;
    pModelZone = new cZoneListModel(this);
    pModelZone->joinWith(pComboBoxZone);
    if (lv2g::getInstance()->zoneId != NULL_ID) pModelZone->setCurrent(lv2g::getInstance()->zoneId);
    pModelPlace = new cPlacesInZoneModel(this);
    pModelPlace->joinWith(pComboBoxPLace);
    pComboBoxPLace->setCurrentIndex(0);
    if (!constFilterPlace.isEmpty()) {
        pModelPlace->setConstFilter(constFilterPlace, FT_SQL_WHERE);
    }
    connect(pComboBoxZone,  SIGNAL(currentIndexChanged(int)),   this, SLOT(on_comboBoxZone_currentIndexChanged(int)));
    connect(pComboBoxPLace, SIGNAL(currentIndexChanged(int)),   this, SLOT(on_comboBoxPlace_currentIndexChanged(int)));
    if (pLineEditPlaceFilt != nullptr) {
        connect(pLineEditPlaceFilt, SIGNAL(textChanged(QString)),    this, SLOT(lineEditPlaceFilt_textChanged(QString)));
    }

}

void cSelectPlace::copyCurrents(const cSelectPlace &_o)
{
    bbPlace.begin();
    qlonglong pid = currentPlaceId();
    pComboBoxZone->setCurrentIndex(_o.pComboBoxZone->currentIndex());
    if (pLineEditPlaceFilt != nullptr) {
        QString pattern = _sNul;
        if (_o.pLineEditPlaceFilt != nullptr) {
            pattern = _o.pLineEditPlaceFilt->text();
        }
        pLineEditPlaceFilt->setText(pattern);   // -> lineEditPlaceFilt_textChanged()
    }
    int pix = _o.pComboBoxPLace->currentIndex();
    pComboBoxPLace->setCurrentIndex(pix);       // -> on_comboBoxPlace_currentIndexChanged()
    bbPlace.end(pid != currentPlaceId());
}

void cSelectPlace::setSlave(cSelectPlace *_pSlave, bool disabled)
{
    if (_pSlave == nullptr) {
        if (pSlave != nullptr) pSlave->setDisableWidgets(false);
        pSlave = nullptr;
    }
    else {
        if (pSlave != nullptr && pSlave != _pSlave) EXCEPTION(EPROGFAIL);
        pSlave = _pSlave;
        pSlave->copyCurrents(*this);
        pSlave->cSelectPlace::setDisableWidgets(disabled);
    }
}

void cSelectPlace::setDisableWidgets(bool f)
{
    pComboBoxZone->setDisabled(f);
    if (pLineEditPlaceFilt != nullptr) pLineEditPlaceFilt->setDisabled(f);
    pComboBoxPLace->setDisabled(f);
    if (pButtonPlaceInsert  != nullptr) pButtonPlaceInsert->setDisabled(f);
    if (pButtonPlaceRefresh != nullptr) pButtonPlaceRefresh->setDisabled(f);
    f = f || (currentPlaceId() == NULL_ID);
    if (pButtonPlaceInfo    != nullptr) pButtonPlaceInfo->setDisabled(f);
    if (pButtonPlaceEdit    != nullptr) pButtonPlaceEdit->setDisabled(f);
}

void cSelectPlace::setPlaceInsertButton(QAbstractButton * p)
{
    pButtonPlaceInsert = p;
    connect(p, SIGNAL(clicked()), this, SLOT(insertPlace()));
}

void cSelectPlace::setPlaceEditButton(QAbstractButton * p)
{
    pButtonPlaceEdit = p;
    connect(p, SIGNAL(clicked()), this, SLOT(editPlace()));
}


void cSelectPlace::setPlaceRefreshButton(QAbstractButton * p)
{
    pButtonPlaceRefresh = p;
    connect(p, SIGNAL(clicked()), this, SLOT(refresh()));
}

void cSelectPlace::setPlaceInfoButton(QAbstractButton * p)
{
    pButtonPlaceInfo = p;
    connect(p, SIGNAL(clicked()), this, SLOT(placeReport()));
}


bool cSelectPlace::emitChangePlace(bool f)
{
    if (f && bbPlace.test()) {
        placeNameChanged(currentPlaceName());
        placeIdChanged(currentPlaceId());
        if (pButtonPlaceInfo != nullptr) pButtonPlaceInfo->setDisabled(currentPlaceId() == NULL_ID);
        return true;
    }
    return false;
}


void cSelectPlace::setEnabled(bool f)
{
    setDisableWidgets(!f);
}

void cSelectPlace::setDisabled(bool f)
{
    setDisableWidgets(f);
}

void cSelectPlace::refresh(bool f)
{
    bbPlace.begin();
    qlonglong zid = currentZoneId();
    qlonglong pid = currentPlaceId();
    if (pSlave) {
        pSlave->refresh();
    }
    pModelZone->setFilter();
    setCurrentZone(zid);
    pModelPlace->setFilter();
    setCurrentPlace(pid);
    bbPlace.end(f && pid != currentPlaceId());
}

/// Egy új helyiség objektum adatlap megjelkenítése, és az objektum beillesztése az adatbázisba.
/// @return Ha a művelet sikeres, akkor az új objektum ID-je, ha nem akkor NULL_ID.
qlonglong cSelectPlace::insertPlace()
{
    QSqlQuery q = getQuery();
    cRecord *p = recordDialog(q, _sPlaces, qobject_cast<QWidget *>(parent()));
    if (p == nullptr) return NULL_ID;
    qlonglong pid = p->getId();
    delete p;
    bbPlace.begin();
    if (pSlave != nullptr) {
        pSlave->bbPlace.begin();
    }
    pModelPlace->setFilter();
    setCurrentPlace(pid);
    if (pSlave) {
        pSlave->pModelPlace->setFilter();
        pSlave->setCurrentPlace(pid);
        pSlave->bbPlace.end();
    }
    bbPlace.end();
    return pid;
}

/// Szignál:
/// Az aktuális helyiség objektum adatlap megjelkenítése, és szerkesztése. A név változtatás tiltott.
void cSelectPlace::editPlace()
{
    qlonglong pid = currentPlaceId();
    if (pid == NULL_ID) return;
    QSqlQuery q = getQuery();
    cPlace place;
    place.setById(q, pid);
    cTableShape shape;
    shape.setByName(q, _sPlaces);
    shape.fetchFields(q);
    shape.shapeFields.get(_sPlaceName)->enum2setOn(_sFieldFlags, FF_READ_ONLY);   // Set name (place_name) is read only
    cRecord *p = recordDialog(q, shape, qobject_cast<QWidget *>(parent()), &place, pid <= ROOT_PLACE_ID, true);
    pDelete(p);
}

qlonglong cSelectPlace::editCurrentPlace()
{
    qlonglong pid = currentPlaceId();
    if (pid <= ROOT_PLACE_ID) return NULL_ID;   // A NULL, unknown, root nem editálható
    QSqlQuery q = getQuery();
    cPlace place;
    place.setById(q, pid);
    cRecord *p = recordDialog(q, _sPlaces, qobject_cast<QWidget *>(parent()), &place, false, true);
    if (p == nullptr) return NULL_ID;
    pid = p->getId();
    delete p;
    bbPlace.begin();
    if (pSlave != nullptr) {
        pSlave->bbPlace.begin();
    }
    pModelPlace->setFilter();
    setCurrentPlace(pid);
    if (pSlave) {
        pSlave->pModelPlace->setFilter();
        pSlave->setCurrentPlace(pid);
        pSlave->bbPlace.end();
    }
    bbPlace.end();
    return pid;
}

void cSelectPlace::setCurrentZone(qlonglong _zid)
{
    int ix = pModelZone->indexOf(_zid);
    if (ix < 0) EXCEPTION(EDATA, _zid);
    if (ix != pComboBoxZone->currentIndex()) {
        pComboBoxZone->blockSignals(true);
        pComboBoxZone->setCurrentIndex(ix);
        pComboBoxZone->blockSignals(false);
    }
    qlonglong pid = currentPlaceId();   // Save current plac id. We'll keep it if we can.
    bool f = true;
    if (pid == NULL_ID) {
        pModelPlace->setZone(_zid);
    }
    else {
        bbPlace.begin();
        pModelPlace->setZone(_zid);
        ix = pModelPlace->indexOf(pid);
        f = ix >= 0;
        if (f) pComboBoxPLace->setCurrentIndex(ix);
        bbPlace.end(!f);
    }
    if (pSlave != nullptr) {
        if (!bbPlace.test()) pSlave->bbPlace.begin();
        pSlave->setCurrentZone(_zid);
        if (!bbPlace.test()) pSlave->bbPlace.end(!f);
    }
}

void cSelectPlace::setCurrentPlace(qlonglong _pid)
{
    if (_pid == currentPlaceId()) return;
    bbPlace.begin();
    if (_pid == NULL_ID || _pid == UNKNOWN_PLACE_ID) {  // Az 'unknown' nincs a listában! ==> NULL
        if (pModelPlace->nullable) {
            pComboBoxPLace->setCurrentIndex(0);  // -> on_comboBoxZone_currentIndexChanged(ix)
        }
        else {
            EXCEPTION(EDATA);
        }
    }
    else { // NOT NULL
        int ix = pModelPlace->indexOf(_pid);
        if (ix < 0) {
            if (pLineEditPlaceFilt != nullptr) pLineEditPlaceFilt->setText(_sNul);
            setCurrentZone(ALL_PLACE_GROUP_ID);
            ix = pModelPlace->indexOf(_pid);
            if (ix < 0) EXCEPTION(EDATA);
        }
        pComboBoxPLace->setCurrentIndex(ix);    // -> on_comboBoxZone_currentIndexChanged(ix)
    }
    bbPlace.end();
}

void cSelectPlace::placeReport()
{
    qlonglong pid = currentPlaceId();
    if (pid == NULL_ID) return;
    QSqlQuery q = getQuery();
    tStringPair r = htmlReportPlace(q, pid);
    popupReportWindow(static_cast<QWidget *>(this->parent()), r.second, r.first);
}

void cSelectPlace::on_comboBoxZone_currentIndexChanged(int ix)
{
    if (ix < 0) {
        if (bbPlace.test()) {
            DERR() << "Invalid index : " << ix << endl;
        }
        return;
    }
    qlonglong zid = pModelZone->atId(ix);
    setCurrentZone(zid);
}

void cSelectPlace::on_comboBoxPlace_currentIndexChanged(int ix)
{
    if (bbPlace.test()) {
        if (pSlave) pSlave->pComboBoxPLace->setCurrentIndex(ix);
        emitChangePlace(true);
    }
}

void cSelectPlace::lineEditPlaceFilt_textChanged(const QString& s)
{
    bbPlace.begin();
    qlonglong pid = currentPlaceId();
    if (s.isEmpty()) {
        pModelPlace->setFilter(QVariant(), OT_DEFAULT, FT_NO);
    }
    else {
        pModelPlace->setFilter(condAddJoker(s), OT_DEFAULT, FT_LIKE);
    }
    int pix = pModelPlace->indexOf(pid);
    if (pix < 0) pix = 0;
    if (pSlave != nullptr) {
        pSlave->bbPlace.begin();
        if (pSlave->pLineEditPlaceFilt != nullptr) pSlave->pLineEditPlaceFilt->setText(s);
        pSlave->setCurrentPlace(pModelPlace->atId(pix));
        pSlave->bbPlace.end(pix == 0);
    }
    pComboBoxPLace->setCurrentIndex(pix);
    bbPlace.end(pix == 0);
}

/* **** **** */

cSelectNode::cSelectNode(QComboBox *_pZone, QComboBox *_pPlace, QComboBox *_pNode,
            QLineEdit *_pPlaceFilt, QLineEdit *_pNodeFilt,
            const QString& _placeConstFilt, const QString& _nodeConstFilt,
            QWidget *_par)
    : cSelectPlace(_pZone, _pPlace, _pPlaceFilt, _placeConstFilt, _par)
    , bbNode(this, &cSelectNode::emitChangeNode)
    , pComboBoxNode(_pNode)
    , pLineEditNodeFilt(_pNodeFilt)
    , constFilterNode(_nodeConstFilt)
{
    pButtonPatchInsert = pButtonNodeRefresh = pButtonNodeInfo = nullptr;
    pModelNode = nullptr;
    setNodeModel(new cRecordListModel(cPatch().descr(), this), TS_TRUE);
    connect(this, SIGNAL(placeIdChanged(qlonglong)), this, SLOT(setPlaceId(qlonglong)));
    // Ha a kiválasztott hely NULL, akkor kell a zóna változásra is reagálni.
    connect(pComboBoxZone, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBoxZone_currenstIndexChanged(int)));
    if (pLineEditNodeFilt != nullptr) {    // Ha van mintára szűrés
        connect(pLineEditNodeFilt, SIGNAL(textChanged(QString)), this, SLOT(on_lineEditNodeFilt_textChanged(QString)));
    }
    connect(pComboBoxNode, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBoxNode_currenstIndexChanged(int)));
}

void cSelectNode::setNodeModel(cRecordListModel *  _pNodeModel, eTristate _nullable)
{
    bbNode.begin();
    setBool(_pNodeModel->nullable, _nullable);
    _pNodeModel->joinWith(pComboBoxNode);
    pDelete(pModelNode);    // ?
    pModelNode = _pNodeModel;
    pModelNode->setConstFilter(constFilterNode, FT_SQL_WHERE);
    pModelNode->setOwnerId(currentPlaceId(), _sPlaceId, TS_TRUE);
    bbNode.end(false);
}

void cSelectNode::reset(bool f)
{
    qlonglong nid = currentNodeId();
    bbNode.begin();
    pComboBoxZone->setCurrentIndex(0);
    pComboBoxPLace->setCurrentIndex(0);
    pComboBoxNode->setCurrentIndex(0);
    bbNode.end(f && nid != currentNodeId());
}

void cSelectNode::nodeSetNull(bool _sig)
{
    bbNode.begin();
    pComboBoxNode->setCurrentIndex(0);
    bbNode.end(_sig);
}

void cSelectNode::setLocalityFilter()
{
    qlonglong pid = currentPlaceId();
    if (pid != NULL_ID) {
        pModelNode->setOwnerId(pid, _sPlaceId, TS_TRUE, "is_parent_place");
    }
    else {
        pModelNode->setOwnerId(currentZoneId(), _sPlaceId, TS_TRUE, "is_place_in_zone");
    }
}

void cSelectNode::setExcludedNode(qlonglong _nid)
{
    if (_nid == NULL_ID) {
        pModelNode->setFilter(QVariant(), OT_DEFAULT, FT_NO);
    }
    else {
        pModelNode->setFilter(QString("node_id <> %1").arg(_nid), OT_DEFAULT, FT_SQL_WHERE);
    }
}

void cSelectNode::setPatchInsertButton(QAbstractButton * p)
{
    pButtonPatchInsert = p;
    connect(p, SIGNAL(clicked()), this, SLOT(on_buttonPatchInsert_clicked()));
}

void cSelectNode::setNodeRefreshButton(QAbstractButton * p)
{
    pButtonNodeRefresh = p;
    connect(p, SIGNAL(clicked()), this, SLOT(refresh()));
}

void cSelectNode::setNodeInfoButton(QAbstractButton * p)
{
    pButtonNodeInfo = p;
    connect(p, SIGNAL(clicked()), this, SLOT(nodeReport()));
}

void cSelectNode::changeNodeConstFilter(const QString& _sql)
{
    pModelNode->setConstFilter(_sql, FT_SQL_WHERE);
}

void cSelectNode::setDisableWidgets(bool f)
{
    cSelectPlace::setDisableWidgets(f);
    if (pLineEditNodeFilt != nullptr) pLineEditNodeFilt->setDisabled(f);
    pComboBoxNode->setDisabled(f);
    if (pButtonNodeRefresh != nullptr) pButtonNodeRefresh->setDisabled(f);
    if (pButtonPatchInsert != nullptr) pButtonPatchInsert->setDisabled(f);
    if (pButtonNodeInfo    != nullptr) pButtonNodeInfo->setDisabled(f || currentNodeId() == NULL_ID);
}

void cSelectNode::refresh(bool f)
{
    qlonglong nid = currentNodeId();
    bbNode.begin();
    cSelectPlace::refresh();
    setLocalityFilter();
    bbNode.end(f && nid != currentNodeId());
}

bool cSelectNode::emitChangeNode(bool f)
{
    if (f && bbNode.test()) {
        nodeIdChanged(currentNodeId());
        nodeNameChanged(currentNodeName());
        if (pButtonNodeInfo != nullptr) pButtonNodeInfo->setDisabled(currentNodeId() == NULL_ID);
        return true;
    }
    return false;
}

void cSelectNode::setPlaceId(qlonglong pid, bool _sig)
{
    if (currentPlaceId() != pid) {
        bbPlace.begin();
        setCurrentPlace(pid);
        bbPlace.end(false);
    }
    bbNode.begin();
    setLocalityFilter();
    pComboBoxNode->setCurrentIndex(0);
    on_comboBoxNode_currenstIndexChanged(0);
    bbNode.end(_sig);
}

void cSelectNode::setEnabled(bool f)
{
    setDisableWidgets(!f);
}

void cSelectNode::setDisabled(bool f)
{
    setDisableWidgets(f);
}

void cSelectNode::nodeReport()
{
    qlonglong nid = currentNodeId();
    if (nid == NULL_ID) return;
    QSqlQuery q = getQuery();
    popupReportNode(static_cast<QWidget *>(this->parent()), q, nid);
}

eTristate cSelectNode::setCurrentNode(qlonglong _nid)
{
    if (_nid == currentNodeId()) return TS_NULL;
    bbNode.begin();
    int ix = pModelNode->indexOf(_nid);
    if (ix < 0) {   // Ha nincs az aktuális listában
        setCurrentZone(ALL_PLACE_GROUP_ID);
        setCurrentPlace(NULL_ID);
        setPlaceId(NULL_ID, false);
        refresh(false);
        ix = pModelNode->indexOf(_nid);
    }
    eTristate r = TS_TRUE;
    if (ix >= 0) pComboBoxNode->setCurrentIndex(ix);
    else         r = TS_FALSE;
    bbNode.end();
    return r;
}

qlonglong cSelectNode::insertPatch(cPatch *pSample)
{
    QSqlQuery q = getQuery();
    cPatch *p = patchInsertDialog(q, qobject_cast<QWidget *>(parent()), pSample);
    if (p == nullptr) return NULL_ID;  // cancel
    qlonglong pid = p->getId();
    delete p;
    pModelNode->setFilter();
    setCurrentNode(pid);
    return pid;
}

void cSelectNode::on_buttonPatchInsert_clicked()
{
    cPatch  sample;
    sample.setId(_sPlaceId, currentPlaceId());
    insertPatch(&sample);
}

void cSelectNode::on_lineEditNodeFilt_textChanged(const QString& s)
{
    qlonglong nid = currentNodeId();
    bbNode.begin();
    if (s.isEmpty()) {
        pModelNode->setFilter(QVariant(), OT_DEFAULT, FT_NO);
    }
    else {
        pModelNode->setFilter(condAddJoker(s), OT_DEFAULT, FT_LIKE);
    }
    int nix = pModelNode->indexOf(nid);
    if (nix < 0) {
        nix = 0;
    }
    pComboBoxNode->setCurrentIndex(0);
    bbNode.end(nid != currentNodeId());
}

void cSelectNode::on_comboBoxNode_currenstIndexChanged(int ix)
{
    (void)ix;
    emitChangeNode();
}

void cSelectNode::on_comboBoxZone_currenstIndexChanged(int)
{
    if (currentPlaceId() == NULL_ID) {  // Ha nincs hely, akkor zónára szűrünk
        setPlaceId(NULL_ID);
    }
}


/* ****** */

cSelectLinkedPort::cSelectLinkedPort(QComboBox *_pZone, QComboBox *_pPlace, QComboBox *_pNode, QComboBox *_pPort, QButtonGroup *_pType, QComboBox *_pShare, const tIntVector &_shList,
            QLineEdit *_pPlaceFilt, QLineEdit *_pNodeFilt, const QString& _placeConstFilt, const QString& _nodeConstFilt,
            QWidget *_par)
    : cSelectNode(_pZone, _pPlace, _pNode, _pPlaceFilt, _pNodeFilt, _placeConstFilt, _nodeConstFilt, _par)
{
    lockSlots = 0;
    _isPatch = false;
    pq = newQuery();
    pComboBoxPort    = _pPort;
    pComboBoxShare   = _pShare;
    pButtonGroupType = _pType;
    pModelShare      = new cEnumListModel("portshare", NT_NOT_NULL, _shList, this);
    pModelShare->joinWith(pComboBoxShare);
    //pModelShare->
    pModelPort       = new cRecordListModel("patchable_ports", _sNul, this);
    pModelPort->_setOrder(OT_ASC, _sPortIndex);     // Index szerint sorba
    pModelPort->setOwnerId(NULL_ID, _sNodeId, TS_FALSE);    // Üres lista lessz
    pModelPort->joinWith(pComboBoxPort);
    lastLinkType = LT_FRONT;
    lastShare    = pModelShare->atInt(0);
    lastPortId   = 0;   // Mindegy mennyi, csak ne NULL_ID legyen
    setNodeId(NULL_ID);
    connect(this,          SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(setNodeId(qlonglong)));
    connect(pButtonGroupType,    SIGNAL(buttonClicked(int)), this, SLOT(setLinkTypeByButtons(int)));
    connect(pComboBoxPort, SIGNAL(currentIndexChanged(int)), this, SLOT(portChanged(int)));
    connect(pComboBoxShare,SIGNAL(currentIndexChanged(int)), this, SLOT(changedShareType(int)));
}

cSelectLinkedPort::~cSelectLinkedPort()
{
    delete pq;
}

void cSelectLinkedPort::setLink(cPhsLink& _lnk)
{
    qlonglong pid = _lnk.getId(_sPortId2);
    if (pid == NULL_ID) {
        pComboBoxNode->setCurrentIndex(0);
        return;
    }
    cNPort p;
    cPatch *pn;
    p.setById(*pq, pid);
    pn = cNode::getNodeObjById(*pq, p.getId(_sNodeId));
    setCurrentPlace(pn->getId(_sPlaceId));

    int ix = pModelNode->indexOf(pn->getId());
    // EZ ITT NEM jÓ!
    // Ha lépkedünk a linkeken, akkor ez előfordulhat ! Pl. ha meg van adva szűrés.
    // JAVÍTANDÓ!!!
    if (ix <= 0) EXCEPTION(EPROGFAIL);
    pComboBoxNode->setCurrentIndex(ix);
    _isPatch = pn->tableoid() == cPatch::_descr_cPatch().tableoid();
    ix = pModelPort->indexOf(pid);
    if (ix < 0) EXCEPTION(EPROGFAIL);
    pComboBoxPort->setCurrentIndex(ix);

    ix = 0;     // Share value index (comboBox)
    if (_isPatch) {
        ix = pModelShare->indexOf(int(_lnk.getId(_sPortShared)));
        if (ix < 0) EXCEPTION(EPROGFAIL);
    }
    lastShare = pModelShare->atInt(ix);
    lastLinkType = int(_lnk.getId(_sPhsLinkType2));
    pComboBoxShare->setCurrentIndex(ix);
    if ((lastLinkType == LT_TERM) == _isPatch) {
        QString msg = tr("Database or program error. Port %1:%2, link type is %3.").arg(pn->getName(), p.getName(), linkType(lastLinkType, EX_IGNORE));
        EXCEPTION(EDATA, lastLinkType, msg);
    }
    pComboBoxShare->setEnabled(lastLinkType == LT_FRONT);

    pButtonGroupType->button(lastLinkType)->setChecked(true);
    pButtonGroupType->button(LT_TERM)->setDisabled(_isPatch);
    pButtonGroupType->button(LT_FRONT)->setEnabled(_isPatch);
    pButtonGroupType->button(LT_BACK)->setEnabled(_isPatch);
}

void cSelectLinkedPort::setNodeId(qlonglong _nid)
{
    lockSlots++;
    bool nodeIsNull = _nid == NULL_ID;
    pComboBoxPort->setDisabled(nodeIsNull);
    if (!nodeIsNull) {
        cPatch p;
        p.setId(_nid);
        qlonglong toid = p.fetchTableOId(*pq, EX_IGNORE);  // Type?
        if (toid == NULL_ID) {  // Deleted ?!
            lockSlots--;
            refresh();
            return;
        }
        _isPatch = toid == p.tableoid();
        if (_isPatch) {
            if (lastLinkType == LT_TERM) {
                pButtonGroupType->button(LT_FRONT)->setChecked(true);
                lastLinkType = LT_FRONT;
            }
        }
        else {
            if (lastLinkType != LT_TERM) {
                pButtonGroupType->button(LT_TERM)->setChecked(true);
                lastLinkType = LT_TERM;
            }
        }
    }
    pModelPort->setOwnerId(_nid);
    pComboBoxPort->setCurrentIndex(0);
    lockSlots--;
    if (lockSlots == 0) setPortIdByIndex(0);
    else if (lockSlots < 0) EXCEPTION(EPROGFAIL);
}

void cSelectLinkedPort::setPortIdByIndex(int ix)
{
    qlonglong pid = pModelPort->atId(ix);
    if (pid == lastPortId) return;
    lockSlots++;
    lastPortId = pid;
    bool portIsNull = pid == NULL_ID;
    if (!portIsNull) {
        if (_isPatch) {
            if (lastLinkType == LT_TERM) lastLinkType = LT_FRONT;
        }
        else {
            lastLinkType = LT_TERM;
            lastShare = ES_;
            pComboBoxShare->setCurrentIndex(0);
        }
        pButtonGroupType->button(lastLinkType)->setChecked(true);
        pButtonGroupType->button(LT_TERM)->setDisabled(_isPatch);
        pButtonGroupType->button(LT_FRONT)->setEnabled(_isPatch);
        pButtonGroupType->button(LT_BACK)->setEnabled(_isPatch);
        pComboBoxShare->setEnabled(_isPatch);
    }
    else {
        pButtonGroupType->button(LT_TERM) ->setDisabled(true);
        pButtonGroupType->button(LT_FRONT)->setDisabled(true);
        pButtonGroupType->button(LT_BACK) ->setDisabled(true);
        pComboBoxShare->setDisabled(true);
    }
    lockSlots--;
    if (lockSlots == 0) changedLink(pid, lastLinkType, lastShare);
    else if (lockSlots < 0) EXCEPTION(EPROGFAIL);
}

void cSelectLinkedPort::setLinkTypeByButtons(int _id)
{
    lastLinkType = _id;
    if (lockSlots) return;
    changedLink(currentPortId(), _id, lastShare);
}

void cSelectLinkedPort::changedShareType(int ix)
{
    int sh = pModelShare->atInt(ix);
    if (lastShare == sh) return;
    lastShare = sh;
    if (lockSlots == 0) changedLink(currentPortId(), lastLinkType, lastShare);
    else if (lockSlots < 0) EXCEPTION(EPROGFAIL);
}

void cSelectLinkedPort::portChanged(int ix)
{
    if (lockSlots) return;
    changedLink(pModelPort->atId(ix), lastLinkType, lastShare);
}

/* ********************************************************************************* */

cSelectVlan::cSelectVlan(QComboBox *_pComboBoxId, QComboBox *_pComboBoxName, QWidget *_par)
    : QObject(_par)
{
    disableSignal = false;
    pq = newQuery();
    pevNull = &cEnumVal::enumVal(_sDatacharacter, DC_NULL);
    pComboBoxId   = _pComboBoxId;
    pComboBoxName = _pComboBoxName;
    actId         = NULL_ID;
    actName       = _sNul;
    pModelId      = new cStringListDecModel;
    pModelName    = new cStringListDecModel;
    pModelId->setDefDecoration(&cEnumVal::enumVal(_sDatacharacter, DC_ID));
    pModelName->setDefDecoration(&cEnumVal::enumVal(_sDatacharacter, DC_NAME));
    pComboBoxId->setModel(pModelId);
    pComboBoxName->setModel(pModelName);

    idList   << NULL_ID;
    nameList << dcViewShort(DC_NULL);
    sIdList  << dcViewShort(DC_NULL);
    QString sql = "SELECT vlan_id, vlan_name FROM vlans";
    if (execSql(*pq, sql)) do {
        qlonglong id = pq->value(0).toLongLong();
        idList   << id;
        nameList << pq->value(1).toString();
        sIdList  << QString::number(id);
    } while (pq->next());
    pModelId->setStringList(sIdList).setDecorationAt(0, pevNull);
    pModelName->setStringList(nameList).setDecorationAt(0, pevNull);
    pComboBoxId->setCurrentIndex(0);
    pComboBoxName->setCurrentIndex(0);
    actId   = NULL_ID;
    actName = _sNul;

    connect(pComboBoxId,   SIGNAL(currentIndexChanged(int)), this, SLOT(_changedId(int)));
    connect(pComboBoxName, SIGNAL(currentIndexChanged(int)), this, SLOT(_changedName(int)));

    changedId(actId);
    changedName(actName);
}

cSelectVlan::~cSelectVlan()
{
    delete pq;
}

void cSelectVlan::setCurrentByVlan(qlonglong _vid)
{
    if (actId == _vid) return;
    if (_vid == NULL_ID) {
        pComboBoxId->setCurrentIndex(0);
        pComboBoxName->setCurrentIndex(0);
        return;
    }
    int ix = idList.indexOf(_vid);
    if (ix >= 0) {
        pComboBoxId->setCurrentIndex(ix);
        pComboBoxName->setCurrentIndex(ix);
    }
}

void cSelectVlan::setCurrentBySubNet(qlonglong _sid)
{
    if (_sid == NULL_ID) {
        setCurrentByVlan(NULL_ID);
        return;
    }
    cSubNet s;
    s.setById(*pq, _sid);
    qlonglong vid = s.getId(_sVlanId);
    setCurrentByVlan(vid);
}

void cSelectVlan::setDisable(bool f)
{
    disableSignal = true;
    pComboBoxId->setCurrentIndex(0);
    pComboBoxName->setCurrentIndex(0);
    pComboBoxId->setDisabled(f);
    pComboBoxName->setDisabled(f);
    disableSignal = false;
}

void cSelectVlan::_changedId(int ix)
{
    if (disableSignal) return;
    const cEnumVal *pe = pModelId->getDecorationAt(ix);
    enumSetD(pComboBoxId, *pe);
    actId = idList.at(ix);
    changedId(actId);
    pComboBoxName->setCurrentIndex(ix);
}

void cSelectVlan::_changedName(int ix)
{
    if (disableSignal) return;
    const cEnumVal *pe = pModelName->getDecorationAt(ix);
    enumSetD(pComboBoxName, *pe);
    bool isNull = ix == 0 && idList.first() == NULL_ID;
    actName = isNull ? _sNul :nameList.at(ix);
    changedName(actName);
    pComboBoxId->setCurrentIndex(ix);
}

/* ********************************************************************************* */

cSelectSubNet::cSelectSubNet(QComboBox *_pComboBoxNet, QComboBox *_pComboBoxName, QWidget *_par)
    : QObject(_par)
{
    disableSignal = false;
    pq = newQuery();
    pevNull = &cEnumVal::enumVal(_sDatacharacter, DC_NULL);
    pComboBoxNet  = _pComboBoxNet;
    pComboBoxName = _pComboBoxName;
    actId         = NULL_ID;
    actName       = _sNul;
    pModelNet     = new cStringListDecModel;
    pModelName    = new cStringListDecModel;
    pModelNet->setDefDecoration(&cEnumVal::enumVal(_sDatacharacter, DC_DATA));
    pModelName->setDefDecoration(&cEnumVal::enumVal(_sDatacharacter, DC_NAME));
    pComboBoxNet->setModel(pModelNet);
    pComboBoxName->setModel(pModelName);

    idList   << NULL_ID;
    nameList << dcViewShort(DC_NULL);
    sNetList << dcViewShort(DC_NULL);
    netList  << netAddress();
    QString sql = "SELECT subnet_id, subnet_name, netaddr FROM subnets";
    if (execSql(*pq, sql)) do {
        QString sNet = pq->value(2).toString();
        idList   << pq->value(0).toLongLong();
        nameList << pq->value(1).toString();
        sNetList << sNet;
        netList  << netAddress(sNet);
    } while (pq->next());
    pModelNet->setStringList(sNetList).setDecorationAt(0, pevNull);;
    pModelName->setStringList(nameList).setDecorationAt(0, pevNull);;
    pComboBoxNet->setCurrentIndex(0);
    pComboBoxName->setCurrentIndex(0);

    connect(pComboBoxNet,  SIGNAL(currentIndexChanged(int)), this, SLOT(_changedNet(int)));
    connect(pComboBoxName, SIGNAL(currentIndexChanged(int)), this, SLOT(_changedName(int)));

    changedId(actId);
    changedName(actName);
}

cSelectSubNet::~cSelectSubNet()
{
    delete pq;
}

qlonglong cSelectSubNet::currentId()
{
    int ix = pComboBoxNet->currentIndex();
    return isContIx(idList, ix) ? idList.at(ix) : NULL_ID;
}


void cSelectSubNet::setCurrentByVlan(qlonglong _vid)
{
    if (_vid == NULL_ID) {
        setCurrentBySubNet(NULL_ID);
        return;
    }
    cSubNet sn;
    sn.setId(_sVlanId, _vid);
    int n = sn.completion(*pq);
    while (n > 1 && sn.getId(__sSubnetType) != NT_PRIMARY) {
        sn.next(*pq);
    }
    setCurrentBySubNet(sn.getId());
}

void cSelectSubNet::setCurrentBySubNet(qlonglong _sid)
{
    if (actId == _sid) return;
    if (_sid == NULL_ID) {
        pComboBoxNet->setCurrentIndex(0);
        pComboBoxName->setCurrentIndex(0);
        return;
    }
    int ix = idList.indexOf(_sid);
    if (ix >= 0) {
        pComboBoxNet->setCurrentIndex(ix);
        pComboBoxName->setCurrentIndex(ix);
    }
}

void cSelectSubNet::setCurrentByAddress(QHostAddress& _a)
{
    if (_a.isNull()) {
        pComboBoxNet->setCurrentIndex(0);
        pComboBoxName->setCurrentIndex(0);
        return;
    }
    for (int i = 1; i < netList.size(); ++i) {
        const QPair<QHostAddress, int>& n = static_cast<const QPair<QHostAddress, int>&>(netList.at(i));
        if (_a.isInSubnet(n)) {
            pComboBoxNet->setCurrentIndex(i);
            pComboBoxName->setCurrentIndex(i);
            return;
        }
    }
}

void cSelectSubNet::setDisable(bool f)
{
    disableSignal = true;
    pComboBoxNet->setCurrentIndex(0);
    pComboBoxName->setCurrentIndex(0);
    pComboBoxNet->setDisabled(f);
    pComboBoxName->setDisabled(f);
    disableSignal = false;
}

void cSelectSubNet::_changedNet(int ix)
{
    if (disableSignal) return;
    const cEnumVal *pe = pModelNet->getDecorationAt(ix);
    enumSetD(pComboBoxNet, *pe);
    actId = idList.at(ix);
    changedId(actId);
    pComboBoxName->setCurrentIndex(ix);
}

void cSelectSubNet::_changedName(int ix)
{
    if (disableSignal) return;
    const cEnumVal *pe = pModelName->getDecorationAt(ix);
    enumSetD(pComboBoxName, *pe);
    bool isNull = ix == 0 && idList.first() == NULL_ID;
    actName = isNull ? _sNul :nameList.at(ix);
    changedName(actName);
    pComboBoxNet->setCurrentIndex(ix);
}

/* ********************************************************************************* */

cStringMapEdit::cStringMapEdit(bool _isDialog, tStringMap& _map, QWidget *par)
    : QObject(par), isDialog(_isDialog), map(_map)
{
    pButtons = nullptr;
    pTableWidget = new QTableWidget(par);
    if (isDialog) {
        _pWidget = new QDialog(par);
        _pWidget->setWindowTitle(tr("Paraméterek"));
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
    pTableWidget->horizontalHeader()->setStretchLastSection(true);
    pTableWidget->setColumnCount(2);
    QStringList head;
    head << tr("Név") << tr("Érték");
    pTableWidget->setHorizontalHeaderLabels(head);
    Qt::ItemFlags flagConst = Qt::ItemIsEnabled;
    Qt::ItemFlags flagEdit  = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    QTableWidgetItem *pi;
    foreach (QString n, map.keys()) {
        rows++;
        pTableWidget->setRowCount(rows);
        pi = new QTableWidgetItem(n);
        pi->setFlags(flagConst);
        pTableWidget->setItem(rows -1, 0, pi);
        pi = new QTableWidgetItem(map[n]);
        pi->setFlags(flagEdit);
        pTableWidget->setItem(rows -1, 1, pi);
    }
    pTableWidget->resizeColumnsToContents();
    connect(pTableWidget, SIGNAL(cellChanged(int,int)), this, SLOT(changed(int,int)));
}

cStringMapEdit::~cStringMapEdit()
{
    delete _pWidget;
}

QDialog& cStringMapEdit::dialog()
{
    if (!isDialog) EXCEPTION(EPROGFAIL);
    return *static_cast<QDialog *>(_pWidget);
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
    QString n = pTableWidget->item(row, 0)->text();
    map[n] = pTableWidget->item(row, 1)->text();
}
