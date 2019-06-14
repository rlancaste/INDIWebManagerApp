/*  INDIWebManagerApp
    Copyright (C) 2019 Robert Lancaster <rlancaste@gmail.com>

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/

#include "opsconfiguration.h"
#include "ui_opsconfiguration.h"
#include <KConfigDialog>
#include "Options.h"
#include <QProcess>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QNetworkReply>
#include <QDialogButtonBox>


OpsConfiguration::OpsConfiguration(MainWindow *parent)
{
    this->parent = parent;
    ui = new Ui::OpsConfiguration;
    ui->setupUi(this);

    updatePythonInstallationStatus();
    updateGSCInstallationStatus();
    connect(ui->installRequirements, &QAbstractButton::clicked,this, &OpsConfiguration::slotInstallRequirements);
    connect(ui->installGSC, SIGNAL(clicked()), this, SLOT(slotInstallGSC()));
    connect(ui->kcfg_GSCPath, &QLineEdit::textChanged, this, &OpsConfiguration::updateGSCInstallationStatus);
    ui->gscInstallCancel->setVisible(false);
    ui->downloadProgress->setVisible(false);

    //Note that all the checkboxes here are "default" ones and should run the update method when changed.
    QList<QCheckBox *> qCheckBoxes = findChildren<QCheckBox *>();
    for (auto &checkbox : qCheckBoxes)
        connect(checkbox, &QCheckBox::clicked, this, &OpsConfiguration::updateFromCheckBoxes);

    //This waits a moment for the kconfig to load the options, then sets the Line Edits to read only appropriagely
    QTimer::singleShot(100, this,  &OpsConfiguration::updateFromCheckBoxes);
}

OpsConfiguration::~OpsConfiguration()
{
    delete ui;
}

/*
 * This method enables the functionality of the default buttons.
 * The line edits are disabled as long as the default button is checked.
 * If the user unchecks the button, it changes to the stored value
 * If the user checks the button, it changes to the default value.
 */
void OpsConfiguration::updateFromCheckBoxes()
{
    ui->kcfg_PythonExecFolder->setReadOnly(ui->kcfg_PythonExecFolderDefault->isChecked());
    ui->kcfg_GSCPath->setReadOnly(ui->kcfg_GSCPathDefault->isChecked());
    ui->kcfg_INDIPrefix->setReadOnly(ui->kcfg_INDIPrefixDefault->isChecked());
    ui->kcfg_INDIServerPath->setReadOnly(ui->kcfg_INDIServerDefault->isChecked());
    ui->kcfg_INDIDriversPath->setReadOnly(ui->kcfg_INDIDriversDefault->isChecked());
    ui->kcfg_INDIConfigPath->setReadOnly(ui->kcfg_INDIConfigPathDefault->isChecked());
    ui->kcfg_GPhotoIOLIBS->setReadOnly(ui->kcfg_GPhotoIOLIBSDefault->isChecked());
    ui->kcfg_GPhotoCAMLIBS->setReadOnly(ui->kcfg_GPhotoCAMLIBSDefault->isChecked());

    if(ui->kcfg_PythonExecFolderDefault->isChecked())
         ui->kcfg_PythonExecFolder->setText(parent->getDefault("PythonExecFolder"));
    else
         ui->kcfg_PythonExecFolder->setText(Options::pythonExecFolder());

    if(ui->kcfg_GSCPathDefault->isChecked())
         ui->kcfg_GSCPath->setText(parent->getDefault("GSCPath"));
    else
         ui->kcfg_GSCPath->setText(Options::gSCPath());

    if(ui->kcfg_INDIPrefixDefault->isChecked())
         ui->kcfg_INDIPrefix->setText(parent->getDefault("INDIPrefix"));
    else
         ui->kcfg_INDIPrefix->setText(Options::iNDIPrefix());

    if(ui->kcfg_INDIServerDefault->isChecked())
         ui->kcfg_INDIServerPath->setText(parent->getDefault("INDIServerPath"));
    else
         ui->kcfg_INDIServerPath->setText(Options::iNDIServerPath());

    if(ui->kcfg_INDIDriversDefault->isChecked())
         ui->kcfg_INDIServerPath->setText(parent->getDefault("INDIDriversPath"));
    else
         ui->kcfg_INDIDriversPath->setText(Options::iNDIDriversPath());

    if(ui->kcfg_INDIConfigPathDefault->isChecked())
         ui->kcfg_INDIConfigPath->setText(parent->getDefault("INDIConfigPath"));
    else
         ui->kcfg_INDIConfigPath->setText(Options::iNDIConfigPath());

    if(ui->kcfg_GPhotoIOLIBSDefault->isChecked())
             ui->kcfg_GPhotoIOLIBS->setText(parent->getDefault("GPhotoIOLIBS"));
        else
             ui->kcfg_GPhotoIOLIBS->setText(Options::gPhotoIOLIBS());

    if(ui->kcfg_GPhotoCAMLIBSDefault->isChecked())
             ui->kcfg_GPhotoCAMLIBS->setText(parent->getDefault("GPhotoCAMLIBS"));
        else
             ui->kcfg_GPhotoCAMLIBS->setText(Options::gPhotoCAMLIBS());
}

