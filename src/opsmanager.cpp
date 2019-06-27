/*  INDIWebManagerApp
    Copyright (C) 2019 Robert Lancaster <rlancaste@gmail.com>

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/

#include "opsmanager.h"
#include "ui_opsmanager.h"
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <KLocalizedString>

OpsManager::OpsManager(MainWindow *parent) : QWidget(parent)
{
   this->parent = parent;
   ui = new Ui::OpsManager;
   ui->setupUi(this);

#ifdef Q_OS_OSX
   startupFilePath = QDir::homePath() + "/Library/LaunchAgents/" + "com.INDIWebManager.LaunchAgent.plist";
#else
   startupFilePath = "/etc/systemd/system/INDIWebManagerApp.service";
#endif

   connect(ui->kcfg_ComputerHostNameAuto, &QCheckBox::clicked, this, &OpsManager::updateFromCheckBoxes);
   connect(ui->kcfg_ComputerIPAddressAuto, &QCheckBox::clicked, this, &OpsManager::updateFromCheckBoxes);
   connect(ui->kcfg_ManagerPortNumberDefault, &QCheckBox::clicked, this, &OpsManager::updateFromCheckBoxes);
   connect(ui->kcfg_LogFilePathDefault, &QCheckBox::clicked, this, &OpsManager::updateFromCheckBoxes);
   connect(ui->launchAtStartup, &QCheckBox::clicked, this, &OpsManager::toggleLaunchAtStartup);

   //This waits a moment for the kconfig to load the options, then sets the Line Edits to read only appropriagely
   QTimer::singleShot(100, this, &OpsManager::updateFromCheckBoxes);
}

OpsManager::~OpsManager()
{
    delete ui;
}

/*
 * This method enables the functionality of the default buttons.
 * The line edits are disabled as long as the default button is checked.
 * If the user unchecks the button, it changes to the stored value
 * If the user checks the button, it changes to the default value.
 */
void OpsManager::updateFromCheckBoxes()
{
    ui->kcfg_ComputerHostName->setReadOnly(ui->kcfg_ComputerHostNameAuto->isChecked());
    ui->kcfg_ComputerIPAddress->setReadOnly(ui->kcfg_ComputerIPAddressAuto->isChecked());
    ui->kcfg_ManagerPortNumber->setReadOnly(ui->kcfg_ManagerPortNumberDefault->isChecked());
    ui->kcfg_LogFilePath->setReadOnly(ui->kcfg_LogFilePathDefault->isChecked());

    if(ui->kcfg_ComputerHostNameAuto->isChecked())
         ui->kcfg_ComputerHostName->setText(parent->getDefault("ComputerHostName"));
    else
         ui->kcfg_ComputerHostName->setText(Options::computerHostName());

    if(ui->kcfg_ComputerIPAddressAuto->isChecked())
         ui->kcfg_ComputerIPAddress->setText(parent->getDefault("ComputerIPAddress"));
    else
         ui->kcfg_ComputerIPAddress->setText(Options::computerIPAddress());

    if(ui->kcfg_ManagerPortNumberDefault->isChecked())
         ui->kcfg_ManagerPortNumber->setText(parent->getDefault("ManagerPortNumber"));
    else
         ui->kcfg_ManagerPortNumber->setText(Options::managerPortNumber());

    if(ui->kcfg_LogFilePathDefault->isChecked())
         ui->kcfg_LogFilePath->setText(parent->getDefault("LogFilePath"));
    else
         ui->kcfg_LogFilePath->setText(Options::logFilePath());

    ui->launchAtStartup->setChecked(checkLaunchAtStartup());

}

