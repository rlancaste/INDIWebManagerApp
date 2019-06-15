/*  INDIWebManagerApp
    Copyright (C) 2019 Robert Lancaster <rlancaste@gmail.com>

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopServices>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSysInfo>
#include <QNetworkInterface>
#include <QMessageBox>
#include "opsconfiguration.h"
#include "opsmanager.h"
#include <QSizePolicy>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <KConfigDialog>
#include <KLocalizedString>
#include <QHostInfo>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    setWindowTitle(i18n("INDI Web Manager App"));
    ui->setupUi(this);

    this->setWindowIcon(QIcon(":/media/images/indi_logo.png"));

    //This should make the icons look good no matter what the theme or mode of the system is.
    int backgroundBrightness = this->palette().color(QWidget::backgroundRole()).lightness();
    if(backgroundBrightness < 100)
    {
        ui->stopWebManager->setIcon(QIcon(":/media/icons/media-playback-stop-dark.svg"));
        ui->restartWebManager->setIcon(QIcon(":/media/icons/media-playback-start-dark.svg"));
        ui->configureWebManager->setIcon(QIcon(":/media/icons/configure-dark.svg"));
    }
    else {
        ui->stopWebManager->setIcon(QIcon(":/media/icons/media-playback-stop.svg"));
        ui->restartWebManager->setIcon(QIcon(":/media/icons/media-playback-start.svg"));
        ui->configureWebManager->setIcon(QIcon(":/media/icons/configure.svg"));
    }

    //This makes the buttons look nice on OS X.
    ui->stopWebManager->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->restartWebManager->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->configureWebManager->setAttribute(Qt::WA_LayoutUsesWidgetRect);

    //This will hide the log and capture the window size to make the later algorithm for showing/hiding work properly.
    ui->logViewer->setVisible(false);
    this->adjustSize();
    minimumWindowSize = this->window()->size();

    //These set up the actions of the controls in the Main Window.
    connect(ui->configureWebManager, &QPushButton::clicked, this, &MainWindow::showPreferences);
    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::showPreferences);
    connect(ui->restartWebManager, &QPushButton::clicked, this, &MainWindow::startWebManager);
    connect(ui->stopWebManager, &QPushButton::clicked, this, &MainWindow::stopWebManager);
    connect(ui->showLog, &QPushButton::toggled, this, &MainWindow::setLogVisible);

    //This sets up the Web Manager to launch in your favorite browser using either the host name or IP Address
    connect(ui->openWebManager, &QPushButton::clicked, this, [this]()
    {
        QUrl url(getWebManagerURL());
        QDesktopServices::openUrl(url);
    });

    //This sets up the INDI Logo to look nice in the app.
    QPixmap pix(":/media/images/indi_logo.png");
    int w = 100;
    int h = 100;
    ui->INDILogo->setPixmap(pix.scaled (w,h,Qt::KeepAspectRatio));

    //This sets up a timer to check the status of the INDI Server at 1 second intervals when it is running.
    serverMonitor.setInterval(1000);
    connect(&serverMonitor, &QTimer::timeout, this, &MainWindow::checkINDIServerStatus);

    //This shows the main window.  For some reason if you don't do this before the check on the next line, it changes the window layout.
    this->show();

    //This will initially set the status displays to false.
    displayServerStatusOnline(false);
    displayManagerStatusOnline(false);

    //This will check if another Web Manager is running on this computer, and kill it if desired.
    if(isWebManagerOnline())
    {
        if(QMessageBox::question(nullptr, "Message", i18n("Alert, an INDI Webmanager is already running on this computer.  Do you want to quit it?")) == QMessageBox::Yes)
        {
            QProcess killWebManager;
            QStringList killParams;
            killParams << "Python" << "indi-web" << "indiserver";
            killWebManager.start("/usr/bin/killall", killParams);
            killWebManager.waitForFinished(300);
        }
    }

    appendLogEntry( "Welcome to INDI Web Manager App " + QString(INDIWebManagerApp_VERSION));
    appendLogEntry( "Build: " + QString(INDI_WEB_MANAGER_APP_BUILD_TS));
    appendLogEntry( "OS: " + QSysInfo::productType() + " " + QSysInfo::productVersion());
    appendLogEntry( "API: " + QSysInfo::buildAbi());
    appendLogEntry( "Arch: " + QSysInfo::currentCpuArchitecture());
    appendLogEntry( "Kernel Type: " + QSysInfo::kernelType());
    appendLogEntry( "Kernel Version: " + QSysInfo::kernelVersion());
    appendLogEntry( "Qt Version: " + QString(QT_VERSION_STR));

     //This will finish setting up the Web Manager and launch it if the requirements are installed and auto launch is selected.
    if(pythonInstalled() && indiWebInstalled())
    {
        updateSettings();
        if(Options::autoLaunchManager())
            startWebManager();
    }
    else
    {
        QMessageBox::information(nullptr, "message", i18n("Please configure the INDI Web Manager.  The Preferences Dialog will now open. \nBoth Python and indiweb need to be installed and configured to use this program."));
        showPreferences();
    }
}

//This deletes the ui variable on shutdown
MainWindow::~MainWindow()
{
    delete ui;
}

/*
 * This method is used to get the default values for the INDI Web Server settings on different systems.
 * It also is used to get automatically generated values.
 */
