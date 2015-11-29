#include "record_table.h"
#include "ui_no_rights.h"


/* ************************************************************************************************* */

QStringList     cDialogButtons::buttonNames;
QList<QIcon>    cDialogButtons::icons;
int cDialogButtons::_buttonNumbers = DBT_BUTTON_NUMBERS;


cDialogButtons::cDialogButtons(int buttons, int buttons2, QWidget *par)
    :QButtonGroup(par)
{
    staticInit();
    if (0 == (buttons & ((1 << _buttonNumbers) - 1))) EXCEPTION(EDATA);
    _pWidget = new QWidget(par);
    if (buttons2 == 0) {        // Egy gombsor
        _pLayout = new QHBoxLayout(_pWidget);
        init(buttons, _pLayout);
    }
    else {                      // Két gombsor
        _pLayout = new QVBoxLayout(_pWidget);
        QHBoxLayout *pLayoutTop   = new QHBoxLayout();
        QHBoxLayout *pLayoutBotom = new QHBoxLayout();
        _pLayout->addLayout(pLayoutTop);
        _pLayout->addLayout(pLayoutBotom);
        init(buttons, pLayoutTop);
        init(buttons2, pLayoutBotom);
    }
}

cDialogButtons::cDialogButtons(const tIntVector& buttons, QWidget *par)
{
    staticInit();
    _pWidget = new QWidget(par);
    if (buttons.indexOf(DBT_BREAK) < 0) {   // Egy sorba
        _pLayout = new QHBoxLayout(_pWidget);
        foreach (int id, buttons) {
            if (id < _buttonNumbers) {
                PDEB(VVERBOSE) << "Add button #" << id << " " << buttonNames[id] << endl;
                QPushButton *pPB = new QPushButton(icons[id], buttonNames[id], _pWidget);
                addButton(pPB, id);
                _pLayout->addWidget(pPB);
            }
            else if (id == DBT_SPACER) _pLayout->addStretch();
            else EXCEPTION(EDATA, id);
        }
    }
    else {
        _pLayout = new QVBoxLayout(_pWidget);
        QHBoxLayout *pL = new QHBoxLayout();
        _pLayout->addLayout(pL);
        foreach (int id, buttons) {
            if (id < _buttonNumbers) {
                PDEB(VVERBOSE) << "Add button #" << id << " " << buttonNames[id] << endl;
                QPushButton *pPB = new QPushButton(icons[id], buttonNames[id], _pWidget);
                addButton(pPB, id);
                pL->addWidget(pPB);
            }
            else if (id == DBT_SPACER) pL->addStretch();
            else if (id == DBT_BREAK)  _pLayout->addLayout(pL = new QHBoxLayout());
            else EXCEPTION(EDATA, id);
        }
    }
}

void cDialogButtons::staticInit()
{
    if (buttonNames.isEmpty()) {
        buttonNames << trUtf8("OK");            icons << QIcon::fromTheme("emblem-default");    // DBT_OK
        buttonNames << trUtf8("Bezár");         icons << QIcon::fromTheme("window-close");      // DBT_CLOSE
        buttonNames << trUtf8("Frissít");       icons << QIcon::fromTheme("reload");            // DBT_REFRESH
        buttonNames << trUtf8("Új");            icons << QIcon::fromTheme("insert-image");      // DBT_INSERT
        buttonNames << trUtf8("Módosít");       icons << QIcon::fromTheme("text-editor");       // DBT_MODIFY
        buttonNames << trUtf8("Ment");          icons << QIcon::fromTheme("list-add");          // DBT_SAVE
        buttonNames << trUtf8("Első");          icons << QIcon::fromTheme("go-first");          // DBT_FIRST
        buttonNames << trUtf8("Előző");         icons << QIcon::fromTheme("go-previous");       // DBT_PREV
        buttonNames << trUtf8("Következő");     icons << QIcon::fromTheme("go-next");           // DBT_NEXT
        buttonNames << trUtf8("Utolsó");        icons << QIcon::fromTheme("go-last");           // DBT_LAST
        buttonNames << trUtf8("Töröl");         icons << QIcon::fromTheme("list-remove");       // DBT_DELETE
        buttonNames << trUtf8("Visszaállít");   icons << QIcon::fromTheme("edit-redo");         // DBT_RESTORE
        buttonNames << trUtf8("Elvet");         icons << QIcon::fromTheme("gtk-cancel");        // DBT_CANCEL
        buttonNames << trUtf8("Alapállapot");   icons << QIcon::fromTheme("go-first");          // DBT_RESET
        buttonNames << trUtf8("Betesz");        icons << QIcon::fromTheme("insert-image");      // DBT_PUT_IN
        buttonNames << trUtf8("Kivesz");        icons << QIcon::fromTheme("list-remove");       // DBT_GET_OUT
        buttonNames << trUtf8("Kibont");        icons << QIcon::fromTheme("zoom-in");           // DBT_EXPAND
        buttonNames << trUtf8("Gyökér");        icons << QIcon();                               // DBT_ROOT
    }
    if (buttonNames.size() != _buttonNumbers) EXCEPTION(EPROGFAIL);
    if (      icons.size() != _buttonNumbers) EXCEPTION(EPROGFAIL);
}


