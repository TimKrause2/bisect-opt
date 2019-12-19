#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    gridLayout = new QGridLayout(ui->centralWidget);
    glwidget = new MyGLWidget(this);
    gridLayout->addWidget(glwidget,0,0,1,1);

    //ui->centralWidget->setLayout(gridLayout);
}

MainWindow::~MainWindow()
{
    delete ui;
}
