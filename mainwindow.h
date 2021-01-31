#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QAbstractItemView>
#include <QInputDialog>
#include <memory>

#include "archive.h"
#include "processingdialog.h"
#include "config.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void new_archive_model();
    void create_empty_archive();
    void load_archive( std::string path_to_archive );
    void reload_archive();
    void write_file_to_current_archive( file* file_model, bool& aborting_var );
    void write_folder_to_current_archive( folder* folder_model, bool& aborting_var );

    archive* archive_ptr = nullptr;
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

private:
    Ui::MainWindow *ui;
};


class TreeWidgetFolder : public QTreeWidgetItem {
// type = 1001
public:
    folder* folder_ptr;
    archive* archive_ptr;

    TreeWidgetFolder(QTreeWidgetItem *parent, folder* ptr_to_folder, archive* ptr_to_archive, bool filesize_scaled );
    TreeWidgetFolder(TreeWidgetFolder *parent, folder* ptr_to_folder, archive* ptr_to_archive, bool filesize_scaled );

    void unpack( std::string path_for_extraction, bool& aborting_var );
    void setDisabled(bool disabled);
    bool operator<(const QTreeWidgetItem &other)const;
};


class TreeWidgetFile : public QTreeWidgetItem {
// type = 1002
public:
    file* file_ptr;
    archive* archive_ptr;

    TreeWidgetFile(TreeWidgetFolder *parent, file* ptr_to_file, archive* ptr_to_archive, bool filesize_scaled );

    void unpack( std::string path_for_extraction, bool& aborting_var );
    void setDisabled(bool disabled);
    bool operator<(const QTreeWidgetItem &other)const;
};

#endif // MAINWINDOW_H




