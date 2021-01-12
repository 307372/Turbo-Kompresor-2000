#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"

#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowIcon( *(new QIcon(":/img/archive.png")));

    archive_ptr = new archive();

    ui->archiveWidget->setColumnWidth(0, 300);
    ui->archiveWidget->setSelectionMode( QAbstractItemView::SelectionMode::ExtendedSelection );
    config_ptr = new Config();

    QAction *settingsAction = ui->menubar->addAction("Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettingsDialog);


    // loading stylesheets (if they exist)
    QFile style_file_dark(":/dark.qss");
    if (style_file_dark.exists()) {
        style_file_dark.open(QFile::ReadOnly | QFile::Text);
        QTextStream stream_dark(&style_file_dark);
        style_dark = stream_dark.readAll();
    }
    else std::cout << "Dark mode file does not exist!" << std::endl;

    QFile style_file_light(":/light.qss");
    if (style_file_light.exists()) {
        style_file_light.open(QFile::ReadOnly | QFile::Text);
        QTextStream stream_light(&style_file_light);
        style_light = stream_light.readAll();
    }
    else std::cout << "Light mode file does not exist!" << std::endl;

    if (config_ptr->get_dark_mode()) this->setStyleSheet(style_dark);
    else this->setStyleSheet(style_light);
}

MainWindow::~MainWindow()
{
    if (archive_ptr != nullptr) delete archive_ptr;
    if (ui          != nullptr) delete ui;
    if (config_ptr  != nullptr) delete config_ptr;
}

void MainWindow::newArchiveModel() {
    ui->archiveWidget->clear();
    if (archive_ptr != nullptr) delete archive_ptr;
    archive_ptr = new archive();
}



void MainWindow::load_archive( std::string path_to_archive ) {
    this->newArchiveModel();
    this->current_archive_path = path_to_archive;
    this->archive_ptr->load( path_to_archive );

    ui->archiveWidget->addTopLevelItem( new TreeWidgetFolder( ui->archiveWidget->invisibleRootItem(), archive_ptr->archive_dir.get(), archive_ptr, config_ptr->get_filesize_scaling() ) );
    this->ui->archiveWidget->expandAll();

    this->ui->archiveWidget->sortByColumn(0, Qt::SortOrder::AscendingOrder);
}

void MainWindow::reload_archive() {
    this->newArchiveModel();
    this->load_archive( current_archive_path );
}

void MainWindow::write_file_to_current_archive( file* file_ptr, bool& aborting_var ) {

    file_ptr->append_to_archive( this->archive_ptr->archive_file, aborting_var );

}

void MainWindow::write_folder_to_current_archive( folder* folder_model, bool& aborting_var ) {
    folder_model->append_to_archive( this->archive_ptr->archive_file, aborting_var );
    this->reload_archive();
}

TreeWidgetFolder::TreeWidgetFolder(QTreeWidgetItem *parent, folder* ptr_to_folder, archive* ptr_to_archive, bool filesize_scaled  )
: QTreeWidgetItem(parent, QStringList() << ptr_to_folder->name.c_str(), QTreeWidgetItem::UserType+1)
{
    this->folder_ptr = ptr_to_folder;
    this->archive_ptr = ptr_to_archive;
    this->setIcon(0, *(new QIcon(":/img/folder.png")));

    if (ptr_to_folder->sibling_ptr) parent->addChild( new TreeWidgetFolder( parent, ptr_to_folder->sibling_ptr.get(), ptr_to_archive, filesize_scaled ) );
    if (ptr_to_folder->child_dir_ptr) this->addChild( new TreeWidgetFolder( this, ptr_to_folder->child_dir_ptr.get(), ptr_to_archive, filesize_scaled ) );
    if (ptr_to_folder->child_file_ptr) this->addChild( new TreeWidgetFile( this, ptr_to_folder->child_file_ptr.get(), ptr_to_archive, filesize_scaled ) );

}



