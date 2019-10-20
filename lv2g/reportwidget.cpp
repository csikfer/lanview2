#include "reportwidget.h"
#include "ui_reportwidget.h"
#include "popupreport.h"
#include <QPrinter>
#include <QPrintDialog>

const enum ePrivilegeLevel cReportWidget::rights = PL_VIEWER;

cReportWidget::cReportWidget(QMdiArea *parent) :
    cIntSubObj(parent),
    ui(new Ui::reportWidget)
{
    pThread = nullptr;
    if (reportTypeName.isEmpty()) {
        appendCont(reportTypeName, tr("Trunks"), reportTypeNote, tr("Trunk port tagokok elleörzése"), RT_TRUNK);
        appendCont(reportTypeName, tr("Links"),  reportTypeNote, tr("Linkek összevetése"),            RT_LINKS);
        appendCont(reportTypeName, tr("Mactab"), reportTypeNote, tr("A MACTAB tébla és a linkek"),    RT_MACTAB);
        appendCont(reportTypeName, tr("VLAN"),   reportTypeNote, tr("Uplink és VLAN-ok"),             RT_UPLINK_VLANS);
    }
    ui->setupUi(this);
    ui->comboBoxType->addItems(reportTypeName);
    ui->comboBoxType->setCurrentIndex(0);
}

cReportWidget::~cReportWidget()
{
    delete ui;
}

void cReportWidget::on_pushButtonStart_clicked()
{
    int rt = ui->comboBoxType->currentIndex();
    if (pThread == nullptr && rt >= 0) {
        pThread = new cReportThread(eReportTypes(rt));
        connect(pThread, SIGNAL(finished()),       this, SLOT(threadReady()));
        connect(pThread, SIGNAL(msgQueued()), this, SLOT(msgReady()));
        ui->pushButtonStart->setDisabled(true);
        ui->pushButtonSave ->setDisabled(true);
        ui->pushButtonPrint->setDisabled(true);
        ui->pushButtonClear->setDisabled(true);
        pThread->start();
    }
    else {
        EXCEPTION(EPROGFAIL, rt);
    }
}

void cReportWidget::on_pushButtonSave_clicked()
{
    static QString fn;
    cFileDialog::textEditToFile(fn, ui->textEditRiport, this);
}

void cReportWidget::on_pushButtonPrint_clicked()
{
    QPrinter prn;
    QPrintDialog dialog(&prn, this);
    if (QDialog::Accepted != dialog.exec()) return;
    ui->textEditRiport->print(&prn);
}

void cReportWidget::on_comboBoxType_currentIndexChanged(int index)
{
    if (isContIx(reportTypeNote, index)) ui->lineEditType->setText(reportTypeNote.at(index));
    ui->pushButtonStart->setDisabled(false);
}

void cReportWidget::on_pushButtonClear_clicked()
{
    ui->textEditRiport->clear();
}

void cReportWidget::msgReady()
{
    if (pThread == nullptr) return;
    QString msg;
    while (!pThread->queue.isEmpty()) {
        msg = pThread->queue.dequeue();
        ui->textEditRiport->append(msg);
    }
}

void cReportWidget::threadReady()
{
    ui->pushButtonSave ->setDisabled(false);
    ui->pushButtonPrint->setDisabled(false);
    ui->pushButtonClear->setDisabled(false);
    pDelete(pThread);
}

cReportThread::cReportThread(cReportWidget::eReportTypes _rt)
    : QThread(), reportType(_rt)
{

}

void cReportThread::run()
{
    switch (reportType) {
    case cReportWidget::RT_TRUNK:       trunkReport();          break;
    case cReportWidget::RT_LINKS:       linksReport();          break;
    case cReportWidget::RT_MACTAB:      mactabReport();         break;
    case cReportWidget::RT_UPLINK_VLANS:uplinkVlansReport();    break;
    }

}

