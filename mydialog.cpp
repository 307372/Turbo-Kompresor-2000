#include "mydialog.h"
#include "ui_mydialog.h"


// compression dialog constructors

MyDialog::MyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyDialog)
{
    ui->setupUi(this);
    this->setModal(true);
    parent_mw = reinterpret_cast<MainWindow*>(parent);

    this->parent_usertype = -1;
    this->twfolder_ptr = nullptr;
    this->twfile_ptr = nullptr;
}

MyDialog::MyDialog(TreeWidgetFolder* twfolder, QStringList file_path_list, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyDialog)
{
    ui->setupUi(this);
    this->setModal(true);
    parent_mw = reinterpret_cast<MainWindow*>(parent);

    this->parent_usertype = 1001;
    this->twfolder_ptr = twfolder;
    this->twfile_ptr = nullptr;
    this->paths_to_files = file_path_list;
}

MyDialog::MyDialog(TreeWidgetFile* twfile, QStringList file_path_list, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyDialog)
{
    ui->setupUi(this);
    this->setModal(true);
    parent_mw = reinterpret_cast<MainWindow*>(parent);

    this->parent_usertype = 1002;
    this->twfolder_ptr = nullptr;
    this->twfile_ptr = twfile;
    this->paths_to_files = file_path_list;
}

// decompression dialog constructors

MyDialog::MyDialog(std::vector<QTreeWidgetItem*> extraction_targets, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyDialog)
{
    this->extraction_targets = extraction_targets;
    ui->setupUi(this);
    this->setModal(true);
    parent_mw = reinterpret_cast<MainWindow*>(parent);

    this->parent_usertype = 997;    // should never be used
    this->twfolder_ptr = nullptr;
    this->twfile_ptr = nullptr;
}




MyDialog::~MyDialog()
{
    delete ui;
    if (my_thread != nullptr) {
        std::cout << "is thread running? " << my_thread->isRunning() << std::endl;
        while(!my_thread->wait(50));   // check if the thread finished working every 50ms

        delete my_thread;
    }
    if (th_compression   != nullptr) delete th_compression;
    if (th_decompression != nullptr) delete th_decompression;
}