TreeWidgetFolder::TreeWidgetFolder(TreeWidgetFolder *parent, folder* ptr_to_folder, archive* ptr_to_archive, bool filesize_scaled  )
: QTreeWidgetItem(parent, QStringList() << ptr_to_folder->name.c_str(), QTreeWidgetItem::UserType+1)
{
    this->folder_ptr = ptr_to_folder;
    this->archive_ptr = ptr_to_archive;
    this->setIcon(0, *(new QIcon(":/img/folder.png")));


    if (ptr_to_folder->sibling_ptr) parent->addChild( new TreeWidgetFolder( parent, ptr_to_folder->sibling_ptr.get(), ptr_to_archive, filesize_scaled ) );

    if (ptr_to_folder->child_dir_ptr) this->addChild( new TreeWidgetFolder( this, ptr_to_folder->child_dir_ptr.get(), ptr_to_archive, filesize_scaled ) );
    if (ptr_to_folder->child_file_ptr) this->addChild( new TreeWidgetFile( this, ptr_to_folder->child_file_ptr.get(), ptr_to_archive, filesize_scaled ) );
}


void TreeWidgetFolder::unpack( std::string path_for_extraction, bool& aborting_var ) {
    std::filesystem::path extraction_path( path_for_extraction );
    if ( !extraction_path.empty() ) {   // check if something is at least written
        std::fstream source( archive_ptr->load_path, std::ios::binary | std::ios::in);
        assert( source.is_open() );

        folder_ptr->unpack( path_for_extraction, source, aborting_var, true );
        source.close();
    }
}

void TreeWidgetFile::unpack( std::string path_for_extraction, bool& aborting_var ) {
    std::filesystem::path extraction_path( path_for_extraction );
    if ( !extraction_path.empty() ) {   // check if something is at least written
        std::fstream source( archive_ptr->load_path, std::ios::binary | std::ios::in);
        assert( source.is_open() );

        file_ptr->unpack( path_for_extraction, source, aborting_var, false );
        source.close();
    }
}

TreeWidgetFile::TreeWidgetFile(TreeWidgetFolder *parent, file* ptr_to_file, archive* ptr_to_archive, bool filesize_scaled)
: QTreeWidgetItem(parent, QStringList() << ptr_to_file->name.c_str() << QString::fromStdString( ptr_to_file->get_uncompressed_filesize_str(filesize_scaled)) << QString::fromStdString(ptr_to_file->get_compressed_filesize_str(filesize_scaled)) << QString::number((float)ptr_to_file->uncompressed_size/(float)(ptr_to_file->compressed_size)), QTreeWidgetItem::UserType+2 )
{
    this->setTextAlignment(1, Qt::AlignRight);
    this->setTextAlignment(2, Qt::AlignRight);
    this->setTextAlignment(3, Qt::AlignRight);
    this->file_ptr = ptr_to_file;
    this->archive_ptr = ptr_to_archive;
    this->setIcon(0, *(new QIcon(":/img/file.png")));
    if (ptr_to_file->sibling_ptr) parent->addChild( new TreeWidgetFile( parent, ptr_to_file->sibling_ptr.get(), ptr_to_archive, filesize_scaled ) );
}






void MainWindow::on_actionNew_archive_triggered()
{
    createEmptyArchive();
}


void MainWindow::on_actionOpen_archive_triggered()
{
    QString filter = "Archive (*.tk2k) ;; All Files (*.*)" ;
    QString file_path = QFileDialog::getOpenFileName(this, "Open an archive", QDir::homePath() + "/Desktop", filter );
    if (!file_path.isEmpty()) {
        load_archive( file_path.toStdString() );
    }
}

void MainWindow::on_buttonExtractSelected_clicked()
{

    auto selected = ui->archiveWidget->selectedItems();

    if (!selected.empty()) {

        std::vector<QTreeWidgetItem*> extraction_targets;
        for (int32_t i=0; i < selected.size(); ++i) extraction_targets.push_back( selected[i] );

        MyDialog md( extraction_targets,  this);
        md.prepare_GUI_decompression();
        md.exec();

        this->reload_archive();
    }
}

