#include "record_table.h"
#include "ui_no_rights.h"


/* ************************************************************************************************* */

QStringList     cDialogButtons::buttonNames;
QList<QIcon>    cDialogButtons::icons;
QList<int>      cDialogButtons::keys;

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
                _pLayout->addWidget(addPB(id, _pWidget));
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
                pL->addWidget(addPB(id, _pWidget));
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
        appendCont(buttonNames, trUtf8("OK"),          icons, QIcon::fromTheme("emblem-default"), keys, Qt::Key_Enter,  DBT_OK);
        appendCont(buttonNames, trUtf8("Bezár"),       icons, QIcon::fromTheme("window-close"),   keys, Qt::Key_Escape, DBT_CLOSE);
        appendCont(buttonNames, trUtf8("Frissít"),     icons, QIcon::fromTheme("reload"),         keys, Qt::Key_F5,     DBT_REFRESH);
        appendCont(buttonNames, trUtf8("Új"),          icons, QIcon::fromTheme("insert-image"),   keys, Qt::Key_Insert, DBT_INSERT);
        appendCont(buttonNames, trUtf8("Módosít"),     icons, QIcon::fromTheme("text-editor"),    keys, 0,              DBT_MODIFY);
        appendCont(buttonNames, trUtf8("Ment"),        icons, QIcon::fromTheme("list-add"),       keys, 0,              DBT_SAVE);
        appendCont(buttonNames, trUtf8("Első"),        icons, QIcon::fromTheme("go-first"),       keys, Qt::Key_Home,   DBT_FIRST);
        appendCont(buttonNames, trUtf8("Előző"),       icons, QIcon::fromTheme("go-previous"),    keys, Qt::Key_PageUp, DBT_PREV);
        appendCont(buttonNames, trUtf8("Következő"),   icons, QIcon::fromTheme("go-next"),        keys, Qt::Key_PageDown,DBT_NEXT);
        appendCont(buttonNames, trUtf8("Utolsó"),      icons, QIcon::fromTheme("go-last"),        keys, Qt::Key_End,    DBT_LAST);
        appendCont(buttonNames, trUtf8("Töröl"),       icons, QIcon::fromTheme("list-remove"),    keys, Qt::Key_Delete, DBT_DELETE);
        appendCont(buttonNames, trUtf8("Visszaállít"), icons, QIcon::fromTheme("edit-redo"),      keys, 0,              DBT_RESTORE);
        appendCont(buttonNames, trUtf8("Elvet"),       icons, QIcon::fromTheme("gtk-cancel"),     keys, Qt::Key_Escape, DBT_CANCEL);
        appendCont(buttonNames, trUtf8("Alapállapot"), icons, QIcon::fromTheme("go-first"),       keys, 0,              DBT_RESET);
        appendCont(buttonNames, trUtf8("Betesz"),      icons, QIcon::fromTheme("insert-image"),   keys, Qt::Key_Plus,   DBT_PUT_IN);
        appendCont(buttonNames, trUtf8("Kivesz"),      icons, QIcon::fromTheme("list-remove"),    keys, Qt::Key_Minus,  DBT_TAKE_OUT);
        appendCont(buttonNames, trUtf8("Kibont"),      icons, QIcon::fromTheme("zoom-in"),        keys, Qt::Key_Plus,   DBT_EXPAND);
        appendCont(buttonNames, trUtf8("Gyökér"),      icons, QIcon(),                            keys, 0,              DBT_ROOT);
        appendCont(buttonNames, trUtf8("Másol"),       icons, QIcon::fromTheme("copy"),           keys, 0,              DBT_COPY);
        appendCont(buttonNames, trUtf8("Kiürít"),      icons, QIcon::fromTheme("edit-clear"),     keys, 0,              DBT_TRUNCATE);
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
cRecordDialogBase::cRecordDialogBase(const cTableShape &__tm, int _buttons, bool dialog, cRecordDialogBase *ownDialog, cRecordsViewBase *ownTab, QWidget *par)
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

/*
cRecord * cRecordDialogBase::insertDialog(QSqlQuery& q, cTableShape *pTableShape, const cRecStaticDescr *pRecDescr, QWidget * _par)
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
                cRecord *pRec = rd.record().dup();
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
                cRecord *pRec = rd.record().dup();
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
*/
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

cRecordDialog::cRecordDialog(const cTableShape& __tm, int _buttons, bool dialog, cRecordDialogBase *ownDialog, cRecordsViewBase *ownTab, QWidget *parent)
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
    maxFields = tableShape.feature(pointCat(_sDialog, _sHeight)).toInt(&ok);
    if (!ok) maxFields = 10;    // Default

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
            pVBoxLayout->addLayout(pSplittLayout, 1);
            pSplitter->addWidget(_frame(pFormLayout, _pWidget));
        }
        else {
            pVBoxLayout->addLayout(pFormLayout, 1);
        }
        pVBoxLayout->addWidget(_pButtons->pWidget(), 0);
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

    if (_pOwnerTable != NULL && _pOwnerTable->owner_id != NULL_ID) {  // Ha van owner, akkor az ID-jét beállítjuk
        int oix = _pRecord->descr().ixToOwner();
        _pRecord->setId(oix, _pOwnerTable->owner_id);
    }
    if (_pOwnerTable != NULL && _pOwnerTable->parent_id != NULL_ID) {  // Ha van parent, akkor az ID-jét beállítjuk  ????
        int pix = _pRecord->descr().ixToParent();
        _pRecord->setId(pix, _pOwnerTable->parent_id);
    }

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
        qlonglong stretch = mf.feature(pointCat(_sStretch, _sVertical), 0);
        if (stretch > 0) {
            QSizePolicy sp = pw->sizePolicy();
            sp.setVerticalStretch(stretch);
            pw->setSizePolicy(sp);
        }
        pw->setObjectName(mf.getName());
        PDEB(VVERBOSE) << "Add row to form : " << mf.getName() << " widget : " << typeid(*pFW).name() << QChar('/') << fieldWidgetType(pFW->wType()) << endl;
        pFormLayout->addRow(mf.getName(_sDialogTitle), pw);
    }
    _pWidget->adjustSize();
    DBGFNL();
}

void cRecordDialog::restore(cRecord *_pRec)
{
    if (_pRec != NULL) {
        pDelete(_pRecord);
        _pRecord = _pRec->dup();
    }
    else if (_pRecord == NULL) {
        _pRecord = new cRecordAny(&rDescr);
    }
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

cRecordDialogInh::cRecordDialogInh(const cTableShape& _tm, tRecordList<cTableShape>& _tms, int _buttons, bool dialog, cRecordDialogBase *ownDialog, cRecordsViewBase *ownTab, QWidget * parent)
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
        cRecord * pRec = new cRecordAny(shape.getName(_sTableName), shape.getName(_sSchemaName));
        if (_pOwnerTable != NULL && _pOwnerTable->owner_id != NULL_ID) {  // Ha van owner, akkor az ID-jét beállítjuk
            int oix = pRec->descr().ixToOwner();
            pRec->setId(oix, _pOwnerTable->owner_id);
        }
        if (_pOwnerTable != NULL && _pOwnerTable->parent_id != NULL_ID) {  // Ha van parent, akkor az ID-jét beállítjuk
            int pix = pRec->descr().ixToParent();
            pRec->setId(pix, _pOwnerTable->parent_id);
            pDlg->_isReadOnly = true;// ??
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

