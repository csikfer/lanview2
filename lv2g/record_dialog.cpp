#include "record_dialog.h"


/* ************************************************************************************************* */

QStringList     cDialogButtons::buttonNames;
QList<QIcon>    cDialogButtons::icons;

cDialogButtons::cDialogButtons(int buttons, int buttons2, QWidget *par)
    :QButtonGroup(par)
{
    if (buttonNames.isEmpty()) {
        buttonNames << trUtf8("OK");            icons << QIcon::fromTheme("emblem-default");
        buttonNames << trUtf8("Bezár");         icons << QIcon::fromTheme("window-close");
        buttonNames << trUtf8("Frissít");       icons << QIcon::fromTheme("reload");
        buttonNames << trUtf8("Beilleszt");     icons << QIcon::fromTheme("insert-image");
        buttonNames << trUtf8("Módosít");       icons << QIcon::fromTheme("text-editor");
        buttonNames << trUtf8("Ment");          icons << QIcon::fromTheme("list-add");
        buttonNames << trUtf8("Első");          icons << QIcon::fromTheme("go-first");
        buttonNames << trUtf8("Előző");         icons << QIcon::fromTheme("go-previous");
        buttonNames << trUtf8("Következő");     icons << QIcon::fromTheme("go-next");
        buttonNames << trUtf8("Utolsó");        icons << QIcon::fromTheme("go-last");
        buttonNames << trUtf8("Töröl");         icons << QIcon::fromTheme("list-remove");
        buttonNames << trUtf8("Visszaállít");   icons << QIcon::fromTheme("edit-redo");
        buttonNames << trUtf8("Elvet");         icons << QIcon::fromTheme("gtk-cancel");
    }
    if (0 == (buttons & ((1 << DBT_BUTTON_NUMBERS) - 1))) EXCEPTION(EDATA);
    _pWidget = new QWidget(par);
    if (buttons2 == 0) {        // Egy gombsor
        _pLayout = NULL;
        _pLayoutBotom = NULL;
        _pLayoutTop = new QHBoxLayout(_pWidget);
        init(buttons, _pLayoutTop);
    }
    else {                      // Két gombsor
        _pLayout = new QVBoxLayout(_pWidget);
        _pLayoutTop   = new QHBoxLayout();
        _pLayoutBotom = new QHBoxLayout();
        _pLayout->addLayout(_pLayoutTop);
        _pLayout->addLayout(_pLayoutBotom);
        init(buttons, _pLayoutTop);
        init(buttons2, _pLayoutBotom);
    }
}