void MainWindow::on_buttonExtractAll_clicked()
{
    // std::cout << ui->archiveWidget->topLevelItem(0)->text(0).toStdString() << std::endl;
    std::cout << archive_ptr->archive_dir->name << std::endl;
    // auto selected = ui->archiveWidget->selectedItems();

    // if (!selected.empty()) {

        std::vector<QTreeWidgetItem*> extraction_targets;
        extraction_targets.push_back(ui->archiveWidget->topLevelItem(0));

        MyDialog md( extraction_targets,  this);
        md.prepare_GUI_decompression();
        md.exec();

        this->reload_archive();
    //}
}





void MainWindow::on_buttonAddNewFile_clicked()
{

    if (!ui->archiveWidget->selectedItems().empty()) {
        auto itm = ui->archiveWidget->selectedItems()[0];
        std::cout << itm->type() << std::endl;
        if (itm->type() == 1001) {  // TreeWidgetFolder
            TreeWidgetFolder* twfolder = static_cast<TreeWidgetFolder*>(itm);
            QFileDialog dialog( this, "Select files which you want to add", QDir::homePath() + "/Desktop" );
            dialog.setFileMode(QFileDialog::ExistingFiles);
            QStringList file_path_list = dialog.getOpenFileNames(this, "Select files which you want to add" );
            if (!file_path_list.isEmpty()) {

                MyDialog md(twfolder, file_path_list, this);
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

                MyDialog md(twfile, file_path_list, this);
                md.prepare_GUI_compression();
                md.exec();

                this->reload_archive();
            }
        }
    }
}

void MainWindow::on_buttonRemoveSelected_clicked()
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
                    this->newArchiveModel();
                }
                else if (reply == QMessageBox::No) {
                    return;
                }
                else assert(false);

                return;
            }

            std::vector<folder*> folders;
            std::vector<file*> files;

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
                else assert(false); // this should never happen
            }

            for (auto single_folder : folders) single_folder->get_ptrs( folders, files );
            for (auto single_file   : files  ) single_file->get_ptrs( files );


            std::fstream dst(temp_path, std::ios::binary | std::ios::out);
            assert(dst.is_open());

            dst.put(0);  // first bit is always 0x0, to make any location = 0 within the archive invalid, like nullptr or sth

            assert(archive_ptr->archive_file.is_open());

            this->archive_ptr->archive_dir->copy_to_another_file(this->archive_ptr->archive_file, dst, 0, 0);

            if (this->archive_ptr->archive_file.is_open()) this->archive_ptr->archive_file.close();
            if (dst.is_open()) dst.close();
            // std::filesystem::copy_file(parent_mw->archive_ptr->load_path, temp_path, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::copy_file(temp_path, archive_ptr->load_path, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::remove(temp_path);

            this->reload_archive();
        }
    }
}