QString MainWindow::getDefault(QString option)
{
    QString snap = QProcessEnvironment::systemEnvironment().value("SNAP");
    QString flat = QProcessEnvironment::systemEnvironment().value("FLATPAK_DEST");

    //This is the folder that python is installed or symlinked to.
    if (option == "PythonExecFolder")
    {
    #ifdef Q_OS_OSX
        //Note this is the Path where python3 gets symlinked by homebrew.
        return "/usr/local/opt/python/libexec/bin";
    #endif
        if (flat.isEmpty() == false)
            return flat + "/bin/";
        else
            return snap + "/usr/bin/";
    }

    //This is the Path to the indiweb executable.  It is where indi-web is installed, typically in the Python Base User Directory.
    else if (option == "indiwebPath")
    {
    #ifdef Q_OS_OSX
        return "/usr/local/bin/indi-web";
    #endif
        return QDir::homePath() + "/.local/bin/indi-web";
    }

    //This is the Path to the GSC data folder.  It includes gsc at the end.
    else if (option == "GSCPath")
    {
    #ifdef Q_OS_OSX
        return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString(), QStandardPaths::LocateDirectory) + "INDIWebManagerApp/gsc";
    #endif
        if (flat.isEmpty() == false)
            return flat + "/share/GSC";
        else
            return snap + "/usr/share/GSC";
    }

    //This is the path to the INDI Prefix.  This is the App bundle loction on OS X
    if (option == "INDIPrefix")
    {
    #ifdef Q_OS_OSX
        QString appPath = QCoreApplication::applicationDirPath();
        return QDir(appPath + "/../../").absolutePath();
    #endif
        return "";
    }

    //This is the path to the folder indiserver is present in, not the indiserver itself.
    if (option == "INDIServerPath")
    {
    #ifdef Q_OS_OSX
        QString appPath = QCoreApplication::applicationDirPath();
        return QDir(appPath + "/indi").absolutePath();
    #endif
        if (flat.isEmpty() == false)
            return flat + "/bin/";
        else
            return snap + "/usr/bin/";
    }

    //This is the folder of the indi drivers and xml files.
    else if (option == "INDIDriversPath")
    {
    #ifdef Q_OS_OSX
        QString appPath = QCoreApplication::applicationDirPath();
        return QDir(appPath + "/../Resources/DriverSupport").absolutePath();
    #elif defined(Q_OS_LINUX)
        if (flat.isEmpty() == false)
            return flat + "/share/indi";
        else
            return snap + "/usr/share/indi";
    #else
        return QStandardPaths::locate(QStandardPaths::GenericDataLocation, "indi", QStandardPaths::LocateDirectory);
    #endif
    }

    //This is the default path to the INDI config files.
    else if (option == "INDIConfigPath")
        return QDir::homePath() + "/.indi";

    //This is the path to the folder indiserver is present in, not the indiserver itself.
    if (option == "GPhotoIOLIBS")
    {
    #ifdef Q_OS_OSX
        QString appPath = QCoreApplication::applicationDirPath();
        return QDir(appPath + "/../Resources/DriverSupport/gphoto/IOLIBS").absolutePath();
    #endif
        return "";
    }

    //This is the path to the folder indiserver is present in, not the indiserver itself.
    if (option == "GPhotoCAMLIBS")
    {
    #ifdef Q_OS_OSX
        QString appPath = QCoreApplication::applicationDirPath();
        return QDir(appPath + "/../Resources/DriverSupport/gphoto/CAMLIBS").absolutePath();
    #endif
        return "";
    }

    //This is the name of the computer on the local network.
    else if (option == "ComputerHostName")
        return QHostInfo::localHostName();

    //This is the IP of the computer on the local network.
    else if (option == "ComputerIPAddress")
    {
        const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
        for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
                return address.toString();
            }
        return localhost.toString();
    }
    //This is the name of the computer on the local network.
    else if (option == "ManagerPortNumber")
        return "8624";

    //This is the name of the computer on the local network.
    else if (option == "LogFilePath")
        return QDir::homePath() + "/.indi/webmanagerlog.txt";

    return QString();
}

