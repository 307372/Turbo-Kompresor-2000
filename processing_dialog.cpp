#include "processing_dialog.h"

#include <cassert>

#include "ui_processing_dialog.h"
#include "misc/processing_helpers.h"
#include "misc/custom_tree_widget_items.h"

ProcessingDialog::ProcessingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProcessingDialog),
    parent_mw(reinterpret_cast<ArchiveWindow*>(parent)),
    twfolder_ptr(nullptr),
    twfile_ptr(nullptr),
    parent_usertype(-1)
{
    ui->setupUi(this);
    this->setModal(true);
}


ProcessingDialog::ProcessingDialog(TreeWidgetFolder* twfolder, QStringList file_path_list, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProcessingDialog),
    parent_mw(reinterpret_cast<ArchiveWindow*>(parent)),
    twfolder_ptr(twfolder),
    twfile_ptr(nullptr),
    parent_usertype(1001),
    paths_to_files(file_path_list)
{
    ui->setupUi(this);
    this->setModal(true);
}


ProcessingDialog::ProcessingDialog(TreeWidgetFile* twfile, QStringList file_path_list, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProcessingDialog),
    parent_mw(reinterpret_cast<ArchiveWindow*>(parent)),
    twfolder_ptr(nullptr),
    twfile_ptr(twfile),
    parent_usertype(1002),
    paths_to_files(file_path_list)
{
    ui->setupUi(this);
    this->setModal(true);
}


ProcessingDialog::ProcessingDialog(std::vector<QTreeWidgetItem*> extraction_targets, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProcessingDialog),
    parent_mw(reinterpret_cast<ArchiveWindow*>(parent)),
    twfolder_ptr(nullptr),
    twfile_ptr(nullptr),
    parent_usertype(997),       // should never be used
    extraction_targets(extraction_targets)
{
    ui->setupUi(this);
    this->setModal(true);
}


ProcessingDialog::~ProcessingDialog()
{
    delete ui;
    if (my_thread != nullptr) {
        while(!my_thread->wait(50));   // check if the thread finished working every 50ms

        delete my_thread;
    }
    if (th_compression   != nullptr) delete th_compression;
    if (th_decompression != nullptr) delete th_decompression;
}


void ProcessingDialog::closeEvent(QCloseEvent *event) {

    if (my_thread != nullptr) {                 // if processing has even been started

        if ( !this->my_thread->isFinished() ) { // make sure processing is finished

            if (this->ui->stackedWidget->currentWidget() == this->ui->page_compression_progress) {
                QString title = "Are you sure you want to cancel this?";
                QString text = "All changes will be reverted!";
                QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons( QMessageBox::Yes | QMessageBox::No );

                QMessageBox::StandardButton reply = QMessageBox::question(this, title, text, buttons);

                if (reply == QMessageBox::Yes) {
                    emit abort_processing();
                    if (my_thread != nullptr) {
                        while(!my_thread->wait(50));   // check if the thread finished working every 50ms
                        this->slot_processing_finished(false);    // finished unsuccessfully
                        QMessageBox::information(this, QString("Failed!"), QString("Processing failed!"));
                    }
                    event->accept();
                }
                else if (reply == QMessageBox::No) event->ignore();

            }
            else if (this->ui->stackedWidget->currentWidget() == this->ui->page_compression_settings) {
                event->accept();
            }
            else if (this->ui->stackedWidget->currentWidget() == this->ui->page_decompression_settings) {
                event->accept();

            }
            else assert(false);
        }
        else {
            event->accept();
        }
    }
}


uint16_t ProcessingDialog::get_flags() {
    std::bitset<16> flags(0);
    flags[1] = ui->checkBox_MTF->isChecked();   // Move-to-front
    flags[2] = ui->checkBox_RLE->isChecked();   // Run-length encoding



    switch (ui->comboBox_entropy_coding->currentIndex()) {
    case 0:     // None
        break;

    case 1:     // Arithmetic coding (naive model)
        flags[3] = true;
        break;

    case 2:     // Arithmetic coding (better model)
        flags[4] = true;
        break;
    }

    if (ui->groupBox_BWT->isChecked()) { // Burrows-Wheeler transform
        switch (ui->comboBox_BWT->currentIndex()) {
        case 0: {

            flags[0] = true;    // BWT (DC3)
            flags[9] = true;    // enforces 8 MiB blocks
            break;
        }

        case 1:
            flags[7] = true;    // BWT (divsufsort)
            break;

        }
    }

    switch (ui->comboBox_checksum->currentIndex()) {
    case 0:     // SHA-1
        flags[15] = true;
        break;

    case 1:     // CRC-32
        flags[14] = true;
        break;

    case 2:     // SHA-256
        flags[13] = true;
        break;
    }


    return (uint16_t)flags.to_ulong();
}


