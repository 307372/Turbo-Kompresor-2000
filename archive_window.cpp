#include "archive_window.h"

#include <cassert>

#include <QFile>

#include "ui_archive_window.h"
#include "settings_dialog.h"
#include "misc/custom_tree_widget_items.h"

ArchiveWindow::ArchiveWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ArchiveWindow)
    , archive_ptr(new Archive())
    , config_ptr(new Config())
{
    ui->setupUi(this);
    this->setWindowIcon( *(new QIcon(":/archive.png")));

    ui->archiveWidget->setColumnWidth(0, 300);
    ui->archiveWidget->setSelectionMode( QAbstractItemView::SelectionMode::ExtendedSelection );

    QAction *settingsAction = ui->menubar->addAction("Settings");
    connect( settingsAction,            &QAction::triggered,    this, &ArchiveWindow::open_settings_dialog );
    connect( ui->buttonRemoveSelected,  &QPushButton::clicked,  this, &ArchiveWindow::remove_selected_clicked );
    connect( ui->actionNewArchive,      &QAction::triggered,    this, &ArchiveWindow::new_archive_triggered );
    connect( ui->actionOpenArchive,     &QAction::triggered,    this, &ArchiveWindow::open_archive_triggered );
    connect( ui->buttonExtractSelected, &QPushButton::clicked,  this, &ArchiveWindow::extract_selected_clicked );
    connect( ui->buttonExtractAll,      &QPushButton::clicked,  this, &ArchiveWindow::extract_all_clicked );
    connect( ui->buttonAddNewFile,      &QPushButton::clicked,  this, &ArchiveWindow::add_new_file_clicked );
    connect( ui->buttonAddNewFolder,    &QPushButton::clicked,  this, &ArchiveWindow::add_new_folder_clicked );

    // loading stylesheets (if they exist)
    QFile style_file_dark(":/dark.qss");
    if (style_file_dark.exists()) {
        style_file_dark.open(QFile::ReadOnly | QFile::Text);
        QTextStream stream_dark(&style_file_dark);
        this->style_dark = stream_dark.readAll();
    }

    QFile style_file_light(":/light.qss");
    if (style_file_light.exists()) {
        style_file_light.open(QFile::ReadOnly | QFile::Text);
        QTextStream stream_light(&style_file_light);
        this->style_light = stream_light.readAll();
    }

    if (config_ptr->get_dark_mode()) this->setStyleSheet(this->style_dark);
    else this->setStyleSheet(this->style_light);
}


ArchiveWindow::~ArchiveWindow()
{
    if (archive_ptr != nullptr) delete archive_ptr;
    if (ui          != nullptr) delete ui;
    if (config_ptr  != nullptr) delete config_ptr;
}


void ArchiveWindow::new_archive_model() {
    ui->archiveWidget->clear();
    if (archive_ptr != nullptr) delete archive_ptr;
    archive_ptr = new Archive();
}


void ArchiveWindow::load_archive( std::string path_to_archive ) {
    this->new_archive_model();
    this->current_archive_path = path_to_archive;
    this->archive_ptr->load( path_to_archive );

    ui->archiveWidget->addTopLevelItem( new TreeWidgetFolder( ui->archiveWidget->invisibleRootItem(), archive_ptr->root_folder.get(), archive_ptr, config_ptr->get_filesize_scaling() ) );
    ui->archiveWidget->topLevelItem(0)->setText(0, QString::fromStdString(std::filesystem::path(path_to_archive).filename()));
    this->ui->archiveWidget->expandAll();

    this->ui->archiveWidget->sortByColumn(0, Qt::SortOrder::AscendingOrder);
}


void ArchiveWindow::reload_archive() {
    this->new_archive_model();
    this->load_archive( current_archive_path );
}


void ArchiveWindow::write_file_to_current_archive( File* file_ptr, bool& aborting_var )
{
    file_ptr->append_to_archive( this->archive_ptr->archive_file, aborting_var );
}


void ArchiveWindow::write_folder_to_current_archive( Folder* folder_model, bool& aborting_var ) {
    folder_model->append_to_archive( this->archive_ptr->archive_file, aborting_var );
    this->reload_archive();
}




void ArchiveWindow::new_archive_triggered()
{
    create_empty_archive();
}


