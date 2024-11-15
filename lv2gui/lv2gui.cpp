#include "lv2gui.h"
#include "cerrormessagebox.h"

#define VERSION_MAJOR   1
#define VERSION_MINOR   00
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
    lanView::appHelp += QObject::tr("-a|--app-name              APP name.\n");
    lanView::appHelp += QObject::tr("-s|--setup                 Setup.\n");
    lanView::appHelp += QObject::tr("-o|--aoto-open <menu name> Menüpont aktíválása\n");
    lanView::appHelp += QObject::tr("-f|--fullscreen [<#scrin>] teljes képernyő\n");
    lanView::appHelp += QObject::tr("-m|--maximize              Maximális ablak.\n");
    lanView::appHelp += QObject::tr("-P|--query-paths           Query paths and exit.\n");
    lanView::appHelp += QObject::tr("-A|--auto-login            Auto login.\n");
}

static void printLocations();

int main(int argc, char * argv[])
{
    cError * pe = nullptr;
    cLv2GQApp app(argc, argv);
    QStringList slAutoOpen;
    QStringList arguments = app.arguments();

    SETAPP();
    lanView::gui = true;
    lv2g::pSplash = new QSplashScreen(QPixmap("://splash.png"));
    lv2g::pSplash->show();
    app.processEvents();
    lv2g::splashMessage(QObject::tr("Program indítás..."));
#if defined(Q_OS_LINUX)
    // Ubuntu alatt valamiért nem midíg jeleni meg a splash, csak később, de akkor már minek
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();
#else
    app.processEvents();
#endif
    lanView::snmpNeeded = false;
    if (0 <= findArg(QChar('P'),QString("query-paths"), arguments)) {
        printLocations();
    }
    if (0 <= findArg(QChar('s'),QString("setup"), arguments)) {
        lv2Gui::_setup = true;
        lanView::sqlNeeded = SN_NO_SQL;
    }
    int i;
    if (0 <= (i = findArg(QChar('a'),QString("app-name"), arguments))
     && (i + 1) < arguments.count()) {
        lanView::appName = arguments[i + 1];
    }
    if (0 <= (i = findArg(QChar('o'),QString("auto-open"), arguments))
     && (i + 1) < arguments.count()) {
        static const QRegularExpression re(",\\s*");
        slAutoOpen << arguments[i + 1].split(re);
    }
    if (0 <= (i = findArg(QChar('f'),QString("fullscreen"), arguments))) {
        lv2Gui::_screen = lv2Gui::FULLSCREEN;
        if ((i + 1) < arguments.count()) {
            QString sn = arguments[i + 1];
            bool ok;
            int n = sn.toInt(&ok);
            if (ok) lv2Gui::_nScreen = n;
        }
    }
    if (0 <= findArg(QChar('m'),QString("maximize"), arguments)) {
        lv2Gui::_screen = lv2Gui::MAXIMIZE;
    }
    if (0 <= findArg(QChar('A'),QString("auto-login"), arguments)) {
        lv2Gui::_autoLogin = true;
    }

    lanView::sqlNeeded  = lv2Gui::_setup ? SN_NO_SQL : SN_SQL_TRY;

    if (lanView::sqlNeeded != SN_NO_SQL) {
        lv2g::splashMessage(QObject::tr("Az adatbázis megnyitása ..."));
    }
    lv2Gui   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        pe = mo.lastError;
    }
    else {
        try {
            switch (lv2Gui::_screen) {
            case lv2Gui::NORMAL:
                mo.pMainWindow->show();
                break;
            case lv2Gui::FULLSCREEN:
                /*
                mo.pMainWindow->showFullScreen();
                mo.pMainWindow->adjustSize();
                QDesktopWidget *pDesktop = QApplication::desktop();
                QWidget *parent_screen = pDesktop->screen(lv2Gui::_nScreen -1);
                QRect geom = parent_screen->geometry();
                mo.pMainWindow->setGeometry(geom);
                mo.pMainWindow->adjustSize();
                break;*/
            case lv2Gui::MAXIMIZE:
                mo.pMainWindow->showMaximized();
                mo.pMainWindow->adjustSize();
                break;
            }
            if (mo.pSplash != nullptr) {
                mo.pSplash->finish(mo.pMainWindow);
            }
            foreach (QString a, slAutoOpen) {
                QMap<QString, QAction *>::iterator i = cMenuAction::actionsMap.find(a);
                if (i != cMenuAction::actionsMap.end()) {
                    emit (*i)->triggered();
                }
            }
            app.exec();
        } CATCHS(pe);
    }
    if (pe != nullptr) {
        if (pe->mErrorCode == eError::EOK) {
            PDEB(INFO) << "**** EOK exit ****" << endl;
        }
        else {
            PDEB(INFO) << "**** ERROR ****" << endl;
            cErrorMessageBox::messageBox(pe);
        }
        exit(pe->mErrorCode);
    }
    PDEB(INFO) << "**** OK ****" << endl;
    exit(0);
}

