#include "lv2widgets.h"
#include "lv2validator.h"

/* **************************************** cImageWindows ****************************************  */

cImageWindow::cImageWindow(QWidget *__par) : QLabel(__par)
{
    pImage = NULL;
    setAcceptDrops(true);
    // Test
/*    QList<QByteArray> lst = QImageReader::supportedImageFormats ();
    QString s;
    foreach (QByteArray a, lst) { s += a + QChar(','); }
    s.chop(1);
    PDEB(INFO) << "QImageReader::supportedImageFormats() : " << s << endl;*/
}

cImageWindow::~cImageWindow()
{
    hide();
    if (pImage) delete pImage;
}

bool cImageWindow::setImage(const QString& __fn, const QString& __t)
{
    hide();
    QString title = __t;
    if (title.isEmpty()) title = "IMAGE : " + __fn;
    setWindowTitle(title);
    if (pImage != NULL) pImage = new QPixmap();
    pImage = new QPixmap(__fn);
    if (pImage->isNull()) return false;
    setPixmap(*pImage);
    show();
    return true;
}

bool cImageWindow::setImage(QSqlQuery __q, qlonglong __id, const QString& __t)
{
    cImage  im;
    im.setId(__id);
    if (1 != im.completion(__q)) return false;
    return setImage(im, __t);
}

bool cImageWindow::setImage(const cImage& __o, const QString& __t)
{
    if (pImage != NULL) pImage = new QPixmap();
    if (!pImage->loadFromData(__o.getImage(), __o.getType())) return false;
    setWindowTitle(__t.isEmpty() ? __o.getName(_sImageNote) : __t);
    setPixmap(*pImage);
    show();
    return true;

}

