#include "lv2validator.h"
#include "QRegExp"


void cIntValidator::fixup(QString &input) const
{
    (void)input;
}

QValidator::State cIntValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return setState(nullable ? Acceptable : Intermediate);
    bool ok;
    qlonglong i = input.toLongLong(&ok);
    if (ok) ok = i >= min;
    if (ok) ok = i <= max;
    return setState(ok ? Acceptable : Invalid);
}

void cRealValidator::fixup(QString &input) const
{
    (void)input;
}

QValidator::State cRealValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return setState(nullable ? Acceptable : Intermediate);
    bool ok;
    double r = input.toDouble(&ok);
    if (ok) ok = r >= min;
    if (ok) ok = r <= max;
    return setState(ok ? Acceptable : Invalid);
}


void cMacValidator::fixup(QString &input) const
{
    (void)input;
}

QValidator::State cMacValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return setState(nullable ? Acceptable : Intermediate);
    cMac    mac(input);
    if (mac.isValid()) return setState(Acceptable);
    QRegExp re("^[\\-:\\dABCDEF]+$", Qt::CaseInsensitive);
    if (!re.exactMatch(input)) return setState(Invalid);
    return setState(Intermediate);
}

void cINetValidator::fixup(QString &input) const
{
    (void)input;
}

QValidator::State cINetValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return setState(nullable ? Acceptable : Intermediate);
    QHostAddress    a(input);
    bool isNull = a.isNull();   // IPV4 esetén hiányos címet is elfogad; IPV6 ?
    if (input.contains(':')) {  // IPV6
        QRegExp re("^[:\\dABCDEF]+$", Qt::CaseInsensitive);
        if (!re.exactMatch(input)) return setState(Invalid);
        return setState(isNull ? Intermediate : Acceptable);  // ??
    }
    else {                      // IPV4
        QStringList nl = input.split('.');
        if (nl.size() > 4) return setState(Invalid);
        bool ok;
        foreach (QString n, nl) {
            if (n.isEmpty()) {
                isNull = true;
                continue;
            }
            int i = n.toInt(&ok);
            if (i < 0 || i > 255 || !ok) return setState(Invalid);
        }
        return setState(isNull || nl.size() != 4 ? Intermediate : Acceptable);
    }
}

void cCidrValidator::fixup(QString &input) const
{
    (void)input;
}

QValidator::State cCidrValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return setState(nullable ? Acceptable : Intermediate);
    netAddress    n(input);
    if (n.isValid()) return setState(Acceptable);
    QRegExp re("^[\\./:\\dABCDEF]+$", Qt::CaseInsensitive);
    if (!re.exactMatch(input)) return setState(Invalid);
    return setState(Intermediate);
}

/* -------------------------- */

cItemDelegateValidator::cItemDelegateValidator(QValidator *pVal, QObject *parent) :
    QItemDelegate(parent)
{
    pValidator = pVal;
}

QWidget *cItemDelegateValidator::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    (void)option; (void)index;
    QLineEdit *editor = new QLineEdit(parent);
    editor->setValidator(pValidator);
    return editor;
}


void cItemDelegateValidator::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
    QString value =index.model()->data(index, Qt::EditRole).toString();
        QLineEdit *line = static_cast<QLineEdit*>(editor);
        line->setText(value);
}


void cItemDelegateValidator::setModelData(QWidget *editor,
                                QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QLineEdit *line = static_cast<QLineEdit*>(editor);
    QString value = line->text();
    model->setData(index, value);
}


void cItemDelegateValidator::updateEditorGeometry(QWidget *editor,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    (void)index;
    editor->setGeometry(option.rect);
}
