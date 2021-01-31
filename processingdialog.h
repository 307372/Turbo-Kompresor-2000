#ifndef PROCESSINGDIALOG_H
#define PROCESSINGDIALOG_H
#include <iostream>
#include <bitset>

#include <QMessageBox>
#include <QString>
#include <QDialog>
#include <QtConcurrent>
#include <QTimer>


class MainWindow;
class TreeWidgetFolder;
class TreeWidgetFile;
class compression_object;
class decompression_object;


#include "mainwindow.h"


namespace Ui {
class ProcessingDialog;
}

class ProcessingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessingDialog( QWidget *parent = nullptr );
    explicit ProcessingDialog( TreeWidgetFolder* twfolder, QStringList file_path_list, QWidget *parent = nullptr );
    explicit ProcessingDialog( TreeWidgetFile* twfile, QStringList file_path_list, QWidget *parent = nullptr );
    explicit ProcessingDialog( std::vector<QTreeWidgetItem*> extraction_targets, QWidget *parent = nullptr );
    ~ProcessingDialog();

    uint16_t progress_step_value = 0;

    void set_progress_file_value(uint16_t value);
    void closeEvent(QCloseEvent *event);
    uint16_t get_flags();
    void prepare_GUI_compression();
    void prepare_GUI_decompression();

signals:
    void abort_processing();

private slots:
    void on_buttonCancel_clicked();

    void on_pushButton_abort_clicked();

    void on_pushButton_compress_clicked();

    void onProgressNextFile(double value);

    void onProgressNextStep(double value);

    void slot_processing_finished(bool successful);

    void on_buttonFinish_clicked();

    void onTimerTimeout();

    void on_buttonDecompressionStart_clicked();

    void on_buttonDecompressionAbort_clicked();

    void on_button_decompression_path_dialog_clicked();

    void displayFailedFiles(QStringList failed_paths);

private:
    Ui::ProcessingDialog *ui;
    MainWindow* parent_mw = nullptr;
    TreeWidgetFolder* twfolder_ptr = nullptr;
    TreeWidgetFile* twfile_ptr = nullptr;
    compression_object* th_compression = nullptr;
    QThread* my_thread = nullptr;
    QTimer* timer_elapsed_time = nullptr;
    decompression_object* th_decompression = nullptr;

    uint32_t progressBarStepMax = 100;
    int16_t parent_usertype = -1;
    QStringList paths_to_files;
    std::chrono::time_point<std::chrono::high_resolution_clock> time_start;
    bool compression;       // true = compression, false = decompression

    std::vector<QTreeWidgetItem*> extraction_targets;

    void compression_start();
    void startProcessingTimer();
    void correct_duplicate_names(file* target_file, folder* parent_folder);
};


// ===== HELPER OBJECT STARTS NOW =====


class compression_object : public QObject
{
    Q_OBJECT
public:
    explicit compression_object( std::vector<file*> file_list, uint16_t* progress_ptr, uint32_t* progressBarStepMax, std::filesystem::path tmp_path );
    ~compression_object();
    void start();

    std::vector<file*> file_list;
    std::fstream temp_output;
    std::filesystem::path temp_path;
    bool aborting_variable;
    uint16_t* progress_step;
    uint32_t* progressBarStepMax = nullptr;

signals:
    void ProgressNextFile(double value);
    void ProgressNextStep(double value);
    void setFilePathLabel(QString path);
    void processing_finished(bool successful);
    void displayFailedFiles(QStringList failed_files);

public slots:
    void start_processing();
    void abort_processing();
};



class decompression_object : public QObject {
    Q_OBJECT
public:
    explicit decompression_object( std::vector<file*> file_list, std::fstream& source, bool validate_integrity, uint16_t* progress_ptr, uint32_t* progressBarStepMax );
    ~decompression_object();
    void start();

    std::vector<file*> file_list;
    std::fstream* source_stream;

    bool aborting_variable;
    uint16_t* progress_step;
    uint32_t* progressBarStepMax = nullptr;

signals:
    void ProgressNextFile(double value);
    void ProgressNextStep(double value);
    void setFilePathLabel(QString path);
    void processing_finished(bool successful);
    void displayFailedFiles(QStringList failed_list);

public slots:
    void start_processing();
    void abort_processing();

private:
    bool validate_integrity;
};

#endif // PROCESSINGDIALOG_H