void cImageWindow::mousePressEvent(QMouseEvent * ev)
{
    mousePressed(ev->pos());
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

cFieldEditBase::cFieldEditBase(const cTableShape &_tm, cRecordFieldRef _fr, eSyncType _sy, bool _ro, QWidget *_par)
    : QObject(_par)
    , _descr(_fr.descr())
    , _tableShape(_tm)
    , _value()
{
    _wType      = FEW_UNKNOWN;
    _syType     = _sy;
    _readOnly   = _ro;
    _value      = _fr;
    _pWidget    = NULL;
    _pFieldRef  = new cRecordFieldRef(_fr);
    pq          = newQuery();
    if (_syType & SY_FROM_REC) {
        const cRecord * pr = &_fr.record();
        connect(pr, SIGNAL(modified()), this, SLOT(modRec()));
        connect(pr, SIGNAL(cleared()), this, SLOT(modRec()));
        connect(pr, SIGNAL(fieldModified(int)), this, SLOT(modField(int)));
    }
    if (_syType & SY_TO_REC && _ro) _syType = (eSyncType)(_syType & ~SY_TO_REC);
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
    if (_pWidget   != NULL) delete _pWidget;
    if (_pFieldRef != NULL) delete _pFieldRef;
}

QVariant cFieldEditBase::get() const
{
    _DBGFN() << QChar('/') << _descr<< QChar(' ') <<  debVariantToString(_value) << endl;
    return _value;
}

QString cFieldEditBase::getName() const
{
    return _descr.toName(get());
}

qlonglong cFieldEditBase::getId() const
{
    return _descr.toId(get());
}

int cFieldEditBase::set(const QVariant& _v)
{
    _DBGFN() << QChar('/') << _descr<< QChar(' ') <<  debVariantToString(_v) << endl;
    int st = 0;
    QVariant v = _descr.set(_v, st);
    if (st & cRecord::ES_DEFECTIVE) return -1;
    if (v == _value) return 0;
    _value = v;
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

void cFieldEditBase::_setv(QVariant v)
{
    if (_value == v) {
        _DBGFN() << QChar('/') << _descr<< QChar(' ') <<  debVariantToString(v) << " dropped" << endl;
        return;
    }
    _DBGFN() << QChar('/') << _descr<< QChar(' ') <<  debVariantToString(v) << endl;
    _value = v;
    if (syncType() & SY_TO_REC) {
        if (_value != fieldValue()) (*_pFieldRef) = _value;
    }
}


// SLOT
void cFieldEditBase::modRec()
{
    _DBGFN() << QChar('/') << _descr << endl;
    if (_value != fieldValue()) {
        bool r = set(fieldValue());
        PDEB(VVERBOSE) << "Sync to widget field #" << _pFieldRef->index() << VDEB(_value) << " TO " << fieldToName() << (r ? " OK" : " FAILED") << endl;
    }
}

void cFieldEditBase::modField(int ix)
{
    _DBGFN() << QChar('/') << _descr << endl;
    if (ix == _pFieldRef->index()) modRec();
}

cFieldEditBase *cFieldEditBase::createFieldWidget(const cTableShape& _tm, cRecordFieldRef _fr, eSyncType _sy, bool _ro, QWidget * _par)
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
            cFieldLineWidget *p =  new cFieldLineWidget(_tm, _fr, _sy, true, _par);
            _DBGFNL() << " new cFieldLineWidget" << endl;
            return  p;
        }
        }
    }
    switch (et) {
    case cColStaticDescr::FT_INTEGER:
        if (_fr.descr().fKeyType != cColStaticDescr::FT_NONE) {
            cFKeyWidget *p = new cFKeyWidget(_tm, _fr, _sy, _par);
            _DBGFNL() << " new cFKeyWidget" << endl;
            return p;
        }
        // Nincs break; !!
    case cColStaticDescr::FT_REAL:
    case cColStaticDescr::FT_TEXT:
    case cColStaticDescr::FT_MAC:
    case cColStaticDescr::FT_INET:
    case cColStaticDescr::FT_CIDR: {
        cFieldLineWidget *p = new cFieldLineWidget(_tm, _fr, _sy, ro, _par);
        _DBGFNL() << " new cFieldLineWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_BOOLEAN:
    case cColStaticDescr::FT_ENUM: {
        cEnumComboWidget *p = new cEnumComboWidget(_tm, _fr, _sy, _par);
        _DBGFNL() << " new cEnumComboWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_SET: {
        cSetWidget *p = new cSetWidget(_tm, _fr, _sy, ro, _par);
        _DBGFNL() << " new cSetWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_POLYGON: {
        cPolygonWidget *p = new cPolygonWidget(_tm, _fr, _sy, ro, _par);
        _DBGFNL() << " new cPolygonWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_INTEGER_ARRAY:
    case cColStaticDescr::FT_REAL_ARRAY:
    case cColStaticDescr::FT_TEXT_ARRAY: {
         cArrayWidget *p = new cArrayWidget(_tm, _fr, _sy, ro, _par);
        _DBGFNL() << " new cArrayWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_BINARY: {
        cBinaryWidget *p = new cBinaryWidget(_tm, _fr, _sy, ro, _par);
        _DBGFNL() << " new cBinaryWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_TIME: {
        cTimeWidget *p = new cTimeWidget(_tm, _fr, _sy, _par);
        _DBGFNL() << " new cTimeWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_DATE: {
        cDateWidget *p = new cDateWidget(_tm, _fr, _sy, _par);
        _DBGFNL() << " new cDateWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_DATE_TIME: {
        cDateTimeWidget *p = new cDateTimeWidget(_tm, _fr, _sy, _par);
        _DBGFNL() << " new cDateTimeWidget" << endl;
        return p;
    }
    case cColStaticDescr::FT_INTERVAL: {
        cIntervalWidget *p = new cIntervalWidget(_tm, _fr, _sy, ro, _par);
        _DBGFNL() << " new cIntervalWidget" << endl;
        return p;
    }
    default:
        EXCEPTION(EDATA, et);
        return NULL;
    }
}

/* **************************************** cSetWidget **************************************** */

cSetWidget::cSetWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, bool _ro, QWidget * par)
    : cFieldEditBase(_tm, __fr,_sy, _ro, par)
{
    _wType = FEW_SET;
    _pWidget  = new QWidget(par);
    pButtons  = new QButtonGroup(pWidget());
    pLayout   = new QVBoxLayout;
    pButtons->setExclusive(false);      // SET, több opció is kiválasztható
    widget().setLayout(pLayout);
    int id = 0;
    _bits = _descr.toId(_value);
    foreach (QString e, _descr.enumVals) {
        QString t = cEnumVal::title(*pq, e, _descr.udtName);
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
    connect(pButtons, SIGNAL(buttonClicked(int)), this, SLOT(_set(int)));
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
        int nid = _descr.enumVals.size();
        QAbstractButton *pAB = pButtons->button(nid);
        if (pAB != NULL) pAB->setChecked(v.isNull());
        _bits = _descr.toId(v);
        if (_bits < 0) _bits = 0;
        for (int id = 0; id < nid && NULL != (pAB = pButtons->button(id)) ; id++) {
            pAB->setChecked(enum2set(id) & _bits);
        }
    }
    return r;
}

