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

void cRecordDialogBase::_pressed(int id)
{
    if      (_pLoop != NULL) _pLoop->exit(id);

    else if (isDialog)       dialog().done(id);
    else                     buttonPressed(id);
}

/* ************************************************************************************************* */

cRecordDialog::cRecordDialog(cRecord &rec, cTableShape& __tm, int _buttons, bool dialog, QWidget *parent)
    : cRecordDialogBase(__tm, _buttons, dialog, parent)
    , record(rec)
    , fields()
{
    pVBoxLayout = NULL;
    //pHBoxLayout = NULL;
    pSplitter = NULL;
    pSplittLayout = NULL;
    pFormLayout = NULL;
    if (descriptor.getName(_sTableName) != record.tableName()) EXCEPTION(EDATA);
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
        cTableShapeField& mf = **i;
        int fieldIx = record.toIndex(mf.getName());
        bool setRo = isReadOnly || mf.getBool(_sIsReadOnly);
        cFieldEditBase *pFW = cFieldEditBase::createFieldWidget(descriptor, record[fieldIx], SY_NO, setRo, _pWidget);
        fields.append(pFW);
        QWidget * pw = pFW->pWidget();
        pw->setObjectName(mf.getName());
        PDEB(VVERBOSE) << "Add row to form : " << mf.getName() << " widget : " << typeid(*pFW).name() << _sSlash << fieldWidgetType(pFW->wType()) << endl;
        pw->adjustSize();
        pFormLayout->addRow(mf.getDescr(), pw);
    }
    _pWidget->adjustSize();
    DBGFNL();
}

void cRecordDialog::set(const cRecord& _r)
{
    if (_r.descr() != record.descr()) EXCEPTION(EDATA);
    int i, n = record.descr().cols();
    for (i = 0; i < n; i++) {
        fields[i]->set(_r.get(i));
    }
}

bool cRecordDialog::get(cRecord& _r)
{
    _errMsg.clear();
    if (_r.descr() != record.descr()) EXCEPTION(EDATA);
    int i, n = record.descr().cols();
    _r.set();
    for (i = 0; i < n; i++) {
        int s = _r._stat;
        _r._stat &= ~cRecord::ES_DEFECTIVE;
        QVariant fv = fields[i]->get();
        PDEB(VERBOSE) << "Dialog -> obj. field " << _r.columnName(i) << " = " << debVariantToString(fv) << endl;
        _r.set(i, fv);
        if (_r._stat & cRecord::ES_DEFECTIVE) {
            DWAR() << "Invalid data : field " << _r.columnName(i) << " = " << debVariantToString(fv) << endl;
            _errMsg += trUtf8("Adat hiba a %1 mezőnél\n").arg(_r.columnName(i));
        }
        _r._stat |= s & cRecord::ES_DEFECTIVE;
    }
    return 0 == (_r._stat & cRecord::ES_DEFECTIVE);
}


void cRecordDialog::restore()
{
    set(record);
}

/* ***************************************************************************************************** */

cRecordDialogTab::cRecordDialogTab(const cTableShape& _tm, tRecordList<cTableShape>& _tms, int _buttons, qlonglong _oid, bool dialog, QWidget * parent)
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

void cRecordDialogTab::init(qlonglong _oid)
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
        cAlternate * pRec = new cAlternate(shape.getName(_sTableName), shape.getName(_sSchemaName));
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

int cRecordDialogTab::actTab()
{
    int i =  pTabWidget->currentIndex();
    if (!isContIx(tabs, i)) EXCEPTION(EPROGFAIL, i);
    return i;
}

void cRecordDialogTab::setActTab(int i)
{
    if (!isContIx(tabs, i)) EXCEPTION(EPROGFAIL, i);
    pTabWidget->setCurrentIndex(i);
}

bool cRecordDialogTab::get(cRecord& _r)
{
    _errMsg.clear();
    bool  r = tabs[actTab()]->get(_r);
    if (!r) _errMsg = tabs[actTab()]->errMsg();
    return r;
}
