#include "guidata.h"
#include "record_table.h"
#include "cerrormessagebox.h"


/* ***************************************************************************************************** */

cRecordTableFilter::cRecordTableFilter(cRecordTableFODialog &_par, cRecordTableColumn& _rtc)
    : QObject(&_par)
    , field(_rtc)
    , dialog(_par)
    , param1(), param2()
    , typeList()
{
    closed1 = closed2 = true;
    tTableShapeFilters& filters = field.shapeField.shapeFilters;
    tTableShapeFilters::const_iterator i, e = filters.constEnd();
    types = 0;
    typeList << trUtf8("Nincs szűrés");
    for (i = filters.constBegin(); i != e; ++i) {
        const cTableShapeFilter& filt = **i;
        types |= enum2set(filt.getId(_sFilterType));
        typeList << filt.getName(_sFilterType);
    }
    pFilter = NULL;
    iFilter = -1;
}

cRecordTableFilter::~cRecordTableFilter()
{
    ;
}

void cRecordTableFilter::setFilter(int i)
{
    if (i < 0) {
        pFilter = NULL;
        iFilter = -1;
        param1.clear();
        param2.clear();
        closed1 = closed2 = true;
    }
    else {
        if (iFilter != i) {
            param1.clear();
            param2.clear();
            closed1 = closed2 = true;
        }
        tTableShapeFilters& filters = field.shapeField.shapeFilters;
        if (isContIx(filters, i)) EXCEPTION(EDATA);
        pFilter = filters[i];
        iFilter = i;
    }
}

int cRecordTableFilter::fieldType()
{
    return field.colDescr.eColType;
}

QString cRecordTableFilter::where(QVariantList& qparams)
{
    QString r;
    QString c = field.colDescr.colNameQ();
    if (pFilter != NULL) {
        switch (pFilter->getId(_sFilterType)) {
        case FT_BEGIN:
            r = c + " LIKE ?";
            qparams << QVariant(param1.toString() + "%");
            break;
        case FT_LIKE:
            r = c + " LIKE ?";
            qparams << param1;
            break;
        case FT_SIMILAR:
            r = c + " SIMILAR ?";
            qparams << param1;
            break;
        case FT_REGEXP:
            r = c + " ~ ?";
            qparams << param1;
            break;
        case FT_REGEXPI:
            r = c + " ~* ?";
            qparams << param1;
            break;
        case FT_BIG:
            r = c + (closed1 ? " >= ?" : " > ?");
            qparams << param1;
            break;
        case FT_LITLE:
            r = c + (closed2 ? " <= ?" : " < ?");
            qparams << param2;
            break;
        case FT_INTERVAL:
            r = c + (closed1 ? " >= ?" : " > ?") + " AND " + c + (closed2 ? " <= ?" : " < ?");
            qparams << param1 << param2;
            break;
        case FT_PROC:
            r = param1.toString() + _sABraB + c + _sABraE;
            break;
        case FT_SQL_WHERE:
            r = param1.toString();
            break;
        default:
            EXCEPTION(EPROGFAIL);
        }
    }
    return r;
}

/* ***************************************************************************************************** */

cRecordTableOrd::cRecordTableOrd(cRecordTableFODialog &par,cRecordTableColumn& _rtc, int types)
    : QObject(&par)
    , field(_rtc)
{
    sequence_number = (int)field.shapeField.getId(_sOrdInitSequenceNumber);
    pRowName= new QLineEdit(&par);
    pType   = new QComboBox(&par);
    pUp     = new QPushButton(QIcon::fromTheme("go-up"), pUp->trUtf8("Fel"), &par);
    pDown   = new QPushButton(QIcon::fromTheme("go-down"), pUp->trUtf8("Le"), &par);
    pRowName->setText(field.header.toString());
    pRowName->setEnabled(false);
    types |= enum2set(OT_NO);   // Ha esetleg a nincs rendezés nem lenne benne a set-ben
    for (int i = 0; enum2set(i) <= types ; ++i) {
        if (enum2set(i) & types) pType->addItem(orderType(i));
    }
    QString s = "no";
    if (field.shapeField.isNull(_sOrdInitType) == false) s = field.shapeField.getName(_sOrdInitType);
    pType->setCurrentText(s);
    pType->setAutoCompletion(true);
    connect(pUp,   SIGNAL(clicked()), this, SLOT(up()));
    connect(pDown, SIGNAL(clicked()), this, SLOT(down()));
}

cRecordTableOrd::~cRecordTableOrd()
{

}

QString cRecordTableOrd::ord()
{
    QString r;
    act = orderType(pType->currentText(), false);
    switch (act) {
    case OT_ASC:    r = " ASC ";    break;
    case OT_DESC:   r = " DESC ";   break;
    case OT_NO:     return r;
    default:        EXCEPTION(EPROGFAIL);
    }
    const cColStaticDescr& colDescr =  field.colDescr;
    if (colDescr.fKeyType == cColStaticDescr::FT_NONE) r = colDescr.colNameQ() + r;
    else {
        if (colDescr.fnToName.isEmpty()) EXCEPTION(EDATA, -1, QObject::trUtf8("Az ID->név konverziós függvény nincs definiálva."));
        r = colDescr.fnToName + _sABraB + colDescr.colNameQ() + _sABraE + r;
    }
    return r;
}

void cRecordTableOrd::up()
{
    moveUp(this);
}

void cRecordTableOrd::down()
{
    moveDown(this);
}

