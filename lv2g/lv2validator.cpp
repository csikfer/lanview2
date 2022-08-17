#include "lv2validator.h"

QValidator::State cIntValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    bool ok;
    qlonglong i = input.toLongLong(&ok);
    if (ok) ok = i >= min;
    if (ok) ok = i <= max;
    return ok ? Acceptable : Invalid;
}

QValidator::State cRealValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    bool ok;
    double r = input.toDouble(&ok);
    if (ok) ok = r >= min;
    if (ok) ok = r <= max;
    return ok ? Acceptable : Invalid;
}

void cMacValidator::fixup(QString &input) const
{
    cMac mac(input);
    if (mac.isValid()) input = mac.toString();
}

QValidator::State cMacValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    static const QStringList rexps = {
        cMac::_sMacPattern1,        // Pure hexa
        cMac::_sMacPattern2,        // 6x hexa number, separator: ':' or '-'
        cMac::_sMacPattern3,        // 2x hexa number, separator: '-'
        cMac::_sMacPattern5         // 6x hexa number, separator: space(s)
    };
    for (const QString& sre: rexps) {
        const QRegularExpression re(QRegularExpression::anchoredPattern(sre), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch ma = re.match(input, 0, QRegularExpression::PartialPreferCompleteMatch);
        if (ma.hasMatch()) return Acceptable;
        if (ma.hasPartialMatch()) return Intermediate;
    }
    // 6x decimal number, separator: '.'
    const QRegularExpression re(QRegularExpression::anchoredPattern(cMac::_sMacPattern4), QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch ma = re.match(input, 0, QRegularExpression::PartialPreferCompleteMatch);
    QValidator::State st;
    if      (ma.hasMatch())        st = Acceptable;
    else if (ma.hasPartialMatch()) st = Intermediate;
    else                           return Invalid;
    const QStringList slDecVals = input.split('.');
    for (const QString& sDecVal: slDecVals) {
        int decval = sDecVal.toInt();
        if (decval > 255) return Invalid;
    }
    return st;
}

QValidator::State cINetValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    QHostAddress    a(input);
    if (!a.isNull()) return Acceptable;   // IPV4 esetén hiányos címet is elfogad; IPV6 ?
    if (input.contains(':')) {  // IPV6
        static const QRegularExpression re("^[:\\dABCDEF]+$", QRegularExpression::CaseInsensitiveOption);
        if (!re.match(input).hasMatch()) return Invalid;
        return Intermediate;  // Nincs benne rossz karakter, de nem IP cím
    }
    else {                      // IPV4
        QStringList nl = input.split('.');
        if (nl.size() > 4) return Invalid;
        bool ok;
        foreach (QString n, nl) {
            if (n.isEmpty()) continue;
            int i = n.toInt(&ok);
            if (!ok || i < 0 || i > 255) return Invalid;
        }
        return Intermediate;
    }
}

QValidator::State cCidrValidator::validate(QString &input, int &pos) const
{
    (void)pos;
    input.remove(QChar(' '));
    if (input.isEmpty()) return nullable ? Acceptable : Intermediate;
    netAddress    n(input);
    if (n.isValid()) return Acceptable;
    static const QRegularExpression re("^[\\./:\\dABCDEF]+$", QRegularExpression::CaseInsensitiveOption);
    if (!re.match(input).hasMatch()) return Invalid;
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
