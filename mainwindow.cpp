#include "mainwindow.h"
#include "Lab1/batterywidget.h"
#include "Lab3/storagewindow.h"
#include "Lab4/camerawindow.h"
#include "Lab5/usbwindow.h"
#include "Animation/jakewidget.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QMessageBox>
#include <QEvent>
#include <QHoverEvent>
#include <QPainter>
#include <QPaintEvent>
#include <cstdlib>
#include <ctime>
#include <QDebug>
#include <QMouseEvent>
#include <QDir>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), batteryWidget(nullptr), jakeWidget(nullptr), storageWindow(nullptr), cameraWindow(nullptr), usbWindow(nullptr), isHoveringButton(false), currentBackgroundIndex(0)
{
    setWindowTitle("Лабораторные работы");
    setFixedSize(1200, 700);
    
    // Инициализируем генератор случайных чисел
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Автоматически сканируем все изображения в папке backgrounds
    scanBackgroundImages();
    
    // Загружаем случайное фоновое изображение (если есть)
    loadRandomBackground();
    
    // Настраиваем таймер для смены фона каждые 3 секунды (только если есть изображения)
    backgroundTimer = new QTimer(this);
    connect(backgroundTimer, &QTimer::timeout, this, &MainWindow::changeBackground);
    if (!backgroundImages.isEmpty()) {
        backgroundTimer->start(10*1000); // 3 секунды
    }

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

        // Устанавливаем фильтр событий для кнопок и включаем отслеживание мыши
        button->installEventFilter(this);
        button->setAttribute(Qt::WA_Hover, true);

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

        // Устанавливаем фильтр событий для кнопок и включаем отслеживание мыши
        button->installEventFilter(this);
        button->setAttribute(Qt::WA_Hover, true);

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

    // Создаем Джейка после инициализации UI
    setupJake();
}

MainWindow::~MainWindow()
{
    if (batteryWidget) {
        batteryWidget->close();
        delete batteryWidget;
    }
    if (storageWindow) {
        storageWindow->close();
        delete storageWindow;
    }
    if (cameraWindow) {
        cameraWindow->close();
        delete cameraWindow;
    }
    if (usbWindow) {
        usbWindow->close();
        delete usbWindow;
    }
    if (jakeWidget) {
        jakeWidget->close();
        delete jakeWidget;
    }
}

