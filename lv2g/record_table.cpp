#include "record_table.h"
#include "lv2user.h"
#include "record_tree.h"
#include "record_link.h"
#include "cerrormessagebox.h"
#include "tableexportdialog.h"
#include "lv2validator.h"
#include "popupreport.h"


Ui::noRightsForm * noRightsSetup(QWidget *_pWidget, qlonglong _need, const QString& _obj, const QString& _html)
{
    Ui::noRightsForm *noRights;
    noRights = new Ui::noRightsForm();
    noRights->setupUi(_pWidget);
    noRights->userLE->setText(lanView::user().getName());
    noRights->userRightsLE->setText(privilegeLevel(lanView::user().privilegeLevel()));
    noRights->viewRightsLE->setText(privilegeLevel(_need));
    noRights->viewedLE->setText(_obj);
    if (_html.isEmpty() == false) {
        noRights->msgTE->setHtml(_html);
    }
    return noRights;
}

/* ***************************************************************************************************** */

qlonglong cRecordTableFilter::type2filter()
{
    static QMap<int, qlonglong> map;
    if (map.isEmpty()) {
        QSqlQuery q = getQuery();
        const cColEnumType *pFiltEnum = cColEnumType::fetchOrGet(q, _sFiltertype);
        pFiltEnum->checkEnum(filterType, filterType);
        map[cColStaticDescr::FT_INTEGER]    = enum2set(FT_EQUAL, FT_LITLE, FT_BIG, FT_INTERVAL);
        map[cColStaticDescr::FT_TIME]       = map[cColStaticDescr::FT_INTEGER];
        map[cColStaticDescr::FT_DATE]       = map[cColStaticDescr::FT_INTEGER];
        map[cColStaticDescr::FT_DATE_TIME]  = map[cColStaticDescr::FT_INTEGER];
        map[cColStaticDescr::FT_INTERVAL]   = map[cColStaticDescr::FT_INTEGER];
        map[cColStaticDescr::FT_REAL]       = map[cColStaticDescr::FT_INTEGER];
        map[cColStaticDescr::FT_TEXT]       = enum2set(FT_BEGIN, FT_LIKE, FT_SIMILAR, FT_REGEXP);
        map[cColStaticDescr::FT_BINARY]     = 0L;
        map[cColStaticDescr::FT_BOOLEAN]    = enum2set(FT_BOOLEAN);
        map[cColStaticDescr::FT_POLYGON]    = 0L;
        map[cColStaticDescr::FT_ENUM]       = map[cColStaticDescr::FT_TEXT] | enum2set(FT_ENUM);
        map[cColStaticDescr::FT_SET]        = map[cColStaticDescr::FT_TEXT] | enum2set(FT_SET);
        map[cColStaticDescr::FT_MAC]        = map[cColStaticDescr::FT_TEXT] | map[cColStaticDescr::FT_INTEGER];
        map[cColStaticDescr::FT_INET]       = map[cColStaticDescr::FT_MAC];
        map[cColStaticDescr::FT_CIDR]       = map[cColStaticDescr::FT_TEXT];
    }
    int ft = fieldType();
    qlonglong m = map[ft] | ENUM2SET(FT_NO);
    if (lanView::getInstance()->isAuthorized(PL_ADMIN)) m |= ENUM2SET(FT_SQL_WHERE);
    if (field.pColDescr->isNullable)                    m |= ENUM2SET(FT_NULL);
    // CAST TO ?
    if (ft == cColStaticDescr::FT_TEXT) {
        QStringList tt = field.shapeField.features().sListValue("cast");
        if (!tt.isEmpty()) {
            QSqlQuery q = getQuery();
            const cColEnumType *pTypeEnum = cColEnumType::fetchOrGet(q, _sParamTypeType);
            qlonglong ts = pTypeEnum->lst2set(tt);
            m = filterSetAndTypeSet(m, ts);
        }
    }
    return m;
}

cRecordTableFilter::cRecordTableFilter(cRecordTableFODialog &_par, cRecordTableColumn& _rtc)
    : QObject(&_par)
    , field(_rtc)
    , dialog(_par)
    , param1(), param2()
    , filterTypeList()
{
    closed1 = closed2 = true;
    inverse = false;
    any     = true;
    csi     = false;
    toType  = ENUM_INVALID;
    setOn   = -1;
    setOff  =  0;
    pValidator1 = pValidator2 = NULL;
    if (field.pColDescr == NULL) {
        toTypes  = filtTypesEx = filtTypes = 0;
    }
    else {
        filtTypesEx = type2filter();            // Filter types + type casts (SET + SET)
        filtTypes   = pullFilter(filtTypesEx);  // Filter types (SET)
        toTypes     = pullType(filtTypesEx);    // type casts (SET)
    }
    qlonglong m, i;
    for (i = 0, m = 1; m < filtTypes; m <<= 1, ++i) {
        if (filtTypes & m) {
            filterTypeList << &cEnumVal::enumVal(_sFiltertype, i);
        }
    }
    iFilter = 0;   // No selected filter
}

cRecordTableFilter::~cRecordTableFilter()
{
    ;
}

void cRecordTableFilter::setFilter(int i)
{
    if (iFilter != i) {
        param1.clear();
        param2.clear();
        closed1 = closed2 = true;
        inverse = false;
        csi     = false;
        any     = true;
        toType  =  ENUM_INVALID;
        setOn   = -1;
        setOff  =  0;
        pValidator1 = pValidator2 = NULL;
    }
    iFilter = i;
}

int cRecordTableFilter::fieldType()
{
    int r = field.pColDescr->eColType;
    if (field.pColDescr->fKeyType == cColStaticDescr::FT_NONE) return r;
    if ((r & ~cColStaticDescr::FT_ARRAY) != cColStaticDescr::FT_INTEGER) EXCEPTION(EDATA, r);
    if (field.pColDescr->fKeyType == cColStaticDescr::FT_TEXT_ID) return cColStaticDescr::FT_TEXT;
    return cColStaticDescr::FT_TEXT | (r & cColStaticDescr::FT_ARRAY);
}

const cEnumVal *cRecordTableFilter::pActFilterType()
{
    if (!isContIx(filterTypeList, iFilter)) EXCEPTION(EPROGFAIL, iFilter);
    return filterTypeList.at(iFilter);
}

inline QString arrayAnyAll(bool isArray, bool any, QString o, QString n, int t = ENUM_INVALID)
{
    QString r;
    if (isArray) {
        r = any ? "0 < " : QString("array_length(%1, 1) = ").arg(n);
        r += QString("(SELECT COUNT(*) FROM unnest(%1) AS col WHERE ").arg(n);
        switch (t) {
        case ENUM_INVALID:  r += QString("col %1 ?)").arg(o);                   break;
        case PT_TEXT:       r += QString("col::text %1 ?)").arg(o);             break;
        default:            r += nameToCast(t) + QString("(col) %1 ?)").arg(o);break;
        }
    }
    else {
        switch (t) {
        case ENUM_INVALID:  r = n;                              break;
        case PT_TEXT:       r = n + "::text";                   break;
        default:            r = fnCatParms(nameToCast(t), n);  break;
        }
        r += QString(" %1 ?").arg(o);
    }
    return r;
}

QString cRecordTableFilter::whereLike(const QString& n, bool isArray)
{
    QString o = "LIKE";
    if (csi)     o.prepend('I');
    if (inverse) o.prepend("NOT ");
    return arrayAnyAll(isArray, any, o, n, PT_TEXT);
}

QString cRecordTableFilter::whereLitle(const QString& n, bool af, bool inv, bool clo)
{
    QString o = inv ? (clo ? ">" : ">=") : (clo ? " <=" : "<");
    return arrayAnyAll(af, any, o, n, toType);
}

QString cRecordTableFilter::whereEnum(const QString& n, QVariantList& qparams)
{
    const cColEnumType& et = field.pColDescr->enumType();
    QStringList ons = et.set2lst(setOn, EX_IGNORE);
    if (ons.isEmpty()) {    // there is no adequate value
        return inverse ? QString() : QString("%1 IS NULL").arg(n);
    }
    if (ons.size() == et.enumValues.size()) { // All values are appropriate
        return inverse ? QString("%1 IS NULL").arg(n) : QString();
    }
    qparams << stringListToSql(ons);
    QString r = n;
    r += (inverse ? " <> ALL" : " = ANY");
    r += QString(" (?::%1[])").arg(et);
    return r;
}

QString cRecordTableFilter::whereSet(const QString &n, QVariantList& qparams)
{
    const cColEnumType& et = field.pColDescr->enumType();
    QStringList ons  = et.set2lst(~setOn, EX_IGNORE);    // inverse!
    QStringList offs = et.set2lst(setOff, EX_IGNORE);
    if (ons.isEmpty() && offs.isEmpty()) { // No any condition
        return inverse ? QString("%1 IS NULL").arg(n) : QString();
    }
    QStringList sl;     // Max two expression
    if (!ons.isEmpty()) {
        sl << QString("%1 @> ?::%2[]").arg(n, et);
        qparams << stringListToSql(ons);
    }
    if (!offs.isEmpty()) {
        sl << QString("NOT %1 && ?::%2[]").arg(n, et);
        qparams << stringListToSql(offs);
    }
    return sl.join(" AND ");
}


