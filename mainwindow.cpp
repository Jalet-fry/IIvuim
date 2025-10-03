#include "mainwindow.h"
#include "batterywidget.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Лабораторные работы");
    setFixedSize(1200, 700);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QLabel *titleLabel = new QLabel("Лабораторные работы", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 32px;"
        "    font-weight: bold;"
        "    color: #2E7D32;"
        "    padding: 20px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();

    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(30);

    for (int i = 1; i <= 3; ++i) {
        QPushButton *button = new QPushButton(QString("ЛР %1").arg(i), this);
        button->setFixedSize(250, 80);
        button->setStyleSheet(
            "QPushButton {"
            "    background-color: #2196F3;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 12px;"
            "    font-size: 18px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #1976D2;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #0D47A1;"
            "}"
        );
        leftLayout->addWidget(button);

        if (i == 1) connect(button, SIGNAL(clicked()), this, SLOT(openLab1()));
        if (i == 2) connect(button, SIGNAL(clicked()), this, SLOT(openLab2()));
        if (i == 3) connect(button, SIGNAL(clicked()), this, SLOT(openLab3()));
    }

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(30);

    for (int i = 4; i <= 6; ++i) {
        QPushButton *button = new QPushButton(QString("ЛР %1").arg(i), this);
        button->setFixedSize(250, 80);
        button->setStyleSheet(
            "QPushButton {"
            "    background-color: #FF9800;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 12px;"
            "    font-size: 18px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #F57C00;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #E65100;"
            "}"
        );
        rightLayout->addWidget(button);

        if (i == 4) connect(button, SIGNAL(clicked()), this, SLOT(openLab4()));
        if (i == 5) connect(button, SIGNAL(clicked()), this, SLOT(openLab5()));
        if (i == 6) connect(button, SIGNAL(clicked()), this, SLOT(openLab6()));
    }

    buttonsLayout->addStretch();
    buttonsLayout->addLayout(leftLayout);
    buttonsLayout->addSpacing(100);
    buttonsLayout->addLayout(rightLayout);
    buttonsLayout->addStretch();

    mainLayout->addStretch();
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addStretch();
}

void MainWindow::openLab1() {
    BatteryWidget *batteryWindow = new BatteryWidget(nullptr);
    batteryWindow->showAndStart();
}

void MainWindow::openLab2() {
    QMessageBox::information(this, "ЛР 2", "Открываем лабораторную работу 2");
}

void MainWindow::openLab3() {
    QMessageBox::information(this, "ЛР 3", "Открываем лабораторную работу 3");
}

void MainWindow::openLab4() {
    QMessageBox::information(this, "ЛР 4", "Открываем лабораторную работу 4");
}

void MainWindow::openLab5() {
    QMessageBox::information(this, "ЛР 5", "Открываем лабораторную работу 5");
}

void MainWindow::openLab6() {
    QMessageBox::information(this, "ЛР 6", "Открываем лабораторную работу 6");
}