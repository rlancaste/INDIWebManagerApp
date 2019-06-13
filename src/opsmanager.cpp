/*  INDIWebManagerApp
    Copyright (C) 2019 Robert Lancaster <rlancaste@gmail.com>

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/

#include "opsmanager.h"
#include "ui_opsmanager.h"

OpsManager::OpsManager(MainWindow *parent) : QWidget(parent)
{
   this->parent = parent;
   ui = new Ui::OpsManager;
   ui->setupUi(this);

   connect(ui->kcfg_ComputerHostNameAuto, &QCheckBox::clicked, this, &OpsManager::updateFromCheckBoxes);
   connect(ui->kcfg_ComputerIPAddressAuto, &QCheckBox::clicked, this, &OpsManager::updateFromCheckBoxes);
   connect(ui->kcfg_ManagerPortNumberDefault, &QCheckBox::clicked, this, &OpsManager::updateFromCheckBoxes);
   connect(ui->kcfg_LogFilePathDefault, &QCheckBox::clicked, this, &OpsManager::updateFromCheckBoxes);

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

}
