#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowIcon( *(new QIcon(":/img/archive.png")));

    archive_ptr = new archive();

    ui->archiveWidget->setHeaderLabels(QStringList() << "File" << "Uncompressed Size" << "Compressed Size" << "Compression Ratio");
    ui->archiveWidget->setColumnWidth(0, 500);


}

MainWindow::~MainWindow()
{
    delete archive_ptr;
    delete ui;
}

void MainWindow::newArchiveModel() {
    ui->archiveWidget->clear();
    delete archive_ptr;
    archive_ptr = new archive();
}



void MainWindow::load_archive( std::string path_to_archive ) {
    this->newArchiveModel();
    this->current_archive_path = path_to_archive;
    this->archive_ptr->load( path_to_archive );

    ui->archiveWidget->addTopLevelItem( new TreeWidgetFolder( ui->archiveWidget->invisibleRootItem(), archive_ptr->archive_dir.get(), archive_ptr ) );
    this->ui->archiveWidget->expandAll();
}

void MainWindow::reload_archive() {
    this->newArchiveModel();
    this->load_archive( current_archive_path );
}
void MainWindow::write_file_to_current_archive( file* file_model ) {

    file_model->append_to_archive( this->archive_ptr->archive_file );
    this->reload_archive();
}

void MainWindow::write_folder_to_current_archive( folder* folder_model ) {
    folder_model->append_to_archive( this->archive_ptr->archive_file );
    this->reload_archive();
}

TreeWidgetFolder::TreeWidgetFolder(QTreeWidgetItem *parent, folder* ptr_to_folder, archive* ptr_to_archive  )
    : QTreeWidgetItem(parent, QStringList() << ptr_to_folder->name.c_str(), QTreeWidgetItem::UserType+1)
{
    this->folder_ptr = ptr_to_folder;
    this->archive_ptr = ptr_to_archive;
    this->setIcon(0, *(new QIcon(":/img/folder.png")));

    if (ptr_to_folder->sibling_ptr) parent->addChild( new TreeWidgetFolder( parent, ptr_to_folder->sibling_ptr.get(), ptr_to_archive ) );
    if (ptr_to_folder->child_dir_ptr) this->addChild( new TreeWidgetFolder( this, ptr_to_folder->child_dir_ptr.get(), ptr_to_archive ) );
    if (ptr_to_folder->child_file_ptr) this->addChild( new TreeWidgetFile( this, ptr_to_folder->child_file_ptr.get(), ptr_to_archive ) );

}



TreeWidgetFolder::TreeWidgetFolder(TreeWidgetFolder *parent, folder* ptr_to_folder, archive* ptr_to_archive  )
: QTreeWidgetItem(parent, QStringList() << ptr_to_folder->name.c_str(), QTreeWidgetItem::UserType+1)
{
    this->folder_ptr = ptr_to_folder;
    this->archive_ptr = ptr_to_archive;
    this->setIcon(0, *(new QIcon(":/img/folder.png")));


    if (ptr_to_folder->sibling_ptr) parent->addChild( new TreeWidgetFolder( parent, ptr_to_folder->sibling_ptr.get(), ptr_to_archive ) );

    if (ptr_to_folder->child_dir_ptr) this->addChild( new TreeWidgetFolder( this, ptr_to_folder->child_dir_ptr.get(), ptr_to_archive ) );
    if (ptr_to_folder->child_file_ptr) this->addChild( new TreeWidgetFile( this, ptr_to_folder->child_file_ptr.get(), ptr_to_archive ) );
}


void TreeWidgetFolder::unpack( std::string path_for_extraction ) {
    std::filesystem::path extraction_path( path_for_extraction );
    if ( !extraction_path.empty() ) {   // check if something is at least written
        std::fstream source( archive_ptr->load_path, std::ios::binary | std::ios::in);
        assert( source.is_open() );

        folder_ptr->unpack( path_for_extraction, source, true);
        source.close();
    }
}

void TreeWidgetFile::unpack( std::string path_for_extraction ) {
    std::filesystem::path extraction_path( path_for_extraction );
    if ( !extraction_path.empty() ) {   // check if something is at least written
        std::fstream source( archive_ptr->load_path, std::ios::binary | std::ios::in);
        assert( source.is_open() );

        file_ptr->unpack( path_for_extraction, source, false);
        source.close();
    }
}

TreeWidgetFile::TreeWidgetFile(TreeWidgetFolder *parent, file* ptr_to_file, archive* ptr_to_archive)
    : QTreeWidgetItem(parent, QStringList() << ptr_to_file->name.c_str() << QString::number(ptr_to_file->uncompressed_size) << QString::number(ptr_to_file->compressed_size/8) << QString::number((float)ptr_to_file->uncompressed_size/(float)(ptr_to_file->compressed_size/8)), QTreeWidgetItem::UserType+2 )
{
    this->file_ptr = ptr_to_file;
    this->archive_ptr = ptr_to_archive;
    this->setIcon(0, *(new QIcon(":/img/file.png")));
    if (ptr_to_file->sibling_ptr) parent->addChild( new TreeWidgetFile( parent, ptr_to_file->sibling_ptr.get(), ptr_to_archive ) );
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
    if (!ui->archiveWidget->selectedItems().empty()) {
        auto itm = ui->archiveWidget->selectedItems()[0];
        std::cout << itm->type() << std::endl;
        if (itm->type() == 1001) {  //TreeWidgetFolder
            TreeWidgetFolder* twfolder = static_cast<TreeWidgetFolder*>(itm);
            twfolder->unpack( ui->pathLineEdit->text().toStdString() );
        } else if (itm->type() == 1002) {   //TreeWidgetFile
            TreeWidgetFile* twfile = static_cast<TreeWidgetFile*>(itm);
            twfile->unpack( ui->pathLineEdit->text().toStdString() );
        }
    }
}

