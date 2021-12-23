#include "lv2g.h"
#include "mainwindow.h"
#include "cerrormessagebox.h"
#include "logon.h"
#include "QInputDialog"
#include "QFileDialog"
#include "popupreport.h"

cMainWindow *    lv2g::pMainWindow = nullptr;
QSplashScreen *  lv2g::pSplash = nullptr;
bool lv2g::logonNeeded = false;
bool lv2g::zoneNeeded  = true;
const QString lv2g::sDefaultSplitOrientation= "defaultSplitOrientation";
const QString lv2g::sMaxRows                = "max_rows";
const QString lv2g::sSoundFileAlarm         = "sound_file_alarm";
const QString lv2g::sDialogRows             = "dialog-rows";
const QString lv2g::sHorizontal             = "Horizontal";
const QString lv2g::sVertical               = "Vertical";
const QString lv2g::sNativeMenubar          = "nativeMenubar";

QIcon   lv2g::iconNull;
QIcon   lv2g::iconDefault;

lv2g::lv2g(bool __autoLogin) : lanView()
{
    if (lastError != nullptr) return;
    try {
        // Ubuntu 16.04 Unity-ben  nincs (nem jelenik meg) a natív menü
        nativeMenubar = str2bool(pSet->value(sNativeMenubar).toString(), EX_IGNORE);
        PDEB(VVERBOSE) << VDEB(nativeMenubar) << endl;
        if (!nativeMenubar) {
            PDEB(VVERBOSE) << "Disable native menubar..." << endl;
            QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
        }
        new cMainWindow;    // -> lv2g::pMainWindow
        zoneId = NULL_ID;
        #include "errcodes.h"
        if (dbIsOpen()) {
            subsDbNotif(appName);
            splashMessage(QObject::tr("Logon..."));
            eLogOnResult lor = LR_INITED;
            if (__autoLogin) {
                QString _myDomainName, _myUserName;
                cUser * pMyUser = getOsUser(_myDomainName, _myUserName);
                if (pMyUser != nullptr) {
                    lor = LR_OK;
                    lanView::setUser(pMyUser);
                    zoneId = ALL_PLACE_GROUP_ID;
                }
            }
            if (lor != LR_OK) {
                lor = cLogOn::logOn(zoneNeeded ? &zoneId : nullptr, pMainWindow);
            }
            if (lor != LR_OK) {
                if (lor == LR_INVALID) {
                    EXCEPTION(ELOGON, LR_INVALID, tr("Tul sok hibás bejelentkezési próbálkozás."));
                }
                EXCEPTION(EOK);
            }
            QSqlQuery q = getQuery();
            maxRows = int(cSysParam::getIntegerSysParam(q, sMaxRows, 100));
            bool ok;
            maxRows = pSet->value(sMaxRows, maxRows).toInt(&ok);
            if (!ok) maxRows = int(cSysParam::getIntegerSysParam(q, sMaxRows, 100));
            dialogRows = pSet->value(sDialogRows, 16).toInt();
            soundFileAlarm = pSet->value(sSoundFileAlarm).toString();
        }
        else if (logonNeeded) EXCEPTION(ESQLOPEN, 0, tr("Nincs elérhető adatbázis."));
        defaultSplitOrientation = Qt::Horizontal;
        if (0 == sVertical.compare(pSet->value(sDefaultSplitOrientation).toString(), Qt::CaseInsensitive))
            defaultSplitOrientation = Qt::Vertical;
    } CATCHS(lastError)
    if (dbIsOpen()) {
        splashMessage(QObject::tr("Ellenörzések, és betöltés ..."));
        QSqlQuery q = getQuery();
        if (!zoneNeeded) zoneId = cPlaceGroup().getIdByName(q, _sAll);
        // Enumeration types that do not appear in the database table. Direct control.
        cColEnumType::checkEnum(q, _sDatacharacter, dataCharacter, dataCharacter);
        cColEnumType::checkEnum(q, _sFiltertype, filterType, filterType);
        // In these types of enumeration, the cEnumVals objects must in any case be necessary.
        cEnumVal::enumForce(q, _sDatacharacter);
        cEnumVal::enumForce(q, _sFiltertype);
    }
    // Init icons...
    iconNull.   addFile("://icons/dialog-no.ico",     QSize(), QIcon::Normal, QIcon::On);
    iconNull.   addFile("://icons/dialog-no-off.png", QSize(), QIcon::Normal, QIcon::Off);
    iconDefault.addFile("://icons/go-home-2.ico",     QSize(), QIcon::Normal, QIcon::On);
    iconDefault.addFile("://icons/go-home-2-no.ico",  QSize(), QIcon::Normal, QIcon::Off);
}