void MainWindow::createEmptyArchive() {
    newArchiveModel();

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

void MainWindow::on_buttonAddNewFolder_clicked()
{


    bool aborting_var = false;

    if (!ui->archiveWidget->selectedItems().empty()) {
        auto itm = ui->archiveWidget->selectedItems()[0];
        std::cout << itm->type() << std::endl;

        bool not_canceled = false;

        QString folder_name =  QInputDialog::getText(this, "Name your folder", "Type folder's name here:", QLineEdit::Normal, QString(), &not_canceled);

        if (not_canceled) {
            if (itm->type() == 1001) {  //TreeWidgetFolder
                TreeWidgetFolder* twfolder = static_cast<TreeWidgetFolder*>(itm);
                // If folder is selected, add new folder as it's child
                if (!folder_name.isEmpty()) {

                    folder* new_folder_ptr = twfolder->archive_ptr->add_folder_to_model( twfolder->folder_ptr, folder_name.toStdString() );

                    this->write_folder_to_current_archive( new_folder_ptr, aborting_var );
                }
                else {
                    folder_name = "new folder";
                    folder* new_folder_ptr = twfolder->archive_ptr->add_folder_to_model(twfolder->folder_ptr, folder_name.toStdString());
                    this->write_folder_to_current_archive( new_folder_ptr, aborting_var );
                }
                // If file is selected, add new folder as it's parent's child
            } else if (itm->type() == 1002) {   //TreeWidgetFile
                TreeWidgetFile* twfile = static_cast<TreeWidgetFile*>(itm);

                if (!folder_name.isEmpty()) {

                    folder* new_folder_ptr = twfile->archive_ptr->add_folder_to_model(twfile->file_ptr->parent_ptr, folder_name.toStdString());
                    this->write_folder_to_current_archive( new_folder_ptr, aborting_var );
                }
                else {
                    folder_name = "new folder";
                    folder* new_folder_ptr = twfile->archive_ptr->add_folder_to_model(twfile->file_ptr->parent_ptr, folder_name.toStdString());
                    this->write_folder_to_current_archive( new_folder_ptr, aborting_var );
                }
            }
        }
    }
}

void MainWindow::openSettingsDialog()
{
    SettingsDialog* sd = new SettingsDialog(this, this);
    connect(sd, &SettingsDialog::set_mainwindow_stylesheet, this, &MainWindow::setStyleSheet);

    sd->exec();
    if (current_archive_path != "") reload_archive();
}




bool TreeWidgetFile::operator<(const QTreeWidgetItem &other)const {
    //1001 - TreeWidgetFolder
    //1002 - TreeWidgetFile

    Qt::SortOrder order = this->treeWidget()->header()->sortIndicatorOrder();


    if (order == Qt::AscendingOrder and other.type() == 1001) return false; // folders always go before files
    if (order == Qt::DescendingOrder and other.type() == 1001) return true; // folders always go before files


    int column = treeWidget()->sortColumn();
    switch( column )
    {
    case 0: // sort by name
            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
            break;
    case 1: // sort by size
        if (this->type() == other.type() ) {
            if ( this->type() == 1001 ) { // both this and other are TreeWidgetFolders
                if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
                else return false;
            }
            else {  // both this and other are TreeWidgetFiles
                TreeWidgetFile* twfile = (TreeWidgetFile*)(&other);
                if ( this->file_ptr->uncompressed_size < twfile->file_ptr->uncompressed_size ) return true;
                else return false;
            }
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    case 2: // sort by compressed size
        if (this->type() == other.type() ) {
            if ( this->type() == 1001 ) { // both this and other are TreeWidgetFolders
                if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
                else return false;
            }
            else {  // both this and other are TreeWidgetFiles
                TreeWidgetFile* twfile = (TreeWidgetFile*)(&other);
                if ( this->file_ptr->compressed_size < twfile->file_ptr->compressed_size ) return true;
                else return false;
            }
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    case 3: // sort by compression ratio
        if (this->type() == other.type() ) {
            if ( this->type() == 1001 ) { // both this and other are TreeWidgetFolders
                if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
                else return false;
            }
            else {  // both this and other are TreeWidgetFiles
                TreeWidgetFile* twfile = (TreeWidgetFile*)(&other);
                if ( this->file_ptr->uncompressed_size/this->file_ptr->compressed_size < twfile->file_ptr->uncompressed_size / twfile->file_ptr->compressed_size )
                    return true;
                else return false;
            }
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    default:
        throw NotImplementedException("Something went terribly wrong while sorting.");
    }
}

bool TreeWidgetFolder::operator<(const QTreeWidgetItem &other)const {
    //1001 - TreeWidgetFolder
    //1002 - TreeWidgetFile

    Qt::SortOrder order = this->treeWidget()->header()->sortIndicatorOrder();

    if (order == Qt::AscendingOrder and other.type() == 1002) return true; // folders always go before files
    if (order == Qt::AscendingOrder and other.type() == 1002) return false; // folders always go before files


    int column = treeWidget()->sortColumn();
    switch( column )
    {
    case 0: // sort by name
            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
            break;
    case 1: // sort by size
        if (this->type() == other.type() ) {// both this and other are TreeWidgetFolders

            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
            break;
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    case 2: // sort by compressed size
        if (this->type() == other.type() ) {// both this and other are TreeWidgetFolders

            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    case 3: // sort by compression ratio
        if (this->type() == other.type() ) {// both this and other are TreeWidgetFolders

            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
            break;
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    default:
        throw NotImplementedException("Something went terribly wrong while sorting.");
    }
}





























