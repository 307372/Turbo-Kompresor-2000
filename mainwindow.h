#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <memory>
#include <typeinfo>
#include "archive.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

//void addChild(QTreeWidgetItem *parent, QString name, QString description);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void newArchiveModel();
    void createEmptyArchive();
    void load_archive( std::string path_to_archive );

    archive* archive_ptr;
    //Ui::MainWindow *ui;


private slots:
    //void on_encodebutton_clicked();

    //void on_decodebutton_clicked();

    //void on_iidmodelbutton_clicked();

    //void on_dickensbutton_clicked();

    void on_actionNew_archive_triggered();

    void on_actionOpen_archive_triggered();

    void on_buttonExtractSelected_clicked();
    void on_buttonExtractAll_clicked();
    void on_pathToolButton_clicked();

    void on_buttonAddNewFile_clicked();

private:
    Ui::MainWindow *ui;

};


class TreeWidgetFolder : public QTreeWidgetItem {
// type = 1001
public:
    folder* folder_ptr;
    archive* archive_ptr;
    //UserType

    TreeWidgetFolder(QTreeWidgetItem *parent, folder* ptr_to_folder, archive* ptr_to_archive );
    TreeWidgetFolder(TreeWidgetFolder *parent, folder* ptr_to_folder, archive* ptr_to_archive );
    //void write();
    void unpack( std::string path_for_extraction );
    void setDisabled(bool disabled);
    //void addToTree(QTreeWidgetFolder *parent);
};


class TreeWidgetFile : public QTreeWidgetItem {
// type = 1002
public:
    file* file_ptr;
    archive* archive_ptr;

    TreeWidgetFile(TreeWidgetFolder *parent, file* ptr_to_file, archive* ptr_to_archive );
    //void write();
    void unpack( std::string path_for_extraction );
    void setDisabled(bool disabled);
    //void addToTree(QTreeWidgetFolder *parent);
};






#endif // MAINWINDOW_H