/*
 * This gets the URL to the INDI Web Manager using the users desired method.
 */
QString MainWindow::getWebManagerURL()
{
    if(Options::managerOpeningMethod()==0)
        return QString("http://%1:%2").arg(Options::computerHostName()).arg(Options::managerPortNumber());
    else
        return QString("http://%1:%2").arg(Options::computerIPAddress()).arg(Options::managerPortNumber());
}

/*
 * This gets the URL to the INDI Server using the users desired method.
 */
QString MainWindow::getINDIServerURL()
{
    if(Options::managerOpeningMethod()==0)
        return QString("%1:%2").arg(Options::computerHostName()).arg("7624");
    else
        return QString("%1:%2").arg(Options::computerIPAddress()).arg("7624");
}

/*
 * This method detects whether python3 is installed.
 */
bool MainWindow::pythonInstalled()
{

    bool exists = QFileInfo(getDefault("PythonExecFolder") + "/python").exists();
    if(!exists)
        QMessageBox::information(nullptr, "message", i18n("Python is not installed at the specified path."));
    return exists;
}

/*
 * This method detects whether indi-web is installed.
 */
bool MainWindow::indiWebInstalled()
{
    QProcess testindiweb;
    QStringList argsList;
    argsList << "list";
    testindiweb.start(getDefault("PythonExecFolder") +"/pip", argsList);
    testindiweb.waitForFinished();
    QString listPip(testindiweb.readAllStandardOutput());

    testindiweb.start(getDefault("PythonExecFolder") +"/pip3", argsList);
    testindiweb.waitForFinished();
    QString listPip3(testindiweb.readAllStandardOutput());

    bool exists = listPip.contains("indiweb", Qt::CaseInsensitive) || listPip3.contains("indiweb", Qt::CaseInsensitive);
    if(!exists)
        QMessageBox::information(nullptr, "message", i18n("indiweb is not installed."));
    return exists;
}

/*
 *This method brings up the Preferences Dialog with two pages of options.
 */
void MainWindow::showPreferences()
{
    KConfigDialog *preferencesDialog = new KConfigDialog(this, "Preferences Dialog", Options::self());
    connect(preferencesDialog->button(QDialogButtonBox::Apply), SIGNAL(clicked()), SLOT(updateSettings()));
    connect(preferencesDialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), SLOT(updateSettings()));

    OpsManager *config1 = new OpsManager(this);
    KPageWidgetItem *page1 = preferencesDialog->addPage(config1, i18n("Web Manager Options"));
    page1->setIcon(QIcon(":/media/images/indi_logo.png"));


    OpsConfiguration *config2 = new OpsConfiguration(this);
    KPageWidgetItem *page2 = preferencesDialog->addPage(config2, i18n("Configuration Options"));
    //This should make the icon look good no matter what the theme or mode of the system is.
    int backgroundBrightness = this->palette().color(QWidget::backgroundRole()).lightness();
    if(backgroundBrightness < 100)
        page2->setIcon(QIcon(":/media/icons/configure-dark.svg"));
    else
        page2->setIcon(QIcon(":/media/images/configure.svg"));
    preferencesDialog->show();
}