void cDialogButtons::init(int buttons, QBoxLayout *pL)
{
    pL->addStretch();
    for (int id = 0; _buttonNumbers > id; ++id) {
        if (buttons & enum2set(id)) {
            PDEB(VVERBOSE) << "Add button #" << id << " " << buttonNames[id] << endl;
            QPushButton *pPB = new QPushButton(icons[id], buttonNames[id], _pWidget);
            addButton(pPB, id);
            pL->addWidget(pPB);
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
cRecordDialogBase::cRecordDialogBase(const cTableShape &__tm, int _buttons, bool dialog, QWidget *par, cRecordDialogBase *own)
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
    _pOwner   = own;

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
    if (_pOwner != NULL && _pOwner->disabled()) {
        _isDisabled = true;
        _viewRights = _pOwner->_viewRights;
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
        Ui::noRightsForm *noRighrs = noRighrsSetup(_pWidget, _viewRights, objectName());
        if (_pOwner == NULL) {
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
        if (_isReadOnly || _isDisabled) _pButtons->disableExcept();
        else                            _pButtons->enabeAll();
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

cRecordAny * cRecordDialogBase::insertDialog(QSqlQuery& q, cTableShape *pTableShape, const cRecStaticDescr *pRecDescr, QWidget * _par)
{
    eTableInheritType tit = (eTableInheritType)pTableShape->getId(_sTableInheritType);
    // A dialógusban megjelenítendő nyomógombok.
    int buttons = enum2set(DBT_OK, DBT_INSERT, DBT_CANCEL);
    switch (tit) {
    case TIT_NO:
    case TIT_ONLY: {
        cRecordAny rec(pRecDescr);
        cRecordDialog   rd(*pTableShape, buttons, true, _par);  // A rekord szerkesztő dialógus
        rd.dialog().setModal(true);
        rd.restore(&rec);
        while (1) {
            int r = rd.exec();
            if (r == DBT_INSERT || r == DBT_OK) {   // Csak az OK, és Insert gombra csinálunk valamit
                bool ok = rd.accept();
                cRecordAny *pRec = new cRecordAny(rd.record());
                if (ok) {
                    ok = cRecordViewModelBase::SqlInsert(q, pRec);
                }
                if (ok) {
                    if (r == DBT_OK) {  // Ha OK-t nyomott becsukjuk az dialóg-ot
                        return pRec;
                    }
                    pDelete(pRec);
                    continue;               // Ha Insert-et, akkor folytathatja a következővel
                }
                else {
                    pDelete(pRec);
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
        tableNames << pTableShape->get(_sInheritTableNames).toStringList();;
        tableNames.removeDuplicates();
        foreach (QString tableName, tableNames) {
            shapes << cRecordsViewBase::getInhShape(q, pTableShape, tableName);
        }
        cRecordDialogInh rd(*pTableShape, shapes, buttons, NULL_ID, NULL_ID, true, _par);
        while (1) {
            int r = rd.exec();
            if (r == DBT_INSERT || r == DBT_OK) {
                bool ok = rd.accept();
                cRecordAny *pRec = new cRecordAny(rd.record());
                if (ok) {
                    ok = cRecordViewModelBase::SqlInsert(q, pRec);
                }
                if (ok) {
                    if (r == DBT_OK) {      // Ha OK-t nyomott becsukjuk az dialóg-ot
                        return pRec;
                    }
                    continue;               // Ha Insert-et, akkor folytathatja a következővel
                }
                else {
                    continue;
                }
            }
            break;
        }
    }   break;
    default:
        EXCEPTION(ENOTSUPP);
    }
    return NULL;
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

cRecordDialog::cRecordDialog(const cTableShape& __tm, int _buttons, bool dialog, QWidget *parent, cRecordDialogBase *own)
    : cRecordDialogBase(__tm, _buttons, dialog, parent, own)
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
    const int maxFields = 10;
    DBGFN();
    pFormLayout = new QFormLayout;
    pFormLayout->setObjectName(name + "_Form");
    int n = tableShape.shapeFields.size();
    if (n > maxFields) {
        pSplittLayout = new QBoxLayout(QBoxLayout::LeftToRight);
        pSplitter     = new QSplitter(_pWidget);
        pSplittLayout->addWidget(pSplitter);
    }
    if (_pButtons != NULL) {
        pVBoxLayout = new QVBoxLayout;
        pVBoxLayout->setObjectName(name + "_VBox");
        _pWidget->setLayout(pVBoxLayout);
        if (n > maxFields) {
            pVBoxLayout->addLayout(pSplittLayout);
            pSplitter->addWidget(_frame(pFormLayout, _pWidget));
        }
        else {
            pVBoxLayout->addLayout(pFormLayout);
        }
        pVBoxLayout->addWidget(_pButtons->pWidget());
    }
    else {
        pVBoxLayout = NULL;
        if (n > maxFields) {
             _pWidget->setLayout(pSplittLayout);
             pSplitter->addWidget(_frame(pFormLayout, _pWidget));
        }
        else {
            _pWidget->setLayout(pFormLayout);
        }
    }
    tTableShapeFields::const_iterator i, e = tableShape.shapeFields.cend();
    _pRecord = new cRecordAny(&rDescr);
    int cnt = 0;
    for (i = tableShape.shapeFields.cbegin(); i != e; ++i) {
        const cTableShapeField& mf = **i;
        if (mf.getBool(_sFieldFlags, FF_DIALOG_HIDE)) continue;
        if (++cnt > maxFields) {
            cnt = 0;
            pFormLayout = new QFormLayout;  // !!!
            pSplitter->addWidget(_frame(pFormLayout, _pWidget));
        }
        int fieldIx = rDescr.toIndex(mf.getName());
        bool setRo = _isReadOnly || mf.getBool(_sFieldFlags, FF_READ_ONLY);
        cFieldEditBase *pFW = cFieldEditBase::createFieldWidget(tableShape, mf, (*_pRecord)[fieldIx], setRo, this);
        fields.append(pFW);
        QWidget * pw = pFW->pWidget();
        pw->setObjectName(mf.getName());
        PDEB(VVERBOSE) << "Add row to form : " << mf.getName() << " widget : " << typeid(*pFW).name() << QChar('/') << fieldWidgetType(pFW->wType()) << endl;
        pw->adjustSize();
        pFormLayout->addRow(mf.getName(_sDialogTitle), pw);
    }
    _pWidget->adjustSize();
    DBGFNL();
}

void cRecordDialog::restore(cRecord *_pRec)
{
    if (_pRec != NULL) {
        pDelete(_pRecord);
        _pRecord = new cRecordAny(*_pRec);
    }
    else if (_pRecord == NULL) {
        _pRecord = new cRecordAny(&rDescr);
    }
    int i, n = fields.size();
    for (i = 0; i < n; i++) {
        cFieldEditBase& field = *fields[i];
        field.set(_pRecord->get(field._pFieldRef->index()));
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
        int rfi = field._pFieldRef->index();
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

cRecordDialogInh::cRecordDialogInh(const cTableShape& _tm, tRecordList<cTableShape>& _tms, int _buttons, qlonglong _oid, qlonglong _pid, bool dialog, QWidget * parent)
    : cRecordDialogBase(_tm, _buttons, dialog, parent)
    , tabDescriptors(_tms)
    , tabs()
{
    pVBoxLayout = NULL;
    pTabWidget = NULL;
    if (_isDisabled) return;
    if (_pButtons == NULL) EXCEPTION(EPROGFAIL);
    // if (isReadOnly) EXCEPTION(EDATA);

    init(_oid, _pid);
}

cRecordDialogInh::~cRecordDialogInh()
{
    while (tabs.size()) {
        delete tabs.first();
        tabs.pop_front();
    }
}

void cRecordDialogInh::init(qlonglong _oid, qlonglong _pid)
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
        cRecordDialog * pDlg = new cRecordDialog(shape, 0, false, pTabWidget, this);
        cRecordAny * pRec = new cRecordAny(shape.getName(_sTableName), shape.getName(_sSchemaName));
        if (_oid != NULL_ID) {  // Ha van owner, akkor az ID-jét beállítjuk
            int oix = pRec->descr().ixToOwner();
            pRec->setId(oix, _oid);
        }
        if (_pid != NULL_ID) {  // Ha van parent, akkor az ID-jét beállítjuk
            int pix = pRec->descr().ixToParent();
            pRec->setId(pix, _pid);
            pDlg->_isReadOnly = true;
        }
        pDlg->_pRecord = pRec;
        pDlg->restore();
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
    (void)_pRec;
    if (disabled()) return;
    EXCEPTION(EPROGFAIL);
}

