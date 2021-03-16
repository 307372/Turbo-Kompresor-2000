#include "settings_dialog.h"

#include "ui_settings_dialog.h"


SettingsDialog::SettingsDialog( ArchiveWindow* mw_ptr, QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::SettingsDialog),
    config_ptr(mw_ptr->config_ptr),
    mw_ptr(mw_ptr)
{
    ui->setupUi(this);
    ui->checkBox_ScaledFileSize->setChecked(config_ptr->get_filesize_scaling());
    ui->checkBox_DarkMode->setChecked(config_ptr->get_dark_mode());
}


SettingsDialog::~SettingsDialog()
{
    delete ui;
}


void SettingsDialog::on_pushButton_Cancel_clicked()
{
    close();
}


void SettingsDialog::on_pushButton_Apply_clicked()
{
    if (config_ptr->get_filesize_scaling() != ui->checkBox_ScaledFileSize->isChecked())
        config_ptr->set_filesize_scaling(ui->checkBox_ScaledFileSize->isChecked());

    if (config_ptr->get_dark_mode() != ui->checkBox_DarkMode->isChecked()) {
        emit set_mainwindow_stylesheet( (ui->checkBox_DarkMode->isChecked()) ? mw_ptr->style_dark : mw_ptr->style_light );
        config_ptr->set_dark_mode(ui->checkBox_DarkMode->isChecked());
    }

    close();
}


void SettingsDialog::on_checkBox_DarkMode_stateChanged(int state)
{
    if (state == Qt::Unchecked) {
        this->setStyleSheet(mw_ptr->style_light);
    }
    else if (state == Qt::Checked) {
        this->setStyleSheet(mw_ptr->style_dark);
    }
}