/* ***************************************************************************************************** */

cRecordTableFODialog::cRecordTableFODialog(QSqlQuery *pq, cRecordTable& _rt)
    : QDialog(_rt.pWidget())
    , recordTable(_rt)
    , filters(), ords()
{
    pForm = new Ui::dialogTabFiltOrd();
    pForm->setupUi(this);

    tRecordTableColumns::ConstIterator i, n = recordTable.fields.cend();
    for (i = recordTable.fields.cbegin(); i != n; ++i) {
        cTableShapeField& tsf = (*i)->shapeField;
        int ord = tsf.getId(_sOrdTypes);
        if (ord & ~enum2set(OT_NO)) {
            cRecordTableOrd *pOrd = new cRecordTableOrd(*this, **i, ord);
            ords << pOrd;
            connect(pOrd, SIGNAL(moveUp(cRecordTableOrd*)),   this, SLOT(ordMoveUp(cRecordTableOrd*)));
            connect(pOrd, SIGNAL(moveDown(cRecordTableOrd*)), this, SLOT(ordMoveDown(cRecordTableOrd*)));
        }
        if (tsf.fetchFilters(*pq)) {
            cRecordTableFilter *pFilt = new cRecordTableFilter(*this, **i);
            filters << pFilt;
            pForm->comboBox_colName->addItem(tsf.getName(_sTableShapeFieldName));
        }
    }
    connect(pForm->pushButton_OK, SIGNAL(clicked()), this, SLOT(clickOk()));
    connect(pForm->pushButton_Default, SIGNAL(clicked()), this, SLOT(clickDefault()));
    qSort(ords.begin(), ords.end(), PtrLess<cRecordTableOrd>());
    setGridLayoutOrder();
    iSelFilterCol = -1;
    iSelFilterType = -1;
    if (!filters.isEmpty()) {
        pSelFilter = filters.first();
        pForm->comboBox_colName->setCurrentIndex(0);
        pForm->comboBox_FiltType->addItems(pSelFilter->typeList);
        pForm->comboBox_FiltType->setCurrentIndex(0);
        connect(pForm->comboBox_colName,    SIGNAL(currentIndexChanged(int)),   this, SLOT(filtCol(int)));
        connect(pForm->comboBox_FiltType,   SIGNAL(currentIndexChanged(int)),   this, SLOT(filtType(int)));

        connect(pForm->lineEdit_FiltParam,  SIGNAL(textChanged(QString)),       this, SLOT(changeParam(QString)));
        connect(pForm->textEdit_FiltParam,  SIGNAL(textChanged()),              this, SLOT(changeParam()));
        connect(pForm->spinBox_int1,        SIGNAL(valueChanged(int)),          this, SLOT(changeParam1(int)));
        connect(pForm->spinBox_int2,        SIGNAL(valueChanged(int)),          this, SLOT(changeParam2(int)));
        connect(pForm->doubleSpinBox_intF1, SIGNAL(valueChanged(double)),       this, SLOT(changeParamF1(double)));
        connect(pForm->doubleSpinBox_intF2, SIGNAL(valueChanged(double)),       this, SLOT(changeParamF2(double)));
        connect(pForm->dateTimeEdit_DT1,    SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(changeParamDT1(QDateTime)));
        connect(pForm->dateTimeEdit_DT2,    SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(changeParamDT2(QDateTime)));

        connect(pForm->checkBox_close1,     SIGNAL(toggled(bool)),              this, SLOT(changeClosed1(bool)));
        connect(pForm->checkBox_close2,     SIGNAL(toggled(bool)),              this, SLOT(changeClosed2(bool)));
        connect(pForm->checkBox_closeF1,    SIGNAL(toggled(bool)),              this, SLOT(changeClosed1(bool)));
        connect(pForm->checkBox_closeF2,    SIGNAL(toggled(bool)),              this, SLOT(changeClosed2(bool)));
        connect(pForm->checkBox_closeDT1,   SIGNAL(toggled(bool)),              this, SLOT(changeClosed1(bool)));
        connect(pForm->checkBox_closeDT2,   SIGNAL(toggled(bool)),              this, SLOT(changeClosed2(bool)));
    }
    else {
        pForm->label_noFilter->setText(trUtf8("Nincs lehetőség szűrő feltétel megadására."));
    }
    setFilterDialog();
    pForm->pushButton_Default->setDisabled(true);   // Nincs implementálva !
}

cRecordTableFODialog::~cRecordTableFODialog()
{

}

QString cRecordTableFODialog::where(QVariantList& qparams)
{
    QString r;
    if (!filters.isEmpty()) foreach (cRecordTableFilter *pF, filters) {
        QString rw = pF->where(qparams);
        if (!rw.isEmpty()) {
            if (r.isEmpty()) r = rw;
            else r += " AND " + rw;
        }
    }
    return r;
}

QString cRecordTableFODialog::ord()
{
    QString r;
    if (!ords.isEmpty()) foreach (cRecordTableOrd *pOrd, ords) {
        QString ro = pOrd->ord();
        if (!ro.isEmpty()) {
            if (r.isEmpty()) r = ro;
            else r += _sCommaSp + ro;
        }
    }
    return r;
}

int cRecordTableFODialog::indexOf(cRecordTableOrd * _po)
{
    int i, n = ords.size();
    for (i = 0; i < n; ++i) {
        if (_po == ords[i]) return i;
    }
    EXCEPTION(EPROGFAIL);
    return -1;
}

