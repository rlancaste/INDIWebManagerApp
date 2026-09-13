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
#include <KLocalizedString>


OpsConfiguration::OpsConfiguration(MainWindow *parent)
{
    this->parent = parent;
    ui = new Ui::OpsConfiguration;
    ui->setupUi(this);

    //This updates the status so that the user knows if they are installed when it opens.
    updateIndiwebInstallationStatus();
    updateGSCInstallationStatus();

    //Connects install buttons to their methods
    connect(ui->installRequirements, &QAbstractButton::clicked, this, &OpsConfiguration::slotSetupINDIWeb);
    connect(ui->installGSC, &QAbstractButton::clicked, this, &OpsConfiguration::slotInstallGSC);

    //Connects the line edits to the update status methods so the user can see in real time if the path is right.
    connect(ui->kcfg_GSCPath, &QLineEdit::textChanged, this, &OpsConfiguration::updateGSCInstallationStatus);
    connect(ui->kcfg_INDIWebVENVPath, &QLineEdit::textChanged, this, &OpsConfiguration::updateIndiwebInstallationStatus);

    //Connects all the line edits to the slot PathExists method so the user can see in real time if the path exists.
    connect(ui->kcfg_SystemPython, &QLineEdit::textChanged, this, &OpsConfiguration::slotPathExists);
    connect(ui->kcfg_INDIWebVENVPath, &QLineEdit::textChanged, this, &OpsConfiguration::slotPathExists);
    connect(ui->kcfg_GSCPath, &QLineEdit::textChanged, this, &OpsConfiguration::slotPathExists);
    connect(ui->kcfg_INDIPrefix, &QLineEdit::textChanged, this, &OpsConfiguration::slotPathExists);
    connect(ui->kcfg_INDIServerPath, &QLineEdit::textChanged, this, &OpsConfiguration::slotPathExists);
    connect(ui->kcfg_INDIDriversPath, &QLineEdit::textChanged, this, &OpsConfiguration::slotPathExists);
    connect(ui->kcfg_INDIConfigPath, &QLineEdit::textChanged, this, &OpsConfiguration::slotPathExists);
    connect(ui->kcfg_GPhotoCAMLIBS, &QLineEdit::textChanged, this, &OpsConfiguration::slotPathExists);
    connect(ui->kcfg_GPhotoIOLIBS, &QLineEdit::textChanged, this, &OpsConfiguration::slotPathExists);

    //Hides the installation displays for GSC since it is not currently running
    ui->gscInstallCancel->setVisible(false);
    ui->downloadProgress->setVisible(false);

    //This Disables some setting Controls on Linux that are not used for Linux
    #if defined(Q_OS_LINUX)
        ui->kcfg_INDIPrefix->setEnabled(false);
        ui->kcfg_INDIPrefixDefault->setEnabled(false);
        ui->kcfg_GPhotoIOLIBSDefault->setEnabled(false);
        ui->kcfg_GPhotoCAMLIBSDefault->setEnabled(false);
        ui->kcfg_GPhotoIOLIBS->setEnabled(false);
        ui->kcfg_GPhotoCAMLIBS->setEnabled(false);
    #endif

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
    ui->kcfg_SystemPython->setReadOnly(ui->kcfg_SystemPythonDefault->isChecked());
    ui->kcfg_INDIWebVENVPath->setReadOnly(ui->kcfg_INDIWebVENVPathDefault->isChecked());
    ui->kcfg_GSCPath->setReadOnly(ui->kcfg_GSCPathDefault->isChecked());
    ui->kcfg_INDIPrefix->setReadOnly(ui->kcfg_INDIPrefixDefault->isChecked());
    ui->kcfg_INDIServerPath->setReadOnly(ui->kcfg_INDIServerDefault->isChecked());
    ui->kcfg_INDIDriversPath->setReadOnly(ui->kcfg_INDIDriversDefault->isChecked());
    ui->kcfg_INDIConfigPath->setReadOnly(ui->kcfg_INDIConfigPathDefault->isChecked());
    ui->kcfg_GPhotoIOLIBS->setReadOnly(ui->kcfg_GPhotoIOLIBSDefault->isChecked());
    ui->kcfg_GPhotoCAMLIBS->setReadOnly(ui->kcfg_GPhotoCAMLIBSDefault->isChecked());

    if(ui->kcfg_SystemPythonDefault->isChecked())
         ui->kcfg_SystemPython->setText(parent->getDefault("SystemPython"));
    else
         ui->kcfg_SystemPython->setText(Options::systemPython());

    if(ui->kcfg_INDIWebVENVPathDefault->isChecked())
         ui->kcfg_INDIWebVENVPath->setText(parent->getDefault("INDIWebVENVPath"));
    else
         ui->kcfg_INDIWebVENVPath->setText(Options::iNDIWebVENVPath());

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
         ui->kcfg_INDIDriversPath->setText(parent->getDefault("INDIDriversPath"));
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

void OpsConfiguration::slotPathExists()
{
    QLineEdit *line = qobject_cast<QLineEdit*>(sender());
    if(QFileInfo(line->text()).exists())
        line->setStyleSheet("QLineEdit {color: green;}");
    else
        line->setStyleSheet("QLineEdit {color: white;}");
}

/*
 * This method displays whether indi-web is properly installed in a Virtual Environment.
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
 * This method detects whether Python3 and indi-web are properly installed and updates the status.
 */
void OpsConfiguration::updateIndiwebInstallationStatus()
{
    bool installed = parent->indiWebVENVPathValid(ui->kcfg_INDIWebVENVPath->text());
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
 * This method detects whether gsc is isntalled at the desired path.
 */
bool OpsConfiguration::gscInstalled()
{
    QString gsc = ui->kcfg_GSCPath->text();
    if(!(gsc.endsWith("gsc") || gsc.endsWith("GSC")))
        return false;
    return QDir(ui->kcfg_GSCPath->text()).exists();
}

/*
 * This is the installer method that sets up a virtual environment for indi-web and then installs it.
 * It runs when you click the button.
 */
void OpsConfiguration::slotSetupINDIWeb()
{
    if(parent->indiWebInstalled())
    {
        QMessageBox::information(nullptr, "Message", i18n("INDI Web is already installed in the Selected Virtual Environment."));
        return;
    }

    //This check is performed to make sure the path in the text box matches the current option setting.
    if(Options::systemPython() != ui->kcfg_SystemPython->text())
    {
        QMessageBox::information(nullptr, "Message", i18n("Please click apply after changing the System Python Path before installing."));
        return;
    }

    //This check is performed to make sure the path in the text box matches the current option setting.
    if(Options::iNDIWebVENVPath() != ui->kcfg_INDIWebVENVPath->text())
    {
        QMessageBox::information(nullptr, "Message", i18n("Please click apply after changing the INDI Web Manager VENV Path before installing."));
        return;
    }

    if( !parent->systemPythonInstalled())
    {
        QMessageBox::information(nullptr, "Message", i18n("Your system needs to have Python installed.  Please install python using your system package manager, an official Python Installer, or homebrew, and update the System Python Setting above."));
        return;
    }
    if(parent->indiWebInstalled())
    {
        QMessageBox::information(nullptr, "Message", i18n("indiweb is already installed"));
        return;
    }

    QProcess install;

    install.start(Options::systemPython(), QStringList() << "-m" << "venv" << Options::iNDIWebVENVPath());
    install.waitForFinished();

    if(!parent->pythonVENVExists())
    {
        QMessageBox::information(nullptr, "Message", i18n("The installation of the Virtual Environment for INDI WebManager has failed.  Here is the error information: "));
        QMessageBox::warning(nullptr, "Message", install.errorString());
        return;
    }

    // This folder now exists if it did not before.
    ui->kcfg_INDIWebVENVPath->setStyleSheet("QLineEdit {color: green;}");


    if(!parent->pipInstalledInVENV())
    {
        QMessageBox::information(nullptr, "Message", i18n("Cannot find pip in your INDIWeb Manager VENV Directory. Please install pip, or put a symlink to pip in there."));
        return;
    }

    install.start(Options::iNDIWebVENVPath() + "/bin/pip", QStringList() << "install" << "indiweb");
    install.waitForFinished();

    if(!parent->indiWebInstalled())
    {
        QMessageBox::information(nullptr, "Message", i18n("indiweb install failure"));
        return;
    }

    updateIndiwebInstallationStatus();
}

/*
 * This is the installer method for GSC.
 * It runs when you click the button.
 */
void OpsConfiguration::slotInstallGSC()
{
#ifdef Q_OS_MACOS
    if(Options::gSCPath() != ui->kcfg_GSCPath->text())
    {
        QMessageBox::information(nullptr, "Message", i18n("Please click apply after changing the GSC path before installing."));
        return;
    }
    if(gscInstalled())
    {
        QMessageBox::information(nullptr, "Message", i18n("GSC is already installed."));
        return;
    }
    QString location = ui->kcfg_GSCPath->text();
    if(location.endsWith("gsc") || location.endsWith("GSC"))
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
#else
    QMessageBox::information(nullptr, "Message", i18n("On Linux, please install from the Terminal."));
#endif
}

/*
 * This is the extraction method for gsc.
 * It runs after GSC is downloaded.
 */
void OpsConfiguration::slotExtractGSC()
{
    QString location = ui->kcfg_GSCPath->text();
    if(location.endsWith("gsc") || location.endsWith("GSC"))
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