lv2g::~lv2g()
{
    ;
}

QMdiArea *lv2g::pMdiArea() {
    return pMainWindow->pMdiArea;
}
/// Zóna váltás dialóg
void lv2g::changeZone(QWidget * par)
{
    QSqlQuery q = getQuery();
    cPlaceGroup pg;
    pg.setId(_sPlaceGroupType, PG_ZONE);
    QStringList items;
    items << _sAll;
    if (!pg.fetch(q, false, pg.mask(_sPlaceGroupType), pg.iTab(_sPlaceGroupName))) EXCEPTION(EDATA,0,tr("Nincsenek zónák !"));
    do {
        QString s = pg.getName();
        if (s == _sAll) continue;
        items << s;

    } while (pg.next(q));
    int current = 0;
    if (pg.fetchById(q, zoneId)) current = items.indexOf(pg.getName());
    if (current < 0) current = 0;
    bool ok;
    QString zoneName = QInputDialog::getItem(par, tr("Zóna váltás"), tr("A zóna neve :"), items, current, false, &ok);
    if (ok) {
        zoneId = pg.getIdByName(q, zoneName);
    }
}

void lv2g::splashMessage(const QString& msg)
{
    if (pSplash != nullptr) {
        pSplash->showMessage(msg, Qt::AlignCenter);
        QApplication::processEvents(QEventLoop::AllEvents);
        // Windows-on megjelenik a splash, Ubuntun nem, csak a két ezutáni sorral, de akkor sem biztos.
        // QThread::sleep(1);  // A doksi szerint ez nem kell, de akkor nem jelenik meg, csak késöbb, amikor már minek
        // QApplication::processEvents(QEventLoop::AllEvents);
    }
}

void lv2g::dbNotif(const QString &name, QSqlDriver::NotificationSource source, const QVariant &payload)
{
    if (ONDB(INFO)) {
        QString src;
        switch (source) {
        case QSqlDriver::SelfSource:    src = _sSelf;       break;
        case QSqlDriver::OtherSource:   src = _sOther;      break;
        case QSqlDriver::UnknownSource:
     /* default:  */                    src = _sUnknown;    break;
        }
        cDebug::cout() << HEAD() << QObject::tr("Database notifycation : %1, source %2, payload :").arg(name).arg(src) << debVariantToString(payload) << endl;
    }
    QString sPayload = payload.toString();
    static const QString mm = "Message :";
    if (sPayload.isNull()) return;
    if (0 == sPayload.compare(_sExit, Qt::CaseInsensitive)) QApplication::exit();
    else if (sPayload.startsWith(mm)) {
        QString msg = sPayload.mid(mm.size());
        QString title = tr("Message by database notify :");
        popupTextWindow(pMainWindow, msg, title);
    }
    else {
        DWAR() << "Invalid payload : " << sPayload << endl;
    }
}

/* ******** */

const QString iconBaseName = "://icons/";