void ProcessingDialog::prepare_GUI_compression() {
    this->setWindowTitle( "Compression" );
    compression = true;
    th_decompression = nullptr;
    th_compression   = nullptr;

    ui->stackedWidget->setCurrentIndex(1);
    ui->progressBarFile->setValue(0);
    ui->progressBarTotal->setValue(0);
    ui->buttonCancel->setDisabled(false);
    ui->buttonFinish->setDisabled(true);
}


void ProcessingDialog::prepare_GUI_decompression() {
    this->setWindowTitle( "Decompression" );
    compression = false;
    th_decompression = nullptr;
    th_compression   = nullptr;

    ui->stackedWidget->setCurrentIndex(2);
    ui->progressBarFile->setValue(0);
    ui->progressBarTotal->setValue(0);
    ui->buttonCancel->setDisabled(false);
    ui->buttonFinish->setDisabled(true);
    ui->lineEdit_decompression_path->setText( parent_mw->config_ptr->get_extraction_path().c_str() );
}


void ProcessingDialog::on_buttonCancel_clicked()
{
    this->close();
}


void ProcessingDialog::on_pushButton_abort_clicked()
{
    this->close();
}


void ProcessingDialog::correct_duplicate_names(File* target_file, Folder* parent_folder)
{
    std::vector<std::string> name_list;
    if (parent_folder->child_file_ptr == nullptr) return;
    else {
        File* file_in_dir = parent_folder->child_file_ptr.get();
        name_list.push_back(file_in_dir->name);

        while (file_in_dir->sibling_ptr != nullptr)
        {
            file_in_dir = file_in_dir->sibling_ptr.get();
            name_list.push_back(file_in_dir->name);
        }
        std::string new_name = std::filesystem::path(target_file->name).stem().string() + " (";
        std::string extension = std::filesystem::path(target_file->name).extension().string();
        uint64_t duplicate_counter = 0;
        bool unique_name = false;

        while( !unique_name ) {
            bool found = false;
            uint64_t names_like_current = 0;
            for (auto current_name : name_list)
            {
                if (duplicate_counter == 0)
                {
                    if (target_file->name == current_name)
                    {
                        names_like_current++;
                        if (names_like_current > 1) {
                            duplicate_counter++;
                            found = true;
                            break;
                        }
                    }
                }
                else
                {
                    if(new_name + std::to_string(duplicate_counter) + ")" + extension == current_name)
                    {
                        duplicate_counter++;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) unique_name = true;
        }

        if (duplicate_counter > 0)
        {
            target_file->name = new_name + std::to_string(duplicate_counter) + ")" + extension;
            target_file->name_length = target_file->name.length();
        }
    }

}


void ProcessingDialog::on_pushButton_compress_clicked()
{
    // before we begin, let's create a temp file, on which we will be working
    std::filesystem::copy_file(parent_mw->archive_ptr->load_path, parent_mw->temp_path, std::filesystem::copy_options::overwrite_existing);

    ui->stackedWidget->setCurrentIndex(0);

    uint16_t flags = this->get_flags();
    std::bitset<16> bin_flags(flags);

    std::vector<File*> list_of_files;

    if (parent_usertype == 1001) {  // TreeWidgetFolder
        for (auto &file_path : paths_to_files) {
            File* new_file_ptr = twfolder_ptr->archive_ptr->add_file_to_archive_model( *(twfolder_ptr->folder_ptr), file_path.toStdString(), flags );
            correct_duplicate_names(new_file_ptr, twfolder_ptr->folder_ptr);
            list_of_files.push_back( new_file_ptr );
        }
    }
    else if (parent_usertype == 1002) { // TreeWidgetFile
        for (auto &file_path : paths_to_files) {
            File* new_file_ptr = twfile_ptr->archive_ptr->add_file_to_archive_model( *(twfile_ptr->file_ptr->parent_ptr), file_path.toStdString(), flags );

            correct_duplicate_names(new_file_ptr, twfile_ptr->file_ptr->parent_ptr);
            list_of_files.push_back( new_file_ptr );
        }
    }
    else assert(false); // should never happen

    progress_step_value = 0;
    th_compression = new CompressionObject( list_of_files, &progress_step_value, &progressBarStepMax, parent_mw->temp_path );
    my_thread = new QThread;
    th_compression->moveToThread(my_thread);

    connect( my_thread, &QThread::started , th_compression, &CompressionObject::startProcessing );
    connect( th_compression, &CompressionObject::progressNextFile, ui->progressBarTotal, &QProgressBar::setValue );
    connect( th_compression, &CompressionObject::progressNextStep, ui->progressBarFile,  &QProgressBar::setValue );
    connect( th_compression, &CompressionObject::setFilePathLabel, ui->label_currentfile_value, &QLabel::setText );
    connect( th_compression, &CompressionObject::processingFinished, this,    &ProcessingDialog::slot_processing_finished );
    connect( this, &ProcessingDialog::abort_processing, th_compression, &CompressionObject::abortProcessing, Qt::ConnectionType::DirectConnection );
    connect( th_compression, &CompressionObject::displayFailedFiles, this, &ProcessingDialog::displayFailedFiles );
    start_processing_timer();
    my_thread->start();
}


void ProcessingDialog::onProgressNextFile(double value) {
    ui->progressBarTotal->setValue( (int)value );
    ui->progressBarFile->setValue(0);
}


void ProcessingDialog::onProgressNextStep(double value) {
    ui->progressBarFile->setValue( round(value*100) );
}


void ProcessingDialog::slot_processing_finished(bool successful) {
    timer_elapsed_time->stop();

    if (successful) {
        if (compression) {
            if (parent_mw->archive_ptr->archive_file.is_open()) parent_mw->archive_ptr->archive_file.close();
            // replacing archive with its updated version
            bool result = std::filesystem::copy_file(parent_mw->temp_path, parent_mw->archive_ptr->load_path, std::filesystem::copy_options::overwrite_existing);
            assert( result );   // will fail if anything goes wrong
            // removing temporary file
            result = std::filesystem::remove(parent_mw->temp_path);
            assert( result );

            // reopening fstream to archive
            parent_mw->archive_ptr->archive_file.open( parent_mw->archive_ptr->load_path, std::ios::binary | std::ios::in | std::ios::out );
        }
        QMessageBox::information(this, QString("Success!"), QString("Processing succeeded!"));

    }
    else {
        if (compression) {
            std::filesystem::remove(parent_mw->temp_path);
        }
    }
    ui->buttonFinish->setDisabled(false);
    ui->buttonCancel->setDisabled(true);
}


void ProcessingDialog::on_buttonDecompressionStart_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);

    bool just_files = ui->checkBox_decompression_just_files->isChecked();
    bool confirm_integrity = ui->checkBox_decompression_integrity_validation->isChecked();

    if (ui->checkBox_decompression_remember_path) {
        if (parent_mw->config_ptr->get_extraction_path() != std::filesystem::path( ui->lineEdit_decompression_path->text().toStdString() ))
            parent_mw->config_ptr->set_extraction_path( std::filesystem::path( ui->lineEdit_decompression_path->text().toStdString() ) );
    }

    std::filesystem::path target_path = ui->lineEdit_decompression_path->text().toStdString();


    std::vector<Folder*> folders;
    std::vector<File*> files;

    for (uint32_t i=0; i < extraction_targets.size(); ++i) {
        if ( extraction_targets[i]->type() == 1001 ) {
            // 1001 == TreeWidgetFolder
            folders.emplace_back( reinterpret_cast<TreeWidgetFolder*>( extraction_targets[i])->folder_ptr );
            folders[folders.size()-1]->ptr_already_gotten = true;
        }
        else if ( extraction_targets[i]->type() == 1002 ) {
            // 1002 == TreeWidgetFile
            files.emplace_back( reinterpret_cast<TreeWidgetFile*>(extraction_targets[i])->file_ptr );
            files[files.size()-1]->ptr_already_gotten = true;
        }
        else assert(false); // this should never happen
    }

    for (auto single_folder : folders) single_folder->get_ptrs( folders, files );
    for (auto single_file   : files  ) single_file->get_ptrs( files );



    if ( just_files ) {
        std::filesystem::create_directories( target_path ); // making sure extraction path actually exists
        for ( auto single_file : files ) {
            single_file->set_path(target_path, false);
        }
    }
    else {
        // setting paths for selected files
        // needs to be done after getting a vector of files for extraction,
        // since extraction_targets is a vector ordered by when elements were selected, and not by how deep is the file in the structure

        for (auto single_file : files)  // marking every parent folder of every selected file to also be created
        {
            Folder* nth_parent = single_file->parent_ptr;
            while (nth_parent)
            {
                nth_parent->ptr_already_gotten = true;
                nth_parent = nth_parent->parent_ptr;
            }
        }

        parent_mw->archive_ptr->root_folder->set_path( target_path, true );
        // creating all necessary folders
        for (auto single_folder : folders) std::filesystem::create_directories( single_folder->path );
        for (auto single_file   : files  ) std::filesystem::create_directories( single_file->path   );
    }

    // now that we've got those ptrs, this property needs to be reset, in case it's used later
    for (auto single_folder : folders) single_folder->ptr_already_gotten = false;
    for (auto single_file   : files  ) single_file->ptr_already_gotten = false;

    progress_step_value = 0;
    th_decompression = new DecompressionObject( files, parent_mw->archive_ptr->archive_file, confirm_integrity, &progress_step_value, &progressBarStepMax );

    my_thread = new QThread;
    th_decompression->moveToThread( my_thread );


    connect( my_thread, &QThread::started , th_decompression, &DecompressionObject::startProcessing );
    connect( th_decompression, &DecompressionObject::ProgressNextFile, ui->progressBarTotal, &QProgressBar::setValue );
    connect( th_decompression, &DecompressionObject::ProgressNextStep, ui->progressBarFile,  &QProgressBar::setValue );
    connect( th_decompression, &DecompressionObject::setFilePathLabel, ui->label_currentfile_value, &QLabel::setText );
    connect( th_decompression, &DecompressionObject::processingFinished, this,    &ProcessingDialog::slot_processing_finished );
    connect( this, &ProcessingDialog::abort_processing, th_decompression, &DecompressionObject::abortProcessing, Qt::ConnectionType::DirectConnection );
    connect( th_decompression, &DecompressionObject::displayFailedFiles, this, &ProcessingDialog::displayFailedFiles );

    start_processing_timer();
    my_thread->start();
}


