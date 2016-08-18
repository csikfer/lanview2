
#include "phslinkform.h"

cRecordLink::cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordTable(pts, _isDialog, _upper, par)
{
    ;
}


cRecordLink::~cRecordLink()
{
    ;
}

void cRecordLink::init()
{
    pTimer = NULL;
    pTableView = NULL;
    // Az alapértelmezett gombok:
    buttons << DBT_CLOSE << DBT_SPACER;
    buttons << DBT_REFRESH << DBT_FIRST << DBT_PREV << DBT_NEXT << DBT_LAST;
    if (!(shapeType & ENUM2SET(TS_READ_ONLY))) {
        buttons << DBT_BREAK << DBT_SPACER << DBT_DELETE << DBT_INSERT << DBT_MODIFY;
    }
    flags = 0;
    switch (shapeType & ~ENUM2SET(TS_READ_ONLY)) {
    case ENUM2SET2(TS_CHILD, TS_LINK):
        if (pUpper == NULL) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_CHILD;
        buttons.pop_front();    // A close nem kell
        initSimple(_pWidget);
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


QStringList cRecordLink::where(QVariantList& qParams)
{
    QStringList wl;
    int ofix = recDescr().toIndex(_sNodeId1);
    wl << dQuoted(recDescr().columnName(ofix)) + " = " + QString::number(owner_id);
    wl << filterWhere(qParams);
    wl << refineWhere(qParams);
    return wl;
}

void cRecordLink::insert()
{
    cLinkDialog dialog(true, this);
    dialog.exec();
}

void cRecordLink::modify(eEx __ex)
{
    (void)__ex;
    cLinkDialog dialog(false, this);
    dialog.exec();
}

void cRecordLink::remove()
{

}

cLinkDialog::cLinkDialog(bool isInsert, cRecordLink * __parent)
    : QDialog(__parent == NULL ? NULL : __parent->pWidget())
{
    parent = __parent;
    pActRecord    = NULL;
    parentOwnerId = NULL_ID;
    pq = newQuery();
    if (parent != NULL) {
        if (!isInsert) pActRecord = parent->actRecord();
        parentOwnerId = parent->owner_id;
    }

    pLink1 = new phsLinkWidget(this);
    pLink2 = new phsLinkWidget(this);
    pLink1->pOther = pLink2;
    pLink2->pOther = pLink1;
    pLink1->setFirst(true);
    pLink2->setFirst(false);

    QVBoxLayout *pVBoxL = new QVBoxLayout;
    setLayout(pVBoxL);
    pVBoxL->addWidget(pLink1);
    QFrame *line = new QFrame();
    line->setLineWidth(2);
    line->setMidLineWidth(4);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    pVBoxL->addWidget(line);
    pVBoxL->addWidget(pLink2);
    line = new QFrame();
    line->setLineWidth(2);
    line->setMidLineWidth(4);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    pVBoxL->addWidget(line);

    pLabelCollisions      = new QLabel(trUtf8("Ütköző linkek :"));
    pPushButtonCollisions = new QPushButton(trUtf8("Lekérdezés"));
    pCheckBoxCollisions   = new QCheckBox(trUtf8("Automatikus törlés"));
    pCheckBoxCollisions->setChecked(false);
    pTextEditCollisions   = new QTextEdit;
    pTextEditCollisions->setReadOnly(true);
    QVBoxLayout *pVBoxL2 = new QVBoxLayout;
    pVBoxL2->addWidget(pLabelCollisions);
    pVBoxL2->addWidget(pPushButtonCollisions);
    pVBoxL2->addWidget(pCheckBoxCollisions);
    pVBoxL2->addStretch();
    QHBoxLayout *pHBoxL = new QHBoxLayout;
    pHBoxL->addLayout(pVBoxL2);
    pHBoxL->addWidget(pTextEditCollisions);
    pVBoxL->addLayout(pHBoxL);

    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    pVBoxL->addWidget(line);

    tIntVector buttons;
    buttons << DBT_SPACER << DBT_SAVE << DBT_NEXT << DBT_SPACER << DBT_CANCEL;
    pButtons = new cDialogButtons(buttons);
    pVBoxL->addWidget(pButtons->pWidget());
    connect(pButtons, SIGNAL(buttonPressed(int)), this, SLOT(done(int)));

    init();
}

cLinkDialog::~cLinkDialog()
{
    delete pq;
}

void cLinkDialog::init()
{
    pLink1->init();
    pLink2->init();
}