/*
 * This method should update all the environment variables and the displayed settings in the main window.
 * It should be run after the configuration settings have been changed and any time the user presses the refresh button.
 */
void MainWindow::updateSettings()
{
    bool webManagerWasRunning = webManagerRunning;
    if(webManagerRunning)
        stopWebManager();

    if(Options::computerHostNameAuto())
        Options::setComputerHostName(getDefault("ComputerHostName"));
    if(Options::computerIPAddressAuto())
        Options::setComputerIPAddress(getDefault("ComputerIPAddress"));

    ui->hostDisplay->setText(Options::computerHostName());
    ui->ipDisplay->setText(Options::computerIPAddress());
    ui->displayWebManagerPath->setText(getWebManagerURL());
    ui->displayINDIServerPath->setText(getINDIServerURL());

    configureEnvironmentVariables();

    if(webManagerWasRunning)
        startWebManager();
}

/*
 * This method configures all the environment variables.  It runs in conjuction with the above method.
 * Some of the variables dynamically change with the loation of the App bundle on OS X.
 * All are listed just to make sure none get forgotten.
 */
void MainWindow::configureEnvironmentVariables()
{  
    //Note the Python Exec Variable does not dynamically change.
    //Note the GSC Variable does not dynamically change.
    if(Options::iNDIPrefixDefault())
        Options::setINDIServerPath(getDefault("INDIPrefix"));
    if(Options::iNDIServerDefault())
        Options::setINDIServerPath(getDefault("INDIServerPath"));
    if(Options::iNDIDriversDefault())
        Options::setINDIDriversPath(getDefault("INDIDriversPath"));
    if(Options::gPhotoIOLIBSDefault())
        Options::setGPhotoIOLIBS(getDefault("GPhotoIOLIBS"));
    if(Options::gPhotoCAMLIBSDefault())
        Options::setGPhotoCAMLIBS(getDefault("GPhotoCAMLIBS"));

    QString newPATH = Options::pythonExecFolder() + ":" + Options::iNDIServerPath() + ':' + Options::iNDIDriversPath() + ":usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin";
    insertEnvironmentVariable("PATH", newPATH);

    #ifdef Q_OS_OSX
        insertEnvironmentPath("INDIPREFIX", Options::iNDIPrefix());
        insertEnvironmentPath("IOLIBS", Options::gPhotoIOLIBS());
        insertEnvironmentPath("CAMLIBS", Options::gPhotoCAMLIBS());
    #endif
    insertEnvironmentPath("GSCDAT", Options::gSCPath());
}

/*
 * This Method inserts an environment variable and prints the export command to the log so the user can see what was done.
 * The environment variables should not have escaped characters, but the export command should.
 * This is so that the user can copy the command to the terminal if desired.
 */
void MainWindow::insertEnvironmentVariable(QString variable, QString value)
{
    qputenv(variable.toUtf8().data(), value.toLatin1());
    appendLogEntry("export " + variable + "=" + value.replace(" ", "\\ "));
}

/*
 * This method works just like the insert environment variable method, exept it checks that the path actually exists before inserting it.
 * If the path does not exist, it prints an error message about it.
 */
void MainWindow::insertEnvironmentPath(QString variable, QString filePath)
{
    if (QFileInfo::exists(filePath))
        insertEnvironmentVariable(variable, filePath);
    else
        appendLogEntry(i18n("The Path for the %1 environment variable does not exist on this system.  Please check your settings.  The stated path was: ").arg(variable) + filePath);
}

/*
 * This method starts the INDI Web Manager
 */