const QStringList& resourceIconList()
{
    static QStringList il;
    if (il.isEmpty()) {
        int n = iconBaseName.size();
        QDirIterator it(iconBaseName);
        while (it.hasNext()) {
            QString s = it.next();
            il << s.mid(n);
        }
        il.sort(Qt::CaseInsensitive);
        il.push_front(_sNul);        // NULL
    }
    return il;
}

int indexOfResourceIcon(const QString& _s)
{
    if (_s.isEmpty()) return 0; // Index of NULL
    QString s;
    if (_s.startsWith(iconBaseName)) s = _s.mid(iconBaseName.size());
    else                             s = _s;
    int ix = resourceIconList().indexOf(s);
    return ix > 0 ? ix : 0;
}

/* ******** */

int defaultDataCharacter(const cRecStaticDescr& __d, int __ix)
{
    __d.chkIndex(__ix);
    if (__ix == __d.idIndex(EX_IGNORE))     return DC_ID;
    if (__ix == __d.nameIndex(EX_IGNORE))   return DC_NAME;
    if (__d.primaryKey()[__ix])             return DC_PRIMARY;
    const cColStaticDescr& cd = __d[__ix];
    switch (cd.fKeyType) {
    case cColStaticDescr::FT_SELF:
        return DC_TREE;
    case cColStaticDescr::FT_PROPERTY:
    case cColStaticDescr::FT_OWNER:
    {
        cRecord *pF = cd.pFRec;
        if (pF == nullptr) pF = new cRecordAny(cd.fKeyTable, cd.fKeySchema);
        bool n = pF->isIndex(pF->nameIndex(EX_IGNORE));
        if (cd.pFRec == nullptr) delete pF;
        if (n) return DC_FNAME;
        if (!cd.fnToName.isEmpty())     return DC_DERIVED;
        return DC_FOREIGN;
    }
        // break;
    default:
        break;
    }
    if (__d.isKey(__ix)) return DC_KEY;
    return DC_DATA;
}