void MainWindow::on_buttonExtractAll_clicked()
{
    std::fstream source( this->archive_ptr->load_path );
    assert( source.is_open() );
    this->archive_ptr->unpack_whole_archive(ui->pathLineEdit->text().toStdString(), source );

    source.close();
}

void MainWindow::on_pathToolButton_clicked()
{

    QString filter = "Archive (*.tk2k) ;; All Files (*.*)" ;
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    QString path_to_folder = dialog.getExistingDirectory(this, "Choose where you want extracted files to go");
    ui->pathLineEdit->setText(path_to_folder);
}



void MainWindow::on_buttonAddNewFile_clicked()
{
    if (!ui->archiveWidget->selectedItems().empty()) {
        auto itm = ui->archiveWidget->selectedItems()[0];
        std::cout << itm->type() << std::endl;
        if (itm->type() == 1001) {  //TreeWidgetFolder
            TreeWidgetFolder* twfolder = static_cast<TreeWidgetFolder*>(itm);
            QString file_path = QFileDialog::getOpenFileName(this, "Open an archive", QDir::homePath() + "/Desktop" );
            if (!file_path.isEmpty()) {
                uint16_t flags = (1u<<15u)+(1u<<2u);    // temporary flags
                // std::unique_ptr<folder>* unique_this = archive_ptr->find_folder_in_archive(twfolder->folder_ptr->parent_ptr, twfolder->folder_ptr);
                file* new_file_ptr = twfolder->archive_ptr->add_file_to_archive_model( *(twfolder->folder_ptr), file_path.toStdString(), flags );
                this->write_file_to_current_archive( new_file_ptr );
            }
            //twfolder->
        } else if (itm->type() == 1002) {   //TreeWidgetFile
            TreeWidgetFile* twfile = static_cast<TreeWidgetFile*>(itm);
            QString file_path = QFileDialog::getOpenFileName(this, "Open an archive", QDir::homePath() + "/Desktop" );
            if (!file_path.isEmpty()) {
                uint16_t flags = (1u<<15u)+(1u<<2u);
                // std::unique_ptr<folder>* unique_this = archive_ptr->find_file_in_archive(twfile->file_ptr->parent_ptr, twfile->file_ptr);
                file* new_file_ptr = twfile->archive_ptr->add_file_to_archive_model( *(twfile->file_ptr->parent_ptr), file_path.toStdString(), flags );
                this->write_file_to_current_archive( new_file_ptr );
            }
            //twfile->
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
        archive_ptr->save( current_archive_path );
        this->reload_archive();

        //ui->archiveWidget->addTopLevelItem( new TreeWidgetFolder( ui->archiveWidget->invisibleRootItem(), archive_ptr->archive_dir.get(), archive_ptr ) );
    }
}

void MainWindow::on_buttonAddNewFolder_clicked()
{
    if (!ui->archiveWidget->selectedItems().empty()) {
        auto itm = ui->archiveWidget->selectedItems()[0];
        std::cout << itm->type() << std::endl;

        QString folder_name = QInputDialog::getText(this, "Name your folder", "Type folder's name here:");

        if (itm->type() == 1001) {  //TreeWidgetFolder
            TreeWidgetFolder* twfolder = static_cast<TreeWidgetFolder*>(itm);
            // If folder is selected, add new folder as it's child
            if (!folder_name.isEmpty()) {

                folder* new_folder_ptr = twfolder->archive_ptr->add_folder_to_model( twfolder->folder_ptr, folder_name.toStdString() );

                this->write_folder_to_current_archive( new_folder_ptr );
            }
            else {  // ZACHODZI ROWNIEZ GDY SIE ANULUJE
                folder_name = "new folder";
                folder* new_folder_ptr = twfolder->archive_ptr->add_folder_to_model(twfolder->folder_ptr, folder_name.toStdString());
                this->write_folder_to_current_archive( new_folder_ptr );
            }
            // If file is selected, add new folder as it's parent's child
        } else if (itm->type() == 1002) {   //TreeWidgetFile
            TreeWidgetFile* twfile = static_cast<TreeWidgetFile*>(itm);

            if (!folder_name.isEmpty()) {

                folder* new_folder_ptr = twfile->archive_ptr->add_folder_to_model(twfile->file_ptr->parent_ptr, folder_name.toStdString());
                this->write_folder_to_current_archive( new_folder_ptr );
            }
            else {  // ZACHODZI ROWNIEZ GDY SIE ANULUJE
                folder_name = "new folder";
                folder* new_folder_ptr = twfile->archive_ptr->add_folder_to_model(twfile->file_ptr->parent_ptr, folder_name.toStdString());
                this->write_folder_to_current_archive( new_folder_ptr );
            }
        }
    }
}




































