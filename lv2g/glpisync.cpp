#include "glpisync.h"
#include "ui_glpisync.h"

cGlpiSync::cGlpiSync(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::cGlpiSync)
{
    ui->setupUi(this);
}

cGlpiSync::~cGlpiSync()
{
    delete ui;
}
