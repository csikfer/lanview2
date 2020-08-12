#include "reportwidget.h"
#include "ui_reportwidget.h"
#include "popupreport.h"
#include "cerrormessagebox.h"
#include <QPrinter>
#include <QPrintDialog>

const enum ePrivilegeLevel cReportWidget::rights = PL_VIEWER;
QStringList cReportWidget::reportTypeName;
QStringList cReportWidget::reportTypeNote;
QStringList cReportWidget::reportLevels;
QStringList cReportWidget::reportLinkSubType;


static QString toHtml(const QString& text, eTristate st)
{
    QString r;
    switch (st) {
    case TS_TRUE:  r = toGreen(text);       break;
    case TS_FALSE: r = toHtmlItalic(text);  break;
    case TS_NULL:  r = toRed(text);         break;
    }
    return r;
}


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

        appendCont(reportLevels, tr("Teljes riport"),             RL_ALL);
        appendCont(reportLevels, tr("Hibák és figyelmeztetések"), RL_WARNING);
        appendCont(reportLevels, tr("Csak a hibák"),              RL_ERROR);

        appendCont(reportLinkSubType, tr("Logikai linkek listája."), RLT_LOG_LINKS);
        appendCont(reportLinkSubType, tr("LLDP linkek listája."),    RLT_LLDP_LINKS);
    }
    ui->setupUi(this);
    ui->comboBoxType->addItems(reportTypeName);
    ui->comboBoxType->setCurrentIndex(0);
    ui->progressBar->hide();
}

cReportWidget::~cReportWidget()
{
    delete ui;
}