void cRecordTableFODialog::setGridLayoutOrder()
{
    if (ords.isEmpty()) return;
    int row = 0;
    foreach (cRecordTableOrd *pOrd, ords) {
        pForm->gridLayout_Order->addWidget(pOrd->pRowName, row, 0);
        pForm->gridLayout_Order->addWidget(pOrd->pType,    row, 1);
        pForm->gridLayout_Order->addWidget(pOrd->pUp,      row, 2);
        pForm->gridLayout_Order->addWidget(pOrd->pDown,    row, 3);
        pOrd->disableUp(row == 0);
        ++row;
    }
    ords.last()->disableDown(true);
}
void cRecordTableFODialog::setFilterDialog()
{
    if (iSelFilterCol < 0) {
        iSelFilterType = -1;
        pSelFilter = NULL;
        pForm->comboBox_FiltType->setDisabled(true);
        pForm->lineEdit_colDescr->setText(_sNul);
    }
    else {
        pForm->lineEdit_colDescr->setText(filter().field.shapeField.getName(_sTableShapeFieldTitle));
        pForm->comboBox_FiltType->setDisabled(false);
    }
    if (iSelFilterType < 0) {
        ;
        if (iSelFilterCol >= 0) filter().setFilter(iSelFilterType);
        pForm->stackedWidget->setCurrentWidget(pForm->page_FilterNo);
        pForm->lineEdit_typeTitle->setText(_sNul);
    }
    else {
        filter().setFilter(iSelFilterType);
        int fType = filter().shapeFilter().getId(_sFilterType);
        switch (fType) {
        case FT_BEGIN:
        case FT_LIKE:
        case FT_SIMILAR:
        case FT_REGEXP:
        case FT_REGEXPI:
        case FT_PROC:
            pForm->stackedWidget->setCurrentWidget(pForm->page_FilterLine);
            pForm->lineEdit_FiltParam->setText(filter().param1.toString());
            break;
        case FT_BIG:
        case FT_LITLE:
        case FT_INTERVAL:
            switch (filter().fieldType()) {
            case cColStaticDescr::FT_INTEGER:
                pForm->stackedWidget->setCurrentWidget(pForm->page_interval);
                pForm->spinBox_int1->setDisabled(fType == FT_LITLE);
                pForm->spinBox_int2->setDisabled(fType == FT_BIG);
                pForm->checkBox_close1->setDisabled(fType == FT_LITLE);
                pForm->checkBox_close2->setDisabled(fType == FT_BIG);

                pForm->spinBox_int1->setValue(filter().param1.toInt());
                pForm->spinBox_int2->setValue(filter().param2.toInt());
                pForm->checkBox_close1->setChecked(filter().closed1);
                pForm->checkBox_close2->setChecked(filter().closed2);
                break;
            case cColStaticDescr::FT_REAL:
                pForm->stackedWidget->setCurrentWidget(pForm->page_intervalF);
                pForm->doubleSpinBox_intF1->setDisabled(fType == FT_LITLE);
                pForm->doubleSpinBox_intF2->setDisabled(fType == FT_BIG);
                pForm->checkBox_closeF1->setDisabled(fType == FT_LITLE);
                pForm->checkBox_closeF2->setDisabled(fType == FT_BIG);

                pForm->doubleSpinBox_intF1->setValue(filter().param1.toInt());
                pForm->doubleSpinBox_intF2->setValue(filter().param2.toInt());
                pForm->checkBox_closeF1->setChecked(filter().closed1);
                pForm->checkBox_closeF2->setChecked(filter().closed2);
                break;
            case cColStaticDescr::FT_DATE_TIME:
                pForm->stackedWidget->setCurrentWidget(pForm->page_intervalDT);
                pForm->dateTimeEdit_DT1->setDisabled(fType == FT_LITLE);
                pForm->dateTimeEdit_DT2->setDisabled(fType == FT_BIG);
                pForm->checkBox_closeDT1->setDisabled(fType == FT_LITLE);
                pForm->checkBox_closeDT2->setDisabled(fType == FT_BIG);

                if (filter().param1.isNull()) filter().param1 = QDateTime(QDate(QDate::currentDate().year(), 1, 1), QTime(0,0,0));
                if (filter().param2.isNull()) filter().param2 = QDateTime(QDate::currentDate(), QTime(23, 59, 59, 999));
                pForm->dateTimeEdit_DT1->setDateTime(filter().param1.toDateTime());
                pForm->dateTimeEdit_DT2->setDateTime(filter().param2.toDateTime());
                pForm->checkBox_closeDT1->setChecked(filter().closed1);
                pForm->checkBox_closeDT2->setChecked(filter().closed2);
                break;
            default:
                EXCEPTION(EDATA);
            }
            break;
        case FT_SQL_WHERE:
            pForm->stackedWidget->setCurrentWidget(pForm->page_FilterText);
            pForm->textEdit_FiltParam->setPlainText(filter().param1.toString());
            break;
        }
        QString title = filter().shapeFilter().getName(_sTableShapeFilterNote);
        if (title.isEmpty()) title = cEnumVal::title(*recordTable.pq, filter().shapeFilter().getName(_sFilterType), "filtertype");
        pForm->lineEdit_typeTitle->setText(title);
    }
}

void cRecordTableFODialog::clickOk()
{
    done(DBT_OK);
}

void cRecordTableFODialog::clickDefault()
{
    ;
}

