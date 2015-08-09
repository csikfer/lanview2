#include "record_table.h"
#include "lv2validator.h"
#include "ui_polygoned.h"
#include "ui_arrayed.h"
#include "ui_fkeyed.h"
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
    scale = -1;
    bool f = __o.dataIsPic();
    if (!f) return false;
    const char * _type = __o._getType();
    QByteArray   _data = __o.getImage();
    if (!image.loadFromData(_data, _type)) return false;
    QString title = __t;
    if (title.isEmpty()) title = __o.getName(_sImageNote);
    setWindowTitle(title);
    return resetImage();
}

bool cImageWidget::setText(const QString& _txt)
{
    pLabel = new QLabel();
    pLabel->setText(_txt);
    setWidget(pLabel);
    show();
    return true;
}

void cImageWidget::mousePressEvent(QMouseEvent * ev)
{
    mousePressed(ev->pos());
}

bool cImageWidget::resetImage()
{
    if (scale < 0) scale = 0;
    image.setDevicePixelRatio(1.0/(1.0 + (scale * scaleStep)));
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
        QPolygonF   pol;
        foreach (QPointF p, d.value<tPolygonF>()) {
           pol << p;
        }
        d = QVariant::fromValue(pol);
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
    if (scale >= 0) {
        ++scale;
        resetImage();
    }
}
void cImageWidget::zoomOut()
{
    if (scale >  0) {
        --scale;
        resetImage();
    }
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
    case FEW_ARRAY:         return "cArrayWidget";
    case FEW_POLYGON:       return "cPolygonWidget";
    case FEW_FKEY:          return "cFKeyWidget";
    case FEW_DATE:          return "cDateWidget";
    case FEW_TIME:          return "cTimeWidget";
    case FEW_DATE_TIME:     return "cDateTimeWidget";
    case FEW_INTERVAL:      return "cIntervalWidget";
    case FEW_BINARY:        return "cBinaryWidget";
    case FEW_NULL:          return "cNullWidget";
    default:                return sInvalidEnum();
    }
}

cFieldEditBase::cFieldEditBase(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef _fr, bool _ro, cRecordDialogBase *_par)
    : QObject(_par)
    , _parent(*_par)
    , _recDescr(_fr.descr())
    , _tableShape(_tm)
    , _fieldShape(_tf)
    , _value()
{
    _wType      = FEW_UNKNOWN;
    _readOnly   = _ro || (_fieldShape.isNull(_sEditRights) ? false : !lanView::isAuthorized(_fieldShape.getId(_sEditRights)));
    _value      = _fr;
    _pWidget    = NULL;
    _pFieldRef  = new cRecordFieldRef(_fr);
    pq          = newQuery();
    _nullable   = _fr.isNullable();
    _hasDefault = _fr.descr().colDefault.isNull() == false;
    _isInsert   = _fr.record().isEmpty_();
    if (_isInsert && _hasDefault) {
        _nullView = design().valDefault;
    }
    else if (_nullable) {
        _nullView = design().valNull;
    }

    _DBGFNL() << VDEB(_nullable) << VDEB(_hasDefault) << VDEB(_isInsert) << " Index = " << _pFieldRef->index() << " _value = " << debVariantToString(_value) << endl;
 }

cFieldEditBase::~cFieldEditBase()
{
    if (pq         != NULL) delete pq;
 // if (_pWidget   != NULL) delete _pWidget;
    if (_pFieldRef != NULL) delete _pFieldRef;
}

QVariant cFieldEditBase::get() const
{
    _DBGFN() << QChar('/') << _recDescr<< QChar(' ') <<  debVariantToString(_value) << endl;
    return _value;
}

QString cFieldEditBase::getName() const
{
    return _recDescr.toName(get());
}

qlonglong cFieldEditBase::getId() const
{
    return _recDescr.toId(get());
}

int cFieldEditBase::set(const QVariant& _v)
{
    _DBGFN() << QChar('/') << _recDescr<< QChar(' ') <<  debVariantToString(_v) << endl;
    int st = 0;
    QVariant v = _recDescr.set(_v, st);
    if (st & cRecord::ES_DEFECTIVE) return -1;
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
        _DBGFN() << QChar('/') << _recDescr<< QChar(' ') <<  debVariantToString(v) << " dropped" << endl;
        return;
    }
    _DBGFN() << " { " << _recDescr << " } " <<  debVariantToString(v) << " != " <<  debVariantToString(_value) << endl;
    _value = v;
    changedValue(this);
}

