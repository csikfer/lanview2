#ifndef OBJECT_DIALOG
#define OBJECT_DIALOG

#include "lv2models.h"
#include "lv2widgets.h"
#include "lv2validator.h"
#include "record_dialog.h"
#include "ui_edit_enum_vals.h"
#include "ui_edit_ip.h"

/*
class LV2GSHARED_EXPORT cObjectDialog : public QDialog {
public:
    cObjectDialog(QWidget *parent = NULL, bool ro = false);
    ~cObjectDialog();
    virtual cRecord *get() = 0;
    virtual void set(cRecord *po) = 0;
    static cRecord * insertDialog(QWidget *pPar, cRecord * pSample = NULL);
    static cRecord * editDialog(QWidget *pPar, cRecord * pSample, bool ro = false);
protected:
    virtual bool checkObjectType(cRecord *po) = 0;
    bool        readOnly;
    bool        lockSlot;
    QSqlQuery * pq;
};
*/
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
    cPatchDialog(QWidget *parent = nullptr, bool ro = false);
    ~cPatchDialog();
    cPatch * getPatch();
    void setPatch(const cPatch *pSample);
protected:
    QSqlQuery              *pq;
    Ui::patchSimpleDialog  *pUi;
    cSelectPlace           *pSelectPlace;
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

_GEX cPatch * patchInsertDialog(QSqlQuery& q, QWidget *pPar, cPatch * pSample = nullptr);
_GEX cPatch * patchEditDialog(QSqlQuery& q, QWidget *pPar, cPatch * pSample, bool ro = false);

namespace Ui { class editIp; }
class LV2GSHARED_EXPORT cIpEditWidget : public QWidget {
    Q_OBJECT
public:
    cIpEditWidget(const tIntVector &_types = tIntVector(), QWidget *_par = nullptr);
    ~cIpEditWidget();
    void set(const cIpAddress *po);
    void set(const QHostAddress &a);
    cIpAddress *get(cIpAddress *po) const;
    cIpAddress *get() const { return get(new cIpAddress); }
    const QHostAddress& getAddress() { return actAddress; }
    enum eState {
        IES_OK                  = 0,
        IES_ADDRESS_IS_NULL     = 1,
        IES_ADDRESS_IS_INVALID  = 2,
        IES_ADDRESS_TYPE_IS_NULL= 4,
        IES_SUBNET_IS_NULL      = 8,
        IES_IS_NULL = IES_ADDRESS_IS_NULL | IES_ADDRESS_TYPE_IS_NULL | IES_SUBNET_IS_NULL
    };
    int state() const { return _state; }
    bool isDisabled() { return disabled; }
    void showToolBoxs(bool _info, bool _go, bool _query);
    void enableToolButtons(bool _info, bool _go, bool _query);
    void setInfoToolTip(const QString& _tt) { pUi->toolButtonInfo->setToolTip(_tt); }
    void setGoToolTip(const QString& _tt)   { pUi->toolButtonGo->setToolTip(_tt); }
    void setQueryToolTip(const QString& _tt){ pUi->toolButtonQuery->setToolTip(_tt); }
private:
    int _state;
    Ui::editIp *pUi;
    cLineWidget *pEditNote;
    QSqlQuery * pq;
    qlonglong   ipTypes;
    cSelectVlan *pSelectVlan;
    cSelectSubNet *pSelectSubNet;
    cEnumListModel *pModelIpType;
    cINetValidator *pINetValidator;
    QHostAddress actAddress;
    bool disableSignals;
    bool disabled;
    bool disabled_info;
    bool disabled_go;
    bool disabled_query;

public slots:
    void setAllDisabled(bool f = true);
private slots:
    void _subNetIdChanged(qlonglong _id);
    void on_comboBoxIpType_currentIndexChanged(int index);
    void on_lineEditAddress_textChanged(const QString &arg1);
    void on_toolButtonQuery_clicked();
    void on_toolButtonInfo_clicked();
    void on_toolButtonGo_clicked();

signals:
//    void vlanIdChanged(qlonglong _vid);
//    void subNetIdChanged(qlonglong _sid);
    void changed(const QHostAddress& _a, int _st);
    void query_clicked();
    void info_clicked();
    void go_clicked();
};

namespace Ui { class enumValsWidget; }
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
    cEnumValsEditWidget(QWidget *parent = nullptr);
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