void cRecordTableFODialog::ordMoveUp(cRecordTableOrd * _po)
{
    int i = indexOf(_po);
    if (i == 0) EXCEPTION(EPROGFAIL);
    ords.swap(i, i -1);
    setGridLayoutOrder();
}

void cRecordTableFODialog::ordMoveDown(cRecordTableOrd * _po)
{
    int i = indexOf(_po);
    if (i >= (ords.size() -1)) EXCEPTION(EPROGFAIL);
    ords.swap(i, i +1);
    setGridLayoutOrder();

}

void cRecordTableFODialog::filtCol(int _c)
{
    if (_c == 0) {
        iSelFilterCol = iSelFilterType = -1;
        pSelFilter = NULL;
        pForm->comboBox_FiltType->setCurrentIndex(0);
    }
    else {
        iSelFilterCol = _c -1;
        if (!isContIx(filters, iSelFilterCol)) EXCEPTION(EDATA, iSelFilterCol);
        pSelFilter = filters[iSelFilterCol];
        if (filter().iFilter < 0) {
            iSelFilterType = -1;
            pForm->comboBox_FiltType->setCurrentIndex(0);
        }
        else {
            iSelFilterType = filter().iFilter;
            pForm->comboBox_FiltType->setCurrentIndex(iSelFilterType +1);
        }
    }
}

void cRecordTableFODialog::filtType(int _t)
{
    iSelFilterType = _t -1;
    setFilterDialog();
}

void cRecordTableFODialog::changeParam(QString t)   { filter().param1 = t; }
void cRecordTableFODialog::changeParam()            { filter().param1 = pForm->textEdit_FiltParam->toPlainText(); }
void cRecordTableFODialog::changeParam1(int i)      { filter().param1 = i; }
void cRecordTableFODialog::changeParam2(int i)      { filter().param2 = i; }
void cRecordTableFODialog::changeParamF1(double d)  { filter().param1 = d; }
void cRecordTableFODialog::changeParamF2(double d)  { filter().param2 = d; }
void cRecordTableFODialog::changeParamDT1(QDateTime dt) { filter().param1 = dt; }
void cRecordTableFODialog::changeParamDT2(QDateTime dt) { filter().param2 = dt; }
void cRecordTableFODialog::changeClosed1(bool f)    { filter().closed1 = f; }
void cRecordTableFODialog::changeClosed2(bool f)    { filter().closed2 = f; }

/* ***************************************************************************************************** */

cRecordTableColumn::cRecordTableColumn(cTableShapeField &sf, cRecordTable& table)
    : shapeField(sf)
    , recDescr(table.recDescr())
    , fieldIndex(recDescr.toIndex(shapeField.getName()))
    , colDescr(recDescr.colDescr(fieldIndex))
    , header(shapeField.get(_sTableShapeFieldTitle))
    , fgHeadColor(cRecordTable::defPalette().color(QPalette::Text)) // ??
    , bgHeadColor(cRecordTable::defPalette().color(QPalette::AlternateBase))
    , headFont()
    , fgDataColor(cRecordTable::defPalette().color(QPalette::Text))
    , bgDataColor(cRecordTable::defPalette().color(QPalette::Base))
    , fgExDataColor(cRecordTable::defPalette().color(QPalette::Link))
    , bgExDataColor(cRecordTable::defPalette().color(QPalette::Base))
    , dataFont()
    , fgNullColor(QString("lightcoral"))
    , nullFont()
{
    headAlign = Qt::AlignVCenter | Qt::AlignHCenter;
    dataAlign = Qt::AlignVCenter;
    if (colDescr.eColType == cColStaticDescr::FT_INTEGER && colDescr.fKeyType == cColStaticDescr::FT_NONE) dataAlign |= Qt::AlignRight;
    else if (colDescr.eColType == cColStaticDescr::FT_REAL)                                                dataAlign |= Qt::AlignRight;
    else                                                                                                   dataAlign |= Qt::AlignLeft;
    nullFont.setItalic(true);
    if (recDescr.primaryKey()[fieldIndex]) {
        dataFont.setBold(true);
        headFont.setBold(true);
    }
    if (colDescr.fKeyType != cColStaticDescr::FT_NONE) {
        dataFont.setItalic(true);
        headFont.setItalic(true);
    }
    if (recDescr.nameIndex(false) == fieldIndex) {
        fgDataColor.setNamedColor("seagreen");
        fgHeadColor.setNamedColor("seagreen");
        dataFont.setBold(true);
        headFont.setBold(true);
    }
}

/* ***************************************************************************************************** */

QPalette *cRecordTable::pDefaultPalette = NULL;

cRecordTable::cRecordTable(const QString& _mn, bool _isDialog, QWidget * par)
    : QObject(par)
    , isDialog(_isDialog)
    , viewName()
{
    pMaster = pUpper = NULL;
    pq = newQuery();
    pTabQuery = newQuery();
    pTableShape = new cTableShape();
    pTableShape->setParent(this);
    if (!pTableShape->fetchByName(*pq, _mn) || 0 == pTableShape->fetchFields(*pq)) EXCEPTION(EDATA,-1,_mn);
    init(par);
}

cRecordTable::cRecordTable(qlonglong id, bool _isDialog, QWidget * par, cRecordTable * _upper)
    : QObject(par)
    , isDialog(_isDialog)
    , viewName()
{
    pMaster = pUpper = _upper;
    if (pMaster != NULL && pMaster->pMaster != NULL) pMaster = pMaster->pMaster;

    pq = newQuery();
    pTabQuery = newQuery();
    pTableShape = new cTableShape();
    pTableShape->setParent(this);
    if (!pTableShape->fetchById(*pq, id) || 0 == pTableShape->fetchFields(*pq)) EXCEPTION(EDATA, id);
    init(par);
}

