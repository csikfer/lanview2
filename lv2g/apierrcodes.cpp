#include "apierrcodes.h"

const enum ePrivilegeLevel cErrcodesWidget::rights = PL_OPERATOR;

cErrcodesWidget::cErrcodesWidget(QWidget *par)
: cOwnTab(par)
{
    pUi = new Ui::apiErrorCodes();
    pUi->setupUi(this);

    connect(pUi->closwPB, SIGNAL(clicked()), this, SLOT(endIt()));
    const QMap<int, QString>& emap = cError::errorMap();
    QList<int> codes = emap.keys();
    qSort(codes);
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

