#include "record_dialog.h"
#include "lv2validator.h"

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
    case FEW_RO_LINE:       return "ReadOnly LineEdit";
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
    _readOnly   = _ro;
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
    _DBGFN() << QChar('/') << _recDescr<< QChar(' ') <<  debVariantToString(v) << endl;
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
A 'properties' mező:\n
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
        case cColStaticDescr::FT_TEXT:      isPwd = _fieldShape.findMagic(_sPasswd);                break;
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
    pLayout  = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    pList = new QListView(_pWidget);
    if (_readOnly) {
        pLayout->addWidget(pList);

        pLineEd     = NULL;
        pLeftLayout = NULL;
        pRightLayout= NULL;
        pAddButton  = NULL;
        pDelButton  = NULL;
        pClearButton= NULL;
    }
    else {
        pLeftLayout  = new QVBoxLayout;
        pRightLayout = new QVBoxLayout;

        pLineEd = new QLineEdit(_pWidget);

        pAddButton   = new QPushButton(trUtf8("Hozzáad"), _pWidget);
        pDelButton   = new QPushButton(trUtf8("Töröl"), _pWidget);
        pClearButton = new QPushButton(trUtf8("Ürít"), _pWidget);

        pLayout->addLayout(pLeftLayout);
        pLayout->addLayout(pRightLayout);

        // Left layout
        pLeftLayout->addWidget(pList);
        pLeftLayout->addStretch();
        pLeftLayout->addWidget(pLineEd);

        // Right layout
        pRightLayout->addStretch();
        pRightLayout->addWidget(pAddButton);
        pRightLayout->addWidget(pDelButton);
        pRightLayout->addWidget(pClearButton);
    }
    pModel = new cStringListModel(pWidget());
    pModel->setStringList(_value.toStringList());
    pList->setModel(pModel);
    pList->setSelectionBehavior(QAbstractItemView::SelectRows);
    pList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pList->setAlternatingRowColors(true); //nem mükszik
    setButtons();
    pAddButton->setDisabled(true);
    pDelButton->setDisabled(true);
    if (!_readOnly) {
        connect(pAddButton,   SIGNAL(pressed()), this, SLOT(addRow()));
        connect(pDelButton,   SIGNAL(pressed()), this, SLOT(delRow()));
        connect(pClearButton, SIGNAL(pressed()), this, SLOT(clearRows()));
        connect(pLineEd,      SIGNAL(textChanged(QString)), this, SLOT(changed(QString)));
        connect(pList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
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
    bool empty = pModel->isEmpty();
    pDelButton->setDisabled(empty || !pList->selectionModel()->hasSelection());
    pClearButton->setDisabled(empty);
}

void cArrayWidget::delRow()
{
    QModelIndexList mil = pList->selectionModel()->selectedIndexes();
    pModel->remove(mil);
    setButtons();
    setFromWidget(pModel->stringList());
}

void cArrayWidget::addRow()
{
    *pModel << last;
    setButtons();
    setFromWidget(pModel->stringList());
    last.clear();
    pLineEd->setText(last);
    pAddButton->setDisabled(true);
}

void cArrayWidget::clearRows()
{
    pModel->clear();
    setButtons();
    setFromWidget(pModel->stringList());
}

void cArrayWidget::changed(QString _t)
{
    if (_t.isEmpty()) {
        last.clear();
        pLineEd->setText(last);
        pAddButton->setDisabled(true);
        return;
    }
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
    else    pLineEd->setText(last);
    pAddButton->setDisabled(last.isEmpty());
}

void cArrayWidget::selectionChanged(QItemSelection,QItemSelection)
{
    pDelButton->setDisabled(pModel->isEmpty() || !pList->selectionModel()->hasSelection());
}

/* **************************************** cPolygonWidget ****************************************  */
/**
  Polygon widget form (nem read-only)\n
  <tt>
  +-*_pWidget---------------------------------------------------------------------------+\n
  | +-*pLayout)-----------------------------------------------------------------------+ |\n
  | | +-*pLeftLayout------------------------------------------+ +-*pRightLayout-----+ | |\n
  | | | +-*pTable-------------------------------------------+ | | ^ ^ ^ ^ ^ ^ ^ ^ ^ | | |\n
  | | | | . . . . . . . . . . . . . . . . . . . . . . . . . | | | ^ ^ ^ ^ ^ ^ ^ ^ ^ | | |\n
  | | | | . . . . . . . . . . . . . . . . . . . . . . . . . | | | ^ ^ ^ ^ ^ ^ ^ ^ ^ | | |\n
  | | | +---------------------------------------------------+ | | +-*pAddButton---+ | | |\n
  | | | +-*pXYLayout----------------------------------------+ | | +---------------+ | | |\n
  | | | | +-*pLabelX-+ +-*pLineX-+ +-*pLabelY-+ +-*pLineY-+ | | | +-*pDelButton---+ | | |\n
  | | | | +----------+ +---------+ +----------+ +---------+ | | | +---------------+ | | |\n
  | | | +---------------------------------------------------+ | | +-*pClearButton-+ | | |\n
  | | | ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ | | +---------------+ | | |\n
  | | +-------------------------------------------------------+ +-------------------+ | |\n
  | +---------------------------------------------------------------------------------+ |\n
  +-------------------------------------------------------------------------------------+\n
  </tt>
  A 'properties' mező:\n
  :<b>map</b>=<i>\<sql függvény\></i>: Ha megadtuk, és a rekordnak már van ID-je (nem új rekord felvitel), és a megadott SQL függvény a
                rekord ID alapján visszaadta egy kép (images.image_id) azonosítóját, akkor feltesz egy plussz gombot, ami megjeleníti
                a képet, és azon klikkelve is felvehetünk pontokat.
 */
cPolygonWidget::cPolygonWidget(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, _ro, _par)
    , xPrev(), yPrev()
{
    _wType = FEW_POLYGON;
    _pWidget = new QWidget(_par->pWidget());
    pLayout = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    pTable = new QTableView(_pWidget);
    pImageButton = NULL;
    pMapWin = NULL;
    pMapRec = NULL;
    changeParentIdConnected = false;
    if (_readOnly) {
        pLayout->addWidget(pTable);

        pXYLayout   = NULL;
        pLabelX     = NULL;
        pLineX      = NULL;
        pLabelY     = NULL;
        pLineY      = NULL;
        pLeftLayout = NULL;
        pRightLayout= NULL;
        pAddButton  = NULL;
        pDelButton  = NULL;
        pClearButton= NULL;
    }
    else {
        pLeftLayout  = new QVBoxLayout;
        pRightLayout = new QVBoxLayout;
        pXYLayout    = new QHBoxLayout;

        pLabelX = new QLabel("X", _pWidget);
        pLineX  = new QLineEdit(_pWidget);
        pLabelY = new QLabel("Y", _pWidget);
        pLineY  = new QLineEdit(_pWidget);

        pAddButton   = new QPushButton(trUtf8("Hozzáad"), _pWidget);
        pDelButton   = new QPushButton(trUtf8("Töröl"),   _pWidget);
        pClearButton = new QPushButton(trUtf8("Ürít"),    _pWidget);

        pImageButton = new QPushButton(trUtf8("Alaprajz"), _pWidget);
        pImageButton->hide();
        pZoomIn      = new QPushButton(trUtf8("+"), _pWidget);
        pZoomIn->hide();
        pZoomOut     = new QPushButton(trUtf8("-"), _pWidget);
        pZoomOut->hide();
        pImgButLayout = new QHBoxLayout;
        pImgButLayout->addWidget(pImageButton);
        pImgButLayout->addWidget(pZoomIn);
        pImgButLayout->addWidget(pZoomOut);

        pLayout->addLayout(pLeftLayout);
        pLayout->addLayout(pRightLayout);

        // Left layout
        pLeftLayout->addWidget(pTable);
        pLeftLayout->addStretch();
        pLeftLayout->addLayout(pXYLayout);

        pXYLayout->addWidget(pLabelX);
        pXYLayout->addWidget(pLineX);
        pXYLayout->addWidget(pLabelY);
        pXYLayout->addWidget(pLineY);

        // Right layout
        pRightLayout->addStretch();
        pRightLayout->addWidget(pAddButton);
        pRightLayout->addWidget(pDelButton);
        pRightLayout->addWidget(pClearButton);
        if (pImageButton != NULL) {
            pRightLayout->addLayout(pImgButLayout);
        }

        xOk = yOk = false;
        pAddButton->setDisabled(true);
        pDelButton->setDisabled(true);
    }
    if (_value.isValid()) polygon = _value.value<tPolygonF>();
    pModel = new cPolygonTableModel(this);
    pModel->setPolygon(polygon);
    pTable->setModel(pModel);
    pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pTable->setSelectionMode(QAbstractItemView::ContiguousSelection);
    pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pTable->setAlternatingRowColors(true); //nem mükszik ?
    setButtons();
    if (!_readOnly) {
        connect(pAddButton,   SIGNAL(pressed()), this, SLOT(addRow()));
        connect(pDelButton,   SIGNAL(pressed()), this, SLOT(delRow()));
        connect(pClearButton, SIGNAL(pressed()), this, SLOT(clearRows()));
        connect(pLineX, SIGNAL(textChanged(QString)), this, SLOT(xChanged(QString)));
        connect(pLineY, SIGNAL(textChanged(QString)), this, SLOT(yChanged(QString)));
        connect(pTable->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
        connect(pImageButton, SIGNAL(pressed()), this, SLOT(imageOpen()));
        image();
    }
}

cPolygonWidget::~cPolygonWidget()
{
    pDelete(pMapWin);
}

void cPolygonWidget::image()
{
    bool opened = false;
    if (pMapRec) {
        pImageButton->hide();
        if (pMapWin) {
            pMapWin->close();
            pDelete(pMapWin);
            opened = true;
        }
        pDelete(pMapRec);
    }
    // Alaprajz, ha van
    qlonglong id = NULL_ID;    // Az image rkord ID-je (még NULL)
    // Ha egy 'places' rekordot szerkesztünk, akkor van egy parent_id  mezőnk, amiből megvan az image_id
    if (recDescr() == cPlace().descr()) {
        cFieldEditBase *p = anotherField(_sImageId);
        if (!changeParentIdConnected) {
            changeParentIdConnected = true;
            connect(p, SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(changeParentId(cFieldEditBase*)));
        }
        id = p->getId();
        if (id != NULL_ID) {
            bool ok;
            id = execSqlIntFunction(*pq, &ok, "get_image", id);
            if (!ok) {
                id = NULL_ID;
            }
        }
    }
    // Másik lehetőség, a properties-ben van egy függvénynevünk, ami a rekord id-alapján megadja az image id-t
    else {
        id = _pFieldRef->record().getId();    // Az editálandü rekord ID-je, (read_only mező)
        if (id != NULL_ID) {   // ID nélkül nincs image (ez feature)
            QString map_f = _tableShape.magicParam("map");  // Meg van adva a image id-t visszaadó függvlny neve ?
            if (map_f.isEmpty() == false) {
                bool ok;
                id = execSqlIntFunction(*pq, &ok, map_f, id);
                if (!ok) {
                    id = NULL_ID;
                }
            }
            else id = NULL_ID;
        }
    }
    if (id > 0) { // if (iid != NULL_ID) {
        // Valamiért 0-val tér vissza, NULL helyett :-o
        pMapRec = new cImage();
        pMapRec->setParent(this);
        pMapRec->setById(*pq, id);
        pImageButton->show();
        if (opened) imageOpen();
    }
}

void cPolygonWidget::setButtons()
{
    bool empty = polygon.isEmpty();
    pDelButton->setDisabled(empty || !pTable->selectionModel()->hasSelection());
    pClearButton->setDisabled(empty);
}

int cPolygonWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (r == 1) {
        polygon = _value.value<tPolygonF>();
        pModel->setPolygon(polygon);
        setButtons();
    }
    image();    // Feltételezi, hogy az rekord ID ha van elöbb lett megadva
    return r;
}

void cPolygonWidget::drawPolygon()
{
    if (pMapWin == NULL) return;
    if (polygon.size() > 0) {
        pMapWin->addDraw(QVariant::fromValue(polygon));
    }
    if (!pMapWin->setImage(*pMapRec)) EXCEPTION(EDATA, -1, trUtf8("Nem lehet betölteni az alaprajzot."));
}

void cPolygonWidget::delRow()
{
    QModelIndexList mil = pTable->selectionModel()->selectedRows();
    if (mil.size()) {
        do {
            int row = mil.last().row();
            pModel->remove(row);
            PDEB(VVERBOSE) << "Remove #" << row << endl;
            mil = pTable->selectionModel()->selectedRows();
        } while (mil.size());
        polygon = pModel->polygon();
    }
    else {
        polygon = pModel->pop_back().polygon();
    }
    setButtons();
    setFromWidget(QVariant::fromValue(polygon));
    drawPolygon();
}

void cPolygonWidget::addRow()
{
    QPointF pt(x,y);
    *pModel << pt;
    polygon = pModel->polygon();
    setButtons();
    setFromWidget(QVariant::fromValue(polygon));
    xPrev.clear();
    pLineX->setText(xPrev);
    yPrev.clear();
    pLineY->setText(yPrev);
    pAddButton->setDisabled(true);
    drawPolygon();
}

void cPolygonWidget::clearRows()
{
    polygon.clear();
    pModel->clear();
    setButtons();
    setFromWidget(QVariant::fromValue(polygon));
    drawPolygon();
}

void cPolygonWidget::xChanged(QString _t)
{
    if (_t.isEmpty()) {
        pAddButton->setDisabled(true);
        xPrev.clear();
    }
    else {
        x = _t.toDouble(&xOk);
        if (xOk) xPrev = _t;
        else     pLineX->setText(xPrev);
        pAddButton->setDisabled(xPrev.isEmpty() || yPrev.isEmpty());
    }
}

void cPolygonWidget::yChanged(QString _t)
{
    if (_t.isEmpty()) {
        pAddButton->setDisabled(true);
        yPrev.clear();
    }
    else {
        y = _t.toDouble(&yOk);
        if (yOk) yPrev = _t;
        else     pLineY->setText(yPrev);
        pAddButton->setDisabled(xPrev.isEmpty() || yPrev.isEmpty());
    }
}

void cPolygonWidget::selectionChanged(QItemSelection,QItemSelection)
{
    pDelButton->setDisabled(polygon.isEmpty() || !pTable->selectionModel()->hasSelection());
}

void cPolygonWidget::imageOpen()
{
    if (pMapRec == NULL) EXCEPTION(EPROGFAIL);
    if (pMapWin != NULL) {
        pMapWin->show();
        return;
    }
    pMapWin = new cImageWidget();
    pMapWin->setBrush(QBrush(QColor(Qt::red))).clearDraws();
    drawPolygon();
    if (!isReadOnly()) {
        connect(pMapWin, SIGNAL(mousePressed(QPoint)), this, SLOT(imagePoint(QPoint)));
    }
    pZoomIn->show();
    pZoomOut->show();
    connect(pZoomIn, SIGNAL(pressed()), pMapWin, SLOT(zoomIn()));
    connect(pZoomOut,SIGNAL(pressed()), pMapWin, SLOT(zoomOut()));
    connect(pMapWin, SIGNAL(destroyed(QObject*)), SLOT(destroyedImage(QObject*)));
}

void cPolygonWidget::imagePoint(QPoint _p)
{
    QPointF pt = _p;
    *pModel << pt;
    polygon = pModel->polygon();
    setButtons();
    setFromWidget(QVariant::fromValue(polygon));
    pMapWin->clearDraws();
    if (polygon.size()) {
        pMapWin->addDraw(QVariant::fromValue(polygon));
    }
    if (!pMapWin->setImage(*pMapRec)) EXCEPTION(EDATA, -1, trUtf8("Nem lehet betölteni az alaprajzot."));
}

void cPolygonWidget::destroyedImage(QObject *p)
{
    (void)p;
    pMapWin = NULL;
}

void cPolygonWidget::changeParentId(cFieldEditBase *p)
{
    (void)p;
    image();
}

/* **************************************** cFKeyWidget ****************************************  */

cFKeyWidget::cFKeyWidget(const cTableShape& _tm, const cTableShapeField& _tf, cRecordFieldRef __fr, cRecordDialogBase *_par)
    : cFieldEditBase(_tm, _tf, __fr, false, _par)
{
    _wType = FEW_FKEY;
    QComboBox *pCB = new QComboBox(_par->pWidget());
    _pWidget = pCB;
    pRDescr = cRecStaticDescr::get(_recDescr.fKeyTable, _recDescr.fKeySchema);
    pModel = new cRecordListModel(*pRDescr, pWidget());
    pModel->nullable = _recDescr.isNullable;
    pModel->setToNameF(_recDescr.fnToName);
    pModel->setFilter(_sNul, OT_ASC, FT_NO);
    pCB->setModel(pModel);
    pCB->setEditable(true);
    pCB->setAutoCompletion(true);
    setWidget();
    connect(pCB, SIGNAL(currentIndexChanged(int)), this, SLOT(setFromEdit(int)));
    //connect(pCB, SIGNAL(editTextChanged(QString)), this, SLOT(_edited(QString)));
}

cFKeyWidget::~cFKeyWidget()
{
    ;
}

bool cFKeyWidget::setWidget()
{
    qlonglong id = _recDescr.toId(_value);
    int ix = pModel->indexOf(id);
    if (ix < 0) return false;
    pComboBox()->setCurrentIndex(ix);
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
    setFromWidget(pModel->atId(i));
}

/*
void cFKeyWidget::_edited(QString _txt)
{
    while (!pModel->setFilter(_txt, OT_ASC, FT_BEGIN)) {
        if (_txt.size() == 0) break;
        _txt.chop(1);
        pComboBox()->
    }

}
*/
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
    qlonglong v = _recDescr.toId(_value);
    int msec   = v % 1000; v /= 1000;
    int sec    = v %   60; v /=   60;
    int minute = v %   60; v /=   60;
    int hour   = v %   24; v /=   24;
    QTime t(hour, minute, sec, msec);
    pTimeEd->setTime(t);
    pLineEdDay->setText(QString::number(v));
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
    pRadioButton->setChecked(z);
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
    pRadioButton    = NULL;
    pLoadButton     = NULL;
    pViewButton     = NULL;
    pZoomInButton   = NULL;
    pZoomOutButton  = NULL;
    pImageWidget    = NULL;
    pCImage         = NULL;
    isCImage = recDescr() == cImage().descr();
    _pWidget        = new QWidget(_parent.pWidget());
    pLayout         = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    pRadioButton    = new QRadioButton(_sNULL, _pWidget);
    pRadioButton->setDisabled(_readOnly);
    pLayout->addWidget(pRadioButton);
    if (!_readOnly) {
        pLoadButton  = new QPushButton(trUtf8("Betölt"), _pWidget);
        pLayout->addWidget(pLoadButton);
        connect(pRadioButton, SIGNAL(clicked(bool)), this, SLOT(nullChecked(bool)));
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
        connect(anotherField(_sImageName), SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(changedAnyField(cFieldEditBase*)));
        connect(anotherField(_sImageType), SIGNAL(changedValue(cFieldEditBase*)), this, SLOT(changedAnyField(cFieldEditBase*)));
    }
}


bool cBinaryWidget::setCImage()
{
    setNull();
    if (!isCImage) EXCEPTION(EPROGFAIL);            // Ha nem cImage a rekord, akkor hogy kerültünk ide ?!
    if (pCImage == NULL) pCImage = new cImage;
    pCImage->clear();
    if (data.size() > 0 && !pRadioButton->isChecked()) {
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
        pRadioButton->setChecked(z);
        if (z) {
            data.clear();
        }
        else {
            data = _value.toByteArray();
        }
        setCImage();
    }
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
    pRadioButton->setChecked(false);
    pRadioButton->setCheckable(true);
    setFromWidget(QVariant(data));
}

void cBinaryWidget::setNull()
{
    if (data.isEmpty()) {
        pRadioButton->setChecked(true);
        pRadioButton->setDisabled(true);
    }
    else {
        pRadioButton->setEnabled(true);
    }
    bool f = pRadioButton->isChecked();
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