QString cRecordTableFilter::where(QVariantList& qparams)
{
    QString r;
    if (field.pColDescr == NULL) return r;      // !!!
    int eFilter;    // Filter típus azonostó (ha van)
    if (iFilter >= 0 && (eFilter = filterTypeList.at(iFilter)->toInt()) != FT_NO) {
        bool      isArray = false;                      // A set-et nem tömbként kezeljük.
        QString   colName = field.pColDescr->colNameQ();// Mező név, vagy kifelyezés
        int eColType = field.pColDescr->eColType;
        QString   viewFunction = field.shapeField.feature("view.function"); // Konverziós függvény a megjelenítésnél
        if (viewFunction.isEmpty() == false) {
            eColType = cColStaticDescr::FT_TEXT;    // konvertáltuk a függvénnyel text-re
            colName  = viewFunction + "(" + colName + ")";
        }
        switch (eColType) {
        case cColStaticDescr::FT_TEXT_ARRAY:
            isArray = true;
        case cColStaticDescr::FT_TEXT:
            // toType = ????;
            break;
        case cColStaticDescr::FT_INTEGER:
            // Egész, lehet ID is, van név konverzió ?
            if (!field.pColDescr->fnToName.isEmpty()) {
                colName = field.pColDescr->fnToName + "(" + colName + ")";
            }
            break;
        case cColStaticDescr::FT_INTEGER_ARRAY:
            // Egész tömb, lehet ID is, van név konverzió ?
            if (!field.pColDescr->fnToName.isEmpty()) {
                colName = "(SELECT array_agg(" + fnCatParms(field.pColDescr->fnToName, colName) + ") FROM unnest(" + colName + "))";
            }
            isArray = true;
            break;
        case cColStaticDescr::FT_REAL_ARRAY:
            isArray = true;
            break;
        default:    break;
        }
        QString o;
        switch (eFilter) {
        case FT_BEGIN:
            r = whereLike(colName, isArray);
            qparams << QVariant(param1.toString() + "%");
            break;
        case FT_LIKE:
            r = whereLike(colName, isArray);
            qparams << param1;
            break;
        case FT_SIMILAR:
            o = "SIMILAR TO";
            if (inverse) o.prepend(r);
            r = arrayAnyAll(isArray, any, o, colName, PT_TEXT);
            qparams << param1;
            break;
        case FT_REGEXP:
            o = "~";
            if (csi)   o.append('*');
            if (inverse) o.prepend('!');
            r = arrayAnyAll(isArray, any, o, colName, PT_TEXT);
            qparams << param1;
            break;
        case FT_EQUAL:
            o = inverse ? "<>" : "=";
            r = arrayAnyAll(isArray, any, o, colName, toType);
            qparams << param1;
            break;
        case FT_LITLE:
            r = whereLitle(colName, isArray, inverse, closed2);
            qparams << param2;
            break;
        case FT_BIG:
            r = whereLitle(colName, isArray, !inverse, !closed1);
            qparams << param1;
            break;
        case FT_INTERVAL:
            r = whereLitle(colName, isArray, !inverse, !closed1);
            qparams << param1;
            r += " AND ";
            r += whereLitle(colName, isArray, inverse, closed2);
            qparams << param2;
            break;
        case FT_BOOLEAN:
            if (inverse) r = "NOT ";
            r += colName + "::boolean";
            break;
        case FT_ENUM:
            r = whereEnum(colName, qparams);
            break;
        case FT_SET:
            r = whereSet(colName, qparams);
            break;
        case FT_NULL:
            if (isArray) r = QString("array_lenght(%1, 1) %2 0").arg(colName, inverse ? "<>" : "=");
            else    r = colName + (inverse ? " IS NOT NULL" : " IS NULL");
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

template <class V> void cRecordTableFilter::tSetValidator()
{
    if (dialog.pLineEdit1 != NULL) {
        pValidator1 = new V;
        dialog.pLineEdit1->setValidator(pValidator1);
    }
    if (dialog.pLineEdit2 != NULL) {
        pValidator2 = new V;
        dialog.pLineEdit2->setValidator(pValidator2);
    }
}

void cRecordTableFilter::setValidatorRegExp(const QString& _re)
{
    QRegExp re(_re);
    QRegExpValidator *prev;
    if (dialog.pLineEdit1 != NULL) {
        prev = new QRegExpValidator(re);
        pValidator1 = prev;
        dialog.pLineEdit1->setValidator(pValidator1);
    }
    if (dialog.pLineEdit2 != NULL) {
        prev = new QRegExpValidator(re);
        pValidator2 = prev;
        dialog.pLineEdit2->setValidator(pValidator2);
    }
}


void cRecordTableFilter::setValidator(int i)
{
    switch (i) {
    case PT_INTEGER:
        tSetValidator<QIntValidator>();
        break;
    case PT_REAL:
        tSetValidator<QDoubleValidator>();
        break;
    case PT_TIME:
        setValidatorRegExp(QString("^([0-9]|0[0-9]|1[0-9]|2[0-3]):[0-5][0-9]$"));
        break;
    case PT_DATE:
        setValidatorRegExp(QString("^20\\d\\d[-/](0[1-9]|1[012])[-/](0[1-9]|[12][0-9]|3[01])$"));
        break;
    case PT_DATETIME:
        setValidatorRegExp(QString("^20\\d\\d[-/](0[1-9]|1[012])[-/](0[1-9]|[12][0-9]|3[01])[ T]([0-9]|0[0-9]|1[0-9]|2[0-3]):[0-5][0-9]$"));
        break;
    case PT_INTERVAL:
        setValidatorRegExp(QString("^(\\d+[Dd]ays?)?\\s*([0-9]|0[0-9]|1[0-9]|2[0-3]):[0-5][0-9](\\.\\d+)?$"));
        break;
    case PT_INET:
        tSetValidator<cINetValidator>();
        break;
    case PT_MAC:
        tSetValidator<cMacValidator>();
        break;
    case PT_TEXT:
    default:
        pDelete(pValidator1);
        pDelete(pValidator2);
        break;
    }
}

void cRecordTableFilter::changedParam1(const QString& s)
{
    param1 = s;
}

void cRecordTableFilter::changedParam2(const QString& s)
{
    param2 = s;
}

void cRecordTableFilter::togledClosed1(bool f)
{
    closed1 = f;
}

void cRecordTableFilter::togledClosed2(bool f)
{
    closed2 = f;
}

void cRecordTableFilter::togledCaseSen(bool f)
{
    csi = !f;
}

void cRecordTableFilter::togledInverse(bool f)
{
    inverse = f;
}

void cRecordTableFilter::changedToType(int i)
{
    toType = i;
    setValidator(i);
}

void cRecordTableFilter::changedAnyAll(int i)
{
    any = i == 0;
}


void cRecordTableFilter::changedText()
{
    QPlainTextEdit *pTextEdit = dialog.pTextEdit;
    if (pTextEdit != NULL) {
        param1 = pTextEdit->toPlainText();
    }
    else {
        EXCEPTION(EPROGFAIL);
    }
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
    pRowName->setReadOnly(true);
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
    if (field.fieldIndex == NULL_IX) return r;  // !!!
    act = orderType(pType->currentText(), EX_IGNORE);
    switch (act) {
    case OT_ASC:    r = " ASC ";    break;
    case OT_DESC:   r = " DESC ";   break;
    case OT_NO:     return r;
    default:        EXCEPTION(EPROGFAIL);
    }
    const cColStaticDescr& colDescr =  *field.pColDescr;
    if (colDescr.fKeyType == cColStaticDescr::FT_NONE) r = colDescr.colNameQ() + r;
    else {
        if (colDescr.fnToName.isEmpty()) EXCEPTION(EDATA, -1, QObject::trUtf8("Az ID->név konverziós függvény nincs definiálva."));
        r = colDescr.fnToName + QChar('(') + colDescr.colNameQ() + QChar(')') + r;
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


cEnumCheckBox::cEnumCheckBox(int _e, qlonglong* _pOn, qlonglong* _pOff, const QString& t)
    : QCheckBox(t)
{
    m    = ENUM2SET(_e);
    pOn  = _pOn;
    pOff = _pOff;
    setTristate(pOff != NULL);
    if (pOff == NULL) {
        setChecked(m & *pOn);
    }
    else {
        if (m & *pOn) {
            setCheckState(Qt::PartiallyChecked);
            *pOff &= ~m;
        }
        else if (m & *pOff) {
            setCheckState(Qt::Unchecked);
        }
        else {
            setCheckState(Qt::Checked);
        }
    }
    connect(this, SIGNAL(stateChanged(int)), this, SLOT(_chageStat(int)));
}

void cEnumCheckBox::_chageStat(int st)
{
    if (pOff == NULL) { // ENUM
        switch (st) {
        case Qt::Checked:           *pOn |=  m;             break;
        case Qt::Unchecked:         *pOn &= ~m;             break;
        case Qt::PartiallyChecked:  EXCEPTION(EPROGFAIL);   break;
        }
    }
    else {
        switch (st) {   // on : inverse!
        case Qt::Checked:           *pOn &= ~m; *pOff &= ~m;    break;
        case Qt::Unchecked:         *pOn |=  m; *pOff |=  m;    break;
        case Qt::PartiallyChecked:  *pOn |=  m; *pOff &= ~m;    break;
        }
    }
}

/* ***************************************************************************************************** */

enum eHideRowsColumns {
    HRC_NAME, HRC_TITLE, HRC_HIDE_TAB, HRC_HIDE_TEXT,
    HRC_COLUMNS
};

cRecordTableHideRow::cRecordTableHideRow(int row, cRecordTableHideRows * _par) : QObject(_par)
{
    QTableWidgetItem *pi;
    pParent = _par;
    pColumn = pParent->pParent->recordView.fields[row];

    name  = pColumn->shapeField.getName();
    pi = new QTableWidgetItem(name);
    pParent->setItem(row, HRC_NAME, pi);

    title = pColumn->shapeField.getText(cTableShapeField::LTX_TABLE_TITLE, name);
    pi = new QTableWidgetItem(title);
    pParent->setItem(row, HRC_TITLE, pi);

    pCheckBoxTab   = new QCheckBox;
    pCheckBoxTab->setChecked(0 == (pColumn->fieldFlags & ENUM2SET(FF_TABLE_HIDE)));
    connect(pCheckBoxTab, SIGNAL(toggled(bool)), this, SLOT(on_checkBoxTab_togle(bool)));
    pParent->setCellWidget(row, HRC_HIDE_TAB, pCheckBoxTab);

    pCheckBoxHTML  = new QCheckBox;
    pCheckBoxHTML->setChecked(0 != (pColumn->fieldFlags & ENUM2SET(FF_HTML)));
    connect(pCheckBoxHTML, SIGNAL(toggled(bool)), this, SLOT(on_checkBoxHTML_togle(bool)));
    pParent->setCellWidget(row, HRC_HIDE_TEXT, pCheckBoxHTML);
}

void cRecordTableHideRow::on_checkBoxTab_togle(bool st)
{
    qlonglong& ffTable = pColumn->fieldFlags;
    if (st) {
        ffTable &= ~ENUM2SET(FF_TABLE_HIDE);
    }
    else {
        ffTable |=  ENUM2SET(FF_TABLE_HIDE);
    }
    pParent->refresh();
}

void cRecordTableHideRow::on_checkBoxHTML_togle(bool st)
{
    cTableShapeField& f = pColumn->shapeField;   // HTML
    if (st) {
        f.setOn(_sFieldFlags, ENUM2SET(FF_HTML));
    }
    else {
        f.setOff(_sFieldFlags, ENUM2SET(FF_HTML));
    }
}

int cRecordTableHideRow::columns()
{
    return HRC_COLUMNS;
}

QString cRecordTableHideRow::colTitle(int i)
{
    switch (i) {
    case HRC_NAME:      return trUtf8("Mező név");
    case HRC_TITLE:     return trUtf8("Oszlop név");
    case HRC_HIDE_TAB:  return trUtf8("Táblában");
    case HRC_HIDE_TEXT: return trUtf8("Exportban");
    default:            EXCEPTION(EPROGFAIL);
    }
    return QString();
}

/* ----------------------------------------------------------------------------------------------------- */

cRecordTableHideRows::cRecordTableHideRows(cRecordTableFODialog *_par)
    : QTableWidget()
{
    pParent = _par;
    int n = pParent->recordView.fields.size();
    int cols = cRecordTableHideRow::columns();
    setColumnCount(cols);
    setRowCount(n);
    QStringList hl;
    for (int col = 0; col < cols; ++col) {
        hl << cRecordTableHideRow::colTitle(col);
    }
    setHorizontalHeaderLabels(hl);
    for (int row = 0; row < n; ++row) {
        new cRecordTableHideRow(row, this);
    }
}

void cRecordTableHideRows::refresh()
{
    pParent->recordView.hideColumns();
}

/* ----------------------------------------------------------------------------------------------------- */

cRecordTableFODialog::cRecordTableFODialog(QSqlQuery *pq, cRecordsViewBase &_rt)
    : QDialog(_rt.pWidget())
    , recordView(_rt)
    , filters(), ords()
{
    pTextEdit = NULL;
    pLineEdit1 = NULL;
    pLineEdit2 = NULL;
    pToTypeModel = NULL;
    (void)*pq;
    pForm = new Ui::dialogTabFiltOrd();
    pForm->setupUi(this);

    tRecordTableColumns::ConstIterator i, n = recordView.fields.cend();
    QStringList cols;
    for (i = recordView.fields.cbegin(); i != n; ++i) {
        cTableShapeField& tsf = (*i)->shapeField;
        int ord = tsf.getId(_sOrdTypes);
        if (ord & ~enum2set(OT_NO)) {
            cRecordTableOrd *pOrd = new cRecordTableOrd(*this, **i, ord);
            ords << pOrd;
            connect(pOrd, SIGNAL(moveUp(cRecordTableOrd*)),   this, SLOT(ordMoveUp(cRecordTableOrd*)));
            connect(pOrd, SIGNAL(moveDown(cRecordTableOrd*)), this, SLOT(ordMoveDown(cRecordTableOrd*)));
        }
//        if (recordView.disableFilters == false) {
            cRecordTableFilter *pFilt = new cRecordTableFilter(*this, **i);
            if (pFilt->filtTypes == 0) {
                delete pFilt;
                continue;
            }
            filters << pFilt;
            cols    << tsf.getText(cTableShapeField::LTX_TABLE_TITLE);
//        }
    }
    PDEB(VERBOSE) << trUtf8("Shape : %1 ; filtered fields : %2")
                     .arg(recordView.tableShape().getName(), cols.join(_sCommaSp))
                  << endl;
    pForm->comboBoxCol->addItems(cols);
    connect(pForm->pushButton_OK, SIGNAL(clicked()), this, SLOT(clickOk()));
    connect(pForm->pushButton_Default, SIGNAL(clicked()), this, SLOT(clickDefault()));
    qSort(ords.begin(), ords.end(), PtrLess<cRecordTableOrd>());
    setGridLayoutOrder();
    iSelFilterCol  = -1;
    iSelFilterType = -1;
    pSelFilter = NULL;
    pForm->comboBoxCol->setCurrentIndex(0);
    connect(pForm->comboBoxCol,    SIGNAL(currentIndexChanged(int)),   this, SLOT(filtCol(int)));
    connect(pForm->comboBoxFilt,   SIGNAL(currentIndexChanged(int)),   this, SLOT(filtType(int)));
    filtCol(filters.isEmpty() ? -1 : 0);
    pForm->pushButton_Default->setDisabled(true);   // Nincs implementálva !
    pForm->tabWidget->addTab(new cRecordTableHideRows(this), trUtf8("Oszlopok láthatósága"));
}

cRecordTableFODialog::~cRecordTableFODialog()
{

}

QStringList cRecordTableFODialog::where(QVariantList& qparams)
{
    QStringList r;
    if (!filters.isEmpty()) {
        foreach (cRecordTableFilter *pF, filters) {
            QString rw = pF->where(qparams);
            if (!rw.isEmpty()) {
                r << rw;
            }
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

QComboBox * cRecordTableFODialog::comboBoxAnyAll()
{
    QComboBox *p = new QComboBox(this);
    p->addItem(trUtf8("A tömb legalább egy elemére"));
    p->addItem(trUtf8("A tömb összes elemére"));
    connect(p, SIGNAL(currentIndexChanged(int)), &filter(), SLOT(changedAnyAll(int)));
    return p;
}

QString cRecordTableFODialog::sCheckInverse;

void cRecordTableFODialog::setFilterDialog()
{
    QGridLayout *pGrid = pForm->gridLayoutFilter;
    int i;
    while (0 < (i = pGrid->count())) {  // Clear grid layout
        QLayoutItem *p = pGrid->itemAt(i -1);
        if (p != NULL && p->widget() != NULL) delete p->widget();
    }
    if (sCheckInverse.isEmpty()) {
        sCheckInverse = QObject::trUtf8("Fordított logika");
    }
    pTextEdit = NULL;   // Long text (param1)
    pLineEdit1 = NULL;  // Filter param1
    pLineEdit2 = NULL;  // Filter param2
    pToTypeModel = NULL;
    QLabel *pLabel = NULL;
    QCheckBox *pCheckBoxI = NULL;
    if (iSelFilterCol < 0 || iSelFilterType < 0) return;    // skeep
    filter().setFilter(iSelFilterType);
    const cEnumVal *pType = filter().pActFilterType();
    QString s = pType->getText(cEnumVal::LTX_VIEW_LONG);
    PDEB(VERBOSE) << QString("%1 : Filter #%2/%3 :")
                     .arg(recordView.tableShape().getName()).arg(iSelFilterCol).arg(iSelFilterType)
                  << s << endl;
    pForm->lineEditFilt->setText(s);
    //Szűrési paraméter szöveges beviteli mezők:
    int fType = pType->toInt();         // Filter type
    int dType = filter().fieldType();   // Column type
    switch (fType) {
    case FT_NO:
        break;
    case FT_BEGIN:
    case FT_LIKE:
    case FT_SIMILAR:
    case FT_REGEXP:
        setFilterDialogPattern(fType, dType);
        break;
    case FT_EQUAL:
    case FT_LITLE:
    case FT_BIG:
    case FT_INTERVAL:
        setFilterDialogComp(fType, dType);
        break;
    case FT_SQL_WHERE:
        pLabel    = new QLabel(trUtf8("SQL kifejezés :"));
        pTextEdit  = new QPlainTextEdit(filter().param1.toString());
        pGrid->addWidget(pLabel,    0, 0);
        pGrid->addWidget(pTextEdit, 0, 1);
        connect(pTextEdit, SIGNAL(textChanged()), &filter(), SLOT(changedText()));
        break;
    case FT_BOOLEAN:
    case FT_NULL:
        pCheckBoxI  = new QCheckBox(sCheckInverse);
        pGrid->addWidget(pCheckBoxI,  0, 0);
        connect(pCheckBoxI, SIGNAL(toggled(bool)), &filter(), SLOT(togledInverse(bool)));
        break;
    case FT_ENUM:
    case FT_SET:
        setFilterDialogEnum(filter().field.pColDescr->enumType(), fType == FT_ENUM);
        break;
    }
}

void cRecordTableFODialog::setFilterDialogPattern(int fType, int dType)
{
    static const QString sLabelPattern = trUtf8("Minta :");
    static const QString sCheckCaseSen = trUtf8("Nagybetű érzékeny");
    int row = 0;
    QLabel *   pLabel      = new QLabel(sLabelPattern);
    QLineEdit *pLineEditP1 = new QLineEdit(filter().param1.toString());
    QCheckBox *pCheckBoxI  = new QCheckBox(sCheckInverse);
    QCheckBox *pCheckBoxCS = new QCheckBox(sCheckCaseSen);
    QGridLayout *pGrid = pForm->gridLayoutFilter;
    pGrid->addWidget(pLabel,      row, 0);
    pGrid->addWidget(pLineEditP1, row, 1);  ++row;
    pGrid->addWidget(pCheckBoxI,  row, 1);  ++row;
    pGrid->addWidget(pCheckBoxCS, row, 1);  ++row;
    pCheckBoxCS->setChecked(true);
    if (fType == FT_SIMILAR) {
        pCheckBoxCS->setDisabled(true);
    }
    else {
       connect(pCheckBoxCS, SIGNAL(toggled(bool)), &filter(), SLOT(togledCaseSen(bool)));
    }
    connect(pLineEditP1, SIGNAL(textChanged(QString)), &filter(), SLOT(changedParam1(QString)));
    connect(pCheckBoxI,  SIGNAL(toggled(bool)),        &filter(), SLOT(togledInverse(bool)));
    if (dType & cColStaticDescr::FT_ARRAY) {
        pGrid->addWidget(comboBoxAnyAll(), row, 1);
    }
}

void cRecordTableFODialog::setFilterDialogComp(int fType, int dType)
{
    static const QString sLabelEq = trUtf8("Ezzel egyenlő :");
    static const QString sLabelLi = trUtf8("Ennél kisebb :");
    static const QString sLabelGt = trUtf8("Ennél nagyobb :");
    static const QString sCheckEq = trUtf8("vagy egyenlő");
    static const QString sLabelTy = trUtf8("Típus mint :");
    int row = 0;
    QLabel *   pLabel1     = NULL;
    QLabel *   pLabel2     = NULL;
    QCheckBox *pCheckBoxEq1= NULL;
    QCheckBox *pCheckBoxEq2= NULL;
    QCheckBox *pCheckBoxI  = new QCheckBox(sCheckInverse);;
    QGridLayout *pGrid = pForm->gridLayoutFilter;
    switch (fType) {
    case FT_EQUAL:
        pLabel1     = new QLabel(sLabelEq);
        pLineEdit1  = new QLineEdit(filter().param1.toString());
        break;
    case FT_BIG:
    case FT_INTERVAL:
        pLabel1     = new QLabel(sLabelGt);
        pLineEdit1  = new QLineEdit(filter().param1.toString());
        pCheckBoxEq1= new QCheckBox(sCheckEq);
        break;
    }
    if (fType == FT_LITLE || fType == FT_INTERVAL) {
        pLabel2     = new QLabel(sLabelLi);
        pLineEdit2  = new QLineEdit(filter().param2.toString());
        pCheckBoxEq2= new QCheckBox(sCheckEq);
    }
    if (pLabel1 != NULL) {
        pGrid->addWidget(pLabel1,     row, 0);
        pGrid->addWidget(pLineEdit1,  row, 1);
        connect(pLineEdit1, SIGNAL(textChanged(QString)), &filter(), SLOT(changedParam1(QString)));
        if (pCheckBoxEq1 != NULL) { // If operator is '=', then pCheckBoxEq1 is NULL
            pGrid->addWidget(pCheckBoxEq1, row, 2);
            connect(pCheckBoxEq1, SIGNAL(toggled(bool)), &filter(), SLOT(togledClosed1(bool)));
        }
        row++;
    }
    if (pLabel2 != NULL) {
        pGrid->addWidget(pLabel2,    row, 0);
        pGrid->addWidget(pLineEdit2, row, 1);
        connect(pLineEdit2, SIGNAL(textChanged(QString)), &filter(), SLOT(changedParam2(QString)));
        pGrid->addWidget(pCheckBoxEq2, row, 2);
        connect(pCheckBoxEq2, SIGNAL(toggled(bool)), &filter(), SLOT(togledClosed2(bool)));
        row++;
    }
    pGrid->addWidget(pCheckBoxI,  row, 1);  ++row;
    connect(pCheckBoxI,  SIGNAL(toggled(bool)),        &filter(), SLOT(togledInverse(bool)));
    int t = (dType & ~cColStaticDescr::FT_ARRAY);
    if (t != dType) {   // ARRAY
        pGrid->addWidget(comboBoxAnyAll(), row, 1);
        row++;
    }
    if ((t & ~cColStaticDescr::FT_ARRAY) == cColStaticDescr::FT_TEXT && filter().toTypes != 0) {
        QComboBox *pComboBoxTy = new QComboBox();
        qlonglong tt = filter().toTypes;
        QSqlQuery q = getQuery();
        const cColEnumType *pParamTypeType = cColEnumType::fetchOrGet(q, "paramtype");
        pToTypeModel = new cEnumListModel(pParamTypeType, NT_NOT_NULL, pParamTypeType->lst2lst(pParamTypeType->set2lst(tt)));
        pToTypeModel->joinWith(pComboBoxTy);
        pComboBoxTy->setCurrentIndex(0);
        pGrid->addWidget(new QLabel(sLabelTy), row, 0);
        pGrid->addWidget(pComboBoxTy, row, 1);
        connect(pToTypeModel, SIGNAL(currentEnumChanged(int)), &filter(), SLOT(changedToType(int)));
        filter().changedToType(pToTypeModel->atInt(0));
    }
}

void cRecordTableFODialog::setFilterDialogEnum(const cColEnumType &et, bool _enum)
{
    cRecordTableFilter& f = filter();
    QGridLayout *pGrid = pForm->gridLayoutFilter;
    int i;
    for (i = 0; i < et.enumValues.size(); ++i) {
        QString t = cEnumVal::viewShort(et, i, et.enum2str(i));
        cEnumCheckBox *p = new cEnumCheckBox(i, &f.setOn, _enum ? NULL : &f.setOff, t);
        pGrid->addWidget(p, i, 0);
    }
    QCheckBox *pCheckBoxI  = new QCheckBox(sCheckInverse);;
    pGrid->addWidget(pCheckBoxI,  0, 1);
    connect(pCheckBoxI, SIGNAL(toggled(bool)), &f, SLOT(togledInverse(bool)));
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
    if (iSelFilterCol == _c) return;
    iSelFilterType = iSelFilterCol = -1;
    if (_c != -1) { // If there is any filter
        if (!isContIx(filters, _c)) EXCEPTION(EDATA, _c);
        pSelFilter = filters[_c];
        pForm->lineEditCol->setText(filter().field.shapeField.getText(cTableShapeField::LTX_DIALOG_TITLE,
                                                                      filter().field.shapeField.getName()));
        QStringList items;
        foreach (const cEnumVal *pe, pSelFilter->filterTypeList) {
            items << pe->getText(cEnumVal::LTX_VIEW_SHORT, pe->getName());
        }
        pForm->comboBoxFilt->clear();
        pForm->comboBoxFilt->addItems(items);
        iSelFilterType = filter().iFilter;
        pForm->comboBoxFilt->setCurrentIndex(iSelFilterType);
        iSelFilterCol = _c;
    }
    filtType(iSelFilterType);
}

void cRecordTableFODialog::filtType(int _t)
{
    iSelFilterType = _t;
    setFilterDialog();
}

/* ***************************************************************************************************** */

cRecordTableColumn::cRecordTableColumn(cTableShapeField &sf, cRecordsViewBase &table)
    : shapeField(sf)
    , recDescr(table.recDescr())
    , header(shapeField.getText(cTableShapeField::LTX_TABLE_TITLE, shapeField.getName()))
    , defaultDc(cEnumVal::enumVal(_sDatacharacter, dataCharacter))
{
    fieldIndex = recDescr.toIndex(sf.getName(), EX_IGNORE);
    pColDescr  = NULL;
    textIndex  = NULL_IX;
    pTextEnum  = NULL;
    headAlign  = Qt::AlignVCenter | Qt::AlignHCenter;
    dataAlign  = Qt::AlignVCenter;
    fieldFlags = shapeField.getId(_sFieldFlags);
    if (fieldIndex == NULL_IX) {
        pTextEnum = recDescr.colDescr(recDescr.textIdIndex()).pEnumType;
        if (pTextEnum == NULL) EXCEPTION(EDATA);
        textIndex = pTextEnum->str2enum(sf.getName());
        dataCharacter = DC_TEXT;
    }
    else {
        pColDescr = &recDescr.colDescr(fieldIndex);
        dataCharacter = defaultDataCharter(recDescr, fieldIndex);
        if (pColDescr->eColType == cColStaticDescr::FT_INTEGER && pColDescr->fKeyType == cColStaticDescr::FT_NONE)
            dataAlign |= Qt::AlignRight;
        else if (pColDescr->eColType == cColStaticDescr::FT_REAL)
            dataAlign |= Qt::AlignRight;
        // 'XX_color' vagy 'font' vagy 'tool_tip' flag esetén kell az enum típusa!
        if (pColDescr->eColType == cColStaticDescr::FT_ENUM ||pColDescr->eColType == cColStaticDescr::FT_BOOLEAN) {
            if (fieldFlags & ENUM2SET4(FF_BG_COLOR, FF_FG_COLOR, FF_FONT, FF_TOOL_TIP)) {
                if (pColDescr->eColType == cColStaticDescr::FT_ENUM) {
                    enumTypeName = pColDescr->enumType();
                }
                else {  // FT_BOOLEAN
                    enumTypeName = mCat(recDescr.tableName(), *pColDescr);
                }
            }
        }
        else if (fieldFlags & ENUM2SET2(FF_BG_COLOR, FF_FG_COLOR)) {
            enumTypeName = _sEquSp; // maga a mező a szín
        }
    }
}

/* ***************************************************************************************************** */

cRecordsViewBase::cRecordsViewBase(bool _isDialog, QWidget *par)
    : QObject(par)
    , isDialog(_isDialog)
    , fields()
    , inheritTableList()
    , viewName()
{
    flags = 0;
    isReadOnly = false;
    pq = newQuery();
    pTabQuery = newQuery();
    pTableShape = NULL;
    pModel = NULL;
    pUpper = NULL;
    pMaster = NULL;
    pRecDescr = NULL;
    _pWidget = NULL;
    pMasterSplitter = NULL;
    pButtons = NULL;
    pMainLayout = NULL;
    pLeftWidget = NULL;
    pRightTables = NULL;
    pRightTabWidget = NULL;
    pFODialog   = NULL;
    owner_id = NULL_ID;
    parent_id= NULL_ID;
    pInhRecDescr = NULL;
    tableInhType = TIT_NO;
    pRecordDialog = NULL;
    _pWidget = isDialog ? new QDialog(par) : new QWidget(par);
//    disableFilters = false; // alapértelmezetten vannak szűrők
}

cRecordsViewBase::~cRecordsViewBase()
{
    delete pq;
    delete pTabQuery;
    pDelete(pTableShape);
    pDelete(pRightTables);
}

QDialog& cRecordsViewBase::dialog()
{
    if (!isDialog) EXCEPTION(EPROGFAIL);
    return *dynamic_cast<QDialog *>(_pWidget);
}

void cRecordsViewBase::buttonDisable(int id, bool d)
{
    QAbstractButton * p = pButtons->button(id);
    if (p == NULL) {
        // EXCEPTION(EDATA, id);
        return;
    }
    p->setDisabled(d);
}

cTableShape * cRecordsViewBase::getInhShape(QSqlQuery& q, cTableShape *pTableShape, const QString &_tn, bool ro)
{
    cTableShape *p = new cTableShape();
    // Az alapértelmezett azonos tábla és shape rekord név
    if (p->fetchByName(q, _tn) && _tn == p->getName(_sTableName)) {
        if (ro) p->enum2setOn(_sTableShapeType, TS_READ_ONLY);
        p->fetchFields(q);
        return p;
    }
    // Ha nincs, akkor esetleg jó lessz az alap shape rekord?
    if (_tn == pTableShape->getName(_sTableName)) {
        p->cRecord::set(*pTableShape);
        p->fetchFields(q);
        return p;
    }
    EXCEPTION(EDATA);
    return NULL;
}

const cRecStaticDescr& cRecordsViewBase::inhRecDescr(qlonglong i) const
{
    if (pInhRecDescr == NULL) EXCEPTION(EPROGFAIL);
    if (pInhRecDescr->find(i) == pInhRecDescr->constEnd()) EXCEPTION(EDATA, i);
    return *pInhRecDescr->value(i);
}

const cRecStaticDescr& cRecordsViewBase::inhRecDescr(const QString& tn) const
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

void cRecordsViewBase::buttonPressed(int id)
{
    _DBGFN() << " #" << id << endl;
    switch (id) {
    case DBT_CLOSE:     close();        break;
    case DBT_REFRESH:   refresh();      break;
    case DBT_INSERT:    insert();       break;
    case DBT_SIMILAR:   insert(true);   break;
    case DBT_MODIFY:    modify();       break;
    case DBT_FIRST:     first();        break;
    case DBT_PREV:      prev();         break;
    case DBT_NEXT:      next();         break;
    case DBT_LAST:      last();         break;
    case DBT_DELETE:    remove();       break;
    case DBT_RESET:     reset();        break;
    case DBT_PUT_IN:    putIn();        break;
    case DBT_TAKE_OUT:  takeOut();      break;
    case DBT_COPY:      copy();         break;
    case DBT_RECEIPT:   receipt();      break;
    case DBT_TRUNCATE:  truncate();     break;
    case DBT_REPORT:    report();       break;
    default:
        DWAR() << "Invalid button id : " << id << endl;
        break;
    }
    DBGFNL();
}

// PushButton -> virtual f()

void cRecordsViewBase::close(int r)
{
    if (pRecordDialog != NULL) {
        pRecordDialog->_pressed(DBT_CANCEL);
    }
    if (isDialog) dynamic_cast<QDialog *>(_pWidget)->done(r);
    else closeIt();
}

void cRecordsViewBase::refresh(bool first)
{
    _DBGFN() << VDEB(first) << endl;
    cError *pe = NULL;
    try {
        switch (tableInhType) {
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
    } CATCHS(pe)
    if (pe != NULL) {
        cErrorMessageBox::messageBox(pe, pWidget());
        delete pe;
        pModel->clear();
    }
    DBGFNL();
}

void cRecordsViewBase::insert(bool _similar)
{
    QString sInsertDialog = pTableShape->feature(_sInsert);
    if (flags & RTF_CHILD) {
        if (pUpper == NULL) EXCEPTION(EPROGFAIL);
        if (owner_id == NULL_ID) return;
    }
    if (pRecordDialog != NULL) {
        pDelete(pRecordDialog); // !!!???
        // EXCEPTION(EPROGFAIL);
    }
    parent_id = NULL_ID;
    // Ha TREE, akkor a default parent a kiszelektált sor,
    if (flags & RTF_TREE) {
        cRecord *pARec = actRecord();
        if (pARec != NULL) {    // ha van kiszelektált (egy) sor
            parent_id = pARec->getId();
        }
    }
    if (!sInsertDialog.isEmpty()) {
        cRecord *pRec = NULL;
        cRecordAny sample;
        if (_similar) {
            sample = *actRecord();
            pRec = &sample;
        }
        else if (owner_id != NULL_ID) {
            sample.setType(&recDescr());
            sample.set(ixToOwner(), owner_id);
            pRec = &sample;
        }
        pRec = objectDialog(sInsertDialog, *pq, this->pWidget(), pRec);
        if (pRec == NULL) return;
        pDelete(pRec);
        refresh();
        return;
    }
    // A dialógusban megjelenítendő nyomógombok.
    int buttons = enum2set(DBT_OK, DBT_INSERT, DBT_CANCEL);
    switch (tableInhType) {
    case TIT_NO:
    case TIT_ONLY: {
        cRecordDialog   rd(*pTableShape, buttons, true, NULL, this, pWidget());  // A rekord szerkesztő dialógus
        pRecordDialog = &rd;
        if (_similar) {
            cRecord *pRec = actRecord();    // pointer az aktuális rekordra, a beolvasott/megjelenített rekord listában
            if (pRec != NULL) {
                cRecord *pR = pRec->dup();
                QBitArray c = pR->autoIncrement();
                pR->clear(c);
                rd.restore(pR);
                delete pR;
            }
        }
        while (1) {
            int r = rd.exec();
            if (r == DBT_INSERT || r == DBT_OK) {   // Csak az OK, és Insert gombra csinálunk valamit (egyébként: break)
                bool ok = rd.accept();
                if (ok) {
                    cRecord *pRec = rd.record().dup();
                    ok = pModel->insertRec(pRec);   // A pointer a pModel tulajdona lesz, ha OK
                    if (!ok) pDelete(pRec);
                    else if (flags & RTF_IGROUP) {    // Group, tagja listába van a beillesztés?
                        ok = cGroupAny(*pRec, *(pUpper->actRecord())).insert(*pq, EX_IGNORE);
                        if (!ok) {
                            QMessageBox::warning(pWidget(), dcViewShort(DC_ERROR), trUtf8("A kijelölt tag felvétele az új csoportba sikertelen"),QMessageBox::Ok);
                            refresh();
                            break;
                        }
                    }
                }
                if (ok) {
                    if (r == DBT_OK) break; // Ha OK-t nyomott becsukjuk az dialóg-ot
                    continue;               // Ha Insert-et, akkor folytathatja a következővel
                }
                else {
                    //QMessageBox::warning(pWidget(), trUtf8("Az új rekord beszúrása sikertelen."), rd.errMsg());
                    continue;
                }
            }
            break;  // while
        }
    }   break;      // switch
    case TIT_LISTED_REV: {
        tRecordList<cTableShape> *pShapes = getShapes();
        cRecordDialogInh rd(*pTableShape, *pShapes, buttons, true, NULL, this, pWidget());
        pRecordDialog = &rd;
        if (_similar) {
            cRecord *pRec = actRecord();    // pointer az aktuális rekordra, a beolvasott/megjelenített rekord listában
            if (pRec != NULL) {
                cRecord *pR = pRec->dup();  // Lemásoljuk
                QBitArray c = pR->autoIncrement();
                pR->clear(c);               // Töröljük az "auto increment" típusú mezőket
                rd.restore(pR);
                delete pR;
            }
        }
        while (1) {
            int r = rd.exec();
            if (r == DBT_INSERT || r == DBT_OK) {
                bool ok = rd.accept();
                if (ok) {
                    cRecord *pRec = rd.record().dup();
                    ok = pModel->insertRec(pRec);
                    if (!ok) pDelete(pRec);
                }
                if (ok) {
                    if (r == DBT_OK) break; // Ha OK-t nyomott becsukjuk az dialóg-ot
                    continue;               // Ha Insert-et, akkor folytathatja a következővel
                }
                else {
                    // QMessageBox::warning(pWidget(), trUtf8("Az új rekord beszúrása sikertelen."), rd.errMsg());
                    continue;
                }
            }
            break;
        }
        delete pShapes;
    }   break;
    default:
        EXCEPTION(ENOTSUPP);
    }
    pRecordDialog = NULL;
}

void cRecordsViewBase::modify(eEx __ex)
{
    QModelIndex index = actIndex();
    if (index.isValid() == false) return;
    if (pRecordDialog != NULL) {
        // ESC-el lépett ki, Kideríteni mi ez, becsukja az ablakot, de nem lép ki a loop-bol.
        pDelete(pRecordDialog);
        // EXCEPTION(EPROGFAIL);
    }

    cRecord *pRec = actRecord(index);    // pointer az aktuális rekordra, a beolvasott/megjelenített rekord listában
    if (pRec == NULL) return;
    QString sModifyDialog = pTableShape->feature(_sModify);
    if (!sModifyDialog.isEmpty()) {
        pRec = objectDialog(sModifyDialog, *pq, this->pWidget(), pRec, isReadOnly, true);
        if (pRec == NULL) return;
        pDelete(pRec);
        refresh();
        return;
    }

    pRec = pRec->dup();           // Saját másolat
    int buttons;
    if (isReadOnly) {
        buttons = enum2set(DBT_CANCEL, DBT_NEXT, DBT_PREV);
    }
    else {
        buttons = enum2set(DBT_OK, DBT_CANCEL, DBT_NEXT, DBT_PREV);
        if (recDescr().toIndex(_sAcknowledged, EX_IGNORE) >= 0)  {
            buttons |= enum2set(DBT_RECEIPT);
        }
    }

    cRecordDialogBase *pRd  = NULL;
    cRecordDialogInh * pRdt = NULL;
    cTableShape *    pShape = NULL;
    tRecordList<cTableShape> * pShapes = NULL;

    // Létrehozzuk a dialogot
    switch (tableInhType) {
    case TIT_NO:
    case TIT_ONLY:
        pShape = getInhShape(pTableShape, pRec->descr());
        pRd = new cRecordDialog(*pShape, buttons, true, NULL, this, pWidget());
        pRecordDialog = pRd;
        break;
    case TIT_LISTED_REV: {
        pShapes = getShapes();
        pRdt = new cRecordDialogInh(*pTableShape, *pShapes, buttons, true, NULL, this, pWidget());
        pRecordDialog = pRdt;
        if (pRdt->disabled()) {
            pRd = pRdt;     // Mégsincs tab widget
            pRdt= NULL;     // nincs választás, csak egy üzenet
        }
    }   break;
    default:
        pDelete(pRec);
        pRecordDialog = NULL;
        if (__ex > EX_IGNORE) {
            EXCEPTION(ENOTSUPP);
        }
        return;
    }

    int id = DBT_NEXT;
    while (1) {
        // Hívjuk a megfelelő dialogot
        if (pRdt != NULL) {
            int i, n = pShapes->size();
            bool e = false;
            for (i = 0; i < n; i++) {   // Csak egy tabot engedélyezünk !
                bool f = pRec->tableName() == pShapes->at(i)->getName(_sTableName);
                pRdt->setTabEnabled(i, f);
                if (e && f) EXCEPTION(EPROGFAIL);   // Ha több tabot kéne az gáz
                if (f) {
                    e = f;
                    pRdt->setActTab(i);
                }
            }
            if (!e) EXCEPTION(EPROGFAIL);   // Ha egyet sem az is gáz
            pRd = pRdt->actPDialog();
        }
        pRd->restore(pRec);     // lemásolja a dialogus saját adatterületére
        if (pRdt != NULL) id = pRdt->exec(false);
        else              id =  pRd->exec(false);
        if (pRd->disabled()) id = DBT_CANCEL;
        // Ellenörzés, következő/előző, vagy kilép
        int updateResult = 0;
        switch(id) {
        case DBT_OK:
        case DBT_NEXT:
        case DBT_PREV:
        case DBT_RECEIPT: {
            updateResult = 1;
            if (id == DBT_RECEIPT) {    // Nem szerkeszthető, a rekord nyugtázása
                if (false == pRec->getBool(_sAcknowledged)) {
                    pRec->setOn(_sAcknowledged);
                    updateResult = pModel->updateRec(index, pRec);
                    if (!updateResult) {
                        continue;
                    }
                    // Nincs felszabadítva, de már nem a mienk (betettük a rekord listába, régi elem felszabadítva)
                    pRec = NULL;
                }
                else {
                    pDelete(pRec);
                }
                id = DBT_NEXT;
            }
            else if (isReadOnly) {
                pDelete(pRec);
            }
            else {
                // Update DB
                bool r = pRecordDialog->accept(); // Bevitt adatok rendben?
                if (!r) {
                    // Nem ok az adat
                    QMessageBox::warning(pWidget(), trUtf8("Adat hiba"), pRd->errMsg());
                    // Újra
                    continue;
                }
                else {
                    // Leadminisztráljuk kiírjuk. Ha hiba van, azt a hívott metódus kiírja, és újrázunk.
                    pRec->copy(pRecordDialog->record());
                    updateResult = pModel->updateRec(index, pRec);
                    if (!updateResult) {
                        continue;
                    }
                    // Nincs felszabadítva, de már nem a mienk (betettük a rekord listába, régi elem felszabadítva)
                    pRec = NULL;
                }
            }
            // pRec == NULL
            if (id == DBT_OK) {
                // A row-select-et helyreállítjuk (updateRow elszúrhatta
                if (index.isValid()) {
                    selectRow(index);
                }
                break;    // OK: vége
            }
            // A nexRow prevRow egy allokált objektumot ad vissza a bemásolt adattartalommal.
            if (id == DBT_NEXT) {       // A következő szerkesztése
                pRec = nextRow(&index, updateResult);
            }
            else {                      // Az előző szerkesztése
                pRec = prevRow(&index, updateResult);
            }
            if (pRec == NULL) break;    // Nem volt előző/következő

            continue;
        }
        case DBT_CANCEL:
            pDelete(pRec);
            break;
        default:
            EXCEPTION(EPROGFAIL);
            break;
        }
        break;
    }
    if (pRdt == NULL) pDelete(pRd);
    else              pDelete(pRdt);
    pDelete(pShape);
    pDelete(pShapes);
    pDelete(pRec);
    pRecordDialog = NULL;
}

void cRecordsViewBase::first() { EXCEPTION(ENOTSUPP); }
void cRecordsViewBase::prev()  { EXCEPTION(ENOTSUPP); }
void cRecordsViewBase::next()  { EXCEPTION(ENOTSUPP); }
void cRecordsViewBase::last()  { EXCEPTION(ENOTSUPP); }

void cRecordsViewBase::remove()
{
    DBGFN();
    QModelIndexList mil = selectedRows();
    pModel->removeRecords(mil);
}

void cRecordsViewBase::reset()      { EXCEPTION(ENOTSUPP); }
void cRecordsViewBase::putIn()      { EXCEPTION(ENOTSUPP); }
void cRecordsViewBase::takeOut()    { EXCEPTION(ENOTSUPP); }

void cRecordsViewBase::copy()
{
    QModelIndexList mil = selectedRows();
    if (mil.size() > 0) {
        QString list;
        foreach (QModelIndex mi, mil) {
            cRecord *pr = pModel->record(mi);
            if (pr == NULL) continue;
            list += quotedString(pr->getName()) + ", ";
        }
        list.chop(2);
        QApplication::clipboard()->setText(list);
    }
}

void cRecordsViewBase::receipt()
{
    QModelIndexList mil = selectedRows();
    int ix = recDescr().toIndex(_sAcknowledged);
    bool mod = false;
    foreach (QModelIndex mi, mil) {
        cRecord * pr = pModel->record(mi);
        if (pr->getBool(ix)) continue;
        pr->setOn(ix);
        if (cErrorMessageBox::condMsgBox(pr->tryUpdate(*pq, false, _bit(ix)), pWidget())) {
            mod = true;
            continue;
        }
        break;
    }
    if (mod) refresh();
}

void cRecordsViewBase::truncate()
{
    cRecordAny r(&recDescr());
    if (cErrorMessageBox::condMsgBox(r.tryRemove(*pq, false, QBitArray(1, false)), pWidget())) {
        refresh();
    }
}

void cRecordsViewBase::report()
{
    QModelIndexList mil = selectedRows();
    if (mil.size() == 1) {       // Report single record
        cRecord *po = actRecord();
        QString name = pTableShape->feature(_sReport);
        tStringPair sp = htmlReport(*pq, *po, name, pTableShape);
        popupReportWindow(_pWidget, sp.second, sp.first);
    }
}

qlonglong cRecordsViewBase::actId(eEx __ex)
{
    cRecord *p = actRecord();
    if (p == NULL) return NULL_ID;
    if (p->idIndex(__ex) >= 0) return p->getId();
    return NULL_ID;
}


void cRecordsViewBase::initView()
{
    tableInhType = (eTableInheritType)pTableShape->getId(_sTableInheritType);
    if (tableInhType == TIT_NO) return;     // Nincs öröklés, nem kell view
    inheritTableList = pTableShape->get(_sInheritTableNames).toStringList();
    if (inheritTableList.isEmpty()) return; // Üres a lista, mégnincs öröklés
    QString schema = pTableShape->getName(_sSchemaName);
    viewName = pTableShape->getName(_sTableName) + "_view"; // Az ideiglenes view tábla neve
    QString sql = "CREATE OR REPLACE TEMP VIEW " + viewName + " AS ";
    sql += "SELECT tableoid, * FROM ONLY";
    sql += recDescr().fullTableNameQ();
    pInhRecDescr = new QMap<qlonglong,const cRecStaticDescr *>;
    foreach (QString tn, inheritTableList) {
        const cRecStaticDescr * p = cRecStaticDescr::get(tn, schema);
        pInhRecDescr->insert(p->tableoid(), p);
        sql += "\nUNION ALL\n SELECT tableoid";
        const cRecStaticDescr& d = inhRecDescr(tn);
        int i, n = recDescr().cols();
        for (i = 0; i < n; ++i) {   // Végihrohanunk az első tábla mezőin
            QString fn = recDescr().columnName(i);
            sql += _sCommaSp;
            sql += d.toIndex(fn, EX_IGNORE) < 0 ? _sNULL : fn;  // Ha nincs akkor NULL
        }
        sql += " FROM ONLY " + p->fullTableNameQ();
    }
    PDEB(VVERBOSE) << "Create view : " << sql << endl;
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
}

void cRecordsViewBase::initShape(cTableShape *pts)
{
    if (pts != NULL) pTableShape = pts;
    setObjectName(pTableShape->getName());
    if ((pTableShape->containerValid & ~CV_LL_TEXT) == 0) {
        pTableShape->fetchText(*pq);
    }

    pTableShape->setParent(this);

    if (pTableShape->shapeFields.isEmpty() && 0 == pTableShape->fetchFields(*pq))
        EXCEPTION(EDATA, pTableShape->getId(),
                  trUtf8("A %1 nevű táblában nincs egyetlen oszlop sem.").arg(pTableShape->getName()));

    pRecDescr = cRecStaticDescr::get(pTableShape->getName(_sTableName));
    // Extra típus értékek miatt nem használható a mező alap konverziós metódusa !
    shapeType = enum2set(tableShapeType, pTableShape->get(_sTableShapeType).toStringList(), EX_IGNORE);
    isReadOnly =  ENUM2SET(TS_READ_ONLY) & shapeType;
    shapeType &= ~ENUM2SET(TS_READ_ONLY);
    isReadOnly = isReadOnly || false == pRecDescr->isUpdatable();
    isReadOnly = isReadOnly || false == lanView::isAuthorized(pTableShape->getId(_sEditRights));
    isNoDelete = isReadOnly || false == lanView::isAuthorized(pTableShape->getId(_sRemoveRights));
    isNoInsert = isReadOnly || false == lanView::isAuthorized(pTableShape->getId(_sInsertRights));

    if (shapeType & ENUM2SET(TS_DIALOG))
        EXCEPTION(EDATA, 0, trUtf8("Táblázatos megjelenítés az arra alkalmatlan %1 nevű leíróval.").arg(pts->getName()));

    tTableShapeFields::iterator i, n = pTableShape->shapeFields.end();
    for (i = pTableShape->shapeFields.begin(); i != n; ++i) {
        cError *pe = NULL;
        cRecordTableColumn *p = NULL;
        try {
            p = new cRecordTableColumn(**i, *this);
        } CATCHS(pe)
        if (pe == NULL) {
            fields << p;
        }
        else {
            cErrorMessageBox::messageBox(pe, pWidget(), trUtf8("Column is ignored."));
            delete pe;
        }
    }
    initView();
    // Ha egy egyszerű táblát használnánk al táblaként, nem szívózunk, beállítjuk
    if (pUpper != NULL) {
        qlonglong st = shapeType & ~ENUM2SET2(TS_TABLE, TS_READ_ONLY);
        if (st == ENUM2SET(TS_SIMPLE) || st == ENUM2SET(TS_TREE)) {
            shapeType = (shapeType & ~ENUM2SET(TS_SIMPLE)) | ENUM2SET(TS_CHILD);
        }
    }
}

cRecordsViewBase *cRecordsViewBase::newRecordView(cTableShape *pts, cRecordsViewBase * own, QWidget *par)
{
    cRecordsViewBase *r;
    qlonglong type = pts->getId(_sTableShapeType);
    if ((type & ENUM2SET(TS_MEMBER)) && own != NULL) { // EXCEPTION(ENOTSUPP);
        pts->setId(_sTableShapeType, type &~ENUM2SET(TS_MEMBER));   // Inkább töröljük
    }
    if ((type & ENUM2SET(TS_GROUP))  && own != NULL) { // EXCEPTION(ENOTSUPP);
        pts->setId(_sTableShapeType, type &~ENUM2SET(TS_GROUP));
    }
    if (type & ENUM2SET(TS_TREE)) {
        r = new cRecordTree( pts, false, own, par);
    }
    else if (type & ENUM2SET(TS_LINK)) {
        r = new cRecordLink(pts, false, own, par);
    }
    else {
        r = new cRecordTable(pts, false, own, par);
    }
    r->init();
    return r;
}

int cRecordsViewBase::ixToOwner()
{
    if (pUpper == NULL) EXCEPTION(EPROGFAIL);
    QString key = mCat(pUpper->pTableShape->getName(), _sOwner);
    QString ofn = pTableShape->feature(key);
    int r;
    if (ofn.isEmpty()) {    // Ki kell találni, nincs megadva a features mezőben
        r = recDescr().ixToOwner(pUpper->recDescr().tableName(), EX_IGNORE);
        if (r < 0) {
            QString msg = trUtf8(
                    "A %1 al tábla nézetben (%2 tábla)\n a tulajdonos objektum táblára "
                    "(nézet : %3, tábla %4) mutató ID mező neve (idegen kilcs) nem állpítható meg. "
                    "A tábla nézetben a %5 feature változóban kell megadni a mező nevét, "
                    "ha nincs definiált a távoli kulcs mint owner, és a használandó távoli kulcs így nem állapítható meg.")
                    .arg(pTableShape->getName(), pTableShape->getName(_sTableName),
                         pUpper->pTableShape->getName(), pUpper->pTableShape->getName(_sTableName),
                         key);
            EXCEPTION(EFOUND, 0, msg);
        }
    }
    else {  // A features mezőben definiáltuk
        r = recDescr().toIndex(ofn, EX_IGNORE);
        if (r < 0) {
            QString msg = trUtf8(
                    "A %1 al tábla nézetben (%2 tábla) a %3 feature változó értéke %4. "
                    "Nincs ilyen nevű mező! A változónak a tulajdonos táblára "
                    "(nézet : %5, tábla %6) mutató mező nevét (idegen kulcs) kellene deifiniálnia.")
                    .arg(pTableShape->getName(), pTableShape->getName(_sTableName), key, ofn,
                         pUpper->pTableShape->getName(), pUpper->pTableShape->getName(_sTableName));
            EXCEPTION(EFOUND, 0, msg);
        }
    }
    return r;
}


cRecordsViewBase *cRecordsViewBase::newRecordView(QSqlQuery& q, qlonglong shapeId, cRecordsViewBase * own, QWidget *par)
{
    cTableShape *pts = new cTableShape();
    if (!pts->fetchById(q, shapeId)) EXCEPTION(EFOUND, shapeId);
    return newRecordView(pts, own, par);
}

void cRecordsViewBase::createRightTab()
{
    pRightTabWidget = new QTabWidget();
    pMasterSplitter->addWidget(pRightTabWidget);
}

tRecordList<cTableShape> *cRecordsViewBase::getShapes()
{
    tRecordList<cTableShape>   *pShapes = new tRecordList<cTableShape>;
    QStringList tableNames;
    tableNames << pTableShape->getName(_sTableName);
    tableNames << inheritTableList;
    tableNames.removeDuplicates();
    foreach (QString tableName, tableNames) {
        *pShapes << getInhShape(pTableShape, tableName);
    }
    return pShapes;
}

void cRecordsViewBase::rightTabs(QVariantList& vlids)
{
    foreach (QVariant vid, vlids) {
        bool ok;
        qlonglong id = vid.toLongLong(&ok);
        if (!ok) EXCEPTION(EDATA);
        cRecordsViewBase *prvb = cRecordsViewBase::newRecordView(*pq, id, this);
        prvb->setParent(this);
        *pRightTables << prvb;
        pRightTabWidget->addTab(prvb->pWidget(),
                                prvb->tableShape().getText(cTableShape::LTX_TABLE_TITLE,
                                                        prvb->tableShape().getName()));
    }
}

/// Inicializálja a megjelenítést.
/// Az initSimple() metódust hívja, de létrehozza a splitter widgetet, hogy több táblát lehessen megjeleníteni.
/// A legfelső szintű splitter orientációja két féle lehet, hogy elférjrn kisebb képernyőn is.
/// A második szint (több csak elvileg lehet) mindíg egymás melletti.
///
/// Ha csak egy (child) táblázat van a jobb oldalon:
/// \diafile    record_table2.dia "Egy child tábla" width=8cm
/// Ha több (child) táblázat van a jobb oldalon, akkor azok egy tab-ba kerülnek:
/// \diafile    record_table3.dia "Több child tábla" width=8cm
void cRecordsViewBase::initMaster()
{
    cRecordsViewBase *pRightTable = NULL;
    qlonglong id;
    QVariantList vlids;
    bool ok;

    pMasterLayout = new QHBoxLayout(_pWidget);
    if (pUpper == NULL) pMasterSplitter = new QSplitter(lv2g::getInstance()->defaultSplitOrientation);
    else                pMasterSplitter = new QSplitter(Qt::Horizontal);
    pMasterLayout->addWidget(pMasterSplitter);

    pLeftWidget   = new QWidget();
    initSimple(pLeftWidget);
    pMasterSplitter->addWidget(pLeftWidget);

    vlids = pTableShape->get(_sRightShapeIds).toList(); // Jobb oldali (child) lista (ID)
    if (vlids.isEmpty()) EXCEPTION(EDATA, 0, trUtf8("A jobboldali táblákat azonosító tömb üres."));
    pRightTables = new tRecordsViewBaseList;    // A jobb oldali elemek listája (objektum)
    if ((flags & (RTF_MEMBER | RTF_GROUP))) {   // Az első elem esetén lehet Group/member táblák
        createRightTab();                       // Ez eleve két tábla a jobb oldalon, tab widget kell.
        initGroup(vlids);                       // A két tag-nem tag tábla (egy table_shape objektum, vlids első eleme)
        rightTabs(vlids);                       // A maradék táblák, ha vannak (az első elem törölve a vlids listából)
    }
    else if (vlids.size() == 1) {               // Ha nem kell a tab widget
        id = vlids.at(0).toLongLong(&ok);
        if (!ok) EXCEPTION(EDATA);
        pRightTable = cRecordsViewBase::newRecordView(*pq, id, this);
        pRightTable->setParent(this);
        *pRightTables << pRightTable;
        pMasterSplitter->addWidget(pRightTable->pWidget());
    }
    else {
        createRightTab();
        rightTabs(vlids);
    }
}

void cRecordsViewBase::initGroup(QVariantList& vlids)
{
    qlonglong it, nt;

    // A jobboldali két tábla típusa
    if (flags & (RTF_MEMBER)) {
        it = ENUM2SET(TS_IGROUP);
        nt = ENUM2SET(TS_NGROUP);
    }
    else {  //   RTF_GROUP
        it = ENUM2SET(TS_IMEMBER);
        nt = ENUM2SET(TS_NMEMBER);
    }
    cRecordsViewBase *prvb = NULL;
    cTableShape *pts = new cTableShape();
    bool ok = false;
    qlonglong id = NULL_ID; // comp. warning
    if (vlids.size() > 0) id = vlids.at(0).toLongLong(&ok);   // A group, vagy member tábla a lista első eleme kell legyen !!!!!
    if (!ok) EXCEPTION(EPROGFAIL);
    vlids.pop_front();                  // A maradék lista, további chhild obj-ek
    pts->fetchById(*pq, id);            // Az első két jobb oldali tábla megjelenítését leíró (minta) rekord
    // Az első tábla
    pts->setShapeType(it);     // Itt ez a típus kell, az adatbázisban nem létező értékek
    prvb = cRecordsViewBase::newRecordView(dup(pts), this);
    prvb->setParent(this);
    *pRightTables << prvb;
    pRightTabWidget->addTab(prvb->pWidget(), prvb->tableShape().getText(cTableShape::LTX_MEMBER_TITLE));  // TITLE!!!!
    // A második tábla
    pts->setShapeType(nt);
    prvb = cRecordsViewBase::newRecordView(pts, this);
    prvb->setParent(this);
    *pRightTables << prvb;
    pRightTabWidget->addTab(prvb->pWidget(), prvb->tableShape().getText(cTableShape::LTX_NOT_MEMBER_TITLE));  // TITLE!!!!
}

QStringList cRecordsViewBase::filterWhere(QVariantList& qParams)
{
    if (pFODialog != NULL) return pFODialog->where(qParams);
    return QStringList();
}

void cRecordTable::setEditButtons()
{
    QModelIndexList mix = pTableView->selectionModel()->selectedRows();
    int n = mix.size();
    if (!isReadOnly) {
        buttonDisable(DBT_MODIFY,                n != 1);
        buttonDisable(DBT_TAKE_OUT,              n <  1);
        buttonDisable(DBT_PUT_IN,                n <  1);
        bool fi = isNoInsert || ((flags & (RTF_IGROUP | RTF_IMEMBER | RTF_CHILD)) && (owner_id == NULL_ID));
        buttonDisable(DBT_INSERT,  fi);
        buttonDisable(DBT_SIMILAR, fi || n != 1);
    }
    buttonDisable(DBT_DELETE,  isNoDelete || n <  1 );
    buttonDisable(DBT_RECEIPT, n <  1);
    buttonDisable(DBT_COPY,    n <  1);
}


/// Üres, nem kötelezően implemetálandó. Csak ha van lapozási lehetőség
void cRecordsViewBase::setPageButtons()
{
    ;
}

void cRecordsViewBase::setButtons()
{
    setEditButtons();
    setPageButtons();
}


QStringList cRecordsViewBase::refineWhere(QVariantList& qParams)
{
    QStringList r;
    if (! pTableShape->isNull(_sRefine)) {  // Ha megadtak egy általános érvényű szűrőt
        QString refine = pTableShape->getName(_sRefine);
        if (refine.isEmpty()) return r;     // Mégsincs megadva, üres
        QStringList rl = splitBy(refine);
        r << rl.at(0);
        if ((rl.at(0).count(QChar('?')) + 1) != rl.size())
            EXCEPTION(EDATA, -1, trUtf8("Inkonzisztens adat; refine = %1").arg(rl.join(QChar(':'))));
        for (int i = 1; i < rl.size(); ++i) {  // Paraméter helyettesítések...
            QString n = rl.at(i);
            if      (0 == n.compare(_sUserId,         Qt::CaseInsensitive)) qParams << QVariant(lanView::user().getId());
            else if (0 == n.compare(_sUserName,       Qt::CaseInsensitive)) qParams << QVariant(lanView::user().getName());
            else if (0 == n.compare(_sPlaceGroupId,   Qt::CaseInsensitive)) qParams << QVariant(lv2g::getInstance()->zoneId);
            else if (0 == n.compare(_sPlaceId,        Qt::CaseInsensitive)) qParams << QVariant(lanView::user().getId(_sPlaceId));
            else if (0 == n.compare(_sNodeId,         Qt::CaseInsensitive)) qParams << QVariant(lanView::getInstance()->selfNode().getId());
            else EXCEPTION(EDATA, i, trUtf8("Ismeretlen változónév a refine mezőben %1").arg(n));
        }
    }
    return r;
}

QStringList cRecordsViewBase::where(QVariantList& qParams)
{
    DBGFN();
    QStringList wl;
    int f = flags & (RTF_CHILD | RTF_IGROUP | RTF_NGROUP | RTF_IMEMBER | RTF_NMEMBER);
    if (f) { // A tulaj ID-jére szűrünk, ha van
        if (pUpper == NULL) EXCEPTION(EPROGFAIL);
        if (owner_id == NULL_ID) {  // A tulajdonos/tag rekord nincs kiválasztva
            wl << _sFalse;      // Ezzel jelezzük, hogy egy üres táblát kell megjeleníteni
            return wl;          // Ez egy üres tábla lessz!!
        }
        switch (f) {
        case RTF_CHILD: {
            int ofix = ixToOwner();
            wl << dQuoted(recDescr().columnName(ofix)) + " = " + QString::number(owner_id);
        }   break;
        case RTF_IGROUP:
        case RTF_NGROUP: {
            cGroupAny   g(&recDescr(), &pUpper->recDescr());
            QString w =
                    "EXISTS (SELECT 1 FROM " + g.tableName() +
                    " WHERE " + g.groupIdName() + " = " + mCat(g.group.tableName(), g.groupIdName()) +
                    " AND "  + g.memberIdName() + " = " + QString::number(owner_id) + ")";
            if (f == RTF_NGROUP) w = "NOT " + w;
            wl << w;
        }   break;
        case RTF_IMEMBER:
        case RTF_NMEMBER: {
            cGroupAny   g(&pUpper->recDescr(), &recDescr());
            QString w =
                    "EXISTS (SELECT 1 FROM " + g.tableName() +
                    " WHERE " + g.memberIdName() + " = " + mCat(g.member.tableName(), g.memberIdName()) +
                    " AND "  + g.groupIdName() + " = " + QString::number(owner_id) + ")";
            if (f == RTF_NMEMBER) w = "NOT " + w;
            wl << w;
        }   break;
        default:
            EXCEPTION(EPROGFAIL);
            break;
        }
    }
    wl << filterWhere(qParams);
    wl << refineWhere(qParams);
    DBGFNL();
    return wl;
}

bool cRecordsViewBase::enabledBatchEdit(const cTableShapeField& tsf)
{
    // While it is incorrect in case of inheritance, it is disabled
    if (tableInhType != TIT_NO) return false;
    if (!tsf.getBool(_sFieldFlags, FF_BATCH_EDIT)) return false;            // Mezőnként kell engedélyezni
    if (lanView::isAuthorized(PL_ADMIN)) return true;                       // ADMIN-nak ok
    ePrivilegeLevel pl = (ePrivilegeLevel)privilegeLevel(tsf.feature(_sBatchEdit), EX_IGNORE);
    return lanView::isAuthorized(pl);
}

bool cRecordsViewBase::batchEdit(int logicalindex)
{
    if (!isContIx(pModel->_col2field, logicalindex)) return true;           // Unknown field index
    QList<int> dataFieldIndexList;
    QList<int> shapeFieldIndexList;
    dataFieldIndexList  << pModel->_col2field[logicalindex];
    shapeFieldIndexList << logicalindex;
    const cTableShapeField& tsf = *pTableShape->shapeFields[shapeFieldIndexList.first()];
    if (!enabledBatchEdit(tsf)) return true;
    QModelIndexList mil = selectedRows();
    if (mil.isEmpty()) return true;                                         // nincs kijelölve rekord

    QDialog *pDialog = new QDialog(pWidget());
    Ui_FieldEditDialog *pEd = new Ui_FieldEditDialog;
    pEd->setupUi(pDialog);
    cRecordAny rec(&recDescr());
    QBitArray setMask = _mask(rec.cols(), dataFieldIndexList.first());
    // Addicionális együtt kezelendő mezők
    QString ff = tsf.feature("batch_edit_fields");
    if (!ff.isEmpty()) {
        QStringList fl = ff.split(QRegExp(",\\s*"));
        foreach (QString f, fl) {
            int fix = rec.toIndex(f);
            int mix = pTableShape->shapeFields.indexOf(f);
            dataFieldIndexList  << fix;
            shapeFieldIndexList << mix;
            setMask[fix] = true;
        }
    }

    QList<cFieldEditBase *> febList;
    for (int i = 0; i < dataFieldIndexList.size(); ++i) {
        cRecordFieldRef rfr = rec[dataFieldIndexList[i]];
        const cTableShapeField& sf = *pTableShape->shapeFields[shapeFieldIndexList[i]];
        cFieldEditBase *feb = cFieldEditBase::createFieldWidget(*pTableShape, sf, rfr, false, NULL);
        febList << feb;
        pEd->verticalLayout->insertWidget(i * 2, feb);
        QLabel *pLabel = new QLabel(sf.getText(cTableShapeField::LTX_DIALOG_TITLE));
        pEd->verticalLayout->insertWidget(i * 2, pLabel);
    }
    // Ha modosítottuk a táblát, majd volt rollback
    bool    spoiling = false;
    static const QString tn = "batchEdit";
    while (pDialog->exec() == QDialog::Accepted) {
        sqlBegin(*pq, tn);
        cError  *pe = NULL;
        bool first = true;
        foreach (QModelIndex mi, mil) {
            selectRow(mi);
            cRecord *ar = actRecord();
            try {
                for (int i = 0; i < dataFieldIndexList.size(); ++i) {
                    QVariant v = febList[i]->get();
                    ar->set(dataFieldIndexList[i], v);
                }
                ar->update(*pq, false, setMask);
            } CATCHS(pe);
            if (pe != NULL) {
                spoiling = !first;
                break;
            }
            first = false;
        }
        if (pe != NULL) {
            cErrorMessageBox::messageBox(pe, pDialog);
            pDelete(pe);
            sqlRollback(*pq, tn);
            continue;
        }
        sqlCommit(*pq, tn);
        spoiling = false;
        break;
    }
    delete pDialog;
    // modosítottunk, frissíteni kell
    if (spoiling) {
        refresh();
    }

    return false;
}

void cRecordsViewBase::clickedHeader(int logicalindex)
{
    if (batchEdit(logicalindex)) {
        if (pFODialog == NULL) EXCEPTION(EPROGFAIL);
        int r = pFODialog->exec();
        if (r == DBT_OK) refresh();
    }
}

void cRecordsViewBase::selectionChanged(QItemSelection,QItemSelection)
{
    if (flags & (RTF_OVNER | RTF_MEMBER | RTF_GROUP)) {
        qlonglong _actId = NULL_ID;
        if (pRightTables == NULL) {                              // A konstruktorból hívva még lehet NULL
            if (selectedRows().size() > 0)EXCEPTION(EPROGFAIL); // Ha van kijelölt sor, nem valószínű hogy konstruktorban vagyunk.
        }
        else {
            if (selectedRows().size() == 1) _actId = actId();
            foreach (cRecordsViewBase *pRightTable, *pRightTables) {
                pRightTable->owner_id = _actId;
                qlonglong __actId = pRightTable->actId(EX_IGNORE);
                pRightTable->refresh();
                if (__actId != pRightTable->actId(EX_IGNORE)) {
                    pRightTable->selectionChanged(QItemSelection(), QItemSelection());
                }
            }
        }
    }
    setEditButtons();
}

void cRecordsViewBase::modifyByIndex(const QModelIndex & index)
{
    //selectRow(index);
    (void)index;
    modify(EX_IGNORE);
}

void cRecordsViewBase::hideColumns(void)
{
    for (int i = 0; i < fields.size(); ++i) {
        hideColumn(i, field(i).fieldFlags & ENUM2SET(FF_TABLE_HIDE));
    }
}

/* ----------------------------------------------------------------------------------------------------- */

cRecordTable::cRecordTable(const QString& _mn, bool _isDialog, QWidget * par)
    : cRecordsViewBase(_isDialog, par)
{
    pTableShape = new cTableShape();
    if (!pTableShape->fetchByName(*pq, _mn)) EXCEPTION(EDATA,-1,_mn);
    initShape();
}

cRecordTable::cRecordTable(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordsViewBase(_isDialog, par)
{
    pMaster = pUpper = _upper;
    if (pMaster != NULL && pUpper->pMaster != NULL) pMaster = pUpper->pMaster;
    initShape(pts);
}


cRecordTable::~cRecordTable()
{
}

void cRecordTable::init()
{
    pTimer = NULL;
    pTableView = NULL;
    // Az alapértelmezett gombok:
    buttons << DBT_CLOSE << DBT_SPACER;
    if (pTableShape->isFeature(_sButtonCopy) || pTableShape->isFeature(_sReport)) {
        if (pTableShape->isFeature(_sButtonCopy)) buttons << DBT_COPY;
        if (pTableShape->isFeature(_sReport))     buttons << DBT_REPORT;
        buttons << DBT_SPACER;
    }
    buttons << DBT_REFRESH << DBT_FIRST << DBT_PREV << DBT_NEXT << DBT_LAST;
    if (recDescr().toIndex(_sAcknowledged, EX_IGNORE) >= 0)  {
        buttons << DBT_SPACER << DBT_RECEIPT;
    }
    if (isReadOnly == false) {
        buttons << DBT_BREAK << DBT_SPACER << DBT_DELETE << DBT_INSERT << DBT_SIMILAR << DBT_MODIFY;
    }
    flags = 0;
    if (pUpper != NULL && shapeType < ENUM2SET(TS_LINK)) shapeType |= ENUM2SET(TS_CHILD);
    switch (shapeType & ~ENUM2SET2(TS_TABLE, TS_READ_ONLY)) {
    case ENUM2SET(TS_BARE):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags = RTF_SLAVE;
        buttons.pop_front();    // A close nem kell
        initSimple(_pWidget);
        break;
    case 0:
    case ENUM2SET(TS_SIMPLE):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags = RTF_SINGLE;
        initSimple(_pWidget);
        break;
    case ENUM2SET2(TS_OWNER, TS_MEMBER):
        flags = RTF_OVNER;
    case ENUM2SET(TS_MEMBER):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags |= RTF_MASTER | RTF_MEMBER;
        initMaster();
        break;
    case ENUM2SET2(TS_OWNER, TS_GROUP):
        flags = RTF_OVNER;
    case ENUM2SET(TS_GROUP):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags |= RTF_MASTER | RTF_GROUP;
        initMaster();
        break;
    case ENUM2SET(TS_OWNER):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags = RTF_MASTER | RTF_OVNER;
        initMaster();
        break;
    case ENUM2SET(TS_IGROUP):
    case ENUM2SET(TS_NGROUP):
    case ENUM2SET(TS_IMEMBER):
    case ENUM2SET(TS_NMEMBER):
        if (pUpper == NULL) EXCEPTION(EDATA);
        if (tableInhType != TIT_NO && tableInhType != TIT_ONLY) EXCEPTION(EDATA);
        buttons.clear();
        buttons << DBT_REFRESH << DBT_FIRST << DBT_PREV << DBT_NEXT << DBT_LAST << DBT_SPACER;
        switch (shapeType) {
        case ENUM2SET(TS_IGROUP):
            flags = RTF_SLAVE | RTF_IGROUP;
            if (isReadOnly == false) buttons << DBT_BREAK << DBT_SPACER << DBT_TAKE_OUT << DBT_DELETE << DBT_INSERT << DBT_SIMILAR << DBT_MODIFY;
            break;
        case ENUM2SET(TS_NGROUP):
            flags = RTF_SLAVE | RTF_NGROUP;
            if (isReadOnly == false) buttons << DBT_BREAK << DBT_SPACER << DBT_INSERT << DBT_SIMILAR << DBT_PUT_IN;
            break;
        case ENUM2SET(TS_IMEMBER):
            flags = RTF_SLAVE | RTF_IMEMBER;
            if (isReadOnly == false) buttons << DBT_BREAK << DBT_SPACER << DBT_TAKE_OUT;
            break;
        case ENUM2SET(TS_NMEMBER):
            flags = RTF_SLAVE | RTF_NMEMBER;
            if (isReadOnly == false) buttons << DBT_BREAK << DBT_SPACER << DBT_PUT_IN;
            break;
        default:
            EXCEPTION(EPROGFAIL);
        }
        initSimple(_pWidget);
        break;
    case ENUM2SET(TS_CHILD):
    case ENUM2SET2(TS_CHILD, TS_LINK):
        if (pUpper == NULL) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_CHILD;
        buttons.pop_front();    // A close nem kell
        initSimple(_pWidget);
        break;
    case ENUM2SET2(TS_OWNER, TS_CHILD):
        flags = RTF_OVNER | RTF_SLAVE | RTF_CHILD;
        if (pUpper == NULL) EXCEPTION(EDATA);
        buttons.pop_front();    // A close nem kell
        initMaster();
        break;
    default:
        EXCEPTION(ENOTSUPP, pTableShape->getId(_sTableShapeType),
                  trUtf8("TABLE %1 SHAPE %2 TYPE : %3")
                  .arg(pTableShape->getName(),
                       pTableShape->getName(_sTableName),
                       pTableShape->getName(_sTableShapeType))
                  );
    }
    pTableView->horizontalHeader()->setSectionsClickable(true);
    connect(pTableView->horizontalHeader(), SIGNAL(sectionClicked(int)),       this, SLOT(clickedHeader(int)));
    connect(pTableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(modifyByIndex(QModelIndex)));
    // Auto refresh ?
    qlonglong ar = pTableShape->getId(_sAutoRefresh);
    if (ar > 0) {
        pTimer = new QTimer(this);
        connect(pTimer, SIGNAL(timeout()), this, SLOT(autoRefresh()));
        pTimer->setInterval(ar);
    }
/*  pTableView->setDragDropMode(QAbstractItemView::DragOnly);
    pTableView->setDragEnabled(true); */
    refresh();
}

/// Inicializálja a táblázatos megjelenítést
void cRecordTable::initSimple(QWidget * pW)
{
    pButtons    = new cDialogButtons(buttons);
    pMainLayout = new QVBoxLayout(pW);
    pTableView  = new QTableView();
    pModel      = new cRecordTableModel(*this);
    if (!pTableShape->getBool(_sTableShapeType, TS_BARE)) {
        QString title = pTableShape->getText(cTableShape::LTX_TABLE_TITLE, pTableShape->getName());
        if (title.size() > 0) {
            QLabel *pl = new QLabel(title);
            pMainLayout->addWidget(pl);
        }
    }
    pMainLayout->addWidget(pTableView);
    pMainLayout->addWidget(pButtons->pWidget());
    pTableView->setModel(pTableModel());

    pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QString style = tableShape().getName(_sStyleSheet);
    if (!style.isEmpty()) pTableView->setStyleSheet(style);

    connect(pButtons,    SIGNAL(buttonPressed(int)),   this, SLOT(buttonPressed(int)));
    connect(pTableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    if (pFODialog != NULL) EXCEPTION(EPROGFAIL);
    pFODialog = new cRecordTableFODialog(pq, *this);
    // A model konstruktorban nem megy az oszlopok eltüntetése, így mégegyszer...
    for (int i = 0; i < fields.size(); ++i) {
        hideColumn(i, field(i).fieldFlags & ENUM2SET(FF_TABLE_HIDE));
    }
}

void cRecordTable::hideColumn(int ix, bool f)
{
    if (f) {
        pTableView->hideColumn(ix);
    }
    else {
        pTableView->showColumn(ix);
    }
}

void cRecordTable::empty()
{
    pTableModel()->clear();
}



void cRecordTable::first()
{
    pTableModel()->qFirst();
    setButtons();
}

void cRecordTable::prev()
{
    pTableModel()->qPrev();
    setButtons();
}

void cRecordTable::next()
{
    pTableModel()->qNext();
    setButtons();
}

void cRecordTable::last()
{
    pTableModel()->qLast();
    setButtons();
}

void cRecordTable::putIn()
{
    if (pUpper == NULL || pUpper->pRightTables == NULL || pUpper->pRightTables->size() < 2) EXCEPTION(EPROGFAIL);

    cRecord *pM = NULL;
    cRecord *pG = NULL;
    if (((flags & RTF_NGROUP) != 0)) {
        pM = pUpper->actRecord();
        if (pM == NULL) {
            DERR() << trUtf8("Nincs kijelölve a tag rekord a baloldalon!") << endl;
            return;
        }
    }
    else if (((flags & RTF_NMEMBER) != 0)) {
        pG = pUpper->actRecord();
        if (pG == NULL) {
            DERR() << trUtf8("Nincs kijelölve a csoport rekord a baloldalon!") << endl;
            return;
        }
    }
    else {
        EXCEPTION(EPROGFAIL);
    }

    QModelIndexList mil = selectedRows();
    QBitArray rb = pTableModel()->index2map(mil);
    if (rb.count(true) == 0) {
        DERR() << trUtf8("Nincs kijelölve egy csoport vagy tag rekord sem a jobboldalon!") << endl;
        return;
    }

    for (int i = rb.size() -1; i >= 0; i--) {
        bool f = rb[i];
        if (!f) continue;
        if (((flags & RTF_NGROUP) != 0)) {
            pG = record(i);
        }
        else {
            pM = record(i);
        }
        cGroupAny(*pG, *pM).insert(*pq);
        pModel->removeRow(pTableModel()->index(i, 0));
    }
    pUpper->pRightTables->at(0)->refresh();
}

void cRecordTable::takeOut()
{
    if (pUpper == NULL || pUpper->pRightTables == NULL || pUpper->pRightTables->size() < 2) EXCEPTION(EPROGFAIL);

    cRecord *pM = NULL;
    cRecord *pG = NULL;
    if (((flags & RTF_IGROUP) != 0)) {
        pM = pUpper->actRecord();
        if (pM == NULL) {
            DERR() << trUtf8("Nincs kijelölve a tag rekord a baloldalon!") << endl;
            return;
        }
    }
    else if (((flags & RTF_IMEMBER) != 0)) {
        pG = pUpper->actRecord();
        if (pG == NULL) {
            DERR() << trUtf8("Nincs kijelölve a csoport rekord a baloldalon!") << endl;
            return;
        }
    }
    else {
        EXCEPTION(EPROGFAIL);
    }

    QModelIndexList mil = selectedRows();
    QBitArray rb = pTableModel()->index2map(mil);
    if (rb.count(true) == 0) {
        DERR() << trUtf8("Nincs kijelölve egy csoport vagy tag rekord sem a jobboldalon!") << endl;
        return;
    }

    for (int i = rb.size() -1; i >= 0; i--) {
        bool f = rb[i];
        if (!f) continue;
        if (((flags & RTF_IGROUP) != 0)) {
            pG = record(i);
        }
        else {
            pM = record(i);
        }
        cGroupAny(*pG, *pM).remove(*pq);
        pModel->removeRow(pTableModel()->index(i, 0));
    }
    pUpper->pRightTables->at(1)->refresh();
}

#define MAXMAXROWS 100000

void cRecordTable::copy()
{
    cTableExportDialog  dialog(pWidget());
    if (QDialog::Accepted != dialog.exec()) return;
    enum eTableExportWhat   w = dialog.what();
    enum eTableExportTarget t = dialog.target();
    enum eTableExportForm   f = dialog.form();
    QString r;
    switch (w) {
    case TEW_NAME:
        cRecordsViewBase::copy();
        return;
    case TEW_SELECTED:
        switch (f) {
        case TEF_CSV:   r = pTableModel()->toCSV(selectedRows());     break;
        case TEF_HTML:  r = pTableModel()->toHtml(selectedRows());    break;
        default:        EXCEPTION(EPROGFAIL);
        }
        break;
    case TEW_VIEWED:
        switch (f) {
        case TEF_CSV:   r = pTableModel()->toCSV();     break;
        case TEF_HTML:  r = pTableModel()->toHtml();    break;
        default:        EXCEPTION(EPROGFAIL);
        }
        break;
    case TEW_ALL: {
        int mr = pTableModel()->maxRows();
        pTableModel()->_maxRows = MAXMAXROWS;
        refresh(true);
        switch (f) {
        case TEF_CSV:   r = pTableModel()->toCSV();     break;
        case TEF_HTML:  r = pTableModel()->toHtml();    break;
        default:        EXCEPTION(EPROGFAIL);
        }
        pTableModel()->_maxRows = mr;
        break;
    }
    default:
        EXCEPTION(EPROGFAIL);
    }
    if (r.isEmpty()) {
        QMessageBox::warning(pWidget(), dcViewShort(DC_ERROR), trUtf8("Nincs adat."));
        return;
    }
    switch (t) {
    case TET_CLIP:
        QApplication::clipboard()->setText(r);
        break;
    case TET_FILE: {
        QString path = dialog.path();
        while (!path.isEmpty()) {
            QFile f(path);
            if (f.open(QIODevice::WriteOnly)) {
                if (0 < f.write(r.toUtf8())) {
                    f.close();
                    return;
                }
            }
            QMessageBox::warning(pWidget(), dcViewShort(DC_ERROR), trUtf8("Hiba a fájl kiírásánál."));
            path = dialog.getOtherPath();
        }
    }
        break;
    case TET_WIN:
        popupReportWindow(this->pWidget(), r, tableShape().getText(cTableShape::LTX_TABLE_TITLE));
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
}

void cRecordTable::report()
{
    tStringPair sp;
    QModelIndexList mil = selectedRows();
    if (mil.size() == 1) {       // Report single record
        cRecordsViewBase::report();
        return;
    }
    if (mil.size() > 0) {   // Report selected multiple record
        sp.first  = pTableShape->getText(cTableShape::LTX_TABLE_TITLE);
        sp.second = pTableModel()->toHtml(mil);
    }
    else {
        sp.first  = pTableShape->getText(cTableShape::LTX_TABLE_TITLE);
        sp.second = pTableModel()->toHtml();
    }
    popupReportWindow(_pWidget, sp.second, sp.first);
}


void cRecordTable::_refresh(bool all)
{
    QString sql;
    QString only;
    if (tableInhType == TIT_NO || tableInhType == TIT_ONLY) only = "ONLY ";
    if (viewName.isEmpty()) sql = "SELECT NULL,* FROM " + only + pTableShape->getName(_sTableName);
    else                    sql = "SELECT * FROM " + only + viewName;
    QVariantList qParams;
    QStringList wl = where(qParams);
    if (!wl.isEmpty()) {
        if (wl.at(0) == _sFalse) {         // Ha üres..
            pTableModel()->clear();
            return;
        }
        sql += " WHERE " + wl.join(" AND ");
    }

    if (pFODialog != NULL) {
        QString ord = pFODialog->ord();
        if (!ord.isEmpty()) sql += " ORDER BY " + ord;
    }

    PDEB(SQL) << quoted(sql) << "\n bind(s) : \n" << list2string(qParams) << endl;
    if (!pTabQuery->prepare(sql)) SQLPREPERR(*pTabQuery, sql);
    int i = 0;
    foreach (QVariant v, qParams) pTabQuery->bindValue(i++, v);
    if (!pTabQuery->exec()) SQLQUERYERR(*pTabQuery);
    pTableModel()->setRecords(*pTabQuery, all);

    // for (int i = 0; i < fields.size(); ++i) {
    //     hideColumn(i, field(i).fieldFlags & ENUM2SET(FF_TABLE_HIDE));
    // }

    pTableView->resizeColumnsToContents();
}

bool cRecordTable::batchEdit(int logicalindex)
{
    QModelIndexList mil = selectedRows();   // save select
    if (cRecordsViewBase::batchEdit(logicalindex)) return true;
    if (mil.size() > 1) {                   // restore select
        pTableView->clearSelection();
        pTableView->setSelectionMode(QAbstractItemView::MultiSelection);
        foreach (QModelIndex mi, mil) {
            pTableView->selectRow(mi.row());
        }
        pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
    return false;
}


QModelIndexList cRecordTable::selectedRows()
{
    return pTableView->selectionModel()->selectedRows();
}

QModelIndex cRecordTable::actIndex()
{
    QModelIndexList mil = pTableView->selectionModel()->selectedRows();
    if (mil.size() != 1 ) {
        DERR() << endl;
        return QModelIndex();
    }
    return mil.first();
}


cRecord *cRecordTable::actRecord(const QModelIndex &_mi)
{
    QModelIndex mi = _mi;
    if (!mi.isValid()) mi = actIndex();
    if (mi.isValid() && isContIx(pTableModel()->records(), mi.row())) return pTableModel()->records()[mi.row()];
    return NULL;
}

cRecord *cRecordTable::nextRow(QModelIndex *pMi, int _upRes)
{
    (void)_upRes;
    if (pMi->isValid() == false) return NULL;    // Nincs aktív
    int row = pMi->row();
    row++;
    if (row == pTableModel()->size()) {
        if (!pTableModel()->qNextResult()) return NULL; // Az utolsó volt, nincs következő, kész
        pTableModel()->qNext();   // Lapozunk
        row = 0;
    }
    else if (!isContIx(pTableModel()->records(), row)) {
        DWAR() << "Invalid row : " << row << endl;
        *pMi = QModelIndex();
        return NULL;
    }
    *pMi = pTableModel()->index(row, pMi->column());
    if (!pMi->isValid()) {
        DWAR() << "Invalid index : " << row << endl;
        return NULL;
    }
    selectRow(*pMi);
    cRecord *pRec = actRecord(*pMi);
    if (pRec == NULL) return NULL;
    pRec = pRec->dup();
    return pRec;
}

cRecord *cRecordTable::prevRow(QModelIndex *pMi, int _upRes)
{
    (void)_upRes;
    if (pMi->isValid() == false) return NULL;    // Nincs aktív
    int row = pMi->row();
    row--;
    if (row == -1) {
        if (!pTableModel()->qPrevResult()) return NULL; // Az első volt , nincs előző, kész
        pTableModel()->qPrev();   // Lapozunk
        row = pTableModel()->size() -1;
    }
    else if (!isContIx(pTableModel()->records(), row)) {
        DWAR() << "Invalid row : " << row << endl;
        *pMi = QModelIndex();
        return NULL;
    }
    *pMi = pTableModel()->index(row, pMi->column());
    if (!pMi->isValid()) {
        DWAR() << "Invalid index : " << row << endl;
        return NULL;
    }
    selectRow(*pMi);
    cRecord *pRec = actRecord(*pMi);
    if (pRec == NULL) return NULL;
    pRec = pRec->dup();
    return pRec;
}

void cRecordTable::selectRow(const QModelIndex& mi)
{
    int row = mi.row();
    _DBGFN() << " row = " << row << endl;
    pTableView->selectRow(row);
}

void cRecordTable::refresh(bool all)
{
    if (pTimer != NULL) pTimer->start();    // Auto refres (re)start
    QList<qlonglong>    actIdList;
    int row;
    // Ha nem első init, és van ID-je a rekordnak, akkor megjegyezzük az aktív ID-jét
    if (pRecDescr->idIndex(EX_IGNORE) >= 0) {
        foreach (QModelIndex mi, selectedRows()) {
           row = mi.row();
           const tRecords& recs = pTableModel()->records();
           if (isContIx(recs, row)) actIdList << recs.at(row)->getId();
        }
    }
    cRecordsViewBase::refresh(all);
    // Volt aktív sor ID-nk
    if (!actIdList.isEmpty()) {
        pTableView->setSelectionMode(QAbstractItemView::MultiSelection);
        foreach (qlonglong id, actIdList) {
            row = ((cRecordTableModel *)pModel)->records().indexOf(id);
            if (row >= 0) {
                pTableView->selectRow(row);
            }
        }
        pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
}

void cRecordTable::setPageButtons()
{
    if (pTableModel()->qIsPaged()) {
        bool    next = pTableModel()->qNextResult();
        bool    prev = pTableModel()->qPrevResult();
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

cRecord * cRecordTable::record(int i)
{
    if (!isContIx(pTableModel()->_records, i)) EXCEPTION(ENOINDEX, i);
    return pTableModel()->_records[i];
}

cRecord *cRecordTable::record(QModelIndex mi)
{
    return record(mi.row());
}

void cRecordTable::autoRefresh()
{
    QAbstractButton *pb = pButtons->button(DBT_REFRESH);
    if (pb == NULL) EXCEPTION(EPROGFAIL);
    pb->animateClick(500); // 0.5 sec
    refresh(false);
}

