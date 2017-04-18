#include "lv2g.h"
#include "cerrormessagebox.h"
#include "logon.h"

cMainWindow *    lv2g::pMainWindow = NULL;
bool lv2g::logonNeeded = false;
bool lv2g::zoneNeeded  = true;
const QString lv2g::sDefaultSplitOrientation= "defaultSplitOrientation";
const QString lv2g::sMaxRows                = "max_rows";
const QString lv2g::sSoundFileAlarm         = "sound_file_alarm";
const QString lv2g::sDialogRows             = "dialog-rows";
const QString lv2g::sHorizontal             = "Horizontal";
const QString lv2g::sVertical               = "Vertical";

lv2g::lv2g() :
    lanView(),
    pDesign(0)
{
    if (lastError != 0) return;
    try {
        zoneId = NULL_ID;
        #include "errcodes.h"
        pDesign = new lv2gDesign(this);
        if (dbIsOpen()) {
            switch (cLogOn::logOn(zoneNeeded ? &zoneId : NULL)) {
            case LR_OK:         break;
            case LR_INVALID:    EXCEPTION(ELOGON, LR_INVALID, trUtf8("Tul sok hibás bejelentkezési próbálkozás."));
            case LR_CANCEL:     EXCEPTION(ELOGON, LR_CANCEL, trUtf8("Mégsem."));
            default:            EXCEPTION(EOK);
            }
            QSqlQuery q = getQuery();
            maxRows = (int)cSysParam::getIntSysParam(q, sMaxRows, 100);
            bool ok;
            maxRows = pSet->value(sMaxRows, maxRows).toInt(&ok);
            if (!ok) maxRows = (int)cSysParam::getIntSysParam(q, sMaxRows, 100);
            dialogRows = pSet->value(sDialogRows, 16).toInt();
            soundFileAlarm = pSet->value(sSoundFileAlarm).toString();
        }
        else if (logonNeeded) EXCEPTION(ESQLOPEN, 0, trUtf8("Nincs elérhető adatbázis."));
        defaultSplitOrientation = Qt::Horizontal;
        if (0 == sVertical.compare(pSet->value(sDefaultSplitOrientation).toString(), Qt::CaseInsensitive))
            defaultSplitOrientation = Qt::Vertical;
    } CATCHS(lastError)
    if (dbIsOpen() && !zoneNeeded) zoneId = cPlaceGroup().getIdByName(_sAll);
}

lv2g::~lv2g()
{
    ;
}



QPalette *colorAndFont::pal = NULL;

colorAndFont::colorAndFont()
{
    fg = palette().color(QPalette::Text);
    bg = palette().color(QPalette::Base);
}

QPalette& colorAndFont::palette()
{
    if (pal == NULL) pal = new QPalette;
    return *pal;
}


lv2gDesign::lv2gDesign(QObject * par) : QObject(par)
{
    titleError   = trUtf8("Hiba");
    titleWarning = trUtf8("Figyelmeztetés");
    titleInfo    = trUtf8("Megjegyzés");

    valNull     = trUtf8("[NULL]");
    valDefault  = trUtf8("[Default]");
    valAuto     = trUtf8("[Auto]");


    head.font.setBold(true);
    head.bg = colorAndFont::palette().color(QPalette::AlternateBase);

    (void)data;

    id.font.setBold(true);
    id.font.setUnderline(true);
    id.fg.setNamedColor("darkred");

    name.font.setBold(true);
    name.fg.setNamedColor("seagreen");

    primary.font.setBold(true);
    primary.font.setUnderline(true);

    key.font.setBold(true);

    fname.fg.setNamedColor("darkcyan");
    fname.font.setItalic(true);

    derived.fg = colorAndFont::palette().color(QPalette::Link);
    derived.font.setItalic(true);

    tree.fg.setNamedColor("darkblue");
    tree.font.setItalic(true);

    foreign.fg = colorAndFont::palette().color(QPalette::Link);

    null.font.setItalic(true);
    null.fg.setNamedColor("lightcoral");

    warning.font.setItalic(true);
    warning.fg.setNamedColor("red");
}

lv2gDesign::~lv2gDesign()
{
    ;
}

const colorAndFont&   lv2gDesign::operator[](int role) const
{
    switch (role & ~(GDR_COLOR | GDR_FONT)) {
    case GDR_HEAD:      return head;
    case GDR_DATA:      return data;
    case GDR_ID:        return id;
    case GDR_NAME:      return name;
    case GDR_PRIMARY:   return primary;
    case GDR_KEY:       return key;
    case GDR_FNAME:     return fname;
    case GDR_DERIVED:   return derived;
    case GDR_FOREIGN:   return foreign;
    case GDR_TREE:      return tree;
    case GDR_NULL:      return null;
    case GDR_WARNING:   return warning;
    default:            EXCEPTION(ENOINDEX);
    }
    return *(colorAndFont *)NULL;     // Ez sosem hajtódik végre, de ha nincs return, warning-ol.
}