void ArchiveWindow::open_archive_triggered()
{
    QString filter = "Archive (*.tk2k) ;; All Files (*.*)" ;
    QString file_path = QFileDialog::getOpenFileName(this, "Open an archive", QDir::homePath() + "/Desktop", filter );
    if (!file_path.isEmpty()) {
        load_archive( file_path.toStdString() );
    }
}


void ArchiveWindow::extract_selected_clicked()
{
    auto selected = ui->archiveWidget->selectedItems();

    if (!selected.empty()) {

        std::vector<QTreeWidgetItem*> extraction_targets;
        for (int32_t i=0; i < selected.size(); ++i) extraction_targets.push_back( selected[i] );

        ProcessingDialog md( extraction_targets,  this);
        md.prepare_GUI_decompression();
        md.exec();

        this->reload_archive();
    }
}


void ArchiveWindow::extract_all_clicked()
{
    // if there's no open archive, extract nothing
    if (ui->archiveWidget->invisibleRootItem()->childCount() == 0) return;

    std::vector<QTreeWidgetItem*> extraction_targets;
    extraction_targets.push_back(ui->archiveWidget->topLevelItem(0));

    // Open processing dialog
    ProcessingDialog md( extraction_targets,  this);
    md.prepare_GUI_decompression();
    md.exec();

    this->reload_archive();
}


void ArchiveWindow::add_new_file_clicked()
{
    if (!ui->archiveWidget->selectedItems().empty()) {
        auto itm = ui->archiveWidget->selectedItems()[0];

        if (itm->type() == 1001) {  // TreeWidgetFolder
            TreeWidgetFolder* twfolder = static_cast<TreeWidgetFolder*>(itm);
            QFileDialog dialog( this, "Select files which you want to add", QDir::homePath() + "/Desktop" );
            dialog.setFileMode(QFileDialog::ExistingFiles);
            QStringList file_path_list = dialog.getOpenFileNames(this, "Select files which you want to add" );
            if (!file_path_list.isEmpty()) {

                ProcessingDialog md(twfolder, file_path_list, this);
                md.prepare_GUI_compression();
                md.exec();

                this->reload_archive();
            }
        }
        else if (itm->type() == 1002) {   // TreeWidgetFile
            TreeWidgetFile* twfile = static_cast<TreeWidgetFile*>(itm);
            QFileDialog dialog( this, "Select files which you want to add", QDir::homePath() + "/Desktop" );
            dialog.setFileMode(QFileDialog::ExistingFiles);
            QStringList file_path_list = dialog.getOpenFileNames(this, "Select files which you want to add" );
            if (!file_path_list.isEmpty()) {

                ProcessingDialog md(twfile, file_path_list, this);
                md.prepare_GUI_compression();
                md.exec();

                this->reload_archive();
            }
        }
    }
}


