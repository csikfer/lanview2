#include "record_table.h"
#include "ui_no_rights.h"
#include "QRegExp"
#include "lv2validator.h"
#include "object_dialog.h"

/// Tartalom alapján beállítja az aktuális indexet a megadott comboBox-on
/// @param name Az érték, amire az aktuális indexnek mutatnia kell (megjelenített)
/// @param pComboBox A QComboBox objektum pointere, ahhol az indexet be kell állítani.
/// @param __ex Ha értéke EX_IGNORE, akkor ha nem találja a megfelelő elemet, amire az indexet állítani kell,
/// akkor az index a 0 lessz. Ha értéke nem EX_IGNORE, akkor ha nincs megadott elem dob egy kizárást.
/// @return A beállitott index értékkel tér vissza
int _setCurrentIndex(const QString& name, QComboBox * pComboBox, eEx __ex)
{
    int ix = pComboBox->findText(name);
    if (ix < 0) {
        if (EX_IGNORE != __ex) EXCEPTION(EDATA, 0, name);
        ix = 0;
    }
    pComboBox->setCurrentIndex(ix);
    return ix;
}
/// Tartalom alapján beállítja az aktuális indexet a megadott comboBox-on
/// @param name Az érték, amire az aktuális indexnek mutatnia kell. (rekord név!)
/// @param pComboBox A QComboBox objektum pointere, ahhol az indexet be kell állítani.
/// @param pListModel A model pointere (a név, és a megjelenített item nem azonos)
/// @param __ex Ha értéke EX_IGNORE, akkor ha nem találja a megfelelő elemet, amire az indexet állítani kell,
/// akkor az index a 0 lessz. Ha értéke nem EX_IGNORE, akkor ha nincs megadott elem dob egy kizárást.
/// @return A beállitott index értékkel tér vissza
int _setCurrentIndex(const QString& name, QComboBox * pComboBox, cRecordListModel *pListModel, eEx __ex)
{
    int ix = pListModel->indexOf(name);
    if (ix < 0) {
        if (EX_IGNORE != __ex) EXCEPTION(EDATA, 0, name);
        ix = 0;
    }
    pComboBox->setCurrentIndex(ix);
    return ix;
}

/// Egy magadott zóna és hely comboBox pároson állítja be a megadott helyet.
/// A helyeket megjelenítő comboBox-nál feltételezi, hogy a beállított model objektum típusa
/// cPlacesInZoneModel.
/// Ha az aktuális hely listában nincs a megadott hely, akkor a zónát 'all'-ra állítja,
/// aminek az aktuális indexe a zónák listában a 0.
/// Ha a megadott ID az UNKNOWN_PLACE_ID vagy annál kisebb, akkor a beállított hely index a 0,
/// ami az 'unknown' ill. a nincs megadva hely (üres string).
/// @param pid A beállítandő hely (cPlace) ID-je.
/// @param pComboBoxZone A zónákat megjelenítő QComboBox objektum pointere. Az első elem az 'all' nevű zóna.
/// @param pComboBoxPlace A helyeket egjelenítő QComboBox objektum pointere.
/// @param refresh Ha értéke igaz, akkor frissíti a hely listát, a modllen keresztűl.
int _setPlaceComboBoxs(qlonglong pid, QComboBox *pComboBoxZone, QComboBox *pComboBoxPlace, bool refresh)
{
    if (pid <= UNKNOWN_PLACE_ID) {
        pComboBoxPlace->setCurrentIndex(0);
        return 0;
    }
    cPlacesInZoneModel *pModel = (cPlacesInZoneModel *)pComboBoxPlace->model();
    if (refresh) pModel->setFilter();
    int ix = pModel->indexOf(pid);
    if (ix < 0) {   // Nincs ilyen hely a zónában
        pComboBoxZone->setCurrentIndex(0);  // set "all" zone
        pModel->setFilter(ALL_PLACE_GROUP_ID);
        ix = pModel->indexOf(pid);         // Az "all" zónában már lennie kell!
        if (ix < 0) EXCEPTION(EDATA);
    }
    pComboBoxPlace->setCurrentIndex(ix);
    return ix;
}

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
        appendCont(buttonNames, _sNul,                 icons, QIcon(":/icons/refresh.ico"), keys, Qt::Key_F5,     DBT_REFRESH);
        appendCont(buttonNames, trUtf8("Új"),          icons, QIcon(":/icons/insert.ico"),  keys, Qt::Key_Insert, DBT_INSERT);
        appendCont(buttonNames, trUtf8("Hasonló"),     icons, QIcon(":/icons/insert.ico"),  keys, 0,              DBT_SIMILAR);
        appendCont(buttonNames, trUtf8("Módosít"),     icons, QIcon(":/icons/edit.ico"),    keys, 0,              DBT_MODIFY);
        appendCont(buttonNames, trUtf8("Ment"),        icons, QIcon(":/icons/save.ico"),    keys, 0,              DBT_SAVE);
        appendCont(buttonNames, _sNul,                 icons, QIcon(":/icons/first.ico"),   keys, Qt::Key_Home,   DBT_FIRST);
        appendCont(buttonNames, _sNul,                 icons, QIcon(":/icons/previous.ico"),keys, Qt::Key_PageUp, DBT_PREV);
        appendCont(buttonNames, _sNul,                 icons, QIcon(":/icons/next.ico"),    keys, Qt::Key_PageDown,DBT_NEXT);
        appendCont(buttonNames, _sNul,                 icons, QIcon(":/icons/last.ico"),    keys, Qt::Key_End,    DBT_LAST);
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