void MyDialog::closeEvent(QCloseEvent *event) {

    if (my_thread != nullptr) {
        if ( !this->my_thread->isFinished() ) {

            if (this->ui->stackedWidget->currentWidget() == this->ui->page_compression_progress) {
                QString title = "Are you sure you want to cancel this?";
                QString text = "All changes will be reverted!";
                QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons( QMessageBox::Yes | QMessageBox::No );

                QMessageBox::StandardButton reply = QMessageBox::question(this, title, text, buttons);

                if (reply == QMessageBox::Yes) {
                    emit abort_processing();
                    if (my_thread != nullptr) {
                        std::cout << "is thread running? " << my_thread->isRunning() << std::endl;
                        while(!my_thread->wait(50));   // check if the thread finished working every 50ms
                        this->on_processing_finished(false);    // finished unsuccessfully

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
            std::cout << "Closing!" << std::endl;
            event->accept();
        }
    }
}

uint16_t MyDialog::get_flags() {

    std::bitset<16> flags(0);
    flags[0] = ui->checkBox_BWT->isChecked();
    flags[1] = ui->checkBox_MTF->isChecked();
    flags[2] = ui->checkBox_RLE->isChecked();

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

    switch (ui->comboBox_checksum->currentIndex()) {
    case 0:     // SHA-1
        flags[15] = true;
        break;

    case 1:     // CRC-32
        flags[14] = true;
        break;
    }


    return (uint16_t)flags.to_ulong();
}

void MyDialog::prepare_GUI_compression() {
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

void MyDialog::prepare_GUI_decompression() {
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


void MyDialog::on_buttonCancel_clicked()
{
    this->close();
}


void MyDialog::on_pushButton_abort_clicked()
{
    this->close();
}

void MyDialog::on_pushButton_compress_clicked()
{
    // before we begin, let's create a temp file, on which we will be working
    std::filesystem::copy_file(parent_mw->archive_ptr->load_path, parent_mw->temp_path, std::filesystem::copy_options::overwrite_existing);

    ui->stackedWidget->setCurrentIndex(0);

    uint16_t flags = this->get_flags();
    std::vector<file*> list_of_files;

    if (parent_usertype == 1001) {  // TreeWidgetFolder
        for (auto &file_path : paths_to_files) {
            file* new_file_ptr = twfolder_ptr->archive_ptr->add_file_to_archive_model( *(twfolder_ptr->folder_ptr), file_path.toStdString(), flags );
            list_of_files.push_back( new_file_ptr );
        }
    }
    else if (parent_usertype == 1002) { // TreeWidgetFile
        for (auto &file_path : paths_to_files) {
            file* new_file_ptr = twfile_ptr->archive_ptr->add_file_to_archive_model( *(twfile_ptr->file_ptr->parent_ptr), file_path.toStdString(), flags );
            list_of_files.push_back( new_file_ptr );
        }
    }
    else assert(false); // should never happen

    progress_step_value = 0;
    th_compression = new thread_compression( list_of_files, &progress_step_value, parent_mw->temp_path );
    my_thread = new QThread;
    th_compression->moveToThread(my_thread);

    connect( my_thread, &QThread::started , th_compression, &thread_compression::start_processing );
    connect( th_compression, &thread_compression::ProgressNextFile, ui->progressBarTotal, &QProgressBar::setValue );
    connect( th_compression, &thread_compression::ProgressNextStep, ui->progressBarFile,  &QProgressBar::setValue );
    connect( th_compression, &thread_compression::setFilePathLabel, ui->label_currentfile_value, &QLabel::setText );
    connect( th_compression, &thread_compression::processing_finished, this,    &MyDialog::on_processing_finished );
    connect( this, &MyDialog::abort_processing, th_compression, &thread_compression::abort_processing, Qt::ConnectionType::DirectConnection );

    startProcessingTimer();
    my_thread->start();
}

void MyDialog::onProgressNextFile(double value) {
    std::cout << "Event nextfile caugth, value = " << (int)value << std::endl;
    ui->progressBarTotal->setValue( (int)value );
    ui->progressBarFile->setValue(0);
}

void MyDialog::onProgressNextStep(double value) {
    std::cout << "Event nextstep caugth, value = " << round(value*100) << std::endl;
    ui->progressBarFile->setValue( round(value*100) );
}

void MyDialog::on_processing_finished(bool successful) {
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
        std::cout << "Process of modifying the archive seems to have succeeded" << std::endl;
    }
    else {
        if (compression) {
            std::cout << "Processing aborted" << std::endl;

            bool result = std::filesystem::remove(parent_mw->temp_path);
            assert( result );

            std::cout << "Cleanup successful" << std::endl;
        }
    }
    timer_elapsed_time->stop();
    ui->buttonFinish->setDisabled(false);
    ui->buttonCancel->setDisabled(true);
}



void MyDialog::on_buttonDecompressionStart_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);

    bool just_files = ui->checkBox_decompression_just_files->isChecked();
    bool confirm_integrity = ui->checkBox_decompression_integrity_validation->isChecked();

    if (ui->checkBox_decompression_remember_path) {
        if (parent_mw->config_ptr->get_extraction_path() != std::filesystem::path( ui->lineEdit_decompression_path->text().toStdString() ))
            parent_mw->config_ptr->set_extraction_path( std::filesystem::path( ui->lineEdit_decompression_path->text().toStdString() ) );
    }

    std::filesystem::path target_path = ui->lineEdit_decompression_path->text().toStdString();


    std::vector<folder*> folders;
    std::vector<file*> files;

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

        parent_mw->archive_ptr->archive_dir->set_path( target_path, true );
        // creating all necessary folders
        for (auto single_folder : folders) std::filesystem::create_directories( single_folder->path );
        for (auto single_file   : files  ) std::filesystem::create_directories( single_file->path   );
    }

    // now that we've got those ptrs, this property needs to be reset, in case it's used later
    for (auto single_folder : folders) single_folder->ptr_already_gotten = false;
    for (auto single_file   : files  ) single_file->ptr_already_gotten = false;

    progress_step_value = 0;
    th_decompression = new decompression_object( files, parent_mw->archive_ptr->archive_file, confirm_integrity, &progress_step_value );

    my_thread = new QThread;
    th_decompression->moveToThread( my_thread );


    connect( my_thread, &QThread::started , th_decompression, &decompression_object::start_processing );
    connect( th_decompression, &decompression_object::ProgressNextFile, ui->progressBarTotal, &QProgressBar::setValue );
    connect( th_decompression, &decompression_object::ProgressNextStep, ui->progressBarFile,  &QProgressBar::setValue );
    connect( th_decompression, &decompression_object::setFilePathLabel, ui->label_currentfile_value, &QLabel::setText );
    connect( th_decompression, &decompression_object::processing_finished, this,    &MyDialog::on_processing_finished );
    connect( this, &MyDialog::abort_processing, th_decompression, &decompression_object::abort_processing, Qt::ConnectionType::DirectConnection );

    startProcessingTimer();
    my_thread->start();
}



void MyDialog::on_buttonDecompressionAbort_clicked()
{
    this->close();
}

void MyDialog::set_progress_file_value(uint16_t value) {
    ui->progressBarFile->setValue( value );
    std::cout << "file progress value set: " << value << std::endl;
}

void MyDialog::on_buttonFinish_clicked()
{
    this->close();
}

void MyDialog::startProcessingTimer() {
    time_start = std::chrono::high_resolution_clock::now();
    timer_elapsed_time = new QTimer(this);
    connect( timer_elapsed_time, &QTimer::timeout, this, &MyDialog::onTimerTimeout );

    timer_elapsed_time->start(100);  // refresh every 16 ms or slightly more often than 60 times a second

}

void MyDialog::onTimerTimeout() {

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
    ui->progressBarFile->setValue( progress_step_value );
}

void MyDialog::on_button_decompression_path_dialog_clicked()
{
    QString filter = "All Files (*.*)" ;
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    QString path_to_folder = dialog.getExistingDirectory(this, "Choose where you want extracted files to go");
    ui->lineEdit_decompression_path->setText(path_to_folder);
}


thread_compression::thread_compression(std::vector<file*> given_file_list, uint16_t* progress_ptr, std::filesystem::path tmp_path) : QObject(nullptr)
{
    temp_path = tmp_path;
    this->file_list = given_file_list;
    // this->stream = given_stream;
    this->aborting_variable = false;
    this->progress_step = progress_ptr;


}

thread_compression::~thread_compression() {
    if (temp_output.is_open()) temp_output.close();
}


void thread_compression::start_processing() {

    this->temp_output.open(temp_path, std::ios::binary | std::ios::in | std::ios::out);
    assert( temp_output.is_open() );
    start();
}

void thread_compression::abort_processing() {
    aborting_variable = true;
    std::cout << "Abort! Abort! Abort!" << std::endl;
}


void thread_compression::start() {
    std::cout << "thread running\n" << std::flush;

    std::cout << "Aborting variable adress: " << &aborting_variable << std::endl;

    emit ProgressNextFile(0);
    emit ProgressNextStep(0);
    uint16_t i=0;
    for (; i < file_list.size() and !aborting_variable; ++i)
    {
        std::cout << "aborting variable = " << aborting_variable << std::endl;
        emit setFilePathLabel( file_list[i]->path.data() );

        file_list[i]->append_to_archive( temp_output, aborting_variable, false, progress_step );

        emit ProgressNextFile((1.0+i)/(double)file_list.size()*100.0);
    }

    emit ProgressNextStep(100);
    emit processing_finished( !aborting_variable ); // false if aborted

    QThread::currentThread()->quit();
}




decompression_object::decompression_object( std::vector<file*> file_list, std::fstream& source, bool validate_integrity, uint16_t* progress_ptr ) : QObject(nullptr)
{
    this->file_list = file_list;
    this->source_stream = &source;
    this->aborting_variable = false;
    this->validate_integrity = validate_integrity;
    this->progress_step = progress_ptr;
}

decompression_object::~decompression_object() {
}


void decompression_object::start_processing() {
    start();
}

void decompression_object::abort_processing() {
    aborting_variable = true;
    std::cout << "Abort! Abort! Abort!" << std::endl;
}

void decompression_object::start() {
    std::cout << "thread running\n" << std::flush;

    emit ProgressNextFile(0);
    emit ProgressNextStep(0);

    for (uint16_t i=0; i < file_list.size() and !aborting_variable; ++i)
    {
        *progress_step = 0;
        emit ProgressNextStep(0);

        std::filesystem::path label_path = file_list[i]->path;
        label_path.append( file_list[i]->name );
        emit setFilePathLabel( label_path.c_str() );

        file_list[i]->unpack( file_list[i]->path, *source_stream, aborting_variable, false, validate_integrity, progress_step );
        std::cout << "*progress_step = " << *progress_step << std::endl;


        emit ProgressNextFile((1.0+i)/(double)file_list.size()*100.0);
    }
    emit ProgressNextStep(100);
    emit processing_finished( !aborting_variable ); // false if aborted
    QThread::currentThread()->quit();
}


























