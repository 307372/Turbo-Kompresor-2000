#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_encodebutton_clicked();

    void on_decodebutton_clicked();

    void on_iidmodelbutton_clicked();

    void on_dickensbutton_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