/*
 * This method displays whether Homebrew, Python3, and indi-web are properly installed.
 */
void OpsConfiguration::displayInstallationStatus(bool installed)
{
    if(installed)
    {
        ui->installationStatusDisplay->setText(i18n("Installed"));
        ui->installationStatusDisplay->setStyleSheet("QLineEdit {background-color: green;}");
    }
    else
    {
        ui->installationStatusDisplay->setText(i18n("Incomplete"));
        ui->installationStatusDisplay->setStyleSheet("QLineEdit {background-color: red;}");
    }
}

/*
 * This method displays whether GSC is located at the specified path.
 */
void OpsConfiguration::displayGSCInstallationStatus(bool installed)
{
    if(installed)
    {
       ui->gscStatusDisplay->setText(i18n("Installed"));
       ui->gscStatusDisplay->setStyleSheet("QLineEdit {background-color: green;}");
    }
    else
    {
       ui->gscStatusDisplay->setText(i18n("Incomplete"));
       ui->gscStatusDisplay->setStyleSheet("QLineEdit {background-color: red;}");
    }
}

/*
 * This method detects whether Homebrew, Python3, and indi-web are properly installed and updates the status.
 */
void OpsConfiguration::updatePythonInstallationStatus()
{
    bool installed = parent->pythonInstalled() && parent->indiWebInstalled();
    displayInstallationStatus(installed);
}

/*
 * This method detects whether GSC is located at the specified path and updates the status.
 */
void OpsConfiguration::updateGSCInstallationStatus()
{
    bool gscInstall = gscInstalled();
    displayGSCInstallationStatus(gscInstall);
}

/*
 * This method detects whether homebrew is installed.
 */
bool OpsConfiguration::brewInstalled()
{
    return QFileInfo("/usr/local/bin/brew").exists();
}

/*
 * This method detects whether gsc is isntalled at the desired path.
 */
bool OpsConfiguration::gscInstalled()
{
    QString gsc = ui->kcfg_GSCPath->text();
    if(!gsc.endsWith("gsc"))
        return false;
    return QDir(ui->kcfg_GSCPath->text()).exists();
}

/*
 * This is the installer method for homebrew, python, and indi-web.
 * It runs when you click the button.
 */
