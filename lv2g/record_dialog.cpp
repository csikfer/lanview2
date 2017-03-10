#include "record_table.h"
#include "ui_no_rights.h"
#include "QRegExp"
#include "lv2validator.h"


/* ************************************************************************************************* */

#define SINGLE_BUTTON_LINE  1

QStringList     cDialogButtons::buttonNames;
QList<QIcon>    cDialogButtons::icons;
QList<int>      cDialogButtons::keys;

int cDialogButtons::_buttonNumbers = DBT_BUTTON_NUMBERS;


cDialogButtons::cDialogButtons(qlonglong buttons, qlonglong buttons2, QWidget *par)
    :QButtonGroup(par)
{
    staticInit();
    if (0 == (buttons & ((1LL << _buttonNumbers) - 1))) EXCEPTION(EDATA);
    _pWidget = new QWidget(par);
#if SINGLE_BUTTON_LINE
    _pLayout = new QHBoxLayout(_pWidget);
    init(buttons | buttons2, _pLayout);
#else
    if (buttons2 == 0) {        // Egy gombsor
        _pLayout = new QHBoxLayout(_pWidget);
        init(buttons, _pLayout);
    }
    else {                      // Két gombsor
        if (0 == (buttons2 & ((1LL << _buttonNumbers) - 1))) EXCEPTION(EDATA);
        if (0 != (buttons & buttons2)) EXCEPTION(EDATA);
        _pLayout = new QVBoxLayout(_pWidget);
        QHBoxLayout *pLayoutTop   = new QHBoxLayout();
        QHBoxLayout *pLayoutBotom = new QHBoxLayout();
        _pLayout->addLayout(pLayoutTop);
        _pLayout->addLayout(pLayoutBotom);
        init(buttons, pLayoutTop);
        init(buttons2, pLayoutBotom);
    }
#endif //SINGLE_BUTTON_LINE
}

cDialogButtons::cDialogButtons(const tIntVector& buttons, QWidget *par)
{
    staticInit();
    _pWidget = new QWidget(par);
#if !SINGLE_BUTTON_LINE
    if (buttons.indexOf(DBT_BREAK) < 0) {   // Egy sorba
#endif
        _pLayout = new QHBoxLayout(_pWidget);
        foreach (int id, buttons) {
            if (id < _buttonNumbers) {
                PDEB(VVERBOSE) << "Add button #" << id << " " << buttonNames[id] << endl;
                _pLayout->addWidget(addPB(id, _pWidget));
            }
            else if (id == DBT_SPACER) _pLayout->addStretch();
#if SINGLE_BUTTON_LINE
            else if (id == DBT_BREAK)  continue;
#endif
            else EXCEPTION(EDATA, id);
        }
#if !SINGLE_BUTTON_LINE
    }
    else {
        _pLayout = new QVBoxLayout(_pWidget);
        QHBoxLayout *pL = new QHBoxLayout();
        _pLayout->addLayout(pL);
        foreach (int id, buttons) {
            if (id < _buttonNumbers) {
                PDEB(VVERBOSE) << "Add button #" << id << " " << buttonNames[id] << endl;
                pL->addWidget(addPB(id, _pWidget));
            }
            else if (id == DBT_SPACER) pL->addStretch();
            else if (id == DBT_BREAK)  _pLayout->addLayout(pL = new QHBoxLayout());
            else EXCEPTION(EDATA, id);
        }
    }
#endif
}

void cDialogButtons::staticInit()
{
    if (buttonNames.isEmpty()) {
        appendCont(buttonNames, trUtf8("OK"),          icons, QIcon(":/icons/ok.ico"),      keys, Qt::Key_Enter,  DBT_OK);
        appendCont(buttonNames, trUtf8("Bezár"),       icons, QIcon(":/icons/close.ico"),   keys, Qt::Key_Escape, DBT_CLOSE);
        appendCont(buttonNames, trUtf8("Frissít"),     icons, QIcon(":/icons/refresh.ico"), keys, Qt::Key_F5,     DBT_REFRESH);
        appendCont(buttonNames, trUtf8("Új"),          icons, QIcon(":/icons/insert.ico"),  keys, Qt::Key_Insert, DBT_INSERT);
        appendCont(buttonNames, trUtf8("Hasonló"),     icons, QIcon(":/icons/insert.ico"),  keys, 0,              DBT_SIMILAR);
        appendCont(buttonNames, trUtf8("Módosít"),     icons, QIcon(":/icons/edit.ico"),    keys, 0,              DBT_MODIFY);
        appendCont(buttonNames, trUtf8("Ment"),        icons, QIcon(":/icons/save.ico"),    keys, 0,              DBT_SAVE);
        appendCont(buttonNames, trUtf8("Első"),        icons, QIcon(":/icons/first.ico"),   keys, Qt::Key_Home,   DBT_FIRST);
        appendCont(buttonNames, trUtf8("Előző"),       icons, QIcon(":/icons/previous.ico"),keys, Qt::Key_PageUp, DBT_PREV);
        appendCont(buttonNames, trUtf8("Következő"),   icons, QIcon(":/icons/next.ico"),    keys, Qt::Key_PageDown,DBT_NEXT);
        appendCont(buttonNames, trUtf8("Utolsó"),      icons, QIcon(":/icons/last.ico"),    keys, Qt::Key_End,    DBT_LAST);
        appendCont(buttonNames, trUtf8("Töröl"),       icons, QIcon(":/icons/delete.ico"),  keys, Qt::Key_Delete, DBT_DELETE);
        appendCont(buttonNames, trUtf8("Visszaállít"), icons, QIcon(":/icons/undo.ico"),    keys, 0,              DBT_RESTORE);
        appendCont(buttonNames, trUtf8("Elvet"),       icons, QIcon(":/icons/cancel.ico"),  keys, Qt::Key_Escape, DBT_CANCEL);
        appendCont(buttonNames, trUtf8("Alapállapot"), icons, QIcon(":/icons/restore.ico"), keys, 0,              DBT_RESET);
        appendCont(buttonNames, trUtf8("Betesz"),      icons, QIcon(":/icons/add.ico"),     keys, Qt::Key_Plus,   DBT_PUT_IN);
        appendCont(buttonNames, trUtf8("Kivesz"),      icons, QIcon(":/icons/minus.ico"),   keys, Qt::Key_Minus,  DBT_TAKE_OUT);
        appendCont(buttonNames, trUtf8("Kibont"),      icons, QIcon(":/icons/zoom.ico"),    keys, Qt::Key_Plus,   DBT_EXPAND);
        appendCont(buttonNames, trUtf8("Gyökér"),      icons, QIcon(":/icons/restore.ico"), keys, 0,              DBT_ROOT);
        appendCont(buttonNames, trUtf8("Másol"),       icons, QIcon(":/icons/copy.ico"),    keys, 0,              DBT_COPY);
        appendCont(buttonNames, trUtf8("Nyugtáz"),     icons, QIcon(":/icons/check.ico"),   keys, 0,              DBT_RECEIPT);
        appendCont(buttonNames, trUtf8("Kiürít"),      icons, QIcon(":/icons/delete.ico"),  keys, 0,              DBT_TRUNCATE);
        appendCont(buttonNames, trUtf8("Kiegészítés"), icons, QIcon(":/icons/export.ico"),  keys, 0,              DBT_COMPLETE);
    }
    if (buttonNames.size() != _buttonNumbers) EXCEPTION(EPROGFAIL);
    if (      icons.size() != _buttonNumbers) EXCEPTION(EPROGFAIL);
    if (       keys.size() != _buttonNumbers) EXCEPTION(EPROGFAIL);
}

QPushButton *cDialogButtons::addPB(int id, QWidget *par)
{
    QPushButton *pPB = new QPushButton(icons[id], buttonNames[id], par);
    int key = keys[id];
    if (key > 0) pPB->setShortcut(QKeySequence(key));
    addButton(pPB, id);
    return pPB;
}

void cDialogButtons::init(int buttons, QBoxLayout *pL)
{
    pL->addStretch();
    for (int id = 0; _buttonNumbers > id; ++id) {
        if (buttons & enum2set(id)) {
            PDEB(VVERBOSE) << "Add button #" << id << " " << buttonNames[id] << endl;
            pL->addWidget(addPB(id, _pWidget));
            pL->addStretch();
        }
    }
}

void cDialogButtons::enabeAll()
{
    foreach (QAbstractButton *p, buttons()) {
        p->setEnabled(true);
    }
}

void cDialogButtons::disable(qlonglong __idSet)
{
    foreach (QAbstractButton *p, buttons()) {
        if (__idSet && enum2set(id(p))) p->setDisabled(true);
    }
}

void cDialogButtons::disableExcept(qlonglong __idSet)
{
    foreach (QAbstractButton *p, buttons()) {
        int _id = id(p);
        qlonglong b = enum2set(_id);
        if (!(__idSet & b)) p->setDisabled(true);
    }
}


/* ************************************************************************************************* */
cRecordDialogBase::cRecordDialogBase(const cTableShape &__tm, qlonglong _buttons, bool dialog, cRecordDialogBase *ownDialog, cRecordsViewBase *ownTab, QWidget *par)
    : QObject(par)
    , tableShape(__tm)
    , rDescr(*cRecStaticDescr::get(__tm.getName(_sTableName), __tm.getName(_sSchemaName)))
    , name(tableShape.getName())
    , _errMsg()
{
    _isDialog = dialog;
    _isDisabled = false;
    _pRecord = NULL;
    _pLoop   = NULL;
    _pWidget = NULL;
    _pButtons= NULL;
    _pOwnerDialog = ownDialog;
    _pOwnerTable  = ownTab;

    pq = newQuery();
    setObjectName(name);

    // Megfelelő a leíró típusa?
    if (tableShape.getBool(_sTableShapeType, TS_TABLE)) {   // Ez nem jó dialógushoz
        // Keresünk egy táblával azonos nevűt, ha az jó, akkor OK.
        QString tableName = tableShape.getName(_sTableName);
        int e;
        if ((e=0, name == tableName)                                // Ez az azonos nevű leíró, kár keresni
         || (++e, !tableShape.fetchByName(*pq, tableName))          // Nincs
         || (++e, tableShape.getBool(_sTableShapeType, TS_TABLE))){ // Ez sem jobb
            QString msg;
            switch (e) {
            case 0:
                msg = trUtf8("Dialógushoz nem használlható a %1, táblával azonos nevű elsődleges leíró.").arg(name);
                break;
            case 1:
                msg = trUtf8("Dialógushoz nem használlható a %1, leíró, táblával azonos navű pedig nincs").arg(__tm.getName());
                break;
            case 2:
                msg = trUtf8("Dialógushoz nem használlható a %1, leíró, és a táblával azonos nevű %2 leíró sem.").arg(name).arg(tableName);
                break;
            default:
                EXCEPTION(EPROGFAIL, e);
            }
            EXCEPTION(EDATA, e, msg);
        }
    }
    // Ha az owner objektum megnézésére nem jogosult, akkor ezt sem nézheti meg.
    if (_pOwnerDialog != NULL && _pOwnerDialog->disabled()) {
        _isDisabled = true;
        _viewRights = _pOwnerDialog->_viewRights;
    }
    else {
        _viewRights = tableShape.getId(_sViewRights);
        _isDisabled = !lanView::isAuthorized(_viewRights);
    }
    _isReadOnly = _isDisabled || tableShape.getBool(_sTableShapeType, TS_READ_ONLY);
    if (!_isReadOnly) {  // Ha írható, van hozzá joga is ??
        qlonglong rights;
        rights = tableShape.getId(_sEditRights);
        _isReadOnly = !lanView::isAuthorized(rights);
    }

    if (dialog) {
        _pWidget = new QDialog(par, Qt::CustomizeWindowHint|Qt::WindowTitleHint);
        _pWidget->setWindowTitle(tableShape.getName(_sDialogTitle));
    }
    else {
        _pWidget = new QWidget(par);
    }
    _pWidget->setObjectName(name + "_Widget");

    if (_isDisabled) {  // Neki tltva, csak egy egy üzenet erről.
        Ui::noRightsForm *noRighrs = noRightsSetup(_pWidget, _viewRights, objectName());
        if (_pOwnerDialog == NULL) {
            connect(noRighrs->closePB, SIGNAL(pressed()), this, SLOT(cancel()));
        }
        else {          // Az ownernél még szabad lett volna, vannak már gombok
            noRighrs->closePB->hide();
        }
    }
    else {
        if (_buttons != 0) {
            _pButtons = new cDialogButtons(_buttons, 0, _pWidget);
            _pButtons->widget().setObjectName(name + "_Buttons");
            connect(_pButtons, SIGNAL(buttonClicked(int)), this, SLOT(_pressed(int)));
        }
    }
}

cRecordDialogBase::~cRecordDialogBase()
{
    if (_pLoop != NULL) EXCEPTION(EPROGFAIL);
    if (!close()) EXCEPTION(EPROGFAIL);
    pDelete(_pWidget);
    pDelete( pq);
    pDelete(_pRecord);
}

int cRecordDialogBase::exec(bool _close)
{
    if (_pButtons == NULL) {
        if(_isDisabled == false) EXCEPTION(EPROGFAIL);
    }
    else {
        if (_isDisabled) _pButtons->disableExcept();
        else             _pButtons->enabeAll();
    }
    if (_close) return dialog().exec();
    if (_pLoop != NULL) EXCEPTION(EPROGFAIL);
    _pWidget->setWindowModality(Qt::WindowModal);
    show();
    _pLoop = new QEventLoop(_pWidget);
    int r = _pLoop->exec();
    pDelete(_pLoop);
    return r;
}

/// Slot a megnyomtak egy gombot szignálra.
void cRecordDialogBase::_pressed(int id)
{
    if      (_pLoop != NULL) _pLoop->exit(id);

    else if (_isDialog)       dialog().done(id);
    else                     buttonPressed(id);
}

cRecord& cRecordDialogBase::record()
{
    if (_pRecord == NULL) {
        _pRecord = new cRecordAny(&rDescr);
    }
    return *_pRecord;
}


/* ************************************************************************************************* */

cRecordDialog::cRecordDialog(const cTableShape& __tm, qlonglong _buttons, bool dialog, cRecordDialogBase *ownDialog, cRecordsViewBase *ownTab, QWidget *parent)
    : cRecordDialogBase(__tm, _buttons, dialog, ownDialog, ownTab, parent)
    , fields()
{
    pVBoxLayout = NULL;
    //pHBoxLayout = NULL;
    pSplitter = NULL;
    pSplittLayout = NULL;
    pFormLayout = NULL;
    if (_isDisabled) return;
    if (tableShape.shapeFields.size() == 0) EXCEPTION(EDATA);
    init();
}

inline static QFrame * _frame(QLayout * lay, QWidget * par)
{
    QFrame *pW = new QFrame(par);
    pW->setLayout(lay);
    pW->setFrameStyle(QFrame::Panel | QFrame::Raised);
    pW->setLineWidth(2);
    return pW;
}

void cRecordDialog::init()
{
    int maxFields;
    DBGFN();
    bool ok;
    maxFields = tableShape.feature(mCat(_sDialog, _sHeight)).toInt(&ok);
    if (!ok) maxFields = lv2g::getInstance()->dialogRows;    // Default

    pFormLayout = new QFormLayout;
    pFormLayout->setObjectName(name + "_Form");
//    int n = tableShape.shapeFields.size();
    pSplittLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    pSplitter     = new QSplitter(_pWidget);
    pSplittLayout->addWidget(pSplitter);
    if (_pButtons != NULL) {
        pVBoxLayout = new QVBoxLayout;
        pVBoxLayout->setObjectName(name + "_VBox");
        _pWidget->setLayout(pVBoxLayout);
        pVBoxLayout->addLayout(pSplittLayout, 1);
        pSplitter->addWidget(_frame(pFormLayout, _pWidget));
        pVBoxLayout->addWidget(_pButtons->pWidget(), 0);
    }
    else {
        pVBoxLayout = NULL;
         _pWidget->setLayout(pSplittLayout);
         pSplitter->addWidget(_frame(pFormLayout, _pWidget));
    }
    tTableShapeFields::const_iterator i, e = tableShape.shapeFields.cend();
    _pRecord = new cRecordAny(&rDescr);

    if (!tableShape.getStringList(_sTableShapeType).contains(_sReadOnly)) {
        if (_pOwnerTable != NULL && _pOwnerTable->owner_id != NULL_ID) {  // Ha van owner, akkor az ID-jét beállítjuk
            int flags = _pOwnerTable->flags;
            if (flags & RTF_CHILD) {
                int oix = _pRecord->descr().ixToOwner();
                _pRecord->setId(oix, _pOwnerTable->owner_id);
            }
        }
        if (_pOwnerTable != NULL && _pOwnerTable->parent_id != NULL_ID) {  // Ha van parent, akkor az ID-jét beállítjuk  ????
            int pix = _pRecord->descr().ixToParent(EX_IGNORE);  // Ha van önmagára mutató ID (öröklések miatt nem biztos)
            if (pix != NULL_IX) _pRecord->setId(pix, _pOwnerTable->parent_id);
        }
    }

    int cnt = 0;
    for (i = tableShape.shapeFields.cbegin(); i != e; ++i) {
        const cTableShapeField& mf = **i;
        if (mf.getBool(_sFieldFlags, FF_DIALOG_HIDE)) continue;
        int fieldIx = rDescr.toIndex(mf.getName());
        bool setRo = _isReadOnly || mf.getBool(_sFieldFlags, FF_READ_ONLY);
        cFieldEditBase *pFW = cFieldEditBase::createFieldWidget(tableShape, mf, (*_pRecord)[fieldIx], setRo, this);
        fields.append(pFW);
        QWidget * pw = pFW->pWidget();
        qlonglong stretch = mf.feature(mCat(_sStretch, _sVertical), 0);
        if (stretch > 0) {
            QSizePolicy sp = pw->sizePolicy();
            sp.setVerticalStretch(stretch);
            pw->setSizePolicy(sp);
        }
        pw->setObjectName(mf.getName());
        PDEB(VVERBOSE) << "Add row to form : " << mf.getName() << " widget : " << typeid(*pFW).name() << QChar('/') << fieldWidgetType(pFW->wType()) << endl;
        int h = pFW->height();
        cnt += h;
        if (cnt > maxFields) {
            cnt = h;
            pFormLayout = new QFormLayout;  // !!!
            pSplitter->addWidget(_frame(pFormLayout, _pWidget));
        }
        pFormLayout->addRow(mf.getName(_sDialogTitle), pw);
        if (!_pRecord->isNull(fieldIx)) {
            pFW->set(_pRecord->get(fieldIx));
        }
    }
    _pWidget->adjustSize();
    DBGFNL();
}

void cRecordDialog::restore(cRecord *_pRec)
{
    pDelete(_pRecord);
    _pRecord = new cRecordAny(&rDescr);
    if (_pRec != NULL) _pRecord->set(*_pRec);
    int i, n = fields.size();
    for (i = 0; i < n; i++) {
        cFieldEditBase& field = *fields[i];
        field.set(_pRecord->get(field.fieldIndex()));
    }
}

bool cRecordDialog::accept()
{
    _errMsg.clear();    // Töröljük a hiba stringet
    int i, n = fields.size();
    // record.set();            // NEM !! Kinullázzuk a rekordot
    _pRecord->_stat = 0;        // Kinullázzuk, majd mezőnként "felesleges" értékadással gyűjtlük a hiba biteket
    for (i = 0; i < n; i++) {   // Végigszaladunk a mezőkön
        cFieldEditBase& field = *fields[i];
        int rfi = field.fieldIndex();
        if (field.isReadOnly()) continue;      // Feltételezzük, hogy RO esetén az van a mezőben aminek lennie kell.
        if (field._fieldShape.getBool(_sFieldFlags, FF_AUTO_SET)) continue; // Ezt sem kell ellenörizni
        qlonglong s = _pRecord->_stat;                   // Mentjük a hiba bitet,
        _pRecord->_stat &= ~ES_DEFECTIVE; // majd töröljük, mert mezőnkként kell
        QVariant fv = fields[i]->get();     // A mező widget-jéből kivesszük az értéket
        if (fv.isNull() && field._isInsert && field._hasDefault) continue;    // NULL, insert, van alapérték
        PDEB(VERBOSE) << "Dialog -> obj. field " << _pRecord->columnName(i) << " = " << debVariantToString(fv) << endl;
        _pRecord->set(rfi, fv);                      // Az értéket bevéssük a rekordba
        if (_pRecord->_stat & ES_DEFECTIVE) {
            DWAR() << "Invalid data : field " << _pRecord->columnName(rfi) << " = " << debVariantToString(fv) << endl;
            _errMsg += trUtf8("Adat hiba a %1 mezőnél\n").arg(_pRecord->columnName(rfi));
        }
        _pRecord->_stat |= s & ES_DEFECTIVE;
    }
    if (0 == (_pRecord->_stat & ES_DEFECTIVE)) return true;
    QMessageBox::warning(_pWidget, trUtf8("Adat hiba"), _errMsg);
    return false;
}

cFieldEditBase * cRecordDialog::operator[](const QString& __fn)
{
    _DBGFN() << " " << __fn << endl;
    QList<cFieldEditBase *>::iterator i;
    for (i = fields.begin(); i < fields.end(); ++i) {
        int x = i - fields.begin();
        cFieldEditBase *p = *i;
        QString name = p->_fieldShape.getName();
        PDEB(VVERBOSE) << "#" << x << " : " << name << endl;
        if (name == __fn) return p;
    }
    return NULL;
}

/* ***************************************************************************************************** */

cRecordDialogInh::cRecordDialogInh(const cTableShape& _tm, tRecordList<cTableShape>& _tms, qlonglong _buttons, bool dialog, cRecordDialogBase *ownDialog, cRecordsViewBase *ownTab, QWidget * parent)
    : cRecordDialogBase(_tm, _buttons, dialog, ownDialog, ownTab, parent)
    , tabDescriptors(_tms)
    , tabs()
{
    pVBoxLayout = NULL;
    pTabWidget = NULL;
    if (_isDisabled) return;
    if (_pButtons == NULL) EXCEPTION(EPROGFAIL);
//    if (isReadOnly) EXCEPTION(EDATA);
    init();
}

cRecordDialogInh::~cRecordDialogInh()
{
    while (tabs.size()) {
        delete tabs.first();
        tabs.pop_front();
    }
}

void cRecordDialogInh::init()
{
    DBGFN();
    pTabWidget = new QTabWidget;                // Az egész egy tab widgetben, minden rekord típus egy tab.
    pTabWidget->setObjectName(name + "_tab");
    pVBoxLayout = new QVBoxLayout;
    pVBoxLayout->setObjectName(name + "_VBox");
    _pWidget->setLayout(pVBoxLayout);
    pVBoxLayout->addWidget(pTabWidget);
    pVBoxLayout->addWidget(_pButtons->pWidget());   // Nyomógombok a TAB alatt
    // Egyenként megcsináljuk a widgeteket a különböző rekord típusokra.
    int i, n = tabDescriptors.size();
    for (i = 0; i < n; ++i) {
        const cTableShape& shape = *tabDescriptors[i];
        cRecordDialog * pDlg = new cRecordDialog(shape, 0, false, this, _pOwnerTable, pTabWidget);
/*      cRecord * pRec = new cRecordAny(shape.getName(_sTableName), shape.getName(_sSchemaName));
        if (_pOwnerTable != NULL && _pOwnerTable->owner_id != NULL_ID) {  // Ha van owner, akkor az ID-jét beállítjuk
            int oix = pRec->descr().ixToOwner();
            pRec->setId(oix, _pOwnerTable->owner_id);
        }
        if (_pOwnerTable != NULL && _pOwnerTable->parent_id != NULL_ID) {  // Ha van parent, akkor az ID-jét beállítjuk
            int pix = pRec->descr().ixToParent(EX_IGNORE);
            if (pix != NULL_IX) pRec->setId(pix, _pOwnerTable->parent_id);
            pDlg->_isReadOnly = true;// ??
        }
        pDlg->_pRecord = pRec;
        pDlg->restore(); */
        tabs << pDlg;
        pTabWidget->addTab(pDlg->pWidget(), shape.getName(_sDialogTabTitle));
    }
    pTabWidget->setCurrentIndex(0);
    _pWidget->adjustSize(); //?
    DBGFNL();
}