void cReportWidget::on_pushButtonStart_clicked()
{
    int rt = ui->comboBoxType->currentIndex();
    int rl = ui->comboBoxLevel->currentIndex();
    int st = ui->comboBoxSubType->currentIndex();
    if (pThread == nullptr && rt >= 0) {
        pThread = new cReportThread(rt, rl, st);
        connect(pThread, SIGNAL(finished()),   this,            SLOT(threadReady()));
        connect(pThread, SIGNAL(msgQueued()),  this,            SLOT(msgReady()));
        connect(pThread, SIGNAL(progres(int)), ui->progressBar, SLOT(setValue(int)));
        ui->pushButtonStart->setDisabled(true);
        ui->pushButtonSave ->setDisabled(true);
        ui->pushButtonPrint->setDisabled(true);
        ui->pushButtonClear->setDisabled(true);
        ui->progressBar->setValue(0);
        ui->progressBar->show();
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
    QTextDocument *pDoc = ui->textEditRiport->document();
    QFont font;
    int fontPointSize = font.pointSize();
    font.setPointSize(6);
    pDoc->setDefaultFont(font);
    pDoc->print(&prn);
    font.setPointSize(fontPointSize);
    pDoc->setDefaultFont(font);
}

void cReportWidget::on_comboBoxType_currentIndexChanged(int index)
{
    if (isContIx(reportTypeNote, index)) ui->lineEditType->setText(reportTypeNote.at(index));
    ui->comboBoxSubType->clear();
    ui->comboBoxLevel->clear();
    switch (index) {
    case RT_TRUNK:
        ui->comboBoxLevel->addItems(reportLevels);
        ui->comboBoxLevel->setCurrentIndex(RL_WARNING);
        ui->comboBoxLevel->setEnabled(true);
        ui->comboBoxLevel->show();
        ui->comboBoxSubType->hide();
        ui->pushButtonStart->setEnabled(true);
        break;
    case RT_LINKS:
        ui->comboBoxLevel->addItems(reportLevels);
        ui->comboBoxLevel->setCurrentIndex(RL_ERROR);
        ui->comboBoxLevel->setEnabled(true);
        ui->comboBoxLevel->show();
        ui->comboBoxSubType->addItems(reportLinkSubType);
        ui->comboBoxSubType->setCurrentIndex(0);
        ui->comboBoxSubType->show();
        ui->pushButtonStart->setEnabled(true);
        break;
    case RT_UPLINK_VLANS:
        ui->comboBoxLevel->addItems(reportLevels);
        ui->comboBoxLevel->setCurrentIndex(RL_WARNING);
        ui->comboBoxLevel->setEnabled(true);
        ui->comboBoxLevel->show();
        ui->comboBoxSubType->hide();
        ui->pushButtonStart->setEnabled(true);
        break;
    default:
        ui->pushButtonStart->setDisabled(true);
        ui->comboBoxLevel->hide();
        ui->comboBoxSubType->hide();
        break;
    }
}

void cReportWidget::on_pushButtonClear_clicked()
{
    ui->textEditRiport->clear();
    ui->pushButtonStart->setEnabled(true);
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
    ui->progressBar->hide();
    if (pThread->pLastError != nullptr) {
        cErrorMessageBox::messageBox(pThread->pLastError, this, tr("Fatális hiba a riport generálásakor."));
    }
    pDelete(pThread);
}

/* ******************************************************************************************* */

QString cReportThread::sNodeHead;
QString cReportThread::sLogical;
QString cReportThread::sLldp;
QString cReportThread::sLogicalAndLldp;
QString cReportThread::sLinkCaveat;
QString cReportThread::sTrunkCaveat;


cReportThread::cReportThread(int _rt, int _l, int _st)
    : QThread(), reportType(_rt), reportLevel(_l), subType(_st)
{
    tableCellPaddingPix = 2;
    pLastError = nullptr;
    if (sLogical.isEmpty()) {
        sNodeHead = tr("Node : %1");
        sLogical = tr("Logikai");
        sLldp    = tr("LLDP");
        sLogicalAndLldp = tr("Logikai és LLDP");
        sLinkCaveat = tr("Ellentmondó linkek!");
        sTrunkCaveat = tr("Ellentmondásos trunk adatok!");
    }
}

void cReportThread::run()
{
    try {
        switch (reportType) {
        case cReportWidget::RT_TRUNK:       trunkReport();          break;
        case cReportWidget::RT_LINKS:       linksReport();          break;
        case cReportWidget::RT_MACTAB:      mactabReport();         break;
        case cReportWidget::RT_UPLINK_VLANS:uplinkVlansReport();    break;
        }
    } CATCHS(pLastError)
}

void cReportThread::nodeHead(eTristate ok, const QString& title, const QStringList& tableHead, const QList<QStringList>& matrix)
{
    QString s = sNodeHead.arg(title);
    switch (ok) {
    case TS_TRUE:
        if (reportLevel != cReportWidget::RL_ALL) return;   // Skeep
        s = sHtmlLine + htmlGrInf(s);
        break;
    case TS_NULL:
        if (reportLevel == cReportWidget::RL_ERROR) return; // Skeep
        s = sHtmlLine + htmlWarning(s);
        break;
    case TS_FALSE:
        s = sHtmlLine + htmlError(s);
        break;
    }
    sendMsg(s);
    if (!matrix.isEmpty()) {
        s = htmlTable(tableHead, matrix, false, tableCellPaddingPix);
        sendMsg(s);
    }
}

void cReportThread::nodeHead(eTristate ok, const QString &nodeName, const QString &html)
{
    QString s = sNodeHead.arg(nodeName);
    switch (ok) {
    case TS_TRUE:
        if (reportLevel != cReportWidget::RL_ALL) return;   // Skeep
        s = sHtmlLine + htmlGrInf(s);
        break;
    case TS_NULL:
        if (reportLevel == cReportWidget::RL_ERROR) return; // Skeep
        s = sHtmlLine + htmlWarning(s);
        break;
    case TS_FALSE:
        s = sHtmlLine + htmlError(s);
        break;
    }
    sendMsg(s);
    sendMsg(html);
}
/* ------------------------------------------ TRUNK ------------------------------------------- */

class cTrunk {
public:
    class cMember {
    public:
        qlonglong memberPortId;
        QString   memberPortName;
        qlonglong memberPortLogLinkedPortId;
        QString   memberPortLogLinkedPortName;
        qlonglong memberPortLogLinkedNodeId;
        QString   memberPortLogLinkedNodeName;
        qlonglong memberPortLogLinkedTrunkId;
        QString   memberPortLogLinkedTrunkName;
        qlonglong memberPortLogLinkedTrunkNodeId;
        qlonglong memberPortLldpLinkedPortId;
        QString   memberPortLldpLinkedPortName;
        qlonglong memberPortLldpLinkedNodeId;
        QString   memberPortLldpLinkedNodeName;
        qlonglong memberPortLldpLinkedTrunkId;
        QString   memberPortLldpLinkedTrunkName;
        qlonglong memberPortLldpLinkedTrunkNodeId;

        cMember(QSqlQuery& q)
            : memberPortId(::getId(q, 4))
            , memberPortName(q.value(5).toString())
            , memberPortLogLinkedPortId(::getId(q,6))
            , memberPortLogLinkedPortName(q.value(7).toString())
            , memberPortLogLinkedNodeId(::getId(q,8))
            , memberPortLogLinkedNodeName(q.value(9).toString())
            , memberPortLogLinkedTrunkId(::getId(q,10))
            , memberPortLogLinkedTrunkName(q.value(11).toString())
            , memberPortLogLinkedTrunkNodeId(::getId(q,12))
            , memberPortLldpLinkedPortId(::getId(q,13))
            , memberPortLldpLinkedPortName(q.value(14).toString())
            , memberPortLldpLinkedNodeId(::getId(q,15))
            , memberPortLldpLinkedNodeName(q.value(16).toString())
            , memberPortLldpLinkedTrunkId(::getId(q,17))
            , memberPortLldpLinkedTrunkName(q.value(18).toString())
            , memberPortLldpLinkedTrunkNodeId(::getId(q,19))
        { }
    };
    const qlonglong trunkPortId;
    const QString   trunkPortName;
    const qlonglong trunkNodeId;
    const QString   trunkNodeName;
    QList<cMember>  members;

    void addMemberPort(QSqlQuery& q) {
        members << cMember(q);
    }

    cTrunk(QSqlQuery& q)
        : trunkPortId(::getId(q, 0))
        , trunkPortName(q.value(1).toString())
        , trunkNodeId(::getId(q, 2))
        , trunkNodeName(q.value(3).toString())
    {
        addMemberPort(q);
    }
    ~cTrunk() { }

    static cTrunk * fetchFirst(QSqlQuery& q, cReportThread * parent, bool& last) {
        static const QString sqlSelect = "SELECT"
                " tp.port_id,"      //  0:  trunkPortId
                " tp.port_name, "   //  1:  trunkPortName,"
                " tn.node_id,"      //  2:  trunkNodeId
                " tn.node_name,"    //  3:  trunkNodeName
                " mp.port_id,"      //  4:  memberPortId, "
                " mp.port_name, "   //  5:  memberPortName"
                " lp.port_id,"      //  6:  memberPortLogLinkedPortId
                " lp.port_name,"    //  7:  memberPortLogLinkedPortName"
                " ln.node_id,"      //  8:  memberPortLogLinkedNodeId"
                " ln.node_name,"    //  9:  memberPortLogLinkedNodeName"
                " lt.port_id,"      // 10:  memberPortLogLinkedTrunkId"
                " lt.port_name,"    // 11:  memberPortLogLinkedTrunkName"
                " lt.node_id,"      // 12:  memberPortLogLinkedTrunkNodeId"
                " dp.port_id,"      // 13:  memberPortLldpLinkedPortId"
                " dp.port_name,"    // 14:  memberPortLldpLinkedPortName"
                " dn.node_id,"      // 15:  memberPortLldpLinkedNodeId"
                " dn.node_name,"    // 16:  memberPortLldpLinkedNodeName"
                " dt.port_id,"      // 17:  memberPortLldpLinkedTrunkId"
                " dt.port_name,"    // 18:  memberPortLldpLinkedTrunkName"
                " dt.node_id"       // 19:  memberPortLldpLinkedTrunkNodeId"
                ;
        static const QString sqlFrom =
                " FROM interfaces AS tp"                                   // tp: trunk interface
                " JOIN nodes      AS tn ON tp.node_id = tn.node_id"        // tn: node of trunk interface
                " JOIN interfaces AS mp ON tp.port_id = mp.port_staple_id" // mp: trunk member interface
                ;
        static const QString sqlOuterJoin =
                " LEFT OUTER JOIN log_links  AS ll ON ll.port_id1 = mp.port_id" // ll: log link for member port
                " LEFT OUTER JOIN interfaces AS lp ON lp.port_id = ll.port_id2" // lp: member log linked port
                " LEFT OUTER JOIN nodes      AS ln ON ln.node_id = lp.node_id"  // ln: node of member log linked port
                " LEFT OUTER JOIN interfaces AS lt ON lt.port_id = lp.port_staple_id" // lt trunk of member log linked port
                " LEFT OUTER JOIN lldp_links AS dl ON dl.port_id1 = mp.port_id" // dl: log link for member port
                " LEFT OUTER JOIN interfaces AS dp ON dp.port_id = dl.port_id2" // dp: member log linked port
                " LEFT OUTER JOIN nodes      AS dn ON dn.node_id = dp.node_id"  // dn: node of member log linked port
                " LEFT OUTER JOIN interfaces AS dt ON dt.port_id = dp.port_staple_id" // lt trunk of member log linked port
                ;
        static const QString sqlWhere =
                " WHERE tp.iftype_id = (SELECT iftype_id FROM iftypes WHERE iftype_name = 'multiplexor')";
        static const QString sqlOrd =
                " ORDER BY tn.node_name ASC, tp.port_name ASC, mp.port_index ASC, mp.port_name ASC";

        // get first records
        last = !execSql(q, sqlSelect + sqlFrom + sqlOuterJoin + sqlWhere + sqlOrd);
        if (last) {
            return nullptr;
        }
        parent->recordNumber = q.size();
        return fetchNext(q, last);
    }
    static cTrunk * fetchNext(QSqlQuery& q, bool& last)
    {
        if (last) {
            EXCEPTION(EPROGFAIL);
        }
        qlonglong lastTrunkId = NULL_ID;
        cTrunk * pTrunk = nullptr;
        do {
            qlonglong trunk_id  = q.value(0).toLongLong();
            if (pTrunk == nullptr) {        // first member
                lastTrunkId = trunk_id;
                pTrunk = new cTrunk(q);
            }
            else {
                if (lastTrunkId != trunk_id) break; // next trunk
                pTrunk->addMemberPort(q);
            }
            last = !q.next();
        } while (!last);
        return pTrunk;
    }

};

void cReportThread::trunkReport()
{
    static QStringList head;
    static QStringList emptyRow;
    enum eColumn {
        C_MEMBER_PORT, C_LOG_LINK_PORT, C_LOG_LINK_TRUNK, C_LLDP_LINK_PORT, C_LLDP_LINK_TRUNK, C_NOTE,
        C_COLUMNS
    };
    if (head.isEmpty()) {
        appendCont(head, tr("Port tag"),     emptyRow, _sNul, C_MEMBER_PORT);
        appendCont(head, tr("Logikai link"), emptyRow, _sNul, C_LOG_LINK_PORT);
        appendCont(head, tr("Trunk port"),   emptyRow, _sNul, C_LOG_LINK_TRUNK);
        appendCont(head, tr("LLDP link"),    emptyRow, _sNul, C_LLDP_LINK_PORT);
        appendCont(head, tr("Trunk port"),   emptyRow, _sNul, C_LLDP_LINK_TRUNK);
        appendCont(head, tr("Megjegyzés"),   emptyRow, _sNul, C_NOTE);
    }
    QSqlQuery q = getQuery();
    bool last;
    cTrunk *pTrunk = cTrunk::fetchFirst(q, this, last);
    if (pTrunk == nullptr) {
        sendMsg(tr("Nincs egyetlen TRUNK portunk sem."));
        return;
    }
    eTristate nodeOk = TS_TRUE;     // node (trunks) status
    QString html;
    QString s;
    int n = 0;
    qlonglong lastTrunkNodeId = NULL_ID;
    QString   lastTrunkNodeName;
    while (true) {    // for each trunk ports
        QList<QStringList> matrix;      // members table for trunk port
        eTristate trunkOk = TS_TRUE;    // trunk port state
        if (lastTrunkNodeId != pTrunk->trunkNodeId) {   // Next or first node
            if (lastTrunkNodeId != NULL_ID && !html.isEmpty()) { // Next node, and not empty report
                nodeHead(nodeOk, lastTrunkNodeName, html);
            }
            html.clear();
            nodeOk = TS_TRUE;
            lastTrunkNodeId = pTrunk->trunkNodeId;
            lastTrunkNodeName = pTrunk->trunkNodeName;
        }
        qlonglong linkedTrunkId  = NULL_ID;
        foreach (cTrunk::cMember m, pTrunk->members) {  // for each members
            static const QString psep = ":";
            QStringList row(emptyRow);  // Empty row
            eTristate rowOk = TS_TRUE;  // Row status
            row[C_MEMBER_PORT] = toHtml(m.memberPortName);

            // By Logical link
            if (m.memberPortLogLinkedPortId != NULL_ID) {
                row[C_LOG_LINK_PORT] = m.memberPortLogLinkedNodeName + psep + m.memberPortLogLinkedPortName;
                if (m.memberPortLogLinkedTrunkId == NULL_ID) {
                    s = tr("A logikai link szerinti port nem egy trunk tagja.");
                    msgAppend(&row[C_NOTE], s);
                    rowOk = TS_FALSE;
                }
                else {
                    row [C_LOG_LINK_TRUNK] = m.memberPortLogLinkedNodeName + psep +m.memberPortLogLinkedTrunkName;
                    if (linkedTrunkId == NULL_ID) {
                        linkedTrunkId = m.memberPortLogLinkedTrunkId;
                    }
                    else {
                        if (linkedTrunkId != m.memberPortLogLinkedTrunkId) {
                            s = tr("A logikai link szerinti porthoz tartozó trunk nem egyezik az előzővel.");
                            msgAppend(&row[C_NOTE], s);
                            rowOk = TS_FALSE;
                        }
                    }
                }
            }
            else {
                rowOk = TS_NULL;
            }

            // By LLDP link
            if (m.memberPortLldpLinkedPortId != NULL_ID) {
                row[C_LLDP_LINK_PORT] = m.memberPortLldpLinkedNodeName + psep + m.memberPortLldpLinkedPortName;
                if (m.memberPortLldpLinkedTrunkId == NULL_ID) {
                    s = tr("Az LLDP link szerinti port nem egy trunk tagja.");
                    msgAppend(&row[C_NOTE], s);
                    rowOk = TS_FALSE;
                }
                else {
                    row [C_LLDP_LINK_TRUNK] = m.memberPortLldpLinkedNodeName + psep +m.memberPortLldpLinkedTrunkName;
                    if (linkedTrunkId == NULL_ID) {
                        linkedTrunkId = m.memberPortLldpLinkedTrunkId;
                    }
                    else {
                        if (linkedTrunkId != m.memberPortLldpLinkedTrunkId) {
                            s = tr("Az LLDP link szerinti porthoz tartozó trunk nem egyezik az előzővel.");
                            msgAppend(&row[C_NOTE], s);
                            rowOk = TS_FALSE;
                        }
                    }
                }
                if (m.memberPortLogLinkedPortId != NULL_ID) {
                    if (m.memberPortLogLinkedPortId != m.memberPortLldpLinkedPortId) {
                        s = tr("Az LLDP és logikai linkelt port nem azonos.");
                        msgAppend(&row[C_NOTE], s);
                        rowOk = TS_FALSE;
                    }
                }
            }
            else {
                rowOk = TS_NULL;
            }
            switch (rowOk) {
            case TS_NULL:
                trunkOk = tsMin(trunkOk, TS_NULL);
                for (int i = 0; i < C_COLUMNS; ++i) {
                    row[i] = htmlInfo(row[i], true);
                }
                break;
            case TS_FALSE:
                trunkOk = TS_FALSE;
                for (int i = 0; i < C_COLUMNS; ++i) {
                    row[i] = htmlError(row[i], true);
                }
                break;
            case TS_TRUE:
                for (int i = 0; i < C_COLUMNS; ++i) {
                    if (row[C_NOTE].isEmpty()) row[C_NOTE] = _sOk;
                    row[i] = htmlGrInf(row[i], true);
                }
                break;
            }
            matrix << row;
        }
        s = tr("Trunk port : %1").arg(pTrunk->trunkPortName);
        switch (trunkOk) {
        case TS_NULL:
            if (reportLevel <= cReportWidget::RL_WARNING) {
                html += htmlInfo(s);
                html += htmlTable(head, matrix, false, tableCellPaddingPix);
            }
            nodeOk = nodeOk == TS_FALSE ? TS_FALSE : TS_NULL;
            break;
        case TS_FALSE:
            html += htmlError(s);
            html += htmlTable(head, matrix, false, tableCellPaddingPix);
            nodeOk = TS_FALSE;
            break;
        case TS_TRUE:
            if (reportLevel == cReportWidget::RL_ALL) {
                html += htmlGrInf(s);
                html += htmlTable(head, matrix, false, tableCellPaddingPix);
            }
            break;
        }
        progres((++n * 100) / recordNumber);
        if (last) break;
        pTrunk = cTrunk::fetchNext(q, last);
    }
    if (!html.isEmpty()) {
        nodeHead(nodeOk, pTrunk->trunkNodeName, html);
    }
    delete pTrunk;
}

/* ------------------------------------------ LINK ------------------------------------------- */

void cReportThread::linksReport()
{
    // A megjelenített táblázat oszlopai
    enum eColumns { C_PORT1,    // Kiindulási port
                    C_PORT2,    // Linkelt elsődleges (log/lldp)
                    C_PORT3,    // Linkeltr másodlagos (log/lldp)
                    C_PORT4,    // mactab a MAC alapján
                    C_PORT5S,   // mactab az port ID alapján.
                    C_COLUMNS   // az oszlopok száma a táblázatban
                  };
    static QStringList emptyRow;    // Egy üres sor
    if (emptyRow.isEmpty()) {
        appendCont(emptyRow, _sNul, C_PORT1);
        appendCont(emptyRow, _sNul, C_PORT2);
        appendCont(emptyRow, _sNul, C_PORT3);
        appendCont(emptyRow, _sNul, C_PORT4);
        appendCont(emptyRow, _sNul, C_PORT5S);
    }
    QStringList head;
    appendCont(head, tr("Port"), C_PORT1);
    appendCont(head, subType == cReportWidget::RLT_LOG_LINKS ? sLogical : sLldp, C_PORT2);
    appendCont(head, subType != cReportWidget::RLT_LOG_LINKS ? sLogical : sLldp, C_PORT3);
    appendCont(head, tr("mactab/MAC"), C_PORT4);
    appendCont(head, tr("mactab/ID"),  C_PORT5S);
    // 1..: kiindulási port
    // 2..: elsődleges linkelt port
    // 3..: másodlagosan linkelt port
    // 4..: Link a mactab, MAC alapján
    // 5..: Link a mactab, port_id alapján
    static const QString sqlSelect = "SELECT"
            " n1.node_id   AS node_id1,"    //  0: nodeId1
            " n1.node_name AS node_name1,"  //  1: nodeName1
            " i1.port_name AS port_name1,"  //  2: portName1
            " n2.node_name AS node_name2,"  //  3: nodeName2
            " i2.port_id   AS port_id2,"    //  4: portId2
            " i2.port_name AS port_name2,"  //  5: portName2
            " n3.node_name AS node_name3,"  //  6: nodeName3
            " i3.port_id   AS port_id3,"    //  7: portId3
            " i3.port_name AS port_name3,"  //  8: portName3
            " n4.node_name AS node_name4,"  //  9: nodeName4
            " i4.port_id   AS port_id4,"    // 10: portId4
            " i4.port_name AS port_name4,"  // 11: portName4
            " ("
                " SELECT array_agg(i5.port_id)"
                " FROM mactab cm"
                " JOIN interfaces AS i5 USING(hwaddress)"
                " WHERE cm.port_id = i1.port_id"
            " ) AS i5s"                     // 12: portId5List
            ;
    static const QString _sqlFrom =
            " FROM %1 AS ml"        // ml: First link table
            " JOIN interfaces AS i1 ON ml.port_id1 = i1.port_id"    // i1: interface 1 of first link record
            " JOIN interfaces AS i2 ON ml.port_id2 = i2.port_id"    // i2: interface 2 of first link record
            ;
    static const QString _sqlJoin =
            " JOIN nodes  AS n1 ON i1.node_id = n1.node_id"         // n1: node, i1
            " JOIN nodes  AS n2 ON i2.node_id = n2.node_id"         // n2: node, i2
            " LEFT OUTER JOIN %1         AS sl ON i1.port_id = sl.port_id1"     // sl: second link record for i1
            " LEFT OUTER JOIN interfaces AS i3 ON i3.port_id = sl.port_id2"     // i3: i1 linked by second link
            " LEFT OUTER JOIN nodes      AS n3 ON i3.node_id = n3.node_id"      // n3: node, i3
            " LEFT OUTER JOIN mactab     AS mc ON mc.hwaddress = i1.hwaddress"  // mc: mactab record by i1 MAC
            " LEFT OUTER JOIN interfaces AS i4 ON i4.port_id = mc.port_id"      // i4: link by mactab
            " LEFT OUTER JOIN nodes      AS n4 ON n4.node_id = i4.node_id"      // n4: node, i4
            ;
    static const QString sqlOrd =
            " ORDER BY n1.node_name ASC, i1.port_index ASC, i1.port_name ASC"
            ;
    const QString firstLinkTable  = subType == cReportWidget::RLT_LOG_LINKS ? __sLogLinks : _sLldpLinks;
    const QString secondLinkTable = subType != cReportWidget::RLT_LOG_LINKS ? __sLogLinks : _sLldpLinks;
    const QString sqlFrom = _sqlFrom.arg(firstLinkTable);
    const QString sqlJoin = _sqlJoin.arg(secondLinkTable);

    QSqlQuery q = getQuery();
    int n = 0;
    eTristate nodeOk = TS_TRUE;
    QString s;
    QList<QStringList> matrix;
    qlonglong lastNodeId = NULL_ID;
    QString lastNodeName;
    QString sLastTime = (subType == cReportWidget::RLT_LOG_LINKS ?   // 13: LLDP - LastTime
                ", sl.last_time" :
                ", ml.last_time" )
            ;
    execSql(q, sqlSelect + sLastTime + sqlFrom + sqlJoin + sqlOrd);
    if (q.size() <= 0) {
        sendMsg(tr("Nincs egyetlen %1 link rekordunk sem.")
                .arg(subType == cReportWidget::RLT_LOG_LINKS ? sLogical : sLldp));
        return;
    }
    recordNumber = q.size();
    do {
        eTristate linkOk   = TS_TRUE;
        eTristate mactabOk = TS_TRUE;
        QStringList row(emptyRow);
        qlonglong nodeId1 = getId(q, 0);
        if (nodeId1 != lastNodeId) {    // Next or first node
            if (lastNodeId != NULL_ID && !matrix.isEmpty()) {
                nodeHead(nodeOk, lastNodeName, head, matrix);
            }
            nodeOk = TS_TRUE;
            lastNodeId = nodeId1;
            lastNodeName = q.value(1).toString();
            matrix.clear();
        }
        QString portName1 = q.value(2).toString();
        QString nodeName2 = q.value(3).toString();
        qlonglong portId2 = getId(q, 4);
        QString portName2 = q.value(5).toString();
        QString nodeName3 = q.value(6).toString();
        qlonglong portId3 = getId(q, 7);
        QString portName3 = q.value(8).toString();
        QString nodeName4 = q.value(9).toString();
        qlonglong portId4 = getId(q, 10);
        QString portName4 = q.value(11).toString();
        QVariantList portId5List = sqlToIntegerList(q.value(12));
        QVariant vLastTime  = q.value(13);

        row[C_PORT1] = portName1;
        row[C_PORT2] = cNPort::catFullName(nodeName2, portName2);                           // main linked port full name
        if (portId3 != NULL_ID) row[C_PORT3] = cNPort::catFullName(nodeName3, portName3);   // second linked port full name
        if (portId4 != NULL_ID) row[C_PORT4] = cNPort::catFullName(nodeName4, portName4);   // port by mactab
        if (vLastTime.isValid()) {
            int ix = cReportWidget::RLT_LOG_LINKS ? C_PORT2 : C_PORT3;
            row[ix] += " (" + vLastTime.toDateTime().toString(lanView::sDateTimeForm) + ")";
        }

        linkOk = compareId(portId2, portId3);   // compare first and second link
        switch (linkOk) {
        case TS_NULL:   break;
        case TS_FALSE:  row[C_PORT3] = htmlError(row[C_PORT3]);   break;
        case TS_TRUE:   row[C_PORT3] = htmlGrInf(row[C_PORT3]);   break;
        }
        mactabOk = compareId(portId2, portId4);
        switch (mactabOk) {  // compare first link and mactab
        case TS_NULL:
            break;
        case TS_FALSE:
            row[C_PORT4] = htmlError(row[C_PORT4]);
            break;
        case TS_TRUE:
            row[C_PORT4] = htmlGrInf(row[C_PORT4]);
            break;
        }
        if (!portId5List.isEmpty()) {
            s.clear();
            bool found = false;
            QSqlQuery qq = getQuery();
            foreach (QVariant vid, portId5List) {
                qlonglong pid = vid.toLongLong();
                s = cNPort::getFullNameById(qq, pid) + "\n";
                if (portId2 == pid) {
                    mactabOk = TS_TRUE;
                    found = true;
                }
            }
            if (found) row[C_PORT5S] = htmlGrInf(s, true);
            else       row[C_PORT5S] = htmlError(s, true);
        }
        switch (mactabOk) {
        case TS_FALSE:  linkOk = TS_FALSE;  break;
        case TS_NULL:   if (linkOk == TS_TRUE) linkOk = TS_NULL;    break;
        case TS_TRUE:   break;
        }

        switch (linkOk) {
        case TS_FALSE:
            row[C_PORT1] = htmlError(row[C_PORT1]);
            row[C_PORT2] = htmlError(row[C_PORT2]);
            nodeOk = TS_FALSE;
            matrix << row;
            break;
        case TS_NULL:
            row[C_PORT1] = htmlWarning(row[C_PORT1]);
            row[C_PORT2] = htmlWarning(row[C_PORT2]);
            if (nodeOk == TS_TRUE) nodeOk = TS_NULL;
            if (reportLevel <= cReportWidget::RL_WARNING)
                matrix << row;
            break;
        case TS_TRUE:
            row[C_PORT1] = htmlGrInf(row[C_PORT1]);
            row[C_PORT2] = htmlGrInf(row[C_PORT2]);
            if (reportLevel == cReportWidget::RL_ALL)
                matrix << row;
            break;
        }

        progres((++n * 100) / recordNumber);
    } while (q.next());
    if (!matrix.isEmpty()) {
        nodeHead(nodeOk, lastNodeName, head, matrix);
    }
}

/* ------------------------------------------ MACTAB ------------------------------------------- */

void cReportThread::mactabReport()
{

}

/* ------------------------------------------ VLAN ------------------------------------------- */

static QString vlanList(const QStringList& vnames, const QStringList& vtypes)
{
    QString r;
    int i, n = vnames.size();
    if (n != vtypes.size()) EXCEPTION(EPROGFAIL);
    for (i = 0; i < n; ++i) {
        eVlanType vt  = eVlanType(vlanType(vtypes.at(i)));
        const QString& vname = vnames.at(i);
        r += htmlEnumDecoration(vname, cEnumVal::enumVal(_sVlanType, vt)) + _sCommaSp;
    }
    r.chop(_sCommaSp.size());
    return r;
}

static QString trunkMembers(QSqlQuery& q, qlonglong pid)
{
    static const QString sql = "SELECT port_name FROM interfaces WHERE port_staple_id = ?";
    QString r;
    if (execSql(q, sql, pid)) {
        do {
            r += q.value(0).toString() + _sCommaSp;
        } while (q.next());
        r.chop(_sCommaSp.size());
    }
    return r;
}

bool checkVlans(QSqlQuery& q, qlonglong linkedId, const QVariantList& vlanIds, QString& sLinkedVlanList)
{
    QVariantList linkVlanIds;
    QStringList  linkVlanNames;
    QStringList  linkVlanTypes;
    // Check uplink vlans
    static const QString sqlVlan =
            "SELECT vlan_id, vlan_name, vlan_type"
            " FROM port_vlans"
            " JOIN vlans using(vlan_id)"
            " WHERE port_id = ?"
            " ORDER BY vlan_id ASC";
    if (execSql(q, sqlVlan, linkedId)) do {
        linkVlanIds   << q.value(0);
        linkVlanNames << q.value(1).toString();
        linkVlanTypes << q.value(2).toString();
    } while (q.next());
    bool vlanOk = vlanIds.size() == linkVlanIds.size();
    if (vlanOk) {
        int i, n = vlanIds.size();
        for (i = 0; i < n; ++i) {
            if (vlanIds.at(i).toLongLong() != linkVlanIds.at(i)) {
                vlanOk = false;
                break;
            }
        }
    }
    sLinkedVlanList = vlanList(linkVlanNames, linkVlanTypes);
    return vlanOk;
}

void cReportThread::uplinkVlansReport()
{
    // Táblázat oszlopai sorszámok
    enum eColumns { C_PORT, C_PTAGS, C_VLANS, C_LINKED, C_PLTAGS, C_LINK_TYPE, C_VLANS_LINKED, C_NOTE, C_COLUMNS };
    // Táblázat fejléce (oszlop nevek)
    static QStringList header;
    // Egy üres sor
    static QStringList emptyRow;
    // A táblázat fejlécének és az üres sornak a(z ellenörzött) kitöltése
    if (header.isEmpty()) {
        appendCont(header, tr("Port"),        emptyRow, _sNul, C_PORT);
        appendCont(header, tr("Trunk"),       emptyRow, _sNul, C_PTAGS);
        appendCont(header, tr("VLAN-ok"),     emptyRow, _sNul, C_VLANS);
        appendCont(header, tr("Linked port"), emptyRow, _sNul, C_LINKED);
        appendCont(header, tr("Trunk"),       emptyRow, _sNul, C_PLTAGS);
        appendCont(header, tr("Link típusa"), emptyRow, _sNul, C_LINK_TYPE);
        appendCont(header, tr("VLAN-ok"),     emptyRow, _sNul, C_VLANS_LINKED);
        appendCont(header, tr("Megjegyzés"),  emptyRow, _sNul, C_NOTE);
    }
    QSqlQuery q1 = getQuery();
    QSqlQuery q2 =  getQuery();
    // Get switch type node list
    static const QString sqlSwitchs =
            "SELECT DISTINCT node_id, node_name,"
                "ARRAY(SELECT vlan_name FROM vlan_list_by_host AS v"
                    " WHERE v.node_id = n.node_id ORDER BY vlan_id ASC)"
                " AS vlan_ids"
            " FROM nodes AS n"
            " WHERE 'switch' = ANY (node_type)"
            " ORDER BY node_name ASC"
            ;
    if (!execSql(q1, sqlSwitchs)) {
        sendMsg(tr("Nincs mit listázni, nincsenek bejegyzett switch-ek."));
        return;
    }
    QList<qlonglong> switchIds;
    QMap<qlonglong, QString> switchNameMap;
    QMap<qlonglong, QString> switchVLanMap;
    do {
        qlonglong id = q1.value(0).toLongLong();    // node_id
        switchIds << id;                            // ID-k név szerint névsornan
        switchNameMap[id] = q1.value(1).toString(); // node_name
        switchVLanMap[id] = sqlToStringList(q1.value(2)).join(_sCommaSp);   // VLan list
    } while (q1.next());
    q1.finish();
    recordNumber = switchNameMap.size();
    int progresCnt = 0;
    foreach (qlonglong actSwitchId, switchIds) {
        qlonglong portId, lldpLinkedId, logLinkedId, linkedId;
        QString nodeName, portName, note, linkType;
        QVariantList vlanIds, linkedVlanIds;
        QStringList vlanNames, vlanTypes, linkedVlanNames, linkedVlanTypes;
        nodeName = switchNameMap.take(actSwitchId);
        eTristate swOk = TS_TRUE;   // Actual switch is OK
        eTristate portOk, linkOk, trunkOk;
        bool vlanOk;
        QStringList row = emptyRow;
        QList<QStringList>  matrix;
        // Get non trunk ports
        static const QString sqlNonTrunk =
                "WITH ports AS ("
                   " SELECT"
                       " i.port_id AS port_id,"
                       " i.port_name AS port_name,"
                       " (   SELECT port_id2 AS dpid"
                           " FROM lldp_links AS dl"
                           " JOIN interfaces AS di ON dl.port_id2 = di.port_id"
                           " JOIN nodes      AS dn ON di.node_id  = dn.node_id"
                           " WHERE dl.port_id1 = i.port_id AND 'switch' = ANY (dn.node_type)"
                       " ) AS lldp_linked_id,"
                       " (   SELECT port_id2 AS lpid"
                           " FROM log_links AS ll"
                           " JOIN interfaces AS li ON ll.port_id2 = li.port_id"
                           " JOIN nodes      AS ln ON li.node_id  = ln.node_id"
                           " WHERE ll.port_id1 = i.port_id AND 'switch' = ANY (ln.node_type)"
                       " ) AS log_linked_id"
                   " FROM interfaces AS i"
                   " WHERE node_id = ? AND i.port_staple_id IS NULL"
                   " ORDER BY port_index ASC, port_name ASC"
               " )"
               " SELECT"
                   " port_id, port_name, lldp_linked_id, log_linked_id,"
                   " ARRAY_AGG(vlan_id   ORDER BY vlan_id ASC) AS vlan_ids,"
                   " ARRAY_AGG(vlan_name ORDER BY vlan_id ASC) AS vlan_types,"
                   " ARRAY_AGG(vlan_type ORDER BY vlan_id ASC) AS vlan_names"
               " FROM ports"
               " JOIN port_vlans USING(port_id)"
               " JOIN vlans USING(vlan_id)"
               " WHERE log_linked_id IS NOT NULL OR lldp_linked_id IS NOT NULL"
               " GROUP BY port_id, port_name, lldp_linked_id, log_linked_id";
        if (execSql(q1, sqlNonTrunk, actSwitchId)) do {
            portId          = q1.value(0).toLongLong();
            portName        = q1.value(1).toString();
            lldpLinkedId    = ::getId(q1, 2);
            logLinkedId     = ::getId(q1, 3);
            vlanIds         = sqlToIntegerList(q1.value(4));
            vlanNames       = sqlToStringList( q1.value(5));
            vlanTypes       = sqlToStringList( q1.value(6));

            portOk = linkOk = TS_TRUE;
            note.clear();
            row = emptyRow;

            // Check link(s)
            if (logLinkedId == NULL_ID && lldpLinkedId == NULL_ID) {
                EXCEPTION(EPROGFAIL);
            }
            else if (logLinkedId == NULL_ID || lldpLinkedId == NULL_ID) {
                if (lldpLinkedId != NULL_ID) {
                    linkedId = lldpLinkedId;
                    linkType = sLldp;
                }
                else {
                    linkedId = logLinkedId;
                    linkType = sLogical;
                }
                linkOk = TS_NULL;
            }
            else if (logLinkedId != lldpLinkedId) {
                note = sLinkCaveat;
                row[C_PORT]   = htmlItalicInf(portName);
                row[C_VLANS]  = vlanList(vlanNames, vlanTypes);
                row[C_NOTE]   = htmlItalicInf(note);
                matrix << row;
                swOk = tsMin(swOk, portOk = tsMin(portOk, TS_NULL));
                continue;
            }
            else {
                linkedId = lldpLinkedId;
                linkType = sLogicalAndLldp;
                linkOk = TS_TRUE;
            }
            // Check VLANS
            vlanOk = checkVlans(q2, linkedId, vlanIds, row[C_VLANS_LINKED]);
            if (!vlanOk) msgAppend(&note, tr("Nem egyező vlan kiosztás!"));
            portOk = tsMin(portOk, bool2ts(vlanOk));
            swOk   = tsMin(swOk,   portOk);
            if ((portOk == TS_FALSE)
             || (portOk == TS_NULL  && reportLevel <= cReportWidget::RL_WARNING)
             || (portOk == TS_TRUE  && reportLevel == cReportWidget::RL_ALL)) {
                if (note.isNull() && portOk == TS_TRUE) note = _sOk;
                row[C_PORT]         = toHtml(portName, portOk);
                row[C_PTAGS]        = trunkMembers(q1, portId);
                row[C_VLANS]        = vlanList(vlanNames, vlanTypes);
                row[C_LINKED]       = toHtml(cNPort().getFullNameById(q1, linkedId), linkOk);
                // C_VLANS_LINKED
                row[C_LINK_TYPE]    = toHtml(linkType, linkOk);
                row[C_NOTE]         = toHtml(note, portOk);
                matrix << row;
            }
        } while (q1.next());
        // Get trunk ports
        static const QString sqlTrunk =
                "SELECT"
                   " tr.port_name AS trunk_port_name,"
                   " ARRAY("
                       " SELECT mp.port_id"
                       " FROM interfaces AS mp"
                       " WHERE mp.port_staple_id = tr.port_id"
                       " ORDER BY mp.port_index ASC, mp.port_name ASC"
                       " ) AS member_port_ids,"
                  " ARRAY("
                      " SELECT mp.port_name"
                      " FROM interfaces AS mp"
                      " WHERE mp.port_staple_id = tr.port_id"
                      " ORDER BY mp.port_index ASC, mp.port_name ASC"
                      " ) AS member_port_names,"
                   " ARRAY_AGG(vlan_id   ORDER BY vlan_id ASC) AS vlan_ids,"
                   " ARRAY_AGG(vlan_name ORDER BY vlan_id ASC) AS vlan_types,"
                   " ARRAY_AGG(vlan_type ORDER BY vlan_id ASC) AS vlan_names"
                " FROM interfaces AS tr"
                " JOIN iftypes    AS it USING(iftype_id)"
                " JOIN port_vlans USING(port_id)"
                " JOIN vlans      USING(vlan_id)"
                " WHERE tr.node_id = ? AND iftype_name = 'multiplexor'"
                " GROUP BY tr.port_index, trunk_port_name, member_port_ids, member_port_names"
                " ORDER BY tr.port_index ASC, trunk_port_name ASC";
        if (execSql(q1, sqlTrunk, actSwitchId)) do {
            QString trunkName = q1.value(0).toString();
            QVariantList memberPortIds = sqlToIntegerList(q1.value(1));
            if (memberPortIds.isEmpty()) continue;  // Nincsenek tag portok, nincs link sem
            QStringList memberPortNames = sqlToStringList( q1.value(2));
            vlanIds   = sqlToIntegerList(q1.value(3));
            vlanNames = sqlToStringList( q1.value(4));
            vlanTypes = sqlToStringList( q1.value(5));
            linkedId  = NULL_ID;

            portOk = linkOk = trunkOk = TS_TRUE;
            note.clear();
            row = emptyRow;

            foreach (QVariant vPortId, memberPortIds) {
                portId = vPortId.toLongLong();
                logLinkedId  = cLogLink(). getLinked(q2, portId);
                lldpLinkedId = cLldpLink().getLinked(q2, portId);
                if (logLinkedId == NULL_ID && lldpLinkedId == NULL_ID) {
                    linkOk = tsMin(linkOk, TS_NULL);
                }
                else if (logLinkedId != NULL_ID && lldpLinkedId != NULL_ID && logLinkedId != lldpLinkedId) {
                    linkOk = TS_FALSE;
                    break;
                }
                else {
                    qlonglong linkedMemberId = logLinkedId != NULL_ID ? logLinkedId : lldpLinkedId;
                    qlonglong _linkedId = cInterface().setById(q2, linkedMemberId).getId(_sPortStapleId);
                    if (_linkedId == NULL_ID) {
                        trunkOk = TS_FALSE;
                        break;
                    }
                    if (linkedId == NULL_ID) linkedId = _linkedId;
                    else if (linkedId != _linkedId) {
                        trunkOk = TS_FALSE;
                        break;
                    }
                }
            }
            if (tsMin(linkOk, trunkOk) == TS_FALSE) {
                note = sLinkCaveat;
                row[C_PORT]   = toHtml(trunkName, TS_FALSE);
                row[C_PTAGS]  = toHtml(memberPortNames.join(_sCommaSp), tsMin(trunkOk, TS_NULL));
                row[C_VLANS]  = vlanList(vlanNames, vlanTypes);
                row[C_NOTE]   = toHtml(note, TS_FALSE);
                matrix << row;
                swOk = tsMin(swOk, portOk = tsMin(portOk, TS_FALSE));
                continue;

            }
            if (linkedId == NULL_ID) continue;
            // Check VLANS
            vlanOk = checkVlans(q2, linkedId, vlanIds, row[C_VLANS_LINKED]);
            if (!vlanOk) msgAppend(&note, tr("Nem egyező vlan kiosztás!"));
            portOk = tsMin(portOk, bool2ts(vlanOk));
            swOk   = tsMin(swOk,   portOk);
            if ((portOk == TS_FALSE)
             || (portOk == TS_NULL  && reportLevel <= cReportWidget::RL_WARNING)
             || (portOk == TS_TRUE  && reportLevel == cReportWidget::RL_ALL)) {
                if (note.isNull() && portOk == TS_TRUE) note = _sOk;
                row[C_PORT]         = toHtml(trunkName, portOk);
                row[C_PTAGS]        = toHtml(memberPortNames.join(_sCommaSp), trunkOk);
                row[C_VLANS]        = vlanList(vlanNames, vlanTypes);
                row[C_LINKED]       = toHtml(cNPort().getFullNameById(q1, linkedId), linkOk);
                row[C_PLTAGS]       = trunkMembers(q1, linkedId);
                // C_VLANS_LINKED
                row[C_NOTE]         = toHtml(note, portOk);
                matrix << row;
            }
        } while (q1.next());

        nodeHead(swOk, nodeName + "; (" + switchVLanMap[actSwitchId] + ")", header, matrix);
        progres((progresCnt++ + 100) / recordNumber);
    }
}

