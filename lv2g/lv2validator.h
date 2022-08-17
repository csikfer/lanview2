#ifndef LV2VALIDATOR_H
#define LV2VALIDATOR_H
#include "lv2g.h"
#include <QValidator>
#include <QItemDelegate>


/// @class cIntValidator
/// @brief Input validátor az egészekhez, megengedve feltételesen az üres inputot
class LV2GSHARED_EXPORT cIntValidator : public QValidator {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cIntValidator(bool _nullable = false, QObject *parent = 0) : QValidator(parent) {
        nullable = _nullable;
        min = std::numeric_limits<qlonglong>::min();
        max = std::numeric_limits<qlonglong>::max();
    }
    void setMin(qlonglong _mi) { min = _mi; }
    void setMax(qlonglong _ma) { max = _ma; }
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
    qlonglong   min;
    qlonglong   max;
};

/// @class cRealValidator
/// @brief Input validátor az valós számokhoz, megengedve feltételesen az üres inputot
class LV2GSHARED_EXPORT cRealValidator : public QValidator {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cRealValidator(bool _nullable = false, QObject *parent = 0) : QValidator(parent) {
        nullable = _nullable;
        min = std::numeric_limits<double>::min();
        max = std::numeric_limits<double>::max();
    }
    void setMin(double _mi) { min = _mi; }
    void setMax(double _ma) { max = _ma; }
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
    double min;
    double max;
};

/// @class cMacValidator
/// @brief Input validátor a MAC adattípushoz.
class LV2GSHARED_EXPORT cMacValidator : public QValidator {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cMacValidator(bool _nullable = false, QObject *parent=0) : QValidator(parent) { nullable = _nullable; }
    void fixup(QString &input) const;
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
};

/// @class cINetValidator
/// @brief Input validátor a INET adattípushoz.
/// Az adatbázisban az INET típus host és net címet is jelenthet, jelen esetben csak a hoszt cím van engedélyezve.
class LV2GSHARED_EXPORT cINetValidator : public QValidator {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cINetValidator(bool _nullable = false, QObject *parent=0) : QValidator(parent) { nullable = _nullable; }
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
};

/// @class cCidrValidator
/// @brief Input validátor a CIDR adattípushoz. (hálózati cím tartomány)
class LV2GSHARED_EXPORT cCidrValidator : public QValidator {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cCidrValidator(bool _nullable = false, QObject *parent=0) : QValidator(parent) { nullable = _nullable; }
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
};

class LV2GSHARED_EXPORT cItemDelegateValidator : public QItemDelegate
{
    Q_OBJECT
public:
    explicit cItemDelegateValidator(QValidator *pVal, QObject *parent = 0);

protected:
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget * editor, const QModelIndex & index) const;
    void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const;
    void updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const;

    QValidator *pValidator;
signals:

public slots:

};

#endif // LV2VALIDATOR_H