void cDialogButtons::init(int buttons, QHBoxLayout *pL)
{
    pL->addStretch();
    for (int id = 0; DBT_BUTTON_NUMBERS > id; ++id) {
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
    , descriptor(__tm)
    , name(descriptor.getName())
    , _errMsg()
{
    isDialog = dialog;
    _pLoop = NULL;
    _pWidget = NULL;
    _pButtons = NULL;

    pq = newQuery();
    setObjectName(name);
    isReadOnly = descriptor.getBool(_sIsReadOnly);
    if (dialog) _pWidget = new QDialog(par);
    else        _pWidget = new QWidget(par);
    if (_buttons != 0) {
        _pButtons = new cDialogButtons(_buttons, 0, _pWidget);
        _pButtons->widget().setObjectName(name + "_Buttons");
        connect(_pButtons, SIGNAL(buttonClicked(int)), this, SLOT(_pressed(int)));
    }

    _pWidget->setObjectName(name + "_Widget");
    _pWidget->setWindowTitle(descriptor.getDescr());
}

cRecordDialogBase::~cRecordDialogBase()
{
    if (_pLoop != NULL) EXCEPTION(EPROGFAIL);
    if (!close()) EXCEPTION(EPROGFAIL);
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

/// Slot a megnyomtak egy gombot szignálra.
void cRecordDialogBase::_pressed(int id)
{
    if      (_pLoop != NULL) _pLoop->exit(id);

    else if (isDialog)       dialog().done(id);
    else                     buttonPressed(id);
}

/* ************************************************************************************************* */

cRecordDialog::cRecordDialog(cRecord &rec, cTableShape& __tm, int _buttons, bool dialog, QWidget *parent)
    : cRecordDialogBase(__tm, _buttons, dialog, parent)
    , _record(rec)
    , fields()
{
    pVBoxLayout = NULL;
    //pHBoxLayout = NULL;
    pSplitter = NULL;
    pSplittLayout = NULL;
    pFormLayout = NULL;
    if (descriptor.getName(_sTableName) != _record.tableName()) EXCEPTION(EDATA);
    if (descriptor.shapeFields.size() == 0) EXCEPTION(EDATA);
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
    const int maxFields = 15;
    DBGFN();
    pFormLayout = new QFormLayout;
    pFormLayout->setObjectName(name + "_Form");
    int n = descriptor.shapeFields.size();
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
    tTableShapeFields::const_iterator i, e = descriptor.shapeFields.cend();
    int cnt = 0;
    for (i = descriptor.shapeFields.cbegin(); i != e; ++i) {
        if (++cnt > maxFields) {
            cnt = 0;
            pFormLayout = new QFormLayout;  // !!!
            pSplitter->addWidget(_frame(pFormLayout, _pWidget));
        }
        const cTableShapeField& mf = **i;
        int fieldIx = _record.toIndex(mf.getName());
        bool setRo = isReadOnly || mf.getBool(_sIsReadOnly);
        cFieldEditBase *pFW = cFieldEditBase::createFieldWidget(descriptor, _record[fieldIx], SY_NO, setRo, _pWidget);
        fields.append(pFW);
        QWidget * pw = pFW->pWidget();
        pw->setObjectName(mf.getName());
        PDEB(VVERBOSE) << "Add row to form : " << mf.getName() << " widget : " << typeid(*pFW).name() << QChar('/') << fieldWidgetType(pFW->wType()) << endl;
        pw->adjustSize();
        pFormLayout->addRow(mf.getDescr(), pw);
    }
    _pWidget->adjustSize();
    DBGFNL();
}

cRecord& cRecordDialog::record()
{
    return _record;
}

void cRecordDialog::restore()
{
    int i, n = _record.descr().cols();
    for (i = 0; i < n; i++) {
        fields[i]->set(_record.get(i));
    }
}

bool cRecordDialog::accept()
{
    _errMsg.clear();    // Töröljük a hiba stringet
    int i, n = _record.descr().cols();
    // record.set();           // NEM !! Kinullázzuk a rekordot
    for (i = 0; i < n; i++) {   // Végigszaladunk a mezőkön
        cFieldEditBase& fe = *fields[i];
        if (fe.isReadOnly()) continue;      // FEltételezzük, hogy RO esetén az van a mezőben aminek lennie kell.
        int s = _record._stat;                   // Mentjük a hiba bitet,
        _record._stat &= ~cRecord::ES_DEFECTIVE; // majd töröljük, mert mezőnkként kell
        QVariant fv = fields[i]->get();     // A mező widget-jéből kivesszük az értéket
        if (fv.isNull() && fe._isInsert && fe._hasDefault) continue;    // NULL, insert, van alapérték
        PDEB(VERBOSE) << "Dialog -> obj. field " << _record.columnName(i) << " = " << debVariantToString(fv) << endl;
        _record.set(i, fv);                      // Az értéket bevéssük a rekordba
        if (_record._stat & cRecord::ES_DEFECTIVE) {
            DWAR() << "Invalid data : field " << _record.columnName(i) << " = " << debVariantToString(fv) << endl;
            _errMsg += trUtf8("Adat hiba a %1 mezőnél\n").arg(_record.columnName(i));
        }
        _record._stat |= s & cRecord::ES_DEFECTIVE;
    }
    return 0 == (_record._stat & cRecord::ES_DEFECTIVE);
}


/* ***************************************************************************************************** */

cRecordDialogInh::cRecordDialogInh(const cTableShape& _tm, tRecordList<cTableShape>& _tms, int _buttons, qlonglong _oid, bool dialog, QWidget * parent)
    : cRecordDialogBase(_tm, _buttons, dialog, parent)
    , tabDescriptors(_tms)
    , recs()
    , tabs()
{
    pVBoxLayout = NULL;
    pTabWidget = NULL;
    if (_pButtons == NULL) EXCEPTION(EPROGFAIL);
    if (isReadOnly) EXCEPTION(EDATA);

    init(_oid);
}

void cRecordDialogInh::init(qlonglong _oid)
{
    DBGFN();
    pTabWidget = new QTabWidget;
    pTabWidget->setObjectName(name + "_tab");
    pVBoxLayout = new QVBoxLayout;
    pVBoxLayout->setObjectName(name + "_VBox");
    _pWidget->setLayout(pVBoxLayout);
    pVBoxLayout->addWidget(pTabWidget);
    pVBoxLayout->addWidget(_pButtons->pWidget());
    int i, n = tabDescriptors.size();
    for (i = 0; i < n; ++i) {
        cTableShape& shape = *tabDescriptors[i];
        cRecordAny * pRec = new cRecordAny(shape.getName(_sTableName), shape.getName(_sSchemaName));
        if (_oid != NULL_ID) {
            int oix = pRec->descr().ixToOwner();
            pRec->setId(oix, _oid);
        }
        cRecordDialog * pDlg = new cRecordDialog(*pRec, shape, 0, false, pTabWidget);
        recs << pRec;
        tabs << pDlg;
        pTabWidget->addTab(pDlg->pWidget(), shape.getName());
    }
    pTabWidget->setCurrentIndex(0);
    _pWidget->adjustSize();
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