void MainWindow::setupJake()
{
    jakeWidget = new JakeWidget();
    jakeWidget->resize(200, 160);
    jakeWidget->show();
    
    // Устанавливаем начальную позицию Джейка
    QPoint globalCursorPos = QCursor::pos();
    jakeWidget->move(globalCursorPos.x() - jakeWidget->width()/2, 
                     globalCursorPos.y() - jakeWidget->height()/2);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (jakeWidget) {
        if (event->type() == QEvent::HoverEnter) {
            isHoveringButton = true;
            jakeWidget->onButtonHover();
        } else if (event->type() == QEvent::HoverLeave) {
            isHoveringButton = false;
            jakeWidget->onButtonLeave();
        } else if (event->type() == QEvent::MouseButtonPress) {
            jakeWidget->onButtonClick();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::openLab1() {
    BatteryWidget *batteryWindow = new BatteryWidget(nullptr);
    batteryWindow->showAndStart();
    
    if (jakeWidget) {
        jakeWidget->onButtonClick();
    }
}

void MainWindow::openLab2() {
    QMessageBox::information(this, "ЛР 2", "Открываем лабораторную работу 2");
    if (jakeWidget) {
        jakeWidget->onButtonClick();
    }
}

void MainWindow::openLab3() {
    StorageWindow *storageWin = new StorageWindow(nullptr);
    storageWin->show();
    
    if (jakeWidget) {
        jakeWidget->onButtonClick();
    }
}

void MainWindow::openLab4() {
    CameraWindow *cameraWin = new CameraWindow(nullptr);
    cameraWin->show();
    
    if (jakeWidget) {
        jakeWidget->onButtonClick();
    }
}

void MainWindow::openLab5() {
    USBWindow *usbWin = new USBWindow(nullptr);
    usbWin->show();
    
    if (jakeWidget) {
        jakeWidget->onButtonClick();
    }
}

void MainWindow::openLab6() {
    QMessageBox::information(this, "ЛР 6", "Открываем лабораторную работу 6");
    if (jakeWidget) {
        jakeWidget->onButtonClick();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && !backgroundImages.isEmpty()) {
        // При клике левой кнопкой мыши меняем фон (только если есть изображения)
        changeBackground();
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event) {
    if (!backgroundPixmap.isNull() && !backgroundImages.isEmpty()) {
        QPainter painter(this);
        
        // Растягиваем изображение на весь размер окна
        painter.drawPixmap(0, 0, width(), height(), backgroundPixmap);
        
        // Добавляем полупрозрачный слой для лучшей читаемости текста
        painter.fillRect(rect(), QColor(0, 0, 0, 100));
    }
    
    QMainWindow::paintEvent(event);
}

void MainWindow::changeBackground() {
    loadRandomBackground();
    update(); // Перерисовываем окно
}

void MainWindow::loadRandomBackground() {
    if (backgroundImages.isEmpty()) {
        qDebug() << "Нет доступных фоновых изображений";
        return;
    }
    
    // Выбираем случайное изображение, отличное от текущего
    int newIndex;
    do {
        newIndex = rand() % backgroundImages.size();
    } while (newIndex == currentBackgroundIndex && backgroundImages.size() > 1);
    
    currentBackgroundIndex = newIndex;
    QString imagePath = backgroundImages[currentBackgroundIndex];
    
    if (backgroundPixmap.load(imagePath)) {
        // Фоновое изображение успешно загружено
        qDebug() << "Загружен фон:" << imagePath;
    } else {
        qWarning() << "Не удалось загрузить фоновое изображение:" << imagePath;
    }
}

void MainWindow::scanBackgroundImages() {
    backgroundImages.clear();
    
    // Сканируем все изображения в реальной папке backgrounds/
    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif";
    
    // Пробуем несколько возможных путей
    QStringList possiblePaths;
    possiblePaths << "C:/QT_projects/IIvuim/backgrounds";                     // Исходная папка проекта (абсолютный путь)
    possiblePaths << QCoreApplication::applicationDirPath() + "/backgrounds";  // Рядом с exe
    possiblePaths << "backgrounds";                                            // Текущая директория
    possiblePaths << "../backgrounds";                                         // На уровень выше (для build папок)
    possiblePaths << "../../backgrounds";                                      // Два уровня выше
    possiblePaths << QDir::currentPath() + "/backgrounds";                    // Рабочая директория
    
    QDir backgroundDir;
    bool found = false;
    
    for (const QString &path : possiblePaths) {
        backgroundDir.setPath(path);
        if (backgroundDir.exists()) {
            qDebug() << "Найдена папка backgrounds:" << backgroundDir.absolutePath();
            found = true;
            break;
        }
    }
    
    if (found) {
        QStringList allFiles = backgroundDir.entryList(nameFilters, QDir::Files);
        
        for (const QString &fileName : allFiles) {
            QString fullPath = backgroundDir.absoluteFilePath(fileName);
            backgroundImages.append(fullPath);
            qDebug() << "Найдено фоновое изображение:" << fileName;
        }
    } else {
        qWarning() << "Папка backgrounds не найдена. Проверены пути:";
        for (const QString &path : possiblePaths) {
            qWarning() << "  -" << QDir(path).absolutePath();
        }
    }
    
    qDebug() << "Всего найдено фоновых изображений:" << backgroundImages.size();
    
    if (backgroundImages.isEmpty()) {
        qDebug() << "В папке backgrounds нет изображений - фон отключен";
    }
}