#include "mainwindow.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // УВЕЛИЧИВАЕМ РАЗМЕР ОКНА
    setWindowTitle("Лабораторные работы");
    setFixedSize(1200, 700); // БОЛЬШЕ чем было!

    // Центральный виджет
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Главный layout - вертикальный
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Заголовок
    QLabel *titleLabel = new QLabel("Лабораторные работы", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 32px;"  // УВЕЛИЧИЛИ ШРИФТ
        "    font-weight: bold;"
        "    color: #2E7D32;"
        "    padding: 20px;"    // УВЕЛИЧИЛИ ОТСТУПЫ
        "}"
    );
    mainLayout->addWidget(titleLabel);

    // Горизонтальный layout для кнопок
    QHBoxLayout *buttonsLayout = new QHBoxLayout();

    // ЛЕВАЯ КОЛОНКА (3 кнопки)
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(30); // БОЛЬШЕ расстояния между кнопками

    for (int i = 1; i <= 3; ++i) {
        QPushButton *button = new QPushButton(QString("ЛР %1").arg(i), this); // СОКРАТИЛИ НАДПИСЬ
        button->setFixedSize(250, 80); // УВЕЛИЧИЛИ КНОПКИ
        button->setStyleSheet(
            "QPushButton {"
            "    background-color: #2196F3;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 12px;"  // УВЕЛИЧИЛИ ЗАКРУГЛЕНИЯ
            "    font-size: 18px;"      // УВЕЛИЧИЛИ ШРИФТ
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

        // Подключаем кнопки
        if (i == 1) connect(button, SIGNAL(clicked()), this, SLOT(openLab1()));
        if (i == 2) connect(button, SIGNAL(clicked()), this, SLOT(openLab2()));
        if (i == 3) connect(button, SIGNAL(clicked()), this, SLOT(openLab3()));
    }

    // ПРАВАЯ КОЛОНКА (3 кнопки)
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(30);

    for (int i = 4; i <= 6; ++i) {
        QPushButton *button = new QPushButton(QString("ЛР %1").arg(i), this); // СОКРАТИЛИ НАДПИСЬ
        button->setFixedSize(250, 80); // УВЕЛИЧИЛИ КНОПКИ
        button->setStyleSheet(
            "QPushButton {"
            "    background-color: #FF9800;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 12px;"
            "    font-size: 18px;"  // УВЕЛИЧИЛИ ШРИФТ
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

        // Подключаем кнопки
        if (i == 4) connect(button, SIGNAL(clicked()), this, SLOT(openLab4()));
        if (i == 5) connect(button, SIGNAL(clicked()), this, SLOT(openLab5()));
        if (i == 6) connect(button, SIGNAL(clicked()), this, SLOT(openLab6()));
    }

    // Добавляем колонки в горизонтальный layout
    buttonsLayout->addStretch();
    buttonsLayout->addLayout(leftLayout);
    buttonsLayout->addSpacing(100); // БОЛЬШЕ расстояния между колонками
    buttonsLayout->addLayout(rightLayout);
    buttonsLayout->addStretch();

    // Добавляем все в главный layout
    mainLayout->addStretch();
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addStretch();
}

void MainWindow::openLab1() {
    QMessageBox::information(this, "ЛР 1", "Открываем лабораторную работу 1");
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