class cTrunk {
public:
    cTrunk(QSqlQuery& q, qlonglong node_id, qlonglong trunk_id, qlonglong first_member_id) {
        pNode = nullptr;
        pTrunkPort = nullptr;
        pNode = cPatch::getNodeObjById(q, node_id)->reconvert<cNode>();
        pNode->fetchPorts(q);
        pTrunkPort = pNode->ports.get(trunk_id)->reconvert<cInterface>();
        pMemberPorts << pNode->ports.get(first_member_id)->reconvert<cInterface>();
    }
    ~cTrunk() {
        pDelete(pNode);
    }
    void addMemberPort(qlonglong member_id) {
        pMemberPorts << pNode->ports.get(member_id)->reconvert<cInterface>();
    }
    static QList<cTrunk *> * fetchTrunks() {
        static const QString sql =
                "SELECT n.node_id, tp.port_id, mp.port_id"
                 " FROM interfaces AS tp"
                 " JOIN nodes      AS n  ON tp.node_id = n.node_id"
                 " JOIN interfaces AS mp ON tp.port_id = mp.port_staple_id"
                 " WHERE tp.iftype_id = (SELECT iftype_id FROM iftypes WHERE iftype_name = 'multiplexor')"
                 " ORDER BY n.node_name ASC, tp.port_name ASC, mp.port_name ASC";
        QList<cTrunk *> * pTrunks = new QList<cTrunk *>;

        QSqlQuery q = getQuery();
        QSqlQuery q2 = getQuery();

        if (execSql(q, sql)) {
            qlonglong lastTrunkId = NULL_ID;
            cTrunk * pTrunk = nullptr;
            do {
                qlonglong trunk_id  = q.value(1).toLongLong();
                qlonglong member_id = q.value(2).toLongLong();
                if (trunk_id != lastTrunkId) {
                    if (pTrunk != nullptr) (*pTrunks) << pTrunk;
                    qlonglong node_id   = q.value(0).toLongLong();
                    pTrunk = new cTrunk(q2, node_id, trunk_id, member_id);
                    lastTrunkId = trunk_id;
                }
                else {
                    pTrunk->addMemberPort(member_id);
                }
                (*pTrunks) << pTrunk;
            } while (q.next());
        }
        return pTrunks;
    }
    cNode   *           pNode;
    cInterface *        pTrunkPort;
    QList<cInterface *> pMemberPorts;
};

enum eCompareType {
    CT_LOG              = 0,
    CT_LLDP             = 1,
    CT_PORT             = 0,
    CT_TRUNK_PORT       = 2,
    CT_TRUNK_NODE       = 4,
    CT_TRUNK_PORT_ALL   = 6,
    CT_TRUNK_NODE_ALL   = 8,

    CT_LINK_TYPE_MASK   = 1,
    CT_COMP_TYPE_MASK   =14
};

static QString sLnkType(int _ct)
{
    if ((_ct & CT_LINK_TYPE_MASK) == CT_LOG) return QObject::tr("logikai");
    else                                     return QObject::tr("LLDP");
}

static eTristate compareId(qlonglong& id1, qlonglong id2, int _ct, QString& msg)
{
    if (id2 == NULL_ID) {
        if ((_ct & CT_COMP_TYPE_MASK) == CT_TRUNK_PORT_ALL) {
            msgCat(msg, QObject::tr("Az %1 link szerinti port nem trunk tagja!").arg(sLnkType(_ct)));
            return TS_FALSE;
        }
        return TS_NULL;
    }
    if (id1 == NULL_ID) {
        switch (_ct & CT_COMP_TYPE_MASK) {
        case CT_TRUNK_NODE_ALL:
        case CT_TRUNK_PORT_ALL:
            id1 = id2;
        }
        return TS_NULL;
    }
    if (id1 == id2) return TS_TRUE;
    switch (_ct & CT_LINK_TYPE_MASK) {
    case CT_PORT:
        msgCat(msg, QObject::tr("A logikailag ill. LLDP linkelt port nem azonos."));
        break;
    case CT_TRUNK_PORT:
        msgCat(msg, QObject::tr("A logikailag ill. LLDP linkelt port nem azonos trunk tagja."));
        break;
    case CT_TRUNK_NODE:
        msgCat(msg, QObject::tr("A logikailag ill. LLDP linkelt port nem azonos node trunk tagja."));
        break;
    case CT_TRUNK_NODE_ALL:
        msgCat(msg, QObject::tr("A logikailag ill. LLDP linkelt port nem azonos node trunk tagja."));
        break;
    }
    return  TS_FALSE;
}

