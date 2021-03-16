#include "processing_helpers.h"


CompressionObject::CompressionObject(std::vector<File*> given_file_list, uint16_t* progress_ptr, uint32_t* progressBarStepMax, std::filesystem::path tmp_path) : QObject(nullptr)
{
    temp_path = tmp_path;
    this->file_list = given_file_list;
    this->aborting_variable = false;
    this->progress_step = progress_ptr;
    this->progressBarStepMax = progressBarStepMax;
}


CompressionObject::~CompressionObject() {
    if (temp_output.is_open()) temp_output.close();
}


void CompressionObject::startProcessing() {

    this->temp_output.open(temp_path, std::ios::binary | std::ios::in | std::ios::out);
    assert( temp_output.is_open() );
    start();
}


void CompressionObject::abortProcessing() {
    aborting_variable = true;
}


void CompressionObject::start() {
    emit progressNextFile(0);
    emit progressNextStep(0);

    QStringList failed_files;

    uint16_t i=0;
    for (; i < file_list.size(); ++i)
    {
        emit setFilePathLabel( file_list[i]->path.data() );

        *progress_step = 0;
        std::bitset<16> bin_flags(file_list[i]->flags_value);
        *progressBarStepMax = ceil((double)std::filesystem::file_size(file_list[i]->path) / (double)((1ull << 24)-1))*bin_flags.count();

        bool successful = false;
        if (!aborting_variable) successful = file_list[i]->append_to_archive( temp_output, aborting_variable, false, progress_step );

        if (!successful) failed_files.append(QString::fromStdString(file_list[i]->path));
        emit progressNextFile((1.0+i)/(double)file_list.size()*100.0);
    }

    emit progressNextStep(100);
    emit processingFinished( !aborting_variable and failed_files.empty() );
    if (!failed_files.empty()) emit displayFailedFiles(failed_files);

    QThread::currentThread()->quit();
}



DecompressionObject::DecompressionObject( std::vector<File*> file_list, std::fstream& source, bool validate_integrity, uint16_t* progress_ptr, uint32_t* progressBarStepMax ) : QObject(nullptr)
{
    this->file_list = file_list;
    this->source_stream = &source;
    this->aborting_variable = false;
    this->validate_integrity = validate_integrity;
    this->progress_step = progress_ptr;
    this->progress_bar_step_max = progressBarStepMax;
}


DecompressionObject::~DecompressionObject() {
}


void DecompressionObject::startProcessing() {
    start();
}


void DecompressionObject::abortProcessing() {
    aborting_variable = true;
}


void DecompressionObject::start()
{
    emit ProgressNextFile(0);
    emit ProgressNextStep(0);

    QStringList failed_files;

    for (uint16_t i=0; i < file_list.size(); ++i)
    {
        emit ProgressNextStep(0);

        *progress_step = 0;
        std::bitset<16> bin_flags(file_list[i]->flags_value);

        *progress_bar_step_max = ceill((long double)file_list[i]->original_size / (long double)((1ull << 24)-1))*bin_flags.count();

        std::filesystem::path label_path = file_list[i]->path;
        label_path.append( file_list[i]->name );
        emit setFilePathLabel( label_path.c_str() );

        bool successful = false;

        if (!aborting_variable) successful = file_list[i]->unpack( file_list[i]->path, *source_stream, aborting_variable, false, validate_integrity, progress_step );
        if (!successful) failed_files.append(QString::fromStdString(file_list[i]->path + '/' +file_list[i]->name));

        emit ProgressNextFile((1.0+i)/(double)file_list.size()*100.0);
    }

    emit ProgressNextStep(100);
    emit processingFinished( !aborting_variable and failed_files.empty() );

    if (!failed_files.empty()) emit displayFailedFiles(failed_files);
    QThread::currentThread()->quit();
}