QAbstractButton *cDialogButtons::addPB(int id, QWidget *par)
{
    QAbstractButton *pPB;
    if (buttonNames.isEmpty()) {
        pPB = new QToolButton(par);
        pPB->setIcon(icons[id]);
    }
    else {
        pPB = new QPushButton(icons[id], buttonNames[id], par);
    }
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
    tableShape.clone(__tm);

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
    tableShape.fetchText(*pq);
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
        _pWidget->setWindowTitle(tableShape.getText(cTableShape::LTX_DIALOG_TITLE, tableShape.getName()));
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
                int oix;
                if (_pOwnerTable == NULL) oix = _pRecord->descr().ixToOwner();
                else                      oix = _pOwnerTable->ixToOwner();
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
        int flags = _isReadOnly || mf.getBool(_sFieldFlags, FF_READ_ONLY) ? FEB_READ_ONLY : FEB_EDIT;   // FEB_INSERT !!
        int fieldIx = rDescr.toIndex(mf.getName(), EX_IGNORE);
        cFieldEditBase *pFW;
        if (fieldIx == NULL_IX) {
            fieldIx = rDescr.textIdIndex(); // text_id index
            int textIx = rDescr.colDescr(fieldIx).pEnumType->str2enum(mf.getName());   // text index
            pFW = new cLTextWidget(tableShape, mf, (*_pRecord)[fieldIx], textIx, flags, this);
        }
        else {
            pFW = cFieldEditBase::createFieldWidget(tableShape, mf, (*_pRecord)[fieldIx], flags, this);

        }
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
        pFormLayout->addRow(mf.getText(cTableShapeField::LTX_DIALOG_TITLE, mf.getName()), pw);
        /* hülyeség
        if (pFW->isText()) {
            pFW->setName(_pRecord->getText(mf.getName()));
        }
        else {
            if (!_pRecord->isNull(fieldIx)) {
                pFW->set(_pRecord->get(fieldIx));
            }
        } */
    }
    _pWidget->adjustSize();
    DBGFNL();
}