void ArchiveWindow::remove_selected_clicked()
{
    if (!ui->archiveWidget->selectedItems().empty()) {

        QMessageBox::StandardButton u_sure = QMessageBox::warning(this, QString::fromStdString("Removing files"),
                                                                QString::fromStdString("Are you sure you want to delete selected files and/or folders?"),
                                                                QMessageBox::Yes | QMessageBox::No);
        if (u_sure == QMessageBox::Yes) {
            QList<QTreeWidgetItem *> targets = ui->archiveWidget->selectedItems();

            for (auto& itm : targets ) if (itm == ui->archiveWidget->topLevelItem(0))
            {
                QMessageBox::StandardButton reply = QMessageBox::warning(this, QString::fromStdString("Deleting whole archive!"),
                                                                        QString::fromStdString("Are you sure you want to delete the whole archive?"),
                                                                        QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    this->archive_ptr->archive_file.close();
                    std::filesystem::remove(current_archive_path);
                    this->new_archive_model();
                }
                else if (reply == QMessageBox::No) {
                    return;
                }
                else assert(false);

                return;
            }

            std::vector<Folder*> folders;
            std::vector<File*> files;

            for (int i=0; i < targets.size(); ++i) {
                if ( targets[i]->type() == 1001 ) {
                    // 1001 == TreeWidgetFolder
                    folders.emplace_back( reinterpret_cast<TreeWidgetFolder*>( targets[i])->folder_ptr );
                    folders[folders.size()-1]->ptr_already_gotten = true;
                }
                else if ( targets[i]->type() == 1002 ) {
                    // 1002 == TreeWidgetFile
                    files.emplace_back( reinterpret_cast<TreeWidgetFile*>(targets[i])->file_ptr );
                    files[files.size()-1]->ptr_already_gotten = true;
                }
            }

            for (auto single_folder : folders) single_folder->get_ptrs( folders, files );
            for (auto single_file   : files  ) single_file->get_ptrs( files );


            std::fstream dst(temp_path, std::ios::binary | std::ios::out);
            assert(dst.is_open());

            dst.put(0);  // first bit is always 0x0, to make any location = 0 within the archive invalid, like nullptr or sth

            assert(archive_ptr->archive_file.is_open());

            this->archive_ptr->root_folder->copy_to_another_archive(this->archive_ptr->archive_file, dst, 0, 0);

            if (this->archive_ptr->archive_file.is_open()) this->archive_ptr->archive_file.close();
            if (dst.is_open()) dst.close();

            std::filesystem::copy_file(temp_path, archive_ptr->load_path, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::remove(temp_path);

            this->reload_archive();
        }
    }
}


void ArchiveWindow::create_empty_archive()
{
    new_archive_model();

    QString filter = "Archive (*.tk2k) ;; All Files (*.*)" ;
    QString Qfile_path = QFileDialog::getSaveFileName(this, "Select where you want your archive to be", QDir::homePath() + "/Desktop", filter );

    auto file_name = std::filesystem::path( Qfile_path.toStdString() ).filename();
    auto file_path = std::filesystem::path( Qfile_path.toStdString() ).remove_filename();
    if (!Qfile_path.isEmpty()) {
        if ( !file_name.has_extension() )  {
            file_name.concat( this->archive_ptr->extension );
        }

        this->current_archive_path = file_path.append(file_name.string());
        archive_ptr->build_empty_archive( file_name.string() );

        // this operation is so fast, there's no point in aborting it
        bool aborting_var = false;
        archive_ptr->save( current_archive_path, aborting_var );
        this->reload_archive();
    }
}


void ArchiveWindow::add_new_folder_clicked()
{
    bool aborting_var = false;

    if (!ui->archiveWidget->selectedItems().empty()) {
        auto itm = ui->archiveWidget->selectedItems()[0];

        bool not_canceled = false;

        QString folder_name =  QInputDialog::getText(this, "Name your folder", "Type folder's name here:", QLineEdit::Normal, QString(), &not_canceled);

        if (not_canceled) {
            if (itm->type() == 1001) {  //TreeWidgetFolder
                TreeWidgetFolder* twfolder = static_cast<TreeWidgetFolder*>(itm);
                // If folder is selected, add new folder as it's child
                if (!folder_name.isEmpty()) {

                    Folder* new_folder_ptr = twfolder->archive_ptr->add_folder_to_model( twfolder->folder_ptr, folder_name.toStdString() );

                    this->write_folder_to_current_archive( new_folder_ptr, aborting_var );
                }
                else {
                    folder_name = "new folder";
                    Folder* new_folder_ptr = twfolder->archive_ptr->add_folder_to_model(twfolder->folder_ptr, folder_name.toStdString());
                    this->write_folder_to_current_archive( new_folder_ptr, aborting_var );
                }
                // If file is selected, add new folder as it's parent's child
            } else if (itm->type() == 1002) {   //TreeWidgetFile
                TreeWidgetFile* twfile = static_cast<TreeWidgetFile*>(itm);

                if (!folder_name.isEmpty()) {

                    Folder* new_folder_ptr = twfile->archive_ptr->add_folder_to_model(twfile->file_ptr->parent_ptr, folder_name.toStdString());
                    this->write_folder_to_current_archive( new_folder_ptr, aborting_var );
                }
                else {
                    folder_name = "new folder";
                    Folder* new_folder_ptr = twfile->archive_ptr->add_folder_to_model(twfile->file_ptr->parent_ptr, folder_name.toStdString());
                    this->write_folder_to_current_archive( new_folder_ptr, aborting_var );
                }
            }
        }
    }
}


void ArchiveWindow::open_settings_dialog()
{
    SettingsDialog* sd = new SettingsDialog(this, this);
    connect(sd, &SettingsDialog::set_mainwindow_stylesheet, this, &ArchiveWindow::setStyleSheet);

    sd->exec();
    if (current_archive_path != "") reload_archive();
}