cRecordTable::cRecordTable(cTableShape *pts, bool _isDialog, QWidget * par)
    : QObject(par)
    , isDialog(_isDialog)
    , viewName()
{
    pMaster = pUpper = NULL;
    pq = newQuery();
    pTabQuery = newQuery();
    pTableShape = pts;
    if (pTableShape->shapeFields.isEmpty() && 0 == pTableShape->fetchFields(*pq)) EXCEPTION(EDATA, pTableShape->getId(), pTableShape->getName());
    init(par);
}

cRecordTable::~cRecordTable()
{
    delete pq;
    delete pTabQuery;
}

void cRecordTable::init(QWidget *par)
{
    flags = 0;
    pRecDsecr = NULL;
    pTableModel = NULL;
    _pWidget = NULL;
    pButtons = NULL;
    pMainLayer = NULL;
    pTableView = NULL;
    // pMasterLayout = NULL;
    pMasterFrame = NULL;
    pLeftWidget = NULL;
    pRightTable = NULL;
    pFODialog   = NULL;
    owner_id = NULL_ID;
    pInhRecDescr = NULL;
    if (pDefaultPalette == NULL) {
        pDefaultPalette = new QPalette();
    }
    pRecDsecr = cRecStaticDescr::get(pTableShape->getName(_sTableName));
    isReadOnly = pTableShape->getBool(_sIsReadOnly);

    tTableShapeFields::iterator i, n = pTableShape->shapeFields.end();
    for (i = pTableShape->shapeFields.begin(); i != n; ++i) {
        cRecordTableColumn *p = new cRecordTableColumn(**i, *this);
        fields << p;
    }

    initView();

    _pWidget = isDialog ? new QDialog(par) : new QWidget(par);

    switch (pTableShape->getId(_sTableShapeType)) {
    case 0:
    case ENUM2SET(TS_SIMPLE):
        flags = RTF_SINGLE;
        initSimple(_pWidget);
        break;
    case ENUM2SET(TS_TREE):
        flags = RTF_SINGLE;
        initTree();
        break;
    case ENUM2SET(TS_OWNER):
    case ENUM2SET2(TS_OWNER, TS_SIMPLE):
        flags = RTF_MASTER | RTF_OVNER;
        initOwner();
        break;
    case ENUM2SET(TS_CHILD):
        if (pUpper == NULL) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_CHILD;
        initSimple(_pWidget);
        break;
    case ENUM2SET2(TS_OWNER, TS_CHILD):
        flags = RTF_OVNER | RTF_SLAVE | RTF_CHILD;
        initSimple(_pWidget);
        pRightTable = new cRecordTable(pTableShape->getId(_sRightShapeId), false, _pWidget, this);
        break;
    case ENUM2SET(TS_SWITCH):
    default:
        EXCEPTION(ENOTSUPP);
    }
    pFODialog = new cRecordTableFODialog(pq, *this);
    pTableView->horizontalHeader()->setSectionsClickable(true);
    connect(pTableView->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(clickedHeader(int)));
    refresh();
}