void cReportThread::trunkReport()
{
    static int ixNodeId       = NULL_IX;
    static int ixPortStapleId = NULL_IX;
    static QStringList head;
    static QStringList emptyRow;
    enum eColumn {
        C_MEMBER_PORT, C_LOG_LINK_PORT, C_LOG_LINK_TRUNK, C_LLDP_LINK_PORT, C_LLDP_LINK_TRUNK, C_NOTE,
        C_ROWS
    };
    if (ixNodeId == NULL_IX) {
        cInterface o;
        ixNodeId       = o.toIndex(_sNodeId);
        ixPortStapleId = o.toIndex(_sPortStapleId);
        appendCont(head, tr("Port tag"),     emptyRow, _sNul, C_MEMBER_PORT);
        appendCont(head, tr("Logikai link"), emptyRow, _sNul, C_LOG_LINK_PORT);
        appendCont(head, tr("Trunk port"),   emptyRow, _sNul, C_LOG_LINK_TRUNK);
        appendCont(head, tr("LLDP link"),    emptyRow, _sNul, C_LLDP_LINK_PORT);
        appendCont(head, tr("Trunk port"),   emptyRow, _sNul, C_LLDP_LINK_TRUNK);
        appendCont(head, tr("Megjegyzés"),   emptyRow, _sNul, C_NOTE);
    }
    QList<cTrunk *> *pTrunks = cTrunk::fetchTrunks();
    if (pTrunks->isEmpty()) {
        sendMsg(tr("Nincs egyetlen TRUNK portunk sem."));
        delete pTrunks;
        return;
    }
    QList<QStringList> matrix;
    cNode *pLastNode = nullptr;
    foreach (cTrunk *pTrunk ,*pTrunks) {
        QString table;
        QString html;
        eTristate trunkOk = TS_NULL;
        QList<eTristate> rowOks;
        if (pLastNode != pTrunk->pNode) {
            html = htmlWarning(tr("Node : %1").arg(pTrunk->pNode->getName()));
        }
        qlonglong linkedNodeId    = NULL_ID;
        qlonglong logLinkTrunkId  = NULL_ID;
        qlonglong lldpLinkTrunkId = NULL_ID;
        foreach (cInterface *pMemberIf, pTrunk->pMemberPorts) {
            QSqlQuery q = getQuery();
            QStringList row(emptyRow);
            row[C_MEMBER_PORT] = toHtml(pMemberIf->getName());
            qlonglong logLinkedPortId  = cLogLink(). getLinked(q, pMemberIf->getId());
            qlonglong lldpLinkedPortId = cLldpLink().getLinked(q, pMemberIf->getId());
            cInterface logLinkedIf, lldpLinkedIf;
            eTristate rowOk = TS_NULL;
            if (logLinkedPortId != NULL_ID) {
                logLinkedIf.setById(q, logLinkedPortId);
                row[C_LOG_LINK_PORT] = logLinkedIf.getFullName(q);
                rowOk = tsAnd(rowOk, compareId(logLinkTrunkId, logLinkedIf.getId(),         CT_LOG | CT_TRUNK_PORT_ALL, row[C_NOTE]));
                rowOk = tsAnd(rowOk, compareId(linkedNodeId,   logLinkedIf.getId(ixNodeId), CT_LOG | CT_TRUNK_NODE_ALL, row[C_NOTE]));
            }
            if (lldpLinkedPortId != NULL_ID) {
                lldpLinkedIf.setById(q, lldpLinkedPortId);
                row[C_LLDP_LINK_PORT] = lldpLinkedIf.getFullName(q);
                rowOk = tsAnd(rowOk, compareId(logLinkTrunkId, logLinkedIf.getId(),         CT_LOG | CT_TRUNK_PORT_ALL, row[C_NOTE]));
                rowOk = tsAnd(rowOk, compareId(linkedNodeId,   logLinkedIf.getId(ixNodeId), CT_LOG | CT_TRUNK_NODE_ALL, row[C_NOTE]));
            }
            rowOk = tsAnd(rowOk, compareId(lldpLinkedPortId, logLinkedPortId, CT_PORT, row[C_NOTE]));
            trunkOk = tsAnd(trunkOk, rowOk);
            rowOks << rowOk;
            matrix << row;
        }

        html += htmlInfo(tr("Trunk port : %1").arg(pTrunk->pTrunkPort->getName()));
    }
    delete pTrunks;
}

void cReportThread::linksReport()
{

}

void cReportThread::mactabReport()
{

}

void cReportThread::uplinkVlansReport()
{
    static const QString sql =
            "SELECT "
            ;

}