cRecord& cRecordDialogInh::record()
{
    return actDialog().record();
}

int cRecordDialogInh::actTab()
{
    int i =  pTabWidget->currentIndex();
    if (!isContIx(tabs, i)) EXCEPTION(EPROGFAIL, i);
    return i;
}

void cRecordDialogInh::setActTab(int i)
{
    if (!isContIx(tabs, i)) EXCEPTION(EPROGFAIL, i);
    pTabWidget->setCurrentIndex(i);
    if (_pButtons == NULL) EXCEPTION(EPROGFAIL);
}

bool cRecordDialogInh::accept()
{
    _errMsg.clear();
    if (disabled()) return false;
    cRecordDialog *prd = tabs[actTab()];
    bool  r = prd->accept();
    if (!r) _errMsg = prd->errMsg();
    return r;
}

cFieldEditBase * cRecordDialogInh::operator[](const QString& __fn)
{
    return actDialog()[__fn];
}

void cRecordDialogInh::restore(cRecord *_pRec)
{
    if (_pRec == NULL) return;
    if (disabled()) return;
    foreach (cRecordDialog *pTab, tabs) {
        pTab->restore(_pRec);
    };
}

/* ************************************************************************* */

cRecord * insertRecordDialog(QSqlQuery& q, const QString& sn, QWidget *pPar)
{
    cTableShape shape;
    shape.setByName(q, sn);
    shape.fetchFields(q);
    // A dialógusban megjelenítendő nyomógombok.
    int buttons = enum2set(DBT_OK, DBT_CANCEL);
    cRecordDialog   rd(shape, buttons, true, NULL, NULL, pPar);  // A rekord szerkesztő dialógus
    while (true) {
        int r = rd.exec(false);
        if (r == DBT_CANCEL) return NULL;
        if (!rd.accept()) continue;
        if (!cErrorMessageBox::condMsgBox(rd.record().tryInsert(q, true))) continue;
        break;
    }
    rd.close();
    return rd.record().dup();
}

/* ************************************************************************* */

cPPortTableLine::cPPortTableLine(int r, cInsertPatchDialog *par)
{
    parent = par;
    tableWidget = par->pUi->tableWidgetPorts;
    row    = r;
    sharedPortRow = -1;
    comboBoxShare  = new QComboBox();
    for (int i = ES_; i <= ES_NC; ++i) comboBoxShare->addItem(portShare(i));
    tableWidget->setCellWidget(row, CPP_SH_TYPE, comboBoxShare);
    comboBoxPortIx = new QComboBox();
    tableWidget->setCellWidget(row, CPP_SH_IX, comboBoxPortIx);

    connect(comboBoxShare,  SIGNAL(currentIndexChanged(int)), this, SLOT(changeShared(int)));
    connect(comboBoxPortIx, SIGNAL(currentIndexChanged(int)), this, SLOT(changePortIx(int)));
}

void cPPortTableLine::changeShared(int _sh)
{
    (void)_sh;
    if (parent->lockSlot) return;
    parent->updateSharedIndexs();
}

void cPPortTableLine::changePortIx(int ix)
{
    if (parent->lockSlot) return;
    if (ix == 0) sharedPortRow = -1;
    else {
        int i = ix -1;
        if (!isContIx(listPortIxRow, i)) EXCEPTION(EDATA, ix);
        sharedPortRow = listPortIxRow[i];
    }
    parent->updateSharedIndexs();
}

QString cInsertPatchDialog::sPortRefForm;

