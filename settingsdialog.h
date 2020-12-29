#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "config.h"
namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public: 
    explicit SettingsDialog( Config* conf, QWidget *parent = nullptr );
    ~SettingsDialog();

private slots:
    void on_pushButton_Cancel_clicked();

    void on_pushButton_Apply_clicked();

private:
    Ui::SettingsDialog *ui;
    Config* config_ptr;
};

#endif // SETTINGSDIALOG_H
