#ifndef OBJECT_DIALOG
#define OBJECT_DIALOG

#include "lv2models.h"
#include "lv2widgets.h"
#include "record_dialog.h"
#include "ui_edit_enum_vals.h"


namespace Ui {
    class patchSimpleDialog;
}
class cPatchDialog;
class cPPortTableLine;

#if defined(LV2G_LIBRARY)
#  include "ui_dialogpatchsimple.h"
    /// Az objektum a cPatchDialog dialogus port táblázatának egy sora.
    class cPPortTableLine : public QObject {
        friend class cPatchDialog;
        Q_OBJECT
    public:
        cPPortTableLine(int r, cPatchDialog *par);
        /// A táblázatban a sor száma (0,1, ...)
        int                 row;
        /// Másodlagos megosztott port esetén az elsődleges port sorszáma a táblázatban.
        /// Ha nincs megadva akkor -1.
        int                 sharedPortRow;
        /// A parent dialógus pointere
        cPatchDialog*       parent;
        /// A táblázat aminek az objektum egy sora
        QTableWidget *      tableWidget;
        /// Megosztás típusok comboBox widget.
        QComboBox *         comboBoxShare;
        /// comboBox widget, másodlagos megosztáshoz tartozó választható elsődleges portok listája.
        QComboBox *         comboBoxPortIx;
        /// Másodlagos megosztáshoz tartozó választható elsődleges portok listája (sor indexek).
        QList<int>          listPortIxRow;
    private:
        bool lockSlot;
    protected slots:
        void changeShared(int sh);
        void changePortIx(int ix);
    };

    enum {
        CPP_NAME, CPP_TAG, CPP_INDEX, CPP_SH_TYPE, CPP_SH_IX, CPP_NOTE
    };

#endif


class LV2GSHARED_EXPORT cPatchDialog : public QDialog {
    friend class cPPortTableLine;
    Q_OBJECT
public:
    cPatchDialog(QWidget *parent = NULL, bool ro = false);
    ~cPatchDialog();
    cPatch * getPatch();
    void setPatch(const cPatch *pSample);
protected:
    QSqlQuery              *pq;
    Ui::patchSimpleDialog  *pUi;
    cSelectPlace           *pSelectPlace;
    // cZoneListModel         *pModelZone;
    // cPlacesInZoneModel     *pModelPlace;
    bool                    lockSlot;
    QList<cPPortTableLine *>rowsData;
    QList<int>              shPrimRows;
    QMap<int, QList<int> >  shPrimMap;
    bool                    shOk;
    bool                    pNamesOk;
    bool                    pIxOk;
    static QString          sPortRefForm;
    void clearRows();
    void setRows(int rows);
    void setPortShare(int row, int sh);
    void updateSharedIndexs();
    void updatePNameIxOk();
    static QString refName(int row, const QString& name, int ix) {
        return sPortRefForm.arg(row +1).arg(name).arg(ix);
    }
    static QString refName(int row, const QString& name, const QString& ix) {
        return sPortRefForm.arg(row +1).arg(name, ix);
    }
    QString refName(int row);

private slots:
    void changeName(const QString& name);
    void set1port();
    void set2port();
    void set2sharedPort();
    void addPorts();
    void delPorts();
    void changeFrom(int i);
    void changeTo(int i);
    void cellChanged(int row, int col);
    void selectionChanged(const QItemSelection &, const QItemSelection &);
};

_GEX cPatch * patchDialog(QSqlQuery& q, QWidget *pPar, cPatch * pSample = NULL, bool ro = false);

namespace Ui {
    class enumValsWidget;
}
class cEnumValsEditWidget;
class cEnumValRow;

// private class
#if defined(LV2G_LIBRARY)
    class cEnumValRow : public QObject {
        Q_OBJECT
    public:
        cEnumValRow(QSqlQuery& q, const QString& _val, int _row, cEnumValsEditWidget *par);
        void save(QSqlQuery &q);
        cEnumValsEditWidget*parent;
        QTableWidget   *pTableWidget;
        const int       row;
        cEnumVal        rec;
        cColorWidget   *pBgColorWidget;
        cColorWidget   *pFgColorWidget;
        cFontFamilyWidget *pFntFamWidget;
        cFontAttrWidget *pFntAttWidget;
    };

    enum {
        CEV_NAME, CEV_SHORT, CEV_LONG, CEV_BG_COLOR, CEV_FG_COLOR, CEV_FNT_FAM, CEV_FNT_ATT, CEV_NOTE, CEV_TOOL_TIP,
        CEV_COUNT
    };
#endif

inline static void formSetField(QFormLayout *pFormLayout, QWidget *pLabel, QWidget *pField)
{
    int i, row;
    QFormLayout::ItemRole ir;
    i = pFormLayout->indexOf(pLabel);
    pFormLayout->getItemPosition(i, &row, &ir);
    pFormLayout->setWidget(row, QFormLayout::FieldRole, pField);
}

class LV2GSHARED_EXPORT cEnumValsEditWidget : public QWidget {
    friend class cEnumValRow;
    Q_OBJECT
public:
    cEnumValsEditWidget(QWidget *parent = NULL);
    ~cEnumValsEditWidget();
protected:
    bool save();
    bool saveValue();
    bool saveType();
    bool saveBoolean();

    QSqlQuery          *pq;
    Ui::enumValsWidget *pUi;
    cTableShape        *pShape;
    cRecordDialog      *pSinge;
    bool                lockSlot;
    // Val
    int                 langIdVal;
    cSelectLanguage    *pSelLangVal;
    QString             enumValTypeName;
    const cColEnumType *pEnumValType;
    cEnumVal            val;
    cColorWidget       *pWidgetValBgColor;
    cColorWidget       *pWidgetValFgColor;
    cFontFamilyWidget  *pWidgetValFntFam;
    cFontAttrWidget    *pWidgetValFntAtt;
    // Type
    QString             enumTypeTypeName;
    cEnumVal            type;
    const cColEnumType *pEnumTypeType;
    QList<cEnumValRow *>    rows;
    // Boolean
        // true
    cEnumVal boolType, boolTrue, boolFalse;
    cColorWidget       *pWidgetTrueBgColor;
    cColorWidget       *pWidgetTrueFgColor;
    cFontFamilyWidget  *pWidgetTrueFntFam;
    cFontAttrWidget    *pWidgetTrueFntAtt;
        // false
    cColorWidget       *pWidgetFalseBgColor;
    cColorWidget       *pWidgetFalseFgColor;
    cFontFamilyWidget  *pWidgetFalseFntFam;
    cFontAttrWidget    *pWidgetFalseFntAtt;
signals:
    void closeWidget();
protected slots:
    void clicked(QAbstractButton *pButton);

    void setEnumValType(const QString& etn);
    void setEnumValVal(const QString& ev);

    void setEnumTypeType(const QString& etn);

    void setBoolTable(const QString& tn);
    void setBoolField(const QString& fn);
};

class LV2GSHARED_EXPORT cEnumValsEdit
        : public    cIntSubObj
{
    Q_OBJECT
public:
    cEnumValsEdit(QMdiArea *par);
    ~cEnumValsEdit();
    static const enum ePrivilegeLevel rights;
    cEnumValsEditWidget *pEditWidget;
};

#endif // OBJECT_DIALOG