void cRecordTable::initSimple(QWidget * pW)
{
    int buttons, buttons2 = 0;
    int closeBut = 0;
    if (!(flags & RTF_SLAVE)) closeBut = enum2set(DBT_CLOSE);
    if (flags & RTF_SINGLE || isReadOnly) {
        buttons = closeBut | enum2set(DBT_REFRESH, DBT_FIRST, DBT_PREV, DBT_NEXT, DBT_LAST);
        if (isReadOnly == false) {
            buttons |= enum2set(DBT_DELETE, DBT_INSERT, DBT_MODIFY);
        }
    }
    else {
        buttons  = enum2set(DBT_FIRST, DBT_PREV, DBT_NEXT, DBT_LAST);
        buttons2 = closeBut | enum2set(DBT_DELETE, DBT_INSERT, DBT_MODIFY, DBT_REFRESH);
    }
    pButtons    = new cDialogButtons(buttons, buttons2, pW);
    pMainLayer  = new QVBoxLayout(pW);
    pTableView  = new QTableView(pW);
    pTableModel = new cRecordTableModelSql(*this);
    pMainLayer->addWidget(pTableView);
    pMainLayer->addWidget(pButtons->pWidget());
    pTableView->setModel(pTableModel);

    pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(pButtons,    SIGNAL(buttonPressed(int)),   this, SLOT(buttonPressed(int)));
    if (!isReadOnly) {
        connect(pTableModel, SIGNAL(removed(cAlternate*)), this, SLOT(recordRemove(cAlternate*)), Qt::DirectConnection);
        connect(pTableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    }
    if (pMaster != NULL) {
        pMaster->pMasterFrame->addWidget(_pWidget);
    }
}

void cRecordTable::initTree()
{
    EXCEPTION(ENOTSUPP);
}

void cRecordTable::initOwner()
{
    pLeftWidget   = new QWidget(_pWidget);
    initSimple(pLeftWidget);

    pMasterLayout = new QHBoxLayout(_pWidget);
    pMasterFrame = new QSplitter(Qt::Horizontal, _pWidget);
    pMasterLayout->addWidget(pMasterFrame);
    pMasterFrame->addWidget(pLeftWidget);
    pRightTable = new cRecordTable(pTableShape->getId(_sRightShapeId), false, _pWidget, this);
}

QDialog& cRecordTable::dialog()
{
    if (!isDialog) EXCEPTION(EPROGFAIL);
    return *dynamic_cast<QDialog *>(_pWidget);
}

void cRecordTable::empty()
{
    pTableModel->clear();
}

void cRecordTable::close()
{
    if (isDialog) dynamic_cast<QDialog *>(_pWidget)->accept();
    else closeIt();
}

void cRecordTable::first()
{
    pTableModel->qFirst();
    setButtons();
}

void cRecordTable::next()
{
    pTableModel->qNext();
    setButtons();
}

void cRecordTable::prev()
{
    pTableModel->qPrev();
    setButtons();
}

void cRecordTable::last()
{
    pTableModel->qLast();
    setButtons();
}

void cRecordTable::refresh(bool first)
{
    DBGFN();
    switch (tit) {
    case TIT_NO:
    case TIT_ON:
    case TIT_ONLY:
    case TIT_LISTED_REV:
        _refresh(first);
        break;
    default:
        EXCEPTION(ENOTSUPP);
        break;
    }
    setButtons();
    if (pRightTable != NULL) pRightTable->refresh();
    DBGFNL();
}

QString cRecordTable::where(QVariantList& qParams)
{
    QString w;
    if (flags & RTF_CHILD) {     // A tulaj ID-jére szűrünk, ha van
        if (owner_id == NULL_ID) {
            return _sFalse;
        }
        int ofix = recDescr().ixToOwner();
        w = recDescr().columnName(ofix) + " = " + QString::number(owner_id);
    }
    // ... egyenlöre további szűrések nélkül
    (void)qParams;
    return w;
}

void cRecordTable::_refresh(bool first)
{
    QString sql = "SELECT * FROM " + viewName;
    QVariantList qParams;
    QString w = where(qParams);
    if (w == _sFalse) {
        pTableModel->clear();
        return;
    }
    if (!w.isEmpty()) sql += " WHERE " + w;

    QString ord = pFODialog->ord();
    if (!ord.isEmpty()) sql += " ORDER BY " + ord;

    if (!pTabQuery->prepare(sql)) SQLPREPERR(*pTabQuery, sql);
    int i = 0;
    foreach (QVariant v, qParams) pTabQuery->bindValue(i++, v);
    if (!pTabQuery->exec()) SQLQUERYERR(*pTabQuery);
    pTableModel->setRecords(*pTabQuery, first);
}

void cRecordTable::remove()
{
    DBGFN();
    QModelIndexList mil = pTableView->selectionModel()->selectedRows();
    pTableModel->remove(mil);
    refresh();
}

void cRecordTable::insert()
{
    if (flags & RTF_CHILD) {
        if (pUpper == NULL) EXCEPTION(EPROGFAIL);
        if (owner_id == NULL_ID) return;
    }
    int buttons = enum2set(DBT_OK, DBT_INSERT, DBT_CANCEL);
    cAlternate rec;
    switch (tit) {
    case TIT_NO:
    case TIT_ONLY: {
        rec.setType(pRecDsecr);
        if (flags & RTF_CHILD) {
            int ofix = rec.descr().ixToOwner();
            rec[ofix] = owner_id;
        }
        cRecordDialog   rd(rec, *pTableShape, buttons, pWidget());
        while (1) {
            int r = rd.exec();
            if (r == DBT_INSERT || r == DBT_OK) {
                int ok;
                cError *pe = NULL;
                try {
                    bool b;
                    b = rd.get(rec);
                    b = b && rec.insert(*pq);
                    ok =  b ? 1 : 0;
                } CATCHS(pe)
                if (pe != NULL) ok = -1;
                switch (ok) {
                case 1:                     // OK
                    refresh();
                    if (r == DBT_OK) break;
                    continue;
                case 0:
                    QMessageBox::warning(pWidget(), trUtf8("Az új rekord beszúrása sikertelen."), rd.errMsg());
                    continue;
                case -1:
                    cErrorMessageBox::messageBox(pe);
                    pDelete(pe);
                    continue;
                }
            }
            break;
        }
    }   break;
    case TIT_LISTED_REV: {
        tRecordList<cTableShape>    shapes;
        QStringList tableNames;
        tableNames << pTableShape->getName(_sTableName);
        tableNames << inheritTableList;
        foreach (QString tableName, tableNames) {
            shapes << getInhShape(tableName);
        }
        cRecordDialogTab rd(*pTableShape, shapes, buttons, owner_id, true, pWidget());
        while (1) {
            int r = rd.exec();
            if (r == DBT_INSERT || r == DBT_OK) {
                int ok;
                cError *pe = NULL;
                try {
                    rec.setType(&rd.actType());
                    ok = rd.get(rec) && rec.insert(*pq) ? 1 : 0;
                } CATCHS(pe)
                if (pe != NULL) ok = -1;
                switch (ok) {
                case 1:                     // OK
                    refresh();
                    if (r == DBT_OK) break;
                    continue;
                case 0:
                    QMessageBox::warning(pWidget(), trUtf8("Az új rekord beszúrása sikertelen."), rd.errMsg());
                    continue;
                case -1:
                    cErrorMessageBox::messageBox(pe);
                    pDelete(pe);
                    continue;
                }
            }
            break;
        }
    }   break;
    default:
        EXCEPTION(ENOTSUPP);
    }
}

int cRecordTable::actRow()
{
    QModelIndexList mil = pTableView->selectionModel()->selectedRows();
    int row;
    if (mil.size() != 1 || (row = mil[0].row()) >= pTableModel->records().size()) {
        DERR() << endl;
        return -1;
    }
    return row;
}

cAlternate *cRecordTable::actRecord()
{
    int row = actRow();
    if (row < 0) return NULL;
    return pTableModel->records()[row];
}

void cRecordTable::modify()
{
    int row = actRow();
    if (row < 0) return;
    cAlternate *pRec = (cAlternate *)actRecord()->dup();
    int buttons = enum2set(DBT_OK, DBT_CANCEL, DBT_NEXT, DBT_PREV);
    cRecordDialog *    pRd  = NULL;
    cRecordDialogTab * pRdt = NULL;
    cTableShape *    pShape = NULL;
    tRecordList<cTableShape> * pShapes = NULL;

    switch (tit) {
    case TIT_NO:
    case TIT_ONLY:
        pShape = getInhShape(pRec->descr());
        pRd = new cRecordDialog(*pRec, *pShape, buttons, pWidget());
        break;
    case TIT_LISTED_REV: {
        QStringList tableNames;
        pShapes = new tRecordList<cTableShape>;
        tableNames << pTableShape->getName(_sTableName);
        tableNames << inheritTableList;
        foreach (QString tableName, tableNames) {
            cTableShape *pTS = getInhShape(tableName);;
            *pShapes << pTS;
        }
        pRdt = new cRecordDialogTab(*pTableShape, *pShapes, buttons, owner_id, true, pWidget());
    }   break;
    default:
        EXCEPTION(ENOTSUPP);
    }

    int id = DBT_NEXT;
    while (1) {
        if (pRd != NULL) {
            pRd->set(*pRec);
            id = pRd->exec(false);
        }
        else if (pRdt != NULL) {
            int i, n = pShapes->size();
            bool e = false;
            for (i = 0; i < n; i++) {   // Csak egy tabot engedélyezünk !
                bool f = pRec->tableName() == pShapes->at(i)->getName(_sTableName);
                pRdt->setTabEnabled(i, f);
                if (e && f) EXCEPTION(EPROGFAIL);   // Ha több tabot kéne az gáz
                if (f) {
                    e = f;
                    pRdt->setActTab(i);
                    pRdt->set(*pRec);
                }
            }
            if (!e) EXCEPTION(EPROGFAIL);   // Ha egyet sem az is gáz
            id = pRdt->exec(false);
        }
        switch(id) {
        case DBT_OK:
        case DBT_NEXT:
        case DBT_PREV: {
            // Update DB
            bool r = pRd != NULL ? pRd->get( *pRec) : pRdt->get(*pRec);
            if (!r) {
                QMessageBox::warning(pWidget(), trUtf8("Adat hiba"), trUtf8("Nem megfelelő adatok megadása, nincs módosítás."));
                continue;
            }
            else {
                PDEB(VERBOSE) << "Update record : " << pRec->toString() << endl;
                pRec->update(*pq, true);
                PDEB(VVERBOSE) << "Update returned : " << pRec->toString() << endl;
                if (!pTableModel->update(row, pRec)) {  // Ha Ok ő szabadítja fel pRec-et
                    DERR() << "pTableModel->update(" << row << ", " << pRec->toString() << ") hívás sikertelen !" << endl;
                    refresh();
                }
                else {
                    if (flags & RTF_OVNER) {
                        if (pRightTable == NULL) EXCEPTION(EPROGFAIL);
                        pRightTable->owner_id = pRec->getId();
                        pRightTable->refresh();
                    }
                    // Nincs felszabadítva, de már nem a mienk
                    pRec = NULL;
                }
            }
            pDelete(pRec);
            if (id == DBT_OK) break;    // OK: vége
            if (id == DBT_NEXT) {       // A következő szerkesztése
                ++row;
                if (row >= pTableModel->size()) {
                    if (!pTableModel->qNextResult()) break; // Az utolsó volt, nincs következő, kész
                    pTableModel->qNext();   // Lapozunk
                    row = 0;
                }
            }
            else {                      // Az előző szerkesztése
                --row;
                if (row < 0) {
                    if (!pTableModel->qPrevResult()) break; // Az első volt , nincs előző, kész
                    pTableModel->qPrev();   // Lapozunk
                    row = pTableModel->size() -1;
                }
            }
            if (!isContIx(*pTableModel, row)) break;
            pRec = (cAlternate *)pTableModel->records()[row]->dup();
            continue;
        }
        case DBT_CANCEL:
            break;
        default:
            EXCEPTION(EPROGFAIL);
            break;
        }
        break;
    }
    pDelete(pShape);
    pDelete(pShapes);
    pDelete(pRd);
    pDelete(pRdt);
    pDelete(pRec);
}

void cRecordTable::setEditButtons()
{
    if (!isReadOnly) {
        QModelIndexList mix = pTableView->selectionModel()->selectedRows();
        int n = mix.size();
        bool ext = false;
        if (n > 0 && pTableModel->extLines.size() > 0) foreach (QModelIndex mi, mix) {
            if (pTableModel->isExtRow(mi.row())) {
                ext = true;
                break;
            }
        }
        // PDEB(VVERBOSE) << setButtons() << " : " << VDEB(n) << VDEBBOOL(ext) << endl;
        buttonDisable(DBT_DELETE, ext || n <  1);
        buttonDisable(DBT_MODIFY, ext || n != 1);
    }
}

void cRecordTable::setPageButtons()
{
    if (pTableModel->qIsPaged()) {
        bool    next = pTableModel->qNextResult();
        bool    prev = pTableModel->qPrevResult();
        buttonDisable(DBT_FIRST,!prev);
        buttonDisable(DBT_PREV, !prev);
        buttonDisable(DBT_NEXT, !next);
        buttonDisable(DBT_LAST, !next);
    }
    else {
        buttonDisable(DBT_FIRST,true);
        buttonDisable(DBT_PREV, true);
        buttonDisable(DBT_NEXT, true);
        buttonDisable(DBT_LAST, true);
    }
}

void cRecordTable::setButtons()
{
    setEditButtons();
    setPageButtons();
}

void cRecordTable::buttonDisable(int id, bool d)
{
    QAbstractButton * p = pButtons->button(id);
    if (p == NULL) EXCEPTION(EDATA, id);
    p->setDisabled(d);
}

void cRecordTable::initView()
{
    inheritTableList = pTableShape->get(_sInheritTableNames).toStringList();
    tit = (eTableInheritType)pTableShape->getId(_sTableInheritType);
    bool only = tit == TIT_ONLY || !inheritTableList.isEmpty();
    QString schema = pTableShape->getName(_sSchemaName);
    viewName = pTableShape->getName() + "_view";
    QString sql = "CREATE OR REPLACE TEMP VIEW " + viewName + " AS ";
    sql += "SELECT tableoid, * FROM ";
    if (only) sql += "ONLY ";
    sql += recDescr().fullTableNameQ();
    if (! inheritTableList.isEmpty()) {
        pInhRecDescr = new QMap<qlonglong,const cRecStaticDescr *>;
        foreach (QString tn, inheritTableList) {
            const cRecStaticDescr * p = cRecStaticDescr::get(tn, schema);
            pInhRecDescr->insert(p->tableoid(), p);
            sql += "\nUNION\n SELECT tableoid";
            const cRecStaticDescr& d = inhRecDescr(tn);
            int i, n = recDescr().cols();
            for (i = 0; i < n; ++i) {   // Végihrohanunk az első tábla mezőin
                QString fn = recDescr().columnName(i);
                sql += _sCommaSp;
                sql += d.toIndex(fn, false) < 0 ? _sNULL : fn;  // Ha nincs akkor NULL
            }
            sql += " FROM ONLY " + p->fullTableNameQ();
        }
    }
    PDEB(VVERBOSE) << "Create view : " << sql << endl;
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
}

cTableShape * cRecordTable::getInhShape(const QString &_tn)
{
    cTableShape *p = new cTableShape();
    // Az alapértelmezett azonos tábla és shape rekord név
    if (p->fetchByName(*pq, _tn) && _tn == p->getName(_sTableName)) {
        p->fetchFields(*pq);
        return p;
    }
    // Ha nincs, akkor esetleg jó lessz az alap shape rekord?
    if (_tn == pTableShape->getName(_sTableName)) {
        p->set(*pTableShape);
        p->fetchFields(*pq);
        return p;
    }
    EXCEPTION(EDATA);
    return NULL;
}

const cRecStaticDescr& cRecordTable::inhRecDescr(qlonglong i) const
{
    if (pInhRecDescr == NULL) EXCEPTION(EPROGFAIL);
    if (pInhRecDescr->find(i) == pInhRecDescr->constEnd()) EXCEPTION(EDATA, i);
    return *pInhRecDescr->value(i);
}

const cRecStaticDescr& cRecordTable::inhRecDescr(const QString& tn) const
{
    if (pInhRecDescr == NULL) EXCEPTION(EPROGFAIL);
    QMap<qlonglong,const cRecStaticDescr *>::const_iterator i, e = pInhRecDescr->constEnd();
    for (i = pInhRecDescr->constBegin(); i != e; ++i) {
        const cRecStaticDescr& r = **i;
        if (r.tableName() == tn) return r;
    }
    EXCEPTION(EDATA, -1, tn);
    return *(const cRecStaticDescr *)NULL;    // Csak hogy ne ugasson a fordító
}

void cRecordTable::buttonPressed(int id)
{
    switch (id) {
    case DBT_CLOSE:     close();    break;
    case DBT_FIRST:     first();    break;
    case DBT_NEXT:      next();     break;
    case DBT_PREV:      prev();     break;
    case DBT_LAST:      last();     break;
    case DBT_REFRESH:   refresh();  break;
    case DBT_DELETE:    remove();   break;
    case DBT_INSERT:    insert();   break;
    case DBT_MODIFY:    modify();   break;
    default:
        DWAR() << "Invalid button id : " << id << endl;
        break;
    }
}

void cRecordTable::recordRemove(cAlternate * _pr)
{
    PDEB(INFO) << "Remove : " << _pr->toString() << endl;
}

void cRecordTable::selectionChanged(QItemSelection,QItemSelection)
{
    setEditButtons();
    if (flags & RTF_OVNER) {
        if (pRightTable == NULL) EXCEPTION(EPROGFAIL);
        pRightTable->owner_id = actId();
        pRightTable->refresh();
    }
}

void cRecordTable::clickedHeader(int)
{
    int r = pFODialog->exec();
    if (r == DBT_OK) refresh();
}