cFieldEditBase * cFieldEditBase::anotherField(const QString& __fn, bool __ex)
{
    cFieldEditBase *p = _parent[__fn];
    if (p == NULL && __ex) {
        QString se = trUtf8("A keresett %1 mező, nem található a '%2'.'%3'-ból.")
                .arg(__fn).arg(_parent.name).arg(_pFieldRef->columnName());
        EXCEPTION(EDATA, -1, se);
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

cFieldEditBase *cFieldEditBase::createFieldWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef _fr, bool _ro, cRecordDialogBase *_par)
{
    _DBGFN() << QChar(' ') << _tm.tableName() << _sCommaSp << _fr.fullColumnName() << " ..." << endl;
    PDEB(VVERBOSE) << "Field value = " << debVariantToString(_fr) << endl;
    PDEB(VVERBOSE) << "Field descr = " << _fr.descr().allToString() << endl;
    if (!_tf.isNull(_sViewRights) && !lanView::isAuthorized(_tf.getId(_sViewRights))) {
        cNullWidget *p = new cNullWidget(_tm, _tf, _fr, true, _par);
        _DBGFNL() << " new cNullWidget" << endl;
        return p;
    }
    int et = _fr.descr().eColType;
    bool ro = _ro || !(_fr.record().isUpdatable()) || !(_fr.descr().isUpdatable);
    ro = ro || (et == cColStaticDescr::FT_INTEGER && _fr.record().autoIncrement()[_fr.index()]);    // Ha auto increment, akkor RO.
    if (ro) {
        switch (et) {
        // ReadOnly esetén a felsoroltak kivételével egysoros megjelenítés
        case cColStaticDescr::FT_POLYGON:
        case cColStaticDescr::FT_INTEGER_ARRAY:
        case cColStaticDescr::FT_REAL_ARRAY:
        case cColStaticDescr::FT_TEXT_ARRAY:
        case cColStaticDescr::FT_BINARY:
        case cColStaticDescr::FT_INTERVAL:
            break;
        default: {
            cFieldLineWidget *p =  new cFieldLineWidget(_tm, _tf, _fr, true, _par);
            _DBGFNL() << " new cFieldLineWidget" << endl;
            return  p;
        }
        }
    }
    switch (et) {
    case cColStaticDescr::FT_INTEGER:
        if (_fr.descr().fKeyType != cColStaticDescr::FT_NONE) {
            cFKeyWidget *p = new cFKeyWidget(_tm, _tf, _fr, _par);
            _DBGFNL() << " new cFKeyWidget" << endl;
            return p;
        }
        // Nincs break; !!
    case cColStaticDescr::FT_REAL:
    case cColStaticDescr::FT_TEXT:
    case cColStaticDescr::FT_MAC:
    case cColStaticDescr::FT_INET:
    case cColStaticDescr::FT_CIDR: {
        cFieldLineWidget *p = new cFieldLineWidget(_tm, _tf, _fr, ro, _par);
        _DBGFNL() << " new cFieldLineWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_BOOLEAN:
    case cColStaticDescr::FT_ENUM: {
        cEnumComboWidget *p = new cEnumComboWidget(_tm, _tf, _fr, _par);
        _DBGFNL() << " new cEnumComboWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_SET: {
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

/* **************************************** cNullWidget **************************************** */
cNullWidget::cNullWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase* _par)
    : cFieldEditBase(_tm, _tf, __fr, _ro, _par)
{
    _wType = FEW_NULL;
    QLabel *pL = new QLabel(_par->pWidget());
    _pWidget = pL;
    pL->setText(trUtf8("Nem elérhető"));
    pL->setFont(design()[GDR_NULL].font);
}

cNullWidget::~cNullWidget()
{
    ;
}

/* **************************************** cSetWidget **************************************** */

cSetWidget::cSetWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _ro, _par)
{
    _wType = FEW_SET;
    _pWidget  = new QWidget(_par->pWidget());
    pButtons  = new QButtonGroup(pWidget());
    pLayout   = new QVBoxLayout;
    pButtons->setExclusive(false);      // SET, több opció is kiválasztható
    widget().setLayout(pLayout);
    int id = 0;
    _bits = _recDescr.toId(_value);
    foreach (QString e, _recDescr.enumType().enumValues) {
        QString t = cEnumVal::title(*pq, e, _recDescr.udtName);
        QCheckBox *pCB = new QCheckBox(t, pWidget());
        pButtons->addButton(pCB, id);
        pLayout->addWidget(pCB);
        pCB->setChecked(enum2set(id) & _bits);
        pCB->setDisabled(_readOnly);
        ++id;
    }
    if (_nullView.isEmpty() == false) {
        QCheckBox *pCB = new QCheckBox(_nullView, pWidget());
        pCB->setFont(design().null.font);
        QPalette p = pCB->palette();
        p.setColor(QPalette::Active, QPalette::WindowText, design().null.fg);
        pCB->setPalette(p);
        pButtons->addButton(pCB, id);
        pLayout->addWidget(pCB);
        pCB->setChecked(__fr.isNull());
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
        int nid = _recDescr.enumType().enumValues.size();
        QAbstractButton *pAB = pButtons->button(nid);
        if (pAB != NULL) pAB->setChecked(v.isNull());
        _bits = _recDescr.toId(v);
        if (_bits < 0) _bits = 0;
        for (int id = 0; id < nid && NULL != (pAB = pButtons->button(id)) ; id++) {
            pAB->setChecked(enum2set(id) & _bits);
        }
    }
    return r;
}

void cSetWidget::setFromEdit(int id)
{
    _DBGFNL() << id << endl;
    int dummy;
    int n =_pFieldRef->descr().enumType().enumValues.size();
    if (id >= n) {
        for (int i = 0; i < n; ++i) {
            if (_bits & enum2set(i)) pButtons->button(i)->setChecked(false);
        }
        _bits = 0;
        setFromWidget(_recDescr.set(QVariant(), dummy));
    }
    else {
        _bits ^= enum2set(id);
        setFromWidget(_recDescr.set(QVariant(_bits), dummy));
        QAbstractButton *p = pButtons->button(n);
        if (p != NULL && p->isChecked()) p->setChecked(false);
    }
}


/* **************************************** cEnumRadioWidget ****************************************  */

cEnumRadioWidget::cEnumRadioWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _ro, _par)
{
    _wType = FEW_ENUM_RADIO;
    _pWidget = new QWidget(_par->pWidget());
    pButtons  = new QButtonGroup(pWidget());
    pLayout   = new QVBoxLayout;
    pButtons->setExclusive(true);
    widget().setLayout(pLayout);
    int id = 0;
    eval = _recDescr.toId(_value);
    QSqlQuery q = getQuery();
    foreach (QString e, _recDescr.enumType().enumValues) {
        QString t = cEnumVal::title(q, e, _recDescr.udtName);
        QRadioButton *pRB = new QRadioButton(t, pWidget());
        pButtons->addButton(pRB, id);
        pLayout->addWidget(pRB);
        pRB->setChecked(id == eval);
        pRB->setDisabled(_readOnly);
        ++id;
    }
    if (!_nullView.isEmpty()) {
        QRadioButton *pRB = new QRadioButton(_nullView, pWidget());
        pButtons->addButton(pRB, id);
        pLayout->addWidget(pRB);
        pRB->setChecked(eval < 0);
        pRB->setDisabled(_readOnly);
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
        eval = _recDescr.toId(v);
        if (eval >= 0) pButtons->button(eval)->setChecked(true);
        else {
            QAbstractButton *pRB = pButtons->button(_recDescr.enumType().enumValues.size());
            if (pRB != NULL) pRB->setChecked(true);
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
    if (id == _recDescr.enumType().enumValues.size()) {
        if (eval < 0) {
            if (pButtons->button(id)->isChecked() == false) pButtons->button(id)->setChecked(true);
            return;
        }
    }
    else {
        v = eval;
    }
    int dummy;
    setFromWidget(_recDescr.set(v, dummy));
}

/* **************************************** cEnumComboWidget ****************************************  */

cEnumComboWidget::cEnumComboWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, false, _par)
{
    eval = getId();
    _wType = FEW_ENUM_COMBO;
    QComboBox *pCB = new QComboBox(_par->pWidget());
    _pWidget = pCB;
    pCB->addItems(_recDescr.enumType().enumValues);                 // A lehetséges értékek nevei
    if (_nullView.isEmpty() == false) {
        pCB->addItem(_nullView);   // A NULL érték
    }
    pCB->setEditable(false);                        // Nem editálható, választás csak a listából
    setWidget();
    connect(pCB, SIGNAL(activated(int)), this, SLOT(setFromEdit(int)));
}

cEnumComboWidget::~cEnumComboWidget()
{
    ;
}

void cEnumComboWidget::setWidget()
{
    QComboBox *pCB = (QComboBox *)_pWidget;
    if (isContIx(_recDescr.enumType().enumValues, eval)) pCB->setCurrentIndex(eval);
    else pCB->setCurrentIndex(_recDescr.enumType().enumValues.size());
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
    if (eval == id) return;
    qlonglong v = id;
    if (id == _recDescr.enumType().enumValues.size()) v = NULL_ID;
    int dummy;
    setFromWidget(_recDescr.set(QVariant(v), dummy));
}

/* **************************************** cFieldLineWidget ****************************************  */

/*!
A 'features' mező:\n
:<b>passwd</b>: Ha megadtuk, és a mező típusa text, akkor a képernyőn nem olvasható az adat
 */
cFieldLineWidget::cFieldLineWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef _fr, bool _ro, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, _fr, _ro, _par)
{
    _wType = FEW_LINE;  // Widget típus azonosító
    QLineEdit *pLE = new QLineEdit(_par->pWidget());
    _pWidget = pLE;
    isPwd = false;
    bool nullable = _recDescr.isNullable;
    QString tx;
    if (_readOnly == false) {
        tx = _fr.toString();
        _value = QVariant(tx);
        switch (_recDescr.eColType) {
        case cColStaticDescr::FT_INTEGER:   pLE->setValidator(new cIntValidator( nullable, pLE));   break;
        case cColStaticDescr::FT_REAL:      pLE->setValidator(new cRealValidator(nullable, pLE));   break;
        case cColStaticDescr::FT_TEXT:      isPwd = _fieldShape.isFeature(_sPasswd);                  break;
        case cColStaticDescr::FT_MAC:       pLE->setValidator(new cMacValidator( nullable, pLE));   break;
        case cColStaticDescr::FT_INET:      pLE->setValidator(new cINetValidator(nullable, pLE));   break;
        case cColStaticDescr::FT_CIDR:      pLE->setValidator(new cCidrValidator(nullable, pLE));   break;
        default:    EXCEPTION(ENOTSUPP);
        }
        if (isPwd) {
            pLE->setEchoMode(QLineEdit::Password);
            pLE->setText("");
            _value.clear();
        }
        else {
            pLE->setText(_fr);
        }
        connect(pLE, SIGNAL(editingFinished()),  this, SLOT(setFromEdit()));
    }
    else {
        tx = _fr.view(*pq);
        if (isPwd) {
            tx = "********";
        }
        else if (_isInsert) {
            if (recDescr().autoIncrement()[fldIndex()]) tx = design().valAuto;
            else if (_hasDefault) tx = design().valDefault;
        }
        pLE->setText(tx);
        pLE->setReadOnly(true);
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
        QLineEdit *pLE = (QLineEdit *)_pWidget;
        if (_readOnly == false) {
            pLE->setText(_recDescr.toName(_value));
        }
        else {
            pLE->setText(_recDescr.toView(*pq, _value));
        }
    }
    return r;
}

void cFieldLineWidget::setFromEdit()
{
    QLineEdit *pLE = (QLineEdit *)_pWidget;
    QVariant v; // NULL
    QString  s = pLE->text();
    if (!(isPwd && s.isEmpty())) {
        v = QVariant(s);
    }
    setFromWidget(v);
    valid();
}


/* **************************************** cArrayWidget ****************************************  */


cArrayWidget::cArrayWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _ro, _par)
    , last()
{
    _wType   = FEW_ARRAY;

    _pWidget = new QWidget(_par->pWidget());
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
        connect(pUi->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(changed(QString)));
        connect(pUi->listView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
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

void cArrayWidget::selectionChanged(QItemSelection,QItemSelection)
{
    QModelIndexList mil = pUi->listView->selectionModel()->selectedColumns();
    selectedNum = mil.size();
    if (selectedNum == 0) actIndex = mil.first();
    else                  actIndex = QModelIndex();
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
        switch (_recDescr.eColType) {
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
            EXCEPTION(ENOTSUPP, _recDescr.eColType);
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

/* **************************************** cPolygonWidget ****************************************  */
/**
  A 'features' mező:\n
  :<b>map</b>=<i>\<sql függvény\></i>: Ha megadtuk, és a rekordnak már van ID-je (nem új rekord felvitel), és a megadott SQL függvény a
                rekord ID alapján visszaadta egy kép (images.image_id) azonosítóját, akkor feltesz egy plussz gombot, ami megjeleníti
                a képet, és azon klikkelve is felvehetünk pontokat.
 */
cPolygonWidget::cPolygonWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _ro, _par)
    , xPrev(), yPrev()
{
    _wType = FEW_POLYGON;

    pMapWin = NULL;
    pCImage = NULL;
    epic = NO_ANY_PIC;
    parentOrPlace_id = NULL_ID;
    selectedRowNum = 0;
    xOk = yOk = xyOk = false ;

    _pWidget = new QWidget(_par->pWidget());
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
    if (recDescr() == cPlace().descr()) {
        cFieldEditBase *p = anotherField(_sParentId);
        connect(p, SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(changeId(cFieldEditBase*)));
        epic = IS_PLACE_REC;
    }
    // Másik lehetőség, a features-ben van egy függvénynevünk, ami a rekord id-alapján megadja az image id-t
    else {
        id2imageFun = _tableShape.feature("map");  // Meg van adva a image id-t visszaadó függvlny neve ?
        if (id2imageFun.isEmpty() == false) {
            cFieldEditBase *p = anotherField(recDescr().idName());
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
    setButtons();
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

QModelIndex cPolygonWidget::actIndex(bool __ex)
{
    if (__ex && !_actIndex.isValid()) EXCEPTION(EDATA);
    return _actIndex;
}

int cPolygonWidget::actRow(bool __ex)
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
    pMapWin = new cImagePolygonWidget(!isReadOnly(), _parent.pWidget());
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
    _wType = FEW_FKEY;

    _pWidget = new QWidget(_par->pWidget());
    pUi = new Ui_fKeyEd;
    pUi->setupUi(_pWidget);

    pRDescr = cRecStaticDescr::get(_recDescr.fKeyTable, _recDescr.fKeySchema);
    pModel = new cRecordListModel(*pRDescr, pWidget());
    pModel->nullable = _recDescr.isNullable;
    pModel->setToNameF(_recDescr.fnToName);
    pModel->setFilter(_sNul, OT_ASC, FT_NO);
    pUi->comboBox->setModel(pModel);
    pUi->pushButtonEdit->setDisabled(true);
    pTableShape = new cTableShape();
    if (pTableShape->fetchByName(pRDescr->tableName())) {   // Ha meg tudjuk jeleníteni (azonos nevű shape)
        pUi->pushButtonNew->setEnabled(true);
        connect(pUi->pushButtonEdit, SIGNAL(pressed()), this, SLOT(modifyF()));
        connect(pUi->pushButtonNew,  SIGNAL(pressed()), this, SLOT(inserF()));
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
    ;
}

bool cFKeyWidget::setWidget()
{
    qlonglong id = _recDescr.toId(_value);
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

void cFKeyWidget::inserF()
{
    if (pTableShape == NULL) EXCEPTION(EPROGFAIL);
    cRecordAny *pRec = cRecordDialogBase::insertDialog(*pq, pTableShape, pRDescr);
    pModel->setFilter(_sNul, OT_ASC, FT_NO);
    if (pRec != NULL) {
        pUi->comboBox->setCurrentIndex(pModel->indexOf(pRec->getName()));
        pDelete(pRec);
    }
}

void cFKeyWidget::modifyF()
{
    if (pTableShape == NULL) EXCEPTION(EPROGFAIL);
    qlonglong id = pModel->atId(pUi->comboBox->currentIndex());
    cRecordAny r(pRDescr);
    if (r.fetchById(id)) {
        eTableInheritType   tit = (eTableInheritType)pTableShape->getId(_sTableInheritType);
        // ...
    }
}

/* **************************************** cDateWidget ****************************************  */


cDateWidget::cDateWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, false, _par)
{
    _wType = FEW_DATE;
    QDateEdit * pDE = new QDateEdit(_par->pWidget());
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
    _wType = FEW_TIME;
    QTimeEdit * pTE = new QTimeEdit(_par->pWidget());
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
    _wType = FEW_DATE_TIME;
    QDateTimeEdit * pDTE = new QDateTimeEdit(_par->pWidget());
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


cIntervalWidget::cIntervalWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _ro, _par)
{
    _wType = FEW_INTERVAL;
    _pWidget = new QWidget(_par->pWidget());
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
        qlonglong v = _recDescr.toId(_value);
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

cBinaryWidget::cBinaryWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _ro, _par)
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
    isCImage   = recDescr() == cidescr;     // éppen egy cImage objektumot szerkesztünk

    _pWidget        = new QWidget(_parent.pWidget());
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
        QMessageBox::warning(pWidget(), trUtf8("Hiba"), trUtf8("A megadott fájl nem olvasható"));
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
