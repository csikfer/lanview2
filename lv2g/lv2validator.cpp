#include "lv2validator.h"


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
    if (input.isEmpty()) return setState(nullable ? Acceptable : Intermediate);
    static const QStringList rexps = {
        cMac::_sMacPattern1, cMac::_sMacPattern2, cMac::_sMacPattern3, cMac::_sMacPattern4, cMac::_sMacPattern5
    };
    for (auto sre: rexps) {
        const QRegularExpression re(sre, QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch ma = re.match(input);
        if (ma.hasMatch()) return setState(Acceptable);
        if (ma.hasPartialMatch()) return setState(Intermediate);
    }
    return setState(Invalid);
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
    if (!a.isNull()) return setState(Acceptable);   // IPV4 esetén hiányos címet is elfogad; IPV6 ?
    if (input.contains(':')) {  // IPV6
        static const QRegularExpression re("^[:\\dABCDEF]+$", QRegularExpression::CaseInsensitiveOption);
        if (!re.match(input).hasMatch()) return setState(Invalid);
        return setState(Intermediate);  // ??
    }
    else {                      // IPV4
        QStringList nl = input.split('.');
        if (nl.size() > 4) return setState(Invalid);
        bool ok;
        foreach (QString n, nl) {
            if (n.isEmpty()) {
                return setState(Invalid);
            }
            int i = n.toInt(&ok);
            if (i < 0 || i > 255 || !ok) return setState(Invalid);
        }
        return setState(Intermediate);
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
    static const QRegularExpression re("^[\\./:\\dABCDEF]+$", QRegularExpression::CaseInsensitiveOption);
    if (!re.match(input).hasMatch()) return setState(Invalid);
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