bool lv2Gui::_setup = false;
lv2Gui::eScreen lv2Gui::_screen = lv2Gui::NORMAL;
bool lv2Gui::_autoLogin = false;
int  lv2Gui::_nScreen = 1;
lv2Gui::lv2Gui() : lv2g(lv2Gui::_autoLogin)
{
    DBGFN();
    if (lastError == nullptr) {
        try {
            if (pMainWindow == nullptr) EXCEPTION(EPROGFAIL);
            if (nonFatal != nullptr) {
                cErrorMessageBox::messageBox(nonFatal);
                pDelete(nonFatal);
                _setup = true;
            }
            pMainWindow->init(_setup);
        } CATCHS(lastError)
    }
    DBGFNL();
}

lv2Gui::~lv2Gui()
{
    DBGFN();
    if (pMainWindow != nullptr) delete pMainWindow;
    DBGFNL();
}

#include <QLibraryInfo>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define path location
#endif

static void printLocations()
{
    QDialog *d = new QDialog();
    QVBoxLayout *l = new QVBoxLayout();
    d->setLayout(l);
    QPlainTextEdit *t = new QPlainTextEdit();
    l->addWidget(t);
    QString txt;
    txt +=   "Default prefix                           : " + QLibraryInfo::path(QLibraryInfo::PrefixPath);
    txt += "\nDocumentation upon install               : " + QLibraryInfo::path(QLibraryInfo::DocumentationPath);
    txt += "\nAll headers                              : " + QLibraryInfo::path(QLibraryInfo::HeadersPath);
    txt += "\nInstalled libraries                      : " + QLibraryInfo::path(QLibraryInfo::LibrariesPath);
    txt += "\nInstalled exec. requ. by lib. at runtime : " + QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath);
    txt += "\nInstalled Qt binaries (tools and apps)   : " + QLibraryInfo::path(QLibraryInfo::BinariesPath);
    txt += "\nInstalled Qt plugins                     : " + QLibraryInfo::path(QLibraryInfo::PluginsPath);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    txt += "\nInstalled QML ext. to import (QML 1.x)   : " + QLibraryInfo::path(QLibraryInfo::ImportsPath);
#endif
    txt += "\nInstalled QML ext. to import (QML 2.x)   : " + QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath);
    txt += "\nGeneral architecture-dependent Qt data   : " + QLibraryInfo::path(QLibraryInfo::ArchDataPath);
    txt += "\nGeneral architecture-independent Qt data : " + QLibraryInfo::path(QLibraryInfo::DataPath);
    txt += "\nTranslation information for Qt strings   : " + QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    txt += "\nExamples upon install                    : " + QLibraryInfo::path(QLibraryInfo::ExamplesPath);
    txt += "\nInstalled Qt testcases                   : " + QLibraryInfo::path(QLibraryInfo::TestsPath);
    txt += "\nQt settings. Not applicable on Windows   : " + QLibraryInfo::path(QLibraryInfo::SettingsPath);
    t->setPlainText(txt);
    d->exec();
    delete d;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#undef path
#endif
