#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "arithmetic_coding.h"
#include "statistical_tools.h"
#include "integrity_validation.h"
#include <iostream>
#include <climits>
#include <chrono>
#include <fstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_iidmodelbutton_clicked()
{

}

void MainWindow::on_decodebutton_clicked()
{

}

void MainWindow::on_dickensbutton_clicked()
{

}


void MainWindow::on_encodebutton_clicked()
{
    std::string path = ui->path->displayText().toUtf8().constData();
    std::cout << "Generating statistical data..." << std::endl;
    auto t0 = std::chrono::high_resolution_clock::now();
    statistical_tools st(path);
    st.iid_model_chunks();
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Generated in " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << "ms." << std::endl;

    std::cout << "Encoding..." << std::endl;
    t0 = std::chrono::high_resolution_clock::now();
    arithmetic_coding ac;
    std::string encoded_message = ac.encode_file(st.alphabet, st.r, path, 1024 );
    t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Encoding finished in " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << "ms." << std::endl;

    std::cout << "Decoding..." << std::endl;
    t0 = std::chrono::high_resolution_clock::now();
    ac.decode_to_file( encoded_message, st.r, st.alphabet, path + "_decoded", st.file_size );
    t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Decoding finished in " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << "ms." << std::endl;

    std::cout << "Checking file integrity..." << std::endl;
    t0 = std::chrono::high_resolution_clock::now();
    integrity_validation iv;
    std::string a = iv.get_SHA1(path);
    std::cout<<"SHA-1 of original file:\n" << a << std::endl;
    std::string b = iv.get_SHA1( path + "_decoded" );
    std::cout<<"SHA-1 of decoded  file:\n" << b << std::endl;
    t1 = std::chrono::high_resolution_clock::now();

    if (a == b)
    {
        std::cout << "Success!"<<std::endl;
    }
    else
        std::cout << "Something went wrong!"<<std::endl;
    std::cout << "Checked in " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << "ms." << std::endl;
}

































