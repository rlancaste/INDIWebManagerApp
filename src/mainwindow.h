/*  INDIWebManagerApp
    Copyright (C) 2019 Robert Lancaster <rlancaste@gmail.com>

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QProcess>
#include <QPlainTextEdit>
#include <QCloseEvent>
#include <QTimer>
#include <QNetworkAccessManager>
#include "Options.h"
#include "version.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);
    static QString getDefault(QString option);
    QString getWebManagerURL();
    QString getINDIServerURL(QString port);
    bool pythonInstalled();
    bool pipInstalled();
    bool indiWebInstalled();

private:

    Ui::MainWindow *ui;
    bool webManagerRunning = false;
    QPointer<QProcess> webManager;
    void configureEnvironmentVariables();
    void insertEnvironmentVariable(QString variable, QString value);
    void insertEnvironmentPath(QString variable, QString filePath);
    void displayManagerStatusOnline(bool online);
    void displayServerStatusOnline(bool online);
    QSize minimumWindowSize;

    bool isWebManagerOnline();
    bool isINDIServerOnline(QString &activeProfile);
    void checkINDIServerStatus();
    QTimer serverMonitor;
    bool getRunningDrivers(QString &webManagerDrivers);
    QString getINDIServerPort(QString &activeProfile);
    bool getWebManagerResponse(const QUrl &url, QJsonDocument *reply);
    void updateDisplaysforShutDown();



private slots:
    void startWebManager();
    void stopWebManager();
    void appendLogEntry();
    void appendLogEntry(QString text);
    void managerClosed(int result);
    void showPreferences();
    void setLogVisible(bool visible);
    void updateSettings();
};

#endif // MAINWINDOW_H