cIntSubObj::cIntSubObj(QMdiArea *par) :QWidget(par)
{
    if (par == nullptr) {
        pSubWindow = nullptr;
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
    static cError *lastError = nullptr;
    try {
        return QApplication::notify(receiver, event);
    }
    catch(no_init_ *) { // Már letiltottuk a cError dobálást
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


_GEX QPolygonF convertPolygon(const tPolygonF& __pol)
{
    QPolygonF   pol;
    foreach (QPointF p, __pol) {
       pol << p;
    }
    return pol;
}

template <class T> const T& enumCacheFunction(const QString& _t, int _e, int _fix,  QMap<QString, QVector<T> >& cache, const T& _def)
{
    if (_t.isEmpty()) { // Clear cache
        cache.clear();
        return _def;
    }
    typename QMap<QString, QVector<T> >::const_iterator it = cache.find(_t);
    int ix = _e + 1;
    if (it == cache.constEnd()) {
        QSqlQuery q = getQuery();
        const cColEnumType *pType = cColEnumType::fetchOrGet(q, _t, EX_IGNORE);
        int n;
        if (pType == nullptr) {        // boolean ??
            if (_t.count(QChar('.')) != 1) EXCEPTION(EFOUND, 0, _t);
            n = 3;
        }
        else {
            n = pType->enumValues.size() +1;
        }
        QVector<T> v(n, _def);
        for (int i = 0; i < n; i++) {
            const cEnumVal& o = cEnumVal::enumVal(_t, i -1);
            if (!o.isNull(_fix)) {
                v[i] = QColor(o.getName(_fix));
            }
        }
        it = cache.insert(_t, v);
    }
    if (isContIx(it.value(), ix)) {
        return it.value().at(ix);
    }
    PDEB(DERROR) << QString("Ivalid enum value %1, on %2").arg(_e).arg(_t) << endl;
    return _def;

}

const QColor& bgColorByEnum(const QString& __t, int e)
{
    _DBGFN() << VDEB(__t) << VDEB(e) << endl;
    static QMap<QString, QVector<QColor> > colorCache;
    static const QColor defCol(Qt::white);
    return enumCacheFunction(__t, e, cEnumVal::ixBgColor(), colorCache, defCol);
}

const QColor& fgColorByEnum(const QString& __t, int e)
{
    static QMap<QString, QVector<QColor> >   colorCache;
    static const QColor defCol(Qt::black);
    return enumCacheFunction(__t, e, cEnumVal::ixFgColor(), colorCache, defCol);
}

const QFont& fontByEnum(const QString& __t, int _e)
{
    static QMap<QString, QMap<int, QFont> >   fontCache;
    static QFont dummy;
    if (__t.isEmpty()) {
        fontCache.clear();
        return dummy;
    }
    // Megkeressük.
    QMap<QString, QMap<int, QFont> >::const_iterator i = fontCache.find(__t);
    if (i != fontCache.constEnd()) {
        const QMap<int, QFont>& m = i.value();
        const QMap<int, QFont>::const_iterator j = m.find(_e);
        if (j != m.constEnd()) {
            return j.value();
        }
    }
    // Ha nem találtuk, akkor létrehozzuk
    QFont& font = fontCache[__t][_e] = QGuiApplication::font();
    const cEnumVal& e = cEnumVal::enumVal(__t, _e);
    QString family = e.getName(cEnumVal::ixFontFamily());
    if (!family.isEmpty()) {
        font.setFamily(family);
    }
#if 0
    if (!e.isNull(cEnumVal::ixFontAttr())) {
        bool f;
        f = e.getBool(cEnumVal::ixFontAttr(), FA_BOOLD);
        if (font.bold()      != f) font.setBold(f);
        f = e.getBool(cEnumVal::ixFontAttr(), FA_ITALIC);
        if (font.italic()    != f) font.setItalic(f);
        f = e.getBool(cEnumVal::ixFontAttr(), FA_UNDERLINE);
        if (font.underline() != f) font.setUnderline(f);
        f = e.getBool(cEnumVal::ixFontAttr(), FA_STRIKEOUT);
        if (font.strikeOut() != f) font.setStrikeOut(f);
    }
#else
    font.setBold(     e.getBool(cEnumVal::ixFontAttr(), FA_BOOLD));
    font.setItalic(   e.getBool(cEnumVal::ixFontAttr(), FA_ITALIC));
    font.setUnderline(e.getBool(cEnumVal::ixFontAttr(), FA_UNDERLINE));
    font.setStrikeOut(e.getBool(cEnumVal::ixFontAttr(), FA_STRIKEOUT));
#endif
    return font;
}

void enumSetD(QWidget *pW, const QString& _t, int id)
{
    const cEnumVal& ev = cEnumVal::enumVal(_t, id);
    return enumSetD(pW, ev, id);
}

void enumSetD(QWidget *pW, const cEnumVal& ev, int _id)
{
    if (ev.isNull()) return;
    int id = _id == NULL_IX ? ev.toInt() : _id;
    enumSetColor(pW, ev.typeName(), id);
    QString family = ev.getName(cEnumVal::ixFontFamily());
    if (!(ev.isNull(cEnumVal::ixFontAttr()) && family.isEmpty())) {
        QFont font = pW->font();
        if (!family.isEmpty()) {
            font.setFamily(family);
        }
        if (!ev.isNull(cEnumVal::ixFontAttr())) {
            bool f;
            f = ev.getBool(cEnumVal::ixFontAttr(), FA_BOOLD);
            if (font.bold()      != f) font.setBold(f);
            f = ev.getBool(cEnumVal::ixFontAttr(), FA_ITALIC);
            if (font.italic()    != f) font.setItalic(f);
            f = ev.getBool(cEnumVal::ixFontAttr(), FA_UNDERLINE);
            if (font.underline() != f) font.setUnderline(f);
            f = ev.getBool(cEnumVal::ixFontAttr(), FA_STRIKEOUT);
            if (font.strikeOut() != f) font.setStrikeOut(f);
        }
        pW->setFont(font);
    }
    QString sToolTip = ev.getText(cEnumVal::LTX_TOOL_TIP);
    if (!sToolTip.isEmpty()) pW->setToolTip(sToolTip);
}

void enumSetD(QWidget *pW, const QString& _t, int id, qlonglong ff, int dcId)
{
    QString sType;
    int     iVal;

    if (ff & ENUM2SET(FF_BG_COLOR)) { sType = _t;              iVal = id; }
    else                            { sType = _sDatacharacter; iVal = dcId; }
    enumSetBgColor(pW, sType, iVal);

    if (ff & ENUM2SET(FF_FG_COLOR)) { sType = _t;              iVal = id; }
    else                            { sType = _sDatacharacter; iVal = dcId; }
    enumSetFgColor(pW, sType, iVal);

    if (ff & ENUM2SET(FF_FONT))     { sType = _t;              iVal = id; }
    else                            { sType = _sDatacharacter; iVal = dcId; }
    const cEnumVal& ev = cEnumVal::enumVal(sType, iVal);
    if (!(ev.isEmpty(cEnumVal::ixFontFamily()) && ev.isEmpty(cEnumVal::ixFontAttr()))) {
        pW->setFont(fontByEnum(sType, iVal));
    }
}

QVariant enumRole(const cEnumVal& ev, int role, int e)
{
    QString s;
    QVariant r;
    switch (role) {
    case Qt::TextColorRole:
        s = ev.getName(cEnumVal::ixFgColor());
        if (!s.isEmpty()) r = QColor(s);
        break;
    case Qt::BackgroundRole:
        s = ev.getName(cEnumVal::ixBgColor());
        if (!s.isEmpty()) r = QColor(s);
        break;
    case Qt::DisplayRole:
        r = ev.getText(cEnumVal::LTX_VIEW_SHORT, ev.getName());
        break;
    case Qt::ToolTipRole:
        s = ev.getText(cEnumVal::LTX_TOOL_TIP);
        if (!s.isEmpty()) r = s;
        break;
    case Qt::FontRole:
        r = fontByEnum(ev.typeName(), e == NULL_IX ? ev.toInt() : e);
        break;
    case Qt::DecorationRole:
        s = ev.getName(_sIcon);
        if (!s.isEmpty()) r = resourceIcon(s);
        break;
    default:
        break;
    }
    return r;
}

QVariant enumRole(const QString& _t, int id, int role, const QString& dData)
{
    switch (role) {
    case Qt::TextColorRole:     return fgColorByEnum(_t, id);
    case Qt::BackgroundRole:    return bgColorByEnum(_t, id);
    case Qt::DisplayRole:       return cEnumVal::viewShort(_t, id, dData);
    case Qt::ToolTipRole:       return cEnumVal::toolTip(_t, id);
    case Qt::FontRole:          return fontByEnum(_t, id);
    default:                    return QVariant();
    }
}

QVariant dcRole(int id, int role)
{
    switch (role) {
    case Qt::TextColorRole:     return dcFgColor(id);
    case Qt::BackgroundRole:    return dcBgColor(id);
    case Qt::DisplayRole:       return dcViewShort(id);
    case Qt::ToolTipRole:       return cEnumVal::toolTip(_sDatacharacter, id);
    case Qt::FontRole:          return fontByEnum(_sDatacharacter, id);
    default:                    return QVariant();
    }
}

QString condAddJoker(const QString& pat)
{
    static const QChar joker('%');
    static const QString re = "[%\?]";
    if (!pat.contains(QRegExp(re))) return joker + pat + joker;
    return pat;
}


void clearWidgets(QLayout * layout) {
   if (! layout)
      return;
   while (auto item = layout->takeAt(0)) {
      delete item;
   }
}
