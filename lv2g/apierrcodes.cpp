#include "apierrcodes.h"

const enum ePrivilegeLevel cErrcodesWidget::rights = PL_OPERATOR;

cErrcodesWidget::cErrcodesWidget(QMdiArea *par)
: cIntSubObj(par)
{
    pUi = new Ui::apiErrorCodes();
    pUi->setupUi(this);

    const QMap<int, QString>& emap = cError::errorMap();
    QList<int> codes = emap.keys();
    std::sort(codes.begin(), codes.end());
    int row = 0;
    pUi->errcodesTW->setRowCount(codes.size());
    foreach (int code, codes) {
        pUi->errcodesTW->setItem(row, 0, new QTableWidgetItem(QString::number(code)));
        pUi->errcodesTW->setItem(row, 1, new QTableWidgetItem(emap[code]));
        ++row;
    }
}

cErrcodesWidget::~cErrcodesWidget()
{
    ;
}