void OpsManager::setLaunchAtStartup(bool launchAtStart)
{
    if(launchAtStart)
    {
        QString fileText = "";

    #ifdef Q_OS_OSX
        fileText = "" \
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" \
        "<plist version=\"1.0\">\n" \
        "<dict>\n" \
        "    <key>Disabled</key>\n" \
        "    <false/>\n" \
        "    <key>Label</key>\n" \
        "    <string>INDI Web Manager App</string>\n" \
        "    <key>ProgramArguments</key>\n" \
        "    <array>\n" \
        "        <string>" + QCoreApplication::applicationDirPath() + "/indiwebmanagerapp</string>\n" \
        "    </array>\n" \
        "    <key>RunAtLoad</key>\n" \
        "    <true/>\n" \
        "</dict>\n" \
        "</plist>";

        QFile file( startupFilePath );
        if ( file.open(QIODevice::ReadWrite) )
        {
            QTextStream stream( &file );
            stream << fileText << endl;
        }
    #else
        fileText = "" \
        "[Unit]\n" \
        "Description=INDI Web Manager App\n" \
        "After=multi-user.target\n" \
        "\n" \
        "[Service]\n" \
        "Type=idle\n" \
        "User=" + qgetenv("USER") + "\n" \
        "ExecStart=" + QCoreApplication::applicationDirPath() + "/indiwebmanagerapp\n" \
        "\n" \
        "[Install]\n" \
        "WantedBy=multi-user.target";

        QString tempFile =  QDir::homePath() + "/INDIWebManagerApp.service";
        QFile file2(tempFile);
        if ( file2.open(QIODevice::ReadWrite) )
        {
            QTextStream stream( &file2 );
            stream << fileText << endl;
        }
        bool ok;
        QString password = QInputDialog::getText(nullptr, "Get Password",
                                                 i18n("To create the service file and enable the service, we need to use sudo. \nYour admin password please:"), QLineEdit::Normal,
                                                 "", &ok);
        if (ok && !password.isEmpty())
        {
            QProcess loadService;
            loadService.start("bash", QStringList() << "-c" << "echo " + password + " | sudo -S mv " + tempFile + " " + startupFilePath);
            loadService.waitForFinished();
            loadService.start("bash", QStringList() << "-c" << "echo " + password + " | sudo -S chmod 644 " + startupFilePath);
            loadService.waitForFinished();
            loadService.start("bash", QStringList() << "-c" << "echo " + password + " | sudo -S systemctl daemon-reload");
            loadService.waitForFinished();
            loadService.start("bash", QStringList() << "-c" << "echo " + password + " | sudo -S systemctl enable INDIWebManagerApp.service");
            loadService.waitForFinished();
        }
        else
        {
            QMessageBox::information(nullptr, "message", i18n("Since we cannot get your sudo password, we can't complete your request.  You can try clicking the button again and entering your password, or manually do it using the following steps in a Terminal."));

            QMessageBox::information(nullptr, "message", "sudo mv " + QDir::homePath() + "/INDIWebManagerApp.service /etc/systemd/system/INDIWebManagerApp.service\n" \
                                                         "sudo chmod 644 /etc/systemd/system/INDIWebManagerApp.service\n" \
                                                         "sudo systemctl daemon-reload\n" \
                                                         "sudo systemctl enable INDIWebManagerApp.service\n");
        }
    #endif

    }
    else
    {
    #ifdef Q_OS_OSX
        QFile::remove(startupFilePath);
    #else
        bool ok;
        QString password = QInputDialog::getText(nullptr, "Get Password",
                                                 i18n("To delete the service file and stop the service, we need to use sudo. \nYour admin password please:"), QLineEdit::Normal,
                                                 "", &ok);
        if (ok && !password.isEmpty())
        {
            QProcess loadService;
            loadService.start("bash", QStringList() << "-c" << "echo " + password + " | sudo -S rm " + startupFilePath);
            loadService.waitForFinished();
            loadService.start("bash", QStringList() << "-c" << "echo " + password + " | sudo -S systemctl daemon-reload");
            loadService.waitForFinished();

        }
        else
        {
            QMessageBox::information(nullptr, "message", i18n("Since we cannot get your sudo password, we can't complete your request.  You can try clicking the button again and entering your password, or manually do it using the following steps in a Terminal."));

            QMessageBox::information(nullptr, "message", "sudo rm /etc/systemd/system/INDIWebManagerApp.service\n" \
                                                         "sudo systemctl daemon-reload\n");
        }
    #endif
    }
    ui->launchAtStartup->setChecked(checkLaunchAtStartup());
}

void OpsManager::toggleLaunchAtStartup()
{
    setLaunchAtStartup(!checkLaunchAtStartup());
}

bool OpsManager::checkLaunchAtStartup()
{
    return QFileInfo(startupFilePath).exists();
}
