#include "findbymac.h"
#include "srvdata.h"
#include "lv2link.h"
#include "cerrormessagebox.h"
#include "report.h"

#include "ui_findbymac.h"

const enum ePrivilegeLevel cFindByMac::rights = PL_VIEWER;

cFindByMac::cFindByMac(QMdiArea *parent) :
    cIntSubObj(parent),
    pUi(new Ui::FindByMac)
{
    pq = newQuery();
    pUi->setupUi(this);
    // Query: All known MAC
    QString sql =
            "SELECT hwaddress FROM ("
               " SELECT DISTINCT(hwaddress) FROM arps"
              " UNION DISTINCT"
               " SELECT hwaddress FROM mactab"
              " UNION DISTINCT"
               " SELECT DISTINCT(hwaddress) FROM interfaces WHERE hwaddress IS NOT NULL) AS macs"
             " ORDER BY hwaddress ASC";
    QStringList items;
    if (execSql(*pq, sql)) do {
        // pUi->comboBox->addItem(pq->value(0).toString()); // SLOW !!
        items << pq->value(0).toString();
    } while (pq->next());
    pUi->comboBox->addItems(items);
    connect(pUi->pushButtonClose,   SIGNAL(clicked()), this, SLOT(endIt()));
    connect(pUi->pushButtonFindMac, SIGNAL(clicked()), this, SLOT(find()));
}

cFindByMac::~cFindByMac()
{
    delete pUi;
    delete pq;
}

void cFindByMac::find()
{
    QString sMac = pUi->comboBox->currentText();
    QString text = reportByMac(*pq, sMac);
    pUi->textEdit->setHtml(text);
}
