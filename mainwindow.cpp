#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    irrWidget = new IrrWidget(ui->irr_widget);
}

MainWindow::~MainWindow()
{
    if (irrWidget) delete irrWidget;
    delete ui;
}