void cSetWidget::_set(int id)
{
    _DBGFNL() << id << endl;
    int dummy;
    int n =_pFieldRef->descr().enumVals.size();
    if (id >= _pFieldRef->descr().enumVals.size()) {
        for (int i = 0; i < n; ++i) {
            if (_bits & enum2set(i)) pButtons->button(i)->setChecked(false);
        }
        _bits = 0;
        _setv(_descr.set(QVariant(), dummy));
    }
    else {
        _bits ^= enum2set(id);
        _setv(_descr.set(QVariant(_bits), dummy));
        QAbstractButton *p = pButtons->button(n);
        if (p != NULL && p->isChecked()) p->setChecked(false);
    }
}


/* **************************************** cEnumRadioWidget ****************************************  */

cEnumRadioWidget::cEnumRadioWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, bool _ro, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy, _ro, par)
{
    _wType = FEW_ENUM_RADIO;
    _pWidget = new QWidget(par);
    pButtons  = new QButtonGroup(pWidget());
    pLayout   = new QVBoxLayout;
    pButtons->setExclusive(true);
    widget().setLayout(pLayout);
    int id = 0;
    eval = _descr.toId(_value);
    QSqlQuery q = getQuery();
    foreach (QString e, _descr.enumVals) {
        QString t = cEnumVal::title(q, e, _descr.udtName);
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
    connect(pButtons, SIGNAL(buttonClicked(int)),  this, SLOT(_set(int)));
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
        eval = _descr.toId(v);
        if (eval >= 0) pButtons->button(eval)->setChecked(true);
        else {
            QAbstractButton *pRB = pButtons->button(_descr.enumVals.size());
            if (pRB != NULL) pRB->setChecked(true);
        }
    }
    return r;
}

void cEnumRadioWidget::_set(int id)
{
    QVariant v;
    if (eval == id) {
        if (pButtons->button(id)->isChecked() == false) pButtons->button(id)->setChecked(true);
        return;
    }
    if (id == _descr.enumVals.size()) {
        if (eval < 0) {
            if (pButtons->button(id)->isChecked() == false) pButtons->button(id)->setChecked(true);
            return;
        }
    }
    else {
        v = eval;
    }
    int dummy;
    _setv(_descr.set(v, dummy));
}

/* **************************************** cEnumComboWidget ****************************************  */

cEnumComboWidget::cEnumComboWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy, false, par)
{
    eval = getId();
    _wType = FEW_ENUM_COMBO;
    QComboBox *pCB = new QComboBox(par);
    _pWidget = pCB;
    pCB->addItems(_descr.enumVals);                 // A lehetséges értékek nevei
    if (_nullView.isEmpty() == false) {
        pCB->addItem(_nullView);   // A NULL érték
    }
    pCB->setEditable(false);                        // Nem editálható, választás csak a listából
    setWidget();
    connect(pCB, SIGNAL(activated(int)), this, SLOT(_set(int)));
}

