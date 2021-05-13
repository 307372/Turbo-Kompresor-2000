#ifndef ARCHIVE_WINDOW_H
#define ARCHIVE_WINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QAbstractItemView>
#include <QInputDialog>
#include <memory>

#include "archive.h"
#include "processing_dialog.h"
#include "config.h"


QT_BEGIN_NAMESPACE
namespace Ui { class ArchiveWindow; }
QT_END_NAMESPACE

class ArchiveWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::ArchiveWindow *ui;
    QIcon ArchiveIcon;

public:
    ArchiveWindow(QWidget *parent = nullptr);
    ~ArchiveWindow();

    void new_archive_model();
    void create_empty_archive();
    void load_archive( std::string path_to_archive );
    void reload_archive();
    void write_file_to_current_archive( File* file_model, bool& aborting_var );
    void write_folder_to_current_archive( Folder* folder_model, bool& aborting_var );

    Archive* archive_ptr = nullptr;
    std::string current_archive_path = "";
    Config* config_ptr = nullptr;

    QString style_dark = "";
    QString style_light = "";

    std::filesystem::path temp_path = std::filesystem::temp_directory_path().append( "tk2k_archive.tmp" );

private slots:
    void new_archive_triggered();

    void open_archive_triggered();

    void extract_selected_clicked();

    void extract_all_clicked();

    void add_new_file_clicked();

    void add_new_folder_clicked();

    void open_settings_dialog();

    void remove_selected_clicked();

    void on_archiveWidget_itemSelectionChanged();
};

#endif // MAINWINDOW_H




