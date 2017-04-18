#ifndef OBJECT_DIALOG
#define OBJECT_DIALOG

#include "lv2models.h"
#include "lv2widgets.h"
#include "record_dialog.h"

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
    cPatchDialog(QWidget *parent = NULL);
    ~cPatchDialog();
    cPatch * getPatch();
    void setPatch(const cPatch *pSample);
protected:
    QSqlQuery              *pq;
    Ui::patchSimpleDialog  *pUi;
    cZoneListModel         *pModelZone;
    cPlacesInZoneModel     *pModelPlace;
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
    void changeFilterZone(int i);
    void newPlace();
    void cellChanged(int row, int col);
    void selectionChanged(const QItemSelection &, const QItemSelection &);
};

_GEX cPatch * patchDialog(QSqlQuery& q, QWidget *pPar, cPatch * pSample = NULL);

namespace Ui {
    class enumValsDialog;
}
class cEnumValsDialog;
class cEnumValRow;

#if defined(LV2G_LIBRARY)
#  include "ui_dialog_enum_vals.h"
    class cEnumValRow : public QObject {
        Q_OBJECT
    public:
        cEnumValRow(QSqlQuery& q, const QString& _val, int _row, cEnumValsDialog *par);
        cEnumValsDialog *parent;
        QTableWidget   *pTableWidget;
        const int       row;
        cEnumVal        rec;
        cColorWidget   *pBgColorWidget;
        cColorWidget   *pFgColorWidget;
    };

    enum {
        CEV_NAME, CEV_SHORT, CEV_LONG, CEV_BG_COLOR, CEV_FG_COLOR, CEV_FONT, CEV_NOTE
    };
#endif

class LV2GSHARED_EXPORT cEnumValsDialog : public QDialog {
    friend class cEnumValRow;
    Q_OBJECT
public:
    cEnumValsDialog(QWidget *parent = NULL);
    ~cEnumValsDialog();
protected:
    QSqlQuery          *pq;
    Ui::enumValsDialog *pUi;
    cTableShape        *pShape;
    cRecordDialog      *pSinge;
    QString             enumTypeName;
    const cColEnumType *pEnumType;
protected slots:
    void clicked(QAbstractButton *pButton);
    void setEnumType(const QString& etn);
};

#endif // OBJECT_DIALOG