void OpsConfiguration::slotInstallRequirements()
{
    #ifdef Q_OS_OSX

    if(brewInstalled() && parent->pythonInstalled() && parent->indiWebInstalled())
    {
        QMessageBox::information(nullptr, "Message", i18n("Homebrew, python, and indiweb are already installed"));
        return;
    }

    if (QMessageBox::question(nullptr, "Message", i18n("This installer will install the following requirements for astrometry.net if they are not installed:\nHomebrew -an OS X Unix Program Package Manager\nPython3 -A Powerful Scripting Language \nindiweb -Python Modules for Astronomy \n Do you wish to continue?")) == QMessageBox::Yes)
    {
        QProcess* install = new QProcess(this);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString path            = env.value("PATH", "");
        env.insert("PATH", "/usr/local/opt/python/libexec/bin:/usr/local/bin:" + path);
        install->setProcessEnvironment(env);

        if(!brewInstalled())
        {
            QMessageBox::information(nullptr, "Message", i18n("Homebrew is not installed.  \nA Terminal window will pop up for you to install Homebrew.  \n When you are all done, then you can close the Terminal and click the setup button again."));
            QStringList installArgs;
            QString homebrewInstallScript =
                    "tell application \"Terminal\"\n"
                    "    do script \"/usr/bin/ruby -e \\\"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)\\\"\"\n"
                    "end tell\n";
            QString bringToFront =
                    "tell application \"Terminal\"\n"
                    "    activate\n"
                    "end tell\n";

            QStringList processArguments;
            processArguments << "-l" << "AppleScript";
            install->start("/usr/bin/osascript", processArguments);
            install->write(homebrewInstallScript.toUtf8());
            install->write(bringToFront.toUtf8());
            install->closeWriteChannel();
            install->waitForFinished();
            return;
        }
        if(!parent->pythonInstalled())
        {
            QMessageBox::information(nullptr, "Message", i18n("Homebrew installed \nPython3 will install when you click Ok \nindiweb waiting . . . \n (Note: this might take a few minutes, please be patient.)"));
            install->start("/usr/local/bin/brew" , QStringList() << "install" << "python3");
            install->waitForFinished();
            if(!parent->pythonInstalled())
            {
                QMessageBox::information(nullptr,  "Message", i18n("Python install failure"));
                return;
            }
        }
        if(!parent->indiWebInstalled())
        {
           QMessageBox::information(nullptr, "Message", i18n("Homebrew installed \nPython3 installed \nindiweb will install when you click Ok \n (Note: this might take a few minutes, please be patient.)"));
            install->start("/usr/local/bin/pip3" , QStringList() << "install" << "indiweb");
            install->waitForFinished();
            if(!parent->indiWebInstalled())
            {
                QMessageBox::information(nullptr, "Message", i18n("indiweb install failure"));
                return;
            }
        }
        QMessageBox::information(nullptr, "Message", i18n("All installations are complete and ready to use."));
    }
#else
        QMessageBox::information(nullptr, "Message", "The installer only works on OS X.  On Linux, please install python and indiweb from the command line.");
#endif
}

/*
 * This is the installer method for GSC.
 * It runs when you click the button.
 */