void cRecordDialog::restore(const cRecord *_pRec)
{
    pDelete(_pRecord);
    if (_pRec != NULL) {
        _pRecord = new cRecordAny(*_pRec);
    }
    else {
        _pRecord = new cRecordAny(&rDescr);
    }
    int i, n = fields.size();
    for (i = 0; i < n; i++) {
        cFieldEditBase& field = *fields[i];
        if (field.isText()) {
            QString tn = field._fieldShape.getName();
            field.set(_pRecord->getText(tn));
        }
        else {
            field.set(_pRecord->get(field.fieldIndex()));
        }
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
        // ?! if (field._fieldShape.getBool(_sFieldFlags, FF_AUTO_SET)) continue; // Ezt sem kell ellenörizni
        QVariant fv = fields[i]->get();     // A mező widget-jéből kivesszük az értéket
        if (field.isText()) {
            _pRecord->setText(field._fieldShape.getName(), fv.toString());
            continue;
        }
        qlonglong s = _pRecord->_stat;                   // Mentjük a hiba bitet,
        _pRecord->_stat &= ~ES_DEFECTIVE; // majd töröljük, mert mezőnkként kell
        if (fv.isNull() && field._isInsert && field._hasDefault) continue;    // NULL, insert, van alapérték
        // PDEB(VERBOSE) << "Dialog -> obj. field " << _pRecord->columnName(rfi) << " = " << debVariantToString(fv) << endl;
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
        pTabWidget->addTab(pDlg->pWidget(), shape.getText(cTableShape::LTX_DIALOG_TAB_TITLE));
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

void cRecordDialogInh::restore(const cRecord *_pRec)
{
    if (_pRec == NULL) return;
    if (disabled()) return;
    foreach (cRecordDialog *pTab, tabs) {
        pTab->restore(_pRec);
    };
}

/* ************************************************************************* */

/// Dialógus ablak megjelenítése egy rekord beillesztéséhez. Ha megnyomták az OK gombot, akkor
/// beilleszti az adatbázisba a rekordot, az új rekord objektum pointerével tér vissza.
/// Csak olvasható objektum esetén, vagy ha "mégse" gombot nyomtuk meg, akkor NULL pointerrel tér vissza.
/// A dialógus ablak megjelenését, tartalmát és működését a megadott rekord leíró és table_shape rekord definiálja.
/// @param q
/// @param sn A table_shape (a megjelenítést leíró) rekord neve
/// @param pPar parent widgewt pointere
/// @param pSample minta rekord
/// @param ro Read-only flag, ha igaz, akkor csak a pSample megjelenítése történik, nincs OK gomb.
cRecord * recordDialog(QSqlQuery& q, const QString& sn, QWidget *pPar, const cRecord *pSample, bool ro)
{
    cTableShape shape;
    if (!shape.fetchByName(q, sn)) {
        shape.setName(_sTableName, sn);
        int n = shape.completion(q);
        if (n == 0) EXCEPTION(EDATA, 0, sn);
        if (n >  1) EXCEPTION(AMBIGUOUS, n, sn);
    }
    if (ro) shape.enum2setOn(_sTableShapeType, TS_READ_ONLY);
    shape.fetchText(q);
    shape.fetchFields(q);
    // A dialógusban megjelenítendő nyomógombok. (Csak az explicit read-only van lekezelve!!))
    int buttons;
    if (shape.getBool(_sTableShapeType, TS_READ_ONLY)) {
        buttons = enum2set(DBT_CANCEL);
    }
    else {
        buttons = enum2set(DBT_OK, DBT_CANCEL);
    }
    cRecordDialog   rd(shape, buttons, true, NULL, NULL, pPar);  // A rekord szerkesztő dialógus
    rd.restore(pSample);
    while (true) {
        int r = rd.exec(false);
        if (r == DBT_CANCEL || ro) return NULL;
        if (!rd.accept()) continue;
        if (!cErrorMessageBox::condMsgBox(rd.record().tryInsert(q))) continue;
        break;
    }
    rd.close();
    return rd.record().dup();
}

/// Dialógus ablak megjelenítése egy rekord beillesztéséhez. Hasonló a recordInsertDialog() híváshoz,
/// de a name alapján más egyedi dialógust hív:
/// | name         | hívott függvény neve |
/// |--------------|----------------------|
/// | cPatchDialog | patchDialog
cRecord *objectDialog(const QString& name, QSqlQuery& q, QWidget *pPar, cRecord * _pSample, bool ro, bool edit)
{
    edit = edit || ro;
    if (0 == name.compare("cPatchDialog", Qt::CaseInsensitive)) {
        cPatch *pSample = NULL;
        if (_pSample != NULL) {
            pSample = new cPatch;
            pSample->setById(q, _pSample->getId());
            pSample->fetchPorts(q);
        }
        else {
            if (edit || ro) EXCEPTION(EPROGFAIL);
        }
        cRecord *pRec;
        if (edit) pRec = patchEditDialog(q, pPar, pSample, ro);
        else      pRec = patchInsertDialog(q, pPar, pSample);
        return pRec;
    }
    EXCEPTION(EFOUND, 0, QObject::trUtf8("Nincs %1 nevű insert dialogus.").arg(name));
    return NULL;    // Warning miatt
}

/* ************************************************************************* */

