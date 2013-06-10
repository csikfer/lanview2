#ifndef LV2VALIDATOR_H
#define LV2VALIDATOR_H
#include "lv2g.h"
#include <QValidator>

/// @class cIntValidator
/// @brief Input validátor az egészekhez, megengedve feltételesen az üres inputot
class cIntValidator : public QValidator {
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cIntValidator(bool _nullable, QObject *parent=0) : QValidator(parent) { nullable = _nullable; }
    void fixup(QString &input) const;
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
};

/// @class cRealValidator
/// @brief Input validátor az valós számokhoz, megengedve feltételesen az üres inputot
class cRealValidator : public QValidator {
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cRealValidator(bool _nullable, QObject *parent=0) : QValidator(parent) { nullable = _nullable; }
    void fixup(QString &input) const;
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
};

/// @class cMacValidator
/// @brief Input validátor a MAC adattípushoz.
class cMacValidator : public QValidator {
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cMacValidator(bool _nullable, QObject *parent=0) : QValidator(parent) { nullable = _nullable; }
    void fixup(QString &input) const;
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
};

/// @class cINetValidator
/// @brief Input validátor a INET adattípushoz.
/// Az adatbázisban az INET típus hosz és net címet is jelenthet, jelen esetben csak a hoszt cím van engedélyezve.
class cINetValidator : public QValidator {
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cINetValidator(bool _nullable, QObject *parent=0) : QValidator(parent) { nullable = _nullable; }
    void fixup(QString &input) const;
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
};

/// @class cINetValidator
/// @brief Input validátor a CIDR adattípushoz. (hálózati cím tartomány)
class cCidrValidator : public QValidator {
public:
    /// Konstruktor
    /// @param _nullable Ha értéke true, akkor az üres input is megengedett
    cCidrValidator(bool _nullable, QObject *parent=0) : QValidator(parent) { nullable = _nullable; }
    void fixup(QString &input) const;
    State validate(QString &input, int &pos) const;
private:
    bool nullable;
};

#endif // LV2VALIDATOR_H