void MainWindow::startWebManager()
{
    if(webManagerRunning)
    {
        stopWebManager();
        webManager.clear();
    }
    webManager = new QProcess(this);

    appendLogEntry(i18n("INDI Web Manager Started."));
    connect(webManager, SIGNAL(finished(int)), this, SLOT(managerClosed(int)));
    webManager->setProcessChannelMode(QProcess::MergedChannels);
    connect(webManager, SIGNAL(readyReadStandardOutput()), this, SLOT(appendLogEntry()));
    QStringList processArguments;
    if(Options::enableVerboseMode())
         processArguments << "--verbose";
    //This one is not optional
    processArguments << "--xmldir" << QDir(Options::iNDIDriversPath()).absolutePath();
    if(Options::enableWebManagerLogFile())
         processArguments << "--logfile" << Options::logFilePath();
    if(!Options::managerPortNumberDefault())
        processArguments << "--port" << Options::managerPortNumber();
    if(!Options::iNDIConfigDefault())
        processArguments << "--conf" << Options::iNDIConfigPath();
    appendLogEntry(Options::indiwebPath() + " " + processArguments.join(" "));
    webManager->start(Options::indiwebPath(), processArguments);
    displayManagerStatusOnline(true);

    serverMonitor.start();
    webManagerRunning = true;

}

/*
 * This method shuts down the INDI Web Manager
 */
void MainWindow::stopWebManager()
{
    disconnect(webManager, SIGNAL(finished(int)), this, SLOT(managerClosed(int)));
    webManagerRunning  = false;
    QProcess killINDIServer;
    QStringList killParams;
    killParams<<"indiserver";
    killINDIServer.start("/usr/bin/killall", killParams);
    killINDIServer.waitForFinished(300);
    webManager->kill();
    appendLogEntry(i18n("INDI Web Manager Shut down successfully."));
    webManager->waitForFinished(300);
    updateDisplaysforShutDown();
}

/*
 * This method uses the method below it to put output from the running web manager into the log in the Main Window and console.
 */
void MainWindow::appendLogEntry()
{
    appendLogEntry(webManager->readAll().trimmed());
}

/*
 * This method puts information into the log in the Main Window.  It also outputs the same message to the console.
 * Note: Should this also append to the web manager log file.  Is this possible?
 */
void MainWindow::appendLogEntry(QString text)
{
    QString entry = QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss:  ") + text;
    ui->webManagerLog->appendPlainText(entry);
    qDebug()<<entry;
}

/*
 * This method is connnected to the running INDI Web Manager.
 * It will run if for some reason the Web Manager gets terminated prematurely.
 * (But not by clicking the stop button or closing the program)
 * It updates the displays in the Main Window and prints a message to the log about the shut down.
 */
void MainWindow::managerClosed(int result)
{
    webManagerRunning = false;
    if(result==0)
        appendLogEntry(i18n("INDI Web Manager Shut down successfully."));
    else
    {
        appendLogEntry(i18n("INDI Web Manager Shut down with error."));
    }
    updateDisplaysforShutDown();
}

/*
 * This updates the displays in the Main Window when the Web Mabager shuts down and stops the monitor.
 */
void MainWindow::updateDisplaysforShutDown()
{
    displayManagerStatusOnline(false);
    displayServerStatusOnline(false);
    ui->activeProfileDisplay->clear();
    ui->driversDisplay->clear();
    serverMonitor.stop();
}

/*
 * This detects if the program is being closed so that we don't leave the Web Manger and INDI Server running by accident.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(webManagerRunning)
        stopWebManager();
    webManager->waitForFinished(300);
    event->accept();
}

/*
 * This method updates the status of the Web Manager in the Main Window.
 * It does not run automatically. It only gets changed when this program changes the Web Manager Status.
 * It does not actually check to see if it is accessible.  This is not optimal, but if there is any shut down of the manager, this should get updated.
 * It cannot use the same scheme I am using for checking the status of the INDI Server since if the web Manager is not running, the monitor is not either.
 */
void MainWindow::displayManagerStatusOnline(bool online)
{
    if(online)
    {
        ui->statusDisplay->setText("Online");
        ui->statusDisplay->setStyleSheet("QLineEdit {background-color: green;}");
    }
    else
    {
        ui->statusDisplay->setText("Offline");
        ui->statusDisplay->setStyleSheet("QLineEdit {background-color: red;}");
    }
}

/*
 * This method shows and hides the log in the Main Window.
 */
void MainWindow::setLogVisible(bool visible)
{
    ui->logViewer->setVisible(visible);
    if(visible)
        this->adjustSize();
    else
        this->setFixedSize(minimumWindowSize);

    this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    this->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);


}


//*******************  The Following methods are for communicating with and updating the status of a runnning INDI Server.