cEnumComboWidget::~cEnumComboWidget()
{
    ;
}

void cEnumComboWidget::setWidget()
{
    QComboBox *pCB = (QComboBox *)_pWidget;
    if (isContIx(_descr.enumVals, eval)) pCB->setCurrentIndex(eval);
    else pCB->setCurrentIndex(_descr.enumVals.size());
}

int cEnumComboWidget::set(const QVariant& v)
{
    int r = cFieldEditBase::set(v);
    if (1 == r) {
        _setv(v);
        eval = getId();
        setWidget();
    }
    return r;
}

void cEnumComboWidget::_set(int id)
{
    if (eval == id) return;
    qlonglong v = id;
    if (id == _descr.enumVals.size()) v = NULL_ID;
    int dummy;
    _setv(_descr.set(QVariant(v), dummy));
}

/* **************************************** cFieldLineWidget ****************************************  */

cFieldLineWidget::cFieldLineWidget(const cTableShape& _tm, cRecordFieldRef _fr, eSyncType _sy, bool _ro, QWidget * par)
    : cFieldEditBase(_tm, _fr, _sy, _ro, par)
{
    _wType = FEW_LINE;  // Widget típus azonosító
    QLineEdit *pLE = new QLineEdit(par);
    _pWidget = pLE;
    bool nullable = _descr.isNullable;
    QString tx;
    if (_readOnly == false) {
        tx = _fr.toString();
        _value = QVariant(tx);
        pLE->setText(_fr);
        switch (_descr.eColType) {
        case cColStaticDescr::FT_INTEGER:   pLE->setValidator(new cIntValidator( nullable, pLE));   break;
        case cColStaticDescr::FT_REAL:      pLE->setValidator(new cRealValidator(nullable, pLE));   break;
        case cColStaticDescr::FT_TEXT:      break;
        case cColStaticDescr::FT_MAC:       pLE->setValidator(new cMacValidator( nullable, pLE));   break;
        case cColStaticDescr::FT_INET:      pLE->setValidator(new cINetValidator(nullable, pLE));   break;
        case cColStaticDescr::FT_CIDR:      pLE->setValidator(new cCidrValidator(nullable, pLE));   break;
        default:    EXCEPTION(ENOTSUPP);
        }
        connect(pLE, SIGNAL(editingFinished()),  this, SLOT(_set()));
    }
    else {
        tx = _fr.view(*pq);
        if (_isInsert) {
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
    int r = cFieldEditBase::set(v);
    if (1 == r) {
        QLineEdit *pLE = (QLineEdit *)_pWidget;
        if (_readOnly == false) {
            pLE->setText(_descr.toName(_value));
        }
        else {
            pLE->setText(_descr.toView(*pq, _value));
        }
    }
    return r;
}

void cFieldLineWidget::_set()
{
    QLineEdit *pLE = (QLineEdit *)_pWidget;
    _setv(QVariant(pLE->text()));
    valid();
}

/* **************************************** cArrayWidget ****************************************  */


cArrayWidget::cArrayWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, bool _ro, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy, _ro, par)
    , last()
{
    _wType   = FEW_ARRAY;
    _pWidget = new QWidget(par);
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
    _setv(pModel->stringList());
}

void cArrayWidget::addRow()
{
    *pModel << last;
    setButtons();
    _setv(pModel->stringList());
    last.clear();
    pLineEd->setText(last);
    pAddButton->setDisabled(true);
}

void cArrayWidget::clearRows()
{
    pModel->clear();
    setButtons();
    _setv(pModel->stringList());
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
    switch (_descr.eColType) {
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
        EXCEPTION(ENOTSUPP, _descr.eColType);
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
 */
cPolygonWidget::cPolygonWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, bool _ro, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy, _ro, par)
    , xPrev(), yPrev()
{
    _wType = FEW_POLYGON;
    _pWidget = new QWidget(par);
    pLayout = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    pTable = new QTableView(_pWidget);
    pImageButton = NULL;
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
        pDelButton   = new QPushButton(trUtf8("Töröl"), _pWidget);
        pClearButton = new QPushButton(trUtf8("Ürít"), _pWidget);


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
    }
}

cPolygonWidget::~cPolygonWidget()
{
    ;
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
    return r;
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
    _setv(QVariant::fromValue(polygon));
}

void cPolygonWidget::addRow()
{
    QPointF pt(x,y);
    *pModel << pt;
    polygon = pModel->polygon();
    setButtons();
    _setv(QVariant::fromValue(polygon));
    xPrev.clear();
    pLineX->setText(xPrev);
    yPrev.clear();
    pLineY->setText(yPrev);
    pAddButton->setDisabled(true);
}

void cPolygonWidget::clearRows()
{
    polygon.clear();
    pModel->clear();
    setButtons();
    _setv(QVariant::fromValue(polygon));
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


/* **************************************** cFKeyWidget ****************************************  */

cFKeyWidget::cFKeyWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy, false, par)
{
    _wType = FEW_FKEY;
    QComboBox *pCB = new QComboBox(par);
    _pWidget = pCB;
    pRDescr = cRecStaticDescr::get(_descr.fKeyTable, _descr.fKeySchema);
    pModel = new cRecordListModel(*pRDescr, pWidget());
    pModel->nullable = _descr.isNullable;
    pModel->setToNameF(_descr.fnToName);
    /* Ez hülyeség, vagy nem értem
    if (_descr.fKeyTable == recDescr().tableName()) {   // saját táblára mutató kulcs
        if (_tableShape.typeIs(TS_CHILD)) {             // CHILD tábla, önagára mutató kulcsal,
            int fix = recDescr().ixToOwner();
            qlonglong   oid = _pFieldRef->record().getId(fix);      // Az ownerre mutató ID
            QString     fnm = _pFieldRef->record().columnNameQ(fix);
            if (oid == NULL_ID) {       // Az owner még nincs az adatbázisban ?
                // Akkor ezt a mezőt nem tudjuk kitölteni, Csak olvasható lessz.
                pDelete(pModel);
                pDelete(_pWidget);
                _readOnly = true;
                if (initIfRo(par)) return;
                EXCEPTION(EPROGFAIL);
            }
            QString where = fnm + " = " + QString::number(oid);     // Csak az azonos ownerű rekordok kellenek
            pModel->setConstFilter(where, FT_SQL_WHERE);
        }
//      else {  // Ide kéne egy szűrés, hogy valóban rendes fa legyen
//      }
    }*/
    pModel->setFilter(_sNul, OT_ASC, FT_NO);
    pCB->setModel(pModel);
    pCB->setEditable(true);
    pCB->setAutoCompletion(true);
    setWidget();
    connect(pCB, SIGNAL(currentIndexChanged(int)), this, SLOT(_set(int)));
    //connect(pCB, SIGNAL(editTextChanged(QString)), this, SLOT(_edited(QString)));
}

cFKeyWidget::~cFKeyWidget()
{
    ;
}

bool cFKeyWidget::setWidget()
{
    qlonglong id = _descr.toId(_value);
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

void cFKeyWidget::_set(int i)
{
    _setv(pModel->atId(i));
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


cDateWidget::cDateWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy, false, par)
{
    _wType = FEW_DATE;
    QDateEdit * pDE = new QDateEdit(par);
    _pWidget = pDE;
    connect(pDE, SIGNAL(dateChanged(QDate)),  this, SLOT(_set(QDate)));
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

void cDateWidget::_set(QDate d)
{
    _setv(QVariant(d));
}

/* **************************************** cTimeWidget ****************************************  */

cTimeWidget::cTimeWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy, false, par)
{
    _wType = FEW_TIME;
    QTimeEdit * pTE = new QTimeEdit(par);
    _pWidget = pTE;
    connect(pTE, SIGNAL(TimeChanged(QTime)),  this, SLOT(_set(QTime)));
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

void cTimeWidget::_set(QTime d)
{
    _setv(QVariant(d));
}

/* **************************************** cDateTimeWidget ****************************************  */

cDateTimeWidget::cDateTimeWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy,false, par)
{
    _wType = FEW_DATE_TIME;
    QDateTimeEdit * pDTE = new QDateTimeEdit(par);
    _pWidget = pDTE;
    connect(pDTE, SIGNAL(dateTimeChanged(QDateTime)),  this, SLOT(_set(QDateTime)));
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

void cDateTimeWidget::_set(QDateTime d)
{
   _setv(QVariant(d));
}

/* **************************************** cIntervalWidget ****************************************  */


cIntervalWidget::cIntervalWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, bool _ro, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy, _ro, par)
{
    _wType = FEW_INTERVAL;
    _pWidget = new QWidget(par);
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
        connect(pLineEdDay, SIGNAL(editingFinished()),  this, SLOT(_set()));
        connect(pTimeEd,    SIGNAL(editingFinished()),  this, SLOT(_set()));
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
    qlonglong v = _descr.toId(_value);
    int msec   = v % 1000; v /= 1000;
    int sec    = v %   60; v /=   60;
    int minute = v %   60; v /=   60;
    int hour   = v %   24; v /=   24;
    QTime t(hour, minute, sec, msec);
    pTimeEd->setTime(t);
    pLineEdDay->setText(QString::number(v));
}

void cIntervalWidget::_set()
{
    _setv(getFromWideget());
}

/* **************************************** cBinaryWidget ****************************************  */

cBinaryWidget::cBinaryWidget(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy, bool _ro, QWidget * par)
    : cFieldEditBase(_tm, __fr, _sy, _ro, par)
    , data()
{
    _wType = FEW_BINARY;
    _pWidget = new QWidget(par);
    pLayout = new QHBoxLayout;
    _pWidget->setLayout(pLayout);
    bool z = _value.isNull();
    if (_readOnly) {
        pRadioButton = new QRadioButton(_sNULL, _pWidget);
        pLoadButton  = NULL;
        pLayout->addWidget(pRadioButton);
        pRadioButton->setChecked(z);
        pRadioButton->setCheckable(false);
    }
    else {
        pRadioButton = new QRadioButton(_sNULL, _pWidget);
        pLoadButton  = new QPushButton(trUtf8("Betölt"), _pWidget);
        pLayout->addWidget(pRadioButton);
        pLayout->addWidget(pLoadButton);
        connect(pRadioButton, SIGNAL(clicked(bool)), this, SLOT(setNull(bool)));
        connect(pLoadButton, SIGNAL(pressed()), this, SLOT(load()));
        pRadioButton->setChecked(z);
        pRadioButton->setCheckable(!z);
    }
    if (!z) data = _value.toByteArray();
}

cBinaryWidget::~cBinaryWidget()
{
    ;
}

int cBinaryWidget::set(const QVariant& v)
{
    bool r = cFieldEditBase::set(v);
    if (r == 1) {
        bool z = v.isNull();
        pRadioButton->setChecked(z);
        pRadioButton->setCheckable(!z);
        if (z) data.clear();
        else   data = _value.toByteArray();
    }
    return r;
}

void cBinaryWidget::load()
{
    QString fn = QFileDialog::getOpenFileName(pWidget());
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (f.isReadable() == false) {
        QMessageBox::warning(pWidget(), trUtf8("Hiba"), trUtf8("A megadott fájl nem olvasható"));
        return;
    }
    data = f.readAll();
    pRadioButton->setChecked(false);
    pRadioButton->setCheckable(true);
    _setv(QVariant(data));
}

void cBinaryWidget::setNull(bool checked)
{
    if (checked) data.clear();
    bool z = data.isEmpty();
    pRadioButton->setChecked(z);
    pRadioButton->setCheckable(!z);
    _setv(QVariant(data));
}

