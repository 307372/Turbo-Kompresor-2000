#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "config.h"


SettingsDialog::SettingsDialog( Config* conf, QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    this->config_ptr = conf;

    this->ui->checkBox_ScaledFileSize->setChecked(config_ptr->get_filesize_scaling());

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
    if (config_ptr->get_filesize_scaling() != ui->checkBox_ScaledFileSize->isChecked()) config_ptr->set_filesize_scaling(ui->checkBox_ScaledFileSize->isChecked());
    close();
}