/*
 * This method checks on the Status of the INDI Server, gets the active profile, and any running drivers.
 * It then updates the status in the main window.
 * It should be running once a second when the Web Manager is active and running.
 */
void MainWindow::checkINDIServerStatus()
{
    QString activeProfile = "";
    bool INDIServerOnline = isINDIServerOnline(activeProfile);
    displayServerStatusOnline(INDIServerOnline);
    ui->activeProfileDisplay->setText(activeProfile);
    QString webManagerDrivers="";
    getRunningDrivers(webManagerDrivers);
    ui->driversDisplay->setPlainText(webManagerDrivers);
}

/*
 * This method updates the status of the INDI Server in the Main Window.
 */
void MainWindow::displayServerStatusOnline(bool online)
{
    if(online)
    {
        ui->serverStatusDisplay->setText("Online");
        ui->serverStatusDisplay->setStyleSheet("QLineEdit {background-color: green;}");
    }
    else
    {
        ui->serverStatusDisplay->setText("Offline");
        ui->serverStatusDisplay->setStyleSheet("QLineEdit {background-color: red;}");
    }
}

/*
 * This method is intended to figure out if ANY Web Manager is running at the default port to give the user an option to stop it.
 * It checks both the user configured url and the default url.
 */
bool MainWindow::isWebManagerOnline()
{
    QUrl url1(QString("http://localhost:8624/api/server/status"));
    QJsonDocument json;
    bool test1 = getWebManagerResponse( url1, &json);

    QUrl url2(QString(getWebManagerURL() + "/api/server/status"));
    QJsonDocument json2;
    bool test2 = getWebManagerResponse( url2, &json2);

    if(test1 || test2)
        return true;
    else
        return false;
}

/*
 * This method determines if an INDIServer hosted by this program and accessed using the desired method is online.
 */
bool MainWindow::isINDIServerOnline(QString &activeProfile)
{
    QUrl url(QString(getWebManagerURL() + "/api/server/status"));

    QJsonDocument json;
    if (getWebManagerResponse( url, &json))
    {
        QJsonArray array = json.array();
        if (array.isEmpty())
            return false;

        QJsonObject jsonObj = array.first().toObject();
        QString status = jsonObj["status"].toString();
        if(status=="True")
        {
            activeProfile = jsonObj["active_profile"].toString();
            return true;
        }
    }
    return false;
}

/*
 * This method determines what drivers are active on an INDIServer hosted by this program and accessed using the desired method.
 */
bool MainWindow::getRunningDrivers(QString &webManagerDrivers)
{
    QUrl url(QString(getWebManagerURL() + "/api/server/drivers"));

    QJsonDocument json;
    if (getWebManagerResponse( url, &json))
    {
        QJsonArray array = json.array();

        if (array.isEmpty())
            return false;

        QStringList webManagerDriversList;
        for (auto value : array)
        {
            QJsonObject driver = value.toObject();
            webManagerDriversList << driver["name"].toString();
        }
        webManagerDrivers=webManagerDriversList.join("\n");
        return true;
    }
    return false;
}

/*
 * This method is a how the above methods communicate with the web manager and get a response.
 */

bool MainWindow::getWebManagerResponse(const QUrl &url, QJsonDocument *reply)
{
    QNetworkAccessManager manager;
    QNetworkReply *response = nullptr;
    QNetworkRequest request;

    request.setUrl(url);
    response = manager.get(request);

    // Wait synchronously
    QEventLoop event;
    QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();

    if (response->error() == QNetworkReply::NoError)
    {
        if (reply)
        {
            QJsonParseError parseError;
            *reply = QJsonDocument::fromJson(response->readAll(), &parseError);

            if (parseError.error != QJsonParseError::NoError)
            {
                if(webManagerRunning) //This way it doesn't print errors when checking to see if the Web Manager is running at all
                    appendLogEntry(i18n("INDI: JSon error during parsing ") + parseError.errorString());
                return false;
            }
        }
        return true;
    }
    else
    {
        if(webManagerRunning)  //This way it doesn't print errors when checking to see if the Web Manager is running at all
            appendLogEntry(i18n("INDI: Error communicating with INDI Web Manager: ") + response->errorString());
        return false;
    }
}




