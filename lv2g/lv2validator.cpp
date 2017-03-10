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
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    bool ok;
    (void)input.toLongLong(&ok);
    return ok ? Acceptable : Invalid;
}

void cRealValidator::fixup(QString &input) const
{
    (void)input;
}

QValidator::State cRealValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    bool ok;
    (void)input.toDouble(&ok);
    return ok ? Acceptable : Invalid;
}


void cMacValidator::fixup(QString &input) const
{
    (void)input;
}

QValidator::State cMacValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    cMac    mac(input);
    if (mac.isValid()) return Acceptable;
    QRegExp re("^[\\-:\\dABCDEF]+$", Qt::CaseInsensitive);
    if (!re.exactMatch(input)) return Invalid;
    return Intermediate;
}

void cINetValidator::fixup(QString &input) const
{
    (void)input;
}

QValidator::State cINetValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    QHostAddress    a(input);
    if (!a.isNull()) return Acceptable;
    if (input.contains(':')) {  // IPV6
        QRegExp re("^[:\\dABCDEF]+$", Qt::CaseInsensitive);
        if (!re.exactMatch(input)) return Invalid;
        return Intermediate;
    }
    else {                      // IPV4
        QStringList nl = input.split('.');
        if (nl.size() > 4) return Invalid;
        bool ok;
        foreach (QString n, nl) {
            if (n.isEmpty()) continue;
            int i = n.toInt(&ok);
            if (i < 0 || i > 255 || !ok) return Invalid;
        }
        return Intermediate;
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
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    netAddress    n(input);
    if (n.isValid()) return Acceptable;
    QRegExp re("^[\\./:\\dABCDEF]+$", Qt::CaseInsensitive);
    if (!re.exactMatch(input)) return Invalid;
    return Intermediate;
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
    editor->setGeometry(option.rect);
}