void ProcessingDialog::displayFailedFiles(QStringList failed_paths)
{
    QString contents;
    for (int32_t i=0; i < failed_paths.size(); ++i) {
        contents.append(failed_paths[i] + "\n");
    }
    QMessageBox::information(this, "Failed files", contents);
}


void ProcessingDialog::on_buttonDecompressionAbort_clicked()
{
    this->close();
}


void ProcessingDialog::set_progress_file_value(uint16_t value) {
    ui->progressBarFile->setValue( value );
}


void ProcessingDialog::on_buttonFinish_clicked()
{
    this->close();
}


void ProcessingDialog::start_processing_timer() {
    time_start = std::chrono::high_resolution_clock::now();
    timer_elapsed_time = new QTimer(this);
    connect( timer_elapsed_time, &QTimer::timeout, this, &ProcessingDialog::onTimerTimeout );

    timer_elapsed_time->start(100);  // refresh every 16 ms or slightly more often than 60 times a second

}


void ProcessingDialog::onTimerTimeout() {

    uint32_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::high_resolution_clock::now() - time_start ).count();
    elapsed = floor(elapsed/100.0);
    uint16_t tenths_of_second = elapsed % 10;

    elapsed = floor(elapsed/10.0);
    uint16_t seconds = elapsed % 60;

    elapsed = floor(elapsed/60.0);
    uint16_t minutes = elapsed % 60;

    uint16_t hours = floor(elapsed/60.0);

    QString formatted_time = QString::number(hours) + ':';

    if ( minutes < 10 ) formatted_time += '0' + QString::number(minutes) + ':';
    else formatted_time += QString::number(minutes) + ':';

    if ( seconds < 10 ) formatted_time += '0' + QString::number(seconds) + '.';
    else formatted_time += QString::number(seconds) + '.';
    formatted_time += QString::number(tenths_of_second);


    ui->label_duration_value->setText( formatted_time );
    if (progressBarStepMax == 0)
        progressBarStepMax = 1;

    ui->progressBarFile->setValue( round(progress_step_value*100/progressBarStepMax) );
}


void ProcessingDialog::on_button_decompression_path_dialog_clicked()
{
    QString filter = "All Files (*.*)" ;
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::Directory);
    QString path_to_folder = dialog.getExistingDirectory(this, "Choose where you want extracted files to go");
    ui->lineEdit_decompression_path->setText(path_to_folder);
}