eDesignRole lv2gDesign::desRole(const cRecStaticDescr& __d, int __ix)
{
    __d.chkIndex(__ix);
    if (__ix == __d.idIndex(EX_IGNORE))      return GDR_ID;
    if (__ix == __d.nameIndex(EX_IGNORE))    return GDR_NAME;
    if (__d.primaryKey()[__ix])     return GDR_PRIMARY;
    const cColStaticDescr& cd = __d[__ix];
    switch (cd.fKeyType) {
    case cColStaticDescr::FT_SELF:
        return GDR_TREE;
    case cColStaticDescr::FT_PROPERTY:
    case cColStaticDescr::FT_OWNER:
    {
        cRecordAny *pF = cd.pFRec;
        if (pF == NULL) pF = new cRecordAny(cd.fKeyTable, cd.fKeySchema);
        bool n = pF->isIndex(pF->nameIndex(EX_IGNORE));
        if (cd.pFRec == NULL) delete pF;
        if (n) return GDR_FNAME;
        if (!cd.fnToName.isEmpty())     return GDR_DERIVED;
        return GDR_FOREIGN;
    }
        break;
    default:
        break;
    }
    if (__d.isKey(__ix)) return GDR_KEY;
    return GDR_DATA;
}

QString _titleWarning;
QString _titleError;
QString _titleInfo;


QPalette *pDefaultPalette = NULL;
QColor   *pFgNullColor = NULL;


void _setColors()
{
    pDefaultPalette = new QPalette();
    pFgNullColor    = new QColor(QString("lightcoral"));
}

QFont    *pHeadFont = NULL;
QFont    *pDataFont = NULL;
QFont    *pNullFont = NULL;

void _setFonts()
{
    pHeadFont = new QFont();
    pDataFont = new QFont();
    pNullFont = new QFont();
    pHeadFont->setBold(true);
    pNullFont->setItalic(true);
}


cIntSubObj::cIntSubObj(QMdiArea *par) :QWidget(par)
{
    if (par == NULL) {
        pSubWindow = NULL;
    }
    else {
#if 0
        pSubWindow = par->addSubWindow(pWidget());
#else
        pSubWindow = new QMdiSubWindow;
        pSubWindow->setWidget(pWidget());
        pSubWindow->setAttribute(Qt::WA_DeleteOnClose);
        par->addSubWindow(pSubWindow);
#endif
    }
}

void cIntSubObj::endIt()
{
    closeIt();
}

cLv2GQApp::cLv2GQApp(int& argc, char ** argv) : QApplication(argc, argv)
{
    lanView::gui = true;
}


cLv2GQApp::~cLv2GQApp()
{
    ;
}

bool cLv2GQApp::notify(QObject * receiver, QEvent * event)
{
    static cError *lastError = NULL;
    try {
        return QApplication::notify(receiver, event);
    }
    catch(no_init_&) { // Már letiltottuk a cError dobálást
        PDEB(VERBOSE) << "Dropped cError..." << endl;
        return false;
    }
    CATCHS(lastError)
    /*
    PDEB(DERROR) << "Error in " << __PRETTY_FUNCTION__ << endl;
    PDEB(DERROR) << "Receiver : " << receiver->objectName() << "::" << typeid(*receiver).name() << endl;
    PDEB(DERROR) << "Event : " << typeid(*event).name() << endl;  Kiakad ?! */
    cError::mDropAll = true;                    // A továbbiakban nem *cError-al dobja a hibákat, hanem no_init_ -el
    int e = lastError->mErrorCode;
    if (e == eError::EOK) {
        pDelete(lastError);
    }
    else {
        lanView::getInstance()->lastError = lastError;
        cErrorMessageBox::messageBox(lastError);    // Kitesszük a hibaüzenetet,
     }
    QApplication::exit(e);  // kilépünk.
    return false;
}


_GEX QPolygonF convertPolygon(const tPolygonF __pol)
{
    QPolygonF   pol;
    foreach (QPointF p, __pol) {
       pol << p;
    }
    return pol;
}

_GEX const QColor& bgColorByEnum(const QString& __v, const QString& __t)
{
    static QMap<QString, QMap<QString, QColor> >   colorCache;

    QColor& c = colorCache[__t][__v];
    if (c.isValid()) return c;
    QString cn = cEnumVal::bgColor(__v, __t);
    if (cn.isEmpty()) {
        c = QPalette().color(QPalette::Base);
    }
    else {
        c.setNamedColor(cn);
    }
    return c;
}

_GEX const QColor& fgColorByEnum(const QString& __v, const QString& __t)
{
    static QMap<QString, QMap<QString, QColor> >   colorCache;

    QColor& c = colorCache[__t][__v];
    if (c.isValid()) return c;
    QString cn = cEnumVal::fgColor(__v, __t);
    if (cn.isEmpty()) {
        c = QPalette().color(QPalette::Base);
    }
    else {
        c.setNamedColor(cn);
    }
    return c;
}

