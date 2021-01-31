#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "config.h"
#include "mainwindow.h"


namespace Ui {
class SettingsDialog;
}


class SettingsDialog : public QDialog
{
    Q_OBJECT

public: 
    explicit SettingsDialog( MainWindow* mw_ptr, QWidget *parent = nullptr );
    ~SettingsDialog();

signals:
    void set_mainwindow_stylesheet(const QString& styleSheet);

private slots:
    void on_pushButton_Cancel_clicked();

    void on_pushButton_Apply_clicked();

    void on_checkBox_DarkMode_stateChanged(int arg1);

private:
    Ui::SettingsDialog *ui;
    Config* config_ptr = nullptr;
    MainWindow* mw_ptr = nullptr;
};

#endif // SETTINGSDIALOG_H
