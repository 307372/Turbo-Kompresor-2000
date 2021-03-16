#ifndef PROCESSING_HELPERS_H
#define PROCESSING_HELPERS_H

#include <cassert>
#include <cmath>
#include <bitset>

#include <QObject>
#include <QThread>

#include "archive_structures.h"


class CompressionObject : public QObject
{
    Q_OBJECT
public:
    explicit CompressionObject( std::vector<File*> file_list, uint16_t* progress_ptr, uint32_t* progressBarStepMax, std::filesystem::path tmp_path );
    ~CompressionObject();
    void start();

    std::vector<File*> file_list;
    std::fstream temp_output;
    std::filesystem::path temp_path;
    bool aborting_variable;
    uint16_t* progress_step;
    uint32_t* progressBarStepMax = nullptr;

signals:
    void progressNextFile(double value);
    void progressNextStep(double value);
    void setFilePathLabel(QString path);
    void processingFinished(bool successful);
    void displayFailedFiles(QStringList failed_files);

public slots:
    void startProcessing();
    void abortProcessing();
};



class DecompressionObject : public QObject {
    Q_OBJECT
public:
    explicit DecompressionObject( std::vector<File*> file_list, std::fstream& source, bool validate_integrity, uint16_t* progress_ptr, uint32_t* progressBarStepMax );
    ~DecompressionObject();
    void start();

    std::vector<File*> file_list;
    std::fstream* source_stream;

    bool aborting_variable;
    uint16_t* progress_step;
    uint32_t* progress_bar_step_max = nullptr;

signals:
    void ProgressNextFile(double value);
    void ProgressNextStep(double value);
    void setFilePathLabel(QString path);
    void processingFinished(bool successful);
    void displayFailedFiles(QStringList failed_list);

public slots:
    void startProcessing();
    void abortProcessing();

private:
    bool validate_integrity;
};


#endif
