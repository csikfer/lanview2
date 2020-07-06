#ifndef GLPISYNC_H
#define GLPISYNC_H

#include <QWidget>

namespace Ui {
class cGlpiSync;
}

class cGlpiSync : public QWidget
{
    Q_OBJECT

public:
    explicit cGlpiSync(QWidget *parent = nullptr);
    ~cGlpiSync();

private:
    Ui::cGlpiSync *ui;
};

#endif // GLPISYNC_H