void OpsConfiguration::slotInstallGSC()
{
    if(gscInstalled())
    {
        QMessageBox::information(nullptr, "Message", i18n("GSC is already installed."));
        return;
    }
    QString location = ui->kcfg_GSCPath->text();
    if(location.endsWith("gsc"))
        location = location.left(location.length()-3);
    if(!QDir(location).exists())
    {
        bool created = false;
        if(ui->kcfg_GSCPathDefault->isChecked())
            created = QDir().mkpath(location);
        else
            if(QMessageBox::question(nullptr, "Message", i18n("The folder:\n %1 \ndoes not exist.  Would you like to create it?").arg(location)) == QMessageBox::Yes)
                created = QDir().mkpath(location);
        if(!created)
        {
            QMessageBox::information(nullptr, "Message", i18n("Please select another installation location."));
            return;
        }
    }
    QString gscZipPath = location + "/gsc.zip";

    QNetworkAccessManager *manager= new QNetworkAccessManager();

    ui->downloadProgress->setVisible(true);
    ui->downloadProgress->setEnabled(true);

    ui->gscInstallCancel->setVisible(true);
    ui->gscInstallCancel->setEnabled(true);

    QString gscURL = "http://www.indilib.org/jdownloads/Mac/gsc.zip";

    QNetworkReply *response = manager->get(QNetworkRequest(QUrl(gscURL)));

    QMetaObject::Connection *cancelConnection = new QMetaObject::Connection();
    QMetaObject::Connection *replyConnection = new QMetaObject::Connection();
    QMetaObject::Connection *percentConnection = new QMetaObject::Connection();

    *percentConnection=connect(response,&QNetworkReply::downloadProgress,
    [=](qint64 bytesReceived, qint64 bytesTotal){
        ui->downloadProgress->setValue(bytesReceived);
        ui->downloadProgress->setMaximum(bytesTotal);
    });

    *cancelConnection=connect(ui->gscInstallCancel, &QPushButton::clicked,
    [=](){
        qDebug() << "Download Cancelled.";

        if(cancelConnection)
            disconnect(*cancelConnection);
        if(replyConnection)
            disconnect(*replyConnection);

        if(response){
            response->abort();
            response->deleteLater();
        }

        ui->downloadProgress->setVisible(false);
        ui->downloadProgress->setEnabled(false);

        ui->gscInstallCancel->setVisible(false);
        ui->gscInstallCancel->setEnabled(false);

        if(manager)
            manager->deleteLater();

    });

    *replyConnection=connect(response, &QNetworkReply::finished, this,
    [=]() {
        if(response){

            if(cancelConnection)
                disconnect(*cancelConnection);
            if(replyConnection)
                disconnect(*replyConnection);

            ui->downloadProgress->setVisible(false);
            ui->downloadProgress->setEnabled(false);

            ui->gscInstallCancel->setVisible(false);
            ui->gscInstallCancel->setEnabled(false);


            response->deleteLater();
            if(manager)
                manager->deleteLater();
            if (response->error() != QNetworkReply::NoError)
                return;

            QByteArray responseData = response->readAll();

            QFile file(gscZipPath);
            if (QFileInfo(QFileInfo(file).path()).isWritable())
            {
                if (!file.open(QIODevice::WriteOnly))
                {
                    QMessageBox::information(nullptr, "Message", i18n("File write error."));
                    return;
                }
                else
                {
                    file.write(responseData.data(), responseData.size());
                    file.close();
                    slotExtractGSC();
                }
            }
            else
            {
                QMessageBox::information(nullptr, "Message", i18n("GSC parent folder permissions error."));
            }
        }
    });
}

/*
 * This is the extraction method for gsc.
 * It runs after GSC is downloaded.
 */
void OpsConfiguration::slotExtractGSC()
{
    QString location = ui->kcfg_GSCPath->text();
    if(location.endsWith("gsc"))
        location = location.left(location.length()-3);
    QProcess *gscExtractor = new QProcess();
    connect(gscExtractor, SIGNAL(finished(int)), this, SLOT(slotGSCInstallerFinished()));
    connect(gscExtractor, SIGNAL(finished(int)), this, SLOT(gscExtractor.deleteLater()));
    gscExtractor->setWorkingDirectory(location);
    gscExtractor->start("unzip", QStringList() << "-ao"
                                               << "gsc.zip");
}

/*
 * This method tidies up after GSC is installed.
 */
void OpsConfiguration::slotGSCInstallerFinished()
{
    ui->downloadProgress->setEnabled(false);
    ui->downloadProgress->setValue(0);
    ui->downloadProgress->setVisible(false);
    QString location = ui->kcfg_GSCPath->text();
    if(location.endsWith("gsc"))
        location = location.left(location.length()-3);
    else
        ui->kcfg_GSCPath->setText(location + "/gsc");
    QString gscZipPath = location + "/gsc.zip";
    if (QFile(gscZipPath).exists())
        QFile(gscZipPath).remove();
    updateGSCInstallationStatus();
}