cInsertPatchDialog::cInsertPatchDialog(QWidget *parent)
    : QDialog(parent)
    , pUi(new Ui::patchSimpleDialog)
{
    pq = newQuery();
    if (sPortRefForm.isEmpty()) sPortRefForm = trUtf8("#%1 (%2/%3)");
    zoneNames = cPlaceGroup::getAllZones(*pq, &zoneIds);
    pUi->setupUi(this);
    pModelPlace = new cRecordListModel(cPlace::_descr_cPlace());
    pModelPlace->nullable = true;
    sPlaceFilterSQL = "is_group_place(place_id, %1)";
    pUi->comboBoxZone->addItems(zoneNames);
    pUi->comboBoxZone->setCurrentIndex(0);
    QString sql = QString(sPlaceFilterSQL).arg(ALL_PLACE_GROUP_ID);
    pModelPlace->setFilter(QVariant(), OT_ASC, FT_NO);
    pUi->comboBoxPlace->setModel(pModelPlace);
    pUi->comboBoxPlace->setCurrentIndex(0);
    pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    cIntValidator *intValidator = new cIntValidator(false);
    pUi->tableWidgetPorts->setItemDelegateForColumn(CPP_INDEX, new cItemDelegateValidator(intValidator));

    connect(pUi->lineEditName,      SIGNAL(textChanged(QString)),       this, SLOT(changeName(QString)));
    connect(pUi->comboBoxZone,      SIGNAL(currentIndexChanged(int)),   this, SLOT(changeFilterZone(int)));
    connect(pUi->toolButtonNewPlace,SIGNAL(pressed()),                  this, SLOT(newPlace()));
    connect(pUi->spinBoxFrom,       SIGNAL(valueChanged(int)),          this, SLOT(changeFrom(int)));
    connect(pUi->spinBoxTo,         SIGNAL(valueChanged(int)),          this, SLOT(changeTo(int)));
    connect(pUi->pushButtonAddPorts,SIGNAL(pressed()),                  this, SLOT(addPorts()));
    connect(pUi->tableWidgetPorts,  SIGNAL(cellActivated(int,int)),     this, SLOT(cellActivated(int,int)));
    connect(pUi->tableWidgetPorts,  SIGNAL(cellChanged(int,int)),       this, SLOT(cellChanged(int, int)));
    connect(pUi->tableWidgetPorts->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    connect(pUi->pushButtonDelPort, SIGNAL(pressed()),                  this, SLOT(delPorts()));
    connect(pUi->pushButtonPort1,   SIGNAL(pressed()),                  this, SLOT(set1port()));
    connect(pUi->pushButtonPort2,   SIGNAL(pressed()),                  this, SLOT(set2port()));
    connect(pUi->pushButtonPort2Shared, SIGNAL(pressed()),              this, SLOT(set2sharedPort()));
    shOk = pNamesOk = pIxOk = true;
    lockSlot = false;
}

cInsertPatchDialog::~cInsertPatchDialog()
{
    delete pq;
}

cPatch * cInsertPatchDialog::getPatch()
{
    cPatch *p = new cPatch();
    p->setName(pUi->lineEditName->text());
    p->setNote(pUi->lineEditNote->text());
    int i = pUi->comboBoxPlace->currentIndex();
    if (i > 0) p->setId(_sPlaceId, pModelPlace->atId(i));
    int n = pUi->tableWidgetPorts->rowCount();
    for (i = 0; i < n; i++) {
        cPPort *pp = new cPPort();
        pp->setName(             getText(pUi->tableWidgetPorts, i, CPP_NAME));
        pp->setNote(             getText(pUi->tableWidgetPorts, i, CPP_NOTE));
        pp->setName(_sPortTag,   getText(pUi->tableWidgetPorts, i, CPP_TAG));
        pp->setName(_sPortIndex, getText(pUi->tableWidgetPorts, i, CPP_INDEX));
        p->ports << pp;
    }
    return p;
}

void cInsertPatchDialog::clearRows()
{
    while (!rowsData.isEmpty()) {
        delete rowsData.takeLast();
    }
    pUi->tableWidgetPorts->clearContents();
    pUi->tableWidgetPorts->setRowCount(0);
    /*
    int n1 = rowsData.size();
    int n2 = pUi->tableWidgetPorts->rowCount();
    if (n1 != n2) {
        EXCEPTION(EPROGFAIL);
    }
    */
}

void cInsertPatchDialog::setRows(int rows)
{
    int i = pUi->tableWidgetPorts->rowCount();
    if (i != rowsData.size()) EXCEPTION(EDATA);
    if (i == rows) return;
    if (i < rows) {
        pUi->tableWidgetPorts->setRowCount(rows);
        for (; i < rows; ++i) {
            rowsData << new cPPortTableLine(i, this);
        }
    }
    else {
        while (rows < rowsData.size()) {
            delete rowsData.takeLast();
        }
        pUi->tableWidgetPorts->setRowCount(rows);
    }
}

void cInsertPatchDialog::setPortShare(int row, int sh)
{
    if (!isContIx(rowsData, row)) EXCEPTION(ENOINDEX, row);
    rowsData[row]->comboBoxShare->setCurrentIndex(sh);
}

#define _LOCKSLOTS()    _lockSlotSave_ = lockSlot; lockSlot = true
#define LOCKSLOTS()     bool _LOCKSLOTS()
#define UNLOCKSLOTS()   lockSlot = _lockSlotSave_

void cInsertPatchDialog::updateSharedIndexs()
{
    // Az összes elsödleges megosztás listája (sor indexek)
    shPrimRows.clear();
    foreach (cPPortTableLine *pRow, rowsData) {
        int sh = pRow->comboBoxShare->currentIndex();          // megosztás típusa
        if (sh == ES_A || sh == ES_AA) shPrimRows << pRow->row;// Ha elsődleges megosztás (amire hivatkozik a többi)
    }
    // MAP az elsődleges megosztásokra mutatók listái
    shPrimMap.clear();
    QList<int> secondIxs;
    foreach (cPPortTableLine *pRow, rowsData) {
        int sh = pRow->comboBoxShare->currentIndex();   // megosztás típusa
        if (sh == ES_AB || sh == ES_B || sh == ES_BA || sh == ES_BB || sh == ES_C || sh ==  ES_D) {
            int shrow = pRow->sharedPortRow;        // Mutató az elsődleges megosztott portra
            if (shrow >= 0 && !shPrimRows.contains(shrow)) {
                shrow = -1;  // megszűnt
                pRow->sharedPortRow = -1;
            }
            if (shrow >= 0) shPrimMap[shrow] << pRow->row;  // Van mutató, csoportosítéshoz a MAP-ba
            secondIxs << pRow->row;
        }
    }
    shOk = true;
    LOCKSLOTS();
    // Végigszaladunk az összes soron, és frissítjük az elsődlegesre hivatkozások comboBox-át
    foreach (cPPortTableLine *pRow, rowsData) {
        if (secondIxs.contains(pRow->row)) {    // Másodlagos megosztott port (lehet hivatkozás, a)
            QStringList sl;             // Az eredmény lista (ezekre lehet hivatkozni) a teljes lista leválogatásának a célja
            pRow->listPortIxRow.clear();// A táblázatbeli sor idexek (hogy tudjuk is mire hivatkozik a fenti lista)
            int mysh = pRow->comboBoxShare->currentIndex(); // Az aktuális másodlagos megosztás típusa
            // Elsödleges megosztott portokhoz rendelések vizsgálata
            foreach (int pix, shPrimRows) {     // Végivesszük a lehetőségeket (elsödleges megosztások listája)
                QList<int> secs = shPrimMap[pix];       // Ezek a másodlagosak hivatkoznak rá
                QString item = refName(pix);            // A comboBox-ban megjelenő string formátuma
                if (secs.isEmpty()) {   // Erre az elsődlegesre nem hivatkozik senki, akkor bárhol választható
                    sl                  << item;
                    pRow->listPortIxRow << pix;
                }
                else {                  // Van rá hivatkozás, akkor vizsgálódunk
                    int psh = rowsData[pix]->comboBoxShare->currentIndex(); // Az aktuális elsődleges megosztás
                    // ütközés az elsődlegessel
                    if (psh == ES_A && (mysh == ES_AB || mysh == ES_C || mysh == ES_D)) continue;
                    bool collision = false;
                    foreach (int six, secs) {
                        if (six == pRow->row) continue;     // Saját magával nincs értelme az ütközésvizsgálatnak
                        int ssh = rowsData[six]->comboBoxShare->currentIndex(); // akt, másodlagos megosztás
                        if (mysh == ssh) {                  // Azonos megosztás tuti nem lehetséges
                            collision = true;
                            break;
                        }
                        switch (mysh) { // tételesen az összeférhetetlen megosztások
                        case ES_AB:
                            collision = ssh == ES_C || ssh == ES_D;
                            break;
                        case ES_B:
                            collision = ssh == ES_BA || ssh == ES_BB || ssh == ES_C || ssh == ES_D;
                            break;
                        case ES_BA:
                            collision = ssh == ES_B || ssh == ES_C || ssh == ES_D;
                        case ES_BB:
                            collision = ssh == ES_B;
                        case ES_C:
                        case ES_D:
                            collision = ssh == ES_B || ssh == ES_AB;
                        default:
                            EXCEPTION(EDATA);
                            break;
                        }
                        if (collision) break;   // volt ütközés, nem kell tovább vizsgálódni
                    }
                    if (collision) continue;    // Ha ütközés van, nem kerülhet a választási lehetőségek közé.
                    sl                  << item;// Hozzáadjuk a lehetőségek listá(i)hoz
                    pRow->listPortIxRow << pix;
                }
            }
            pRow->comboBoxPortIx->clear();
            pRow->comboBoxPortIx->addItem(_sNul);
            pRow->comboBoxPortIx->addItems(sl);
            if (pRow->sharedPortRow >= 0 && pRow->listPortIxRow.contains(pRow->sharedPortRow)) {
                int ix = pRow->listPortIxRow.indexOf(pRow->sharedPortRow);
                pRow->comboBoxPortIx->setCurrentIndex(ix +1);
            }
            else {
                pRow->sharedPortRow = -1;
                pRow->comboBoxPortIx->setCurrentIndex(0);
                shOk = false;   // Nincs megadva az elsődleges, az nem OK.
            }
        }
        else {                                  // Elsodleges megosztott, vagy nem megosztott
            pRow->comboBoxPortIx->clear();      // Nem hivatkozik senkire
            pRow->sharedPortRow = -1;
        }
    }
    UNLOCKSLOTS();
    changeName(pUi->lineEditName->text());  // Menthető ?
}

void cInsertPatchDialog::updatePNameIxOk()
{
    QStringList nameList;
    QList<int>  indexList;
    pNamesOk = pIxOk = true;
    int n = pUi->tableWidgetPorts->rowCount();
    for (int i = 0; i < n; ++i) {
        if (pNamesOk) {
            QString s = getText(pUi->tableWidgetPorts, i, CPP_NAME);
            if (s.isEmpty() || nameList.contains(s)) {
                pNamesOk = false;
                if (!pIxOk) break;
            }
            nameList << s;
        }
        if (pIxOk) {
            bool ok;
            int ix = getText(pUi->tableWidgetPorts, i, CPP_INDEX).toInt(&ok);
            if (!ok || indexList.contains(ix)) {
                pIxOk = false;
                if (!pNamesOk) break;
            }
            indexList << ix;
        }
    }
}

QString cInsertPatchDialog::refName(int row)
{
    return refName(row, getText(pUi->tableWidgetPorts, row, CPP_NAME),
                        getText(pUi->tableWidgetPorts, row, CPP_INDEX));
}


/* SLOTS */
void cInsertPatchDialog::changeName(const QString& name)
{
    bool f = name.isEmpty() || pUi->tableWidgetPorts->rowCount() == 0 || !shOk || !pNamesOk || !pIxOk;
    pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(f);
}

void cInsertPatchDialog::set1port()
{
    LOCKSLOTS();
    clearRows();
    setRows(1);
    QTableWidgetItem *pItem;
    pItem = new QTableWidgetItem("J1");
    pUi->tableWidgetPorts->setItem(0, CPP_NAME, pItem);
    pItem = new QTableWidgetItem("1");
    pUi->tableWidgetPorts->setItem(0, CPP_INDEX, pItem);
    bool f = pUi->lineEditName->text().isEmpty();
    pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(f);
    UNLOCKSLOTS();
}

void cInsertPatchDialog::set2port()
{
    set1port();
    LOCKSLOTS();
    setRows(2);
    QTableWidgetItem *pItem;
    pItem = new QTableWidgetItem("J2");
    pUi->tableWidgetPorts->setItem(1, CPP_NAME, pItem);
    pItem = new QTableWidgetItem("2");
    pUi->tableWidgetPorts->setItem(1, CPP_INDEX, pItem);
    UNLOCKSLOTS();
}

void cInsertPatchDialog::set2sharedPort()
{
    set2port();
    setPortShare(0, ES_A);
    setPortShare(1, ES_B);
    rowsData[1]->comboBoxPortIx->setCurrentIndex(1);    // Az első portra mutat
}

void cInsertPatchDialog::addPorts()
{
    int from = pUi->spinBoxFrom->value();
    int to   = pUi->spinBoxTo->value();
    int n = to - from +1;
    if (n <= 0) return;
    int row = pUi->tableWidgetPorts->rowCount();
    LOCKSLOTS();
    setRows(row + n);
    QString namePattern = pUi->lineEditNamePattern->text();
    QString tagPattern  = pUi->lineEditTagPattern->text();
    int nameOffs = pUi->spinBoxNameOffs->value();
    int tagOffs  = pUi->spinBoxTagOffs->value();
    QTableWidgetItem *pItem;
    for (int i = from; i <= to; i++, row++) {
        QString name = nameAndNumber(namePattern, i + nameOffs);
        QString tag;
        if (!tagPattern.isEmpty()) tag = nameAndNumber(tagPattern, i + tagOffs);
        pItem = new QTableWidgetItem(name);
        pUi->tableWidgetPorts->setItem(row, CPP_NAME, pItem);
        pItem = new QTableWidgetItem(QString::number(i));
        pUi->tableWidgetPorts->setItem(row, CPP_INDEX, pItem);
        pItem = new QTableWidgetItem(tag);
        pUi->tableWidgetPorts->setItem(row, CPP_TAG, pItem);
    }
    UNLOCKSLOTS();
    updatePNameIxOk();
    changeName(pUi->lineEditName->text());
}

void cInsertPatchDialog::delPorts()
{
    QModelIndexList mil = pUi->tableWidgetPorts->selectionModel()->selectedRows();
    QList<int>  rows;   // Törlendő sorok sorszámai
    foreach (QModelIndex mi, mil) {
        rows << mi.row();
    }
    // sorba rendezzük, mert majd a vége felül törlünk
    std::sort(rows.begin(), rows.end());
    // A hivatkozásokat töröljük a törlendő sorokra
    for (int j = 0; j < rowsData.size(); ++j) {
        int i = rowsData[j]->sharedPortRow;
        if (rows.contains(i)) rowsData[j]->sharedPortRow = -1;  // Törölt sorra nem hivatkozhatunk
    }
    // Törlés a vége felöl
    while (rows.size() > 0) {
        int row = rows.takeLast();
        rowsData.removeAt(row);
        pUi->tableWidgetPorts->removeRow(row);
    }
    // Sor indexek ujra számolása:
    for (int i = 0; i < rowsData.size(); ++i) {
        int old = rowsData[i]->row;
        rowsData[i]->row = i;
        for (int j = 0; j < rowsData.size(); ++j) {
            if (rowsData[j]->sharedPortRow == old) rowsData[j]->sharedPortRow = i;
        }
    }
    updateSharedIndexs();
    // Mentés gomb enhedélyezés, ha ok
    changeName(pUi->lineEditName->text());
}

void cInsertPatchDialog::changeFrom(int i)
{
    bool f = (pUi->spinBoxTo->value() - i) >= 0;
    pUi->pushButtonAddPorts->setEnabled(f);
}

void cInsertPatchDialog::changeTo(int i)
{
    bool f = (i - pUi->spinBoxFrom->value()) >= 0;
    pUi->pushButtonAddPorts->setEnabled(f);
}

void cInsertPatchDialog::changeFilterZone(int i)
{
    if (!isContIx(zoneIds, i)) EXCEPTION(EDATA, i);
    qlonglong id = zoneIds.at(i);
    if (id == ALL_PLACE_GROUP_ID) {
        pModelPlace->setFilter(QVariant(), OT_ASC, FT_NO);
    }
    else {
        QString sql = QString(sPlaceFilterSQL).arg(id);
        pModelPlace->setFilter(sql, OT_ASC, FT_SQL_WHERE);
    }
}

void cInsertPatchDialog::newPlace()
{
    cRecord *p = insertRecordDialog(*pq, _sPlaces, this);
    if (p != NULL) {
        changeFilterZone(pUi->comboBoxZone->currentIndex());
        QString placeName = p->getName();
        int ix = pUi->comboBoxPlace->findText(placeName);
        if (ix > 0) pUi->comboBoxPlace->setCurrentIndex(ix);
        delete p;
    }
}

void cInsertPatchDialog::cellChanged(int row, int col)
{
    if (lockSlot) return;
    QString text = getText(pUi->tableWidgetPorts, row, col);
    int n = pUi->tableWidgetPorts->rowCount();
    QString sRefName;
    switch (col) {
    case CPP_NAME: {
        if (text.isEmpty()) {
            pNamesOk = false;
        }
        else {
            pNamesOk = true;
            QStringList nameList;
            for (int i = 0; i < n; ++i) {
                QString s = getText(pUi->tableWidgetPorts, i, CPP_NAME);
                if (s.isEmpty() || nameList.contains(s)) {
                    pNamesOk = false;
                    break;
                }
                nameList << s;
            }
        }
        sRefName = refName(row, text, getText(pUi->tableWidgetPorts, row, CPP_INDEX));
        break;
    }
    case CPP_INDEX: {
        QList<int>  ixList;
        pIxOk = true;
        for (int i = 0; i < n; ++i) {
            bool ok;
            int ix = getText(pUi->tableWidgetPorts, i, CPP_INDEX).toInt(&ok);
            if (!ok || ixList.contains(ix)) {   // Nem szám, vagy nem egyedi
                pIxOk = false;
                break;
            }
            ixList << ix;
        }
        sRefName = refName(row, getText(pUi->tableWidgetPorts, row, CPP_NAME), text);
        break;
    }
    default:    return;     // Mással nem foglalkozunk
    }
    // Ha vannak index hivatkozások, akkor a generált hivatkozás nevek megváltoztak!
    if (shPrimRows.contains(row)) {
        foreach (cPPortTableLine *pRow, rowsData) {
            QList<int> refList = pRow->listPortIxRow;
            int ix = refList.indexOf(row);
            if (ix < 0) continue;
            pRow->comboBoxPortIx->setItemText(ix + 1, sRefName);
        }
    }
    changeName(pUi->lineEditName->text());
}

void cInsertPatchDialog::selectionChanged(const QItemSelection &, const QItemSelection &)
{
    QModelIndexList mil = pUi->tableWidgetPorts->selectionModel()->selectedRows();
    pUi->pushButtonDelPort->setDisabled(mil.isEmpty());
}

void cInsertPatchDialog::cellActivated(int row, int col)
{
    PDEB(INFO) << VDEB(row) << VDEB(col) << endl;
}

cPatch * insertPatchDialog(QSqlQuery& q, QWidget *pPar)
{
    cInsertPatchDialog dialog(pPar);
    cPatch *p;
    while (true) {
        int r = dialog.exec();
        if (r != QDialog::Accepted) return NULL;
        p = dialog.getPatch();
        if (p == NULL) continue;
        if (!cErrorMessageBox::condMsgBox(p->tryInsert(q, true))) {
            delete p;
            continue;
        }
        break;
    }
    return p;
}
