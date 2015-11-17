#include "record_table.h"


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

/* ************************************************************************************************* */
cRecordDialogBase::cRecordDialogBase(const cTableShape &__tm, int _buttons, bool dialog, QWidget *par)
    : QObject(par)
    , tableShape(__tm)
    , rDescr(*cRecStaticDescr::get(__tm.getName(_sTableName), __tm.getName(_sSchemaName)))
    , name(tableShape.getName())
    , _errMsg()
{
    isDialog = dialog;
    _pRecord = NULL;
    _pLoop   = NULL;
    _pWidget = NULL;
    _pButtons= NULL;

    pq = newQuery();
    setObjectName(name);
    isReadOnly = tableShape.getBool(_sIsReadOnly);
    if (dialog) _pWidget = new QDialog(par);
    else        _pWidget = new QWidget(par);
    if (_buttons != 0) {
        _pButtons = new cDialogButtons(_buttons, 0, _pWidget);
        _pButtons->widget().setObjectName(name + "_Buttons");
        connect(_pButtons, SIGNAL(buttonClicked(int)), this, SLOT(_pressed(int)));
    }

    _pWidget->setObjectName(name + "_Widget");
    _pWidget->setWindowTitle(tableShape.getNote());
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
    if (_pButtons == NULL) EXCEPTION(EPROGFAIL);
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

    else if (isDialog)       dialog().done(id);
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

cRecordDialog::cRecordDialog(const cTableShape& __tm, int _buttons, bool dialog, QWidget *parent)
    : cRecordDialogBase(__tm, _buttons, dialog, parent)
    , fields()
{
    pVBoxLayout = NULL;
    //pHBoxLayout = NULL;
    pSplitter = NULL;
    pSplittLayout = NULL;
    pFormLayout = NULL;
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
        if (++cnt > maxFields) {
            cnt = 0;
            pFormLayout = new QFormLayout;  // !!!
            pSplitter->addWidget(_frame(pFormLayout, _pWidget));
        }
        const cTableShapeField& mf = **i;
        int fieldIx = rDescr.toIndex(mf.getName());
        bool setRo = isReadOnly || mf.getBool(_sIsReadOnly);
        cFieldEditBase *pFW = cFieldEditBase::createFieldWidget(tableShape, mf, (*_pRecord)[fieldIx], setRo, this);
        fields.append(pFW);
        QWidget * pw = pFW->pWidget();
        pw->setObjectName(mf.getName());
        PDEB(VVERBOSE) << "Add row to form : " << mf.getName() << " widget : " << typeid(*pFW).name() << QChar('/') << fieldWidgetType(pFW->wType()) << endl;
        pw->adjustSize();
        pFormLayout->addRow(mf.getNote(), pw);
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
    // record.set();           // NEM !! Kinullázzuk a rekordot
    for (i = 0; i < n; i++) {   // Végigszaladunk a mezőkön
        cFieldEditBase& field = *fields[i];
        int rfi = field._pFieldRef->index();
        if (field.isReadOnly()) continue;      // Feltételezzük, hogy RO esetén az van a mezőben aminek lennie kell.
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
    return 0 == (_pRecord->_stat & ES_DEFECTIVE);
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
    if (_pButtons == NULL) EXCEPTION(EPROGFAIL);
    if (isReadOnly) EXCEPTION(EDATA);

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
        cRecordDialog * pDlg = new cRecordDialog(shape, 0, false, pTabWidget);
        cRecordAny * pRec = new cRecordAny(shape.getName(_sTableName), shape.getName(_sSchemaName));
        if (_oid != NULL_ID) {  // Ha van owner, akkor az ID-jét beállítjuk
            int oix = pRec->descr().ixToOwner();
            pRec->setId(oix, _oid);
        }
        if (_pid != NULL_ID) {  // Ha van parent, akkor az ID-jét beállítjuk
            int pix = pRec->descr().ixToParent();
            pRec->setId(pix, _pid);
            pDlg->isReadOnly = true;
        }
        pDlg->_pRecord = pRec;
        pDlg->restore();
        tabs << pDlg;
        pTabWidget->addTab(pDlg->pWidget(), shape.getName(_sTableName));
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
}

bool cRecordDialogInh::accept()
{
    _errMsg.clear();
    cRecordDialog *prd = tabs[actTab()];
    bool  r = prd->accept();
    if (!r) _errMsg = prd->errMsg();
    return r;
}

cFieldEditBase * cRecordDialogInh::operator[](const QString& __fn)
{
    return actDialog()[__fn];
}
