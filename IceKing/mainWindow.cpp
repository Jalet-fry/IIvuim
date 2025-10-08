#include "mainWindow.h"
#include "pciwindow.h"
#include "storagewindow.h"

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsProxyWidget>
#include <QApplication>
#include <QPropertyAnimation>
#include <QMessageBox>

#include <windows.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    scene(0),
    view(0),
    lab1Btn(0),
    lab2Btn(0),
    lab3Btn(0),
    lab4Btn(0),
    lab5Btn(0),
    lab6Btn(0),
    lab1Proxy(0),
    lab2Proxy(0),
    lab3Proxy(0),
    lab4Proxy(0),
    lab5Proxy(0),
    lab6Proxy(0),
    currentHoverButton(0),
    currentHoverButtonProxy(0),
    animationTimer(0),
    hoverActive(false),
    drumsAnimation(0),
    hugsAnimation(0),
    powerAnimation(0),
    lostAnimation(0),
    pciWindow(0),
    storageWindow(0)
{
    setWindowIcon(QIcon(":/img/ik-ic.ico"));
    if (windowIcon().isNull()) {
        qDebug() << "Error: not found finn-icon.png";
    }

    showFullScreen();

    currentState = IdleAnimation;   // default animation
    hoverActive = false;            // keep if you use it elsewhere

    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene, this);
    setCentralWidget(view);

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    scene->setSceneRect(0, 0, screenGeometry.width(), screenGeometry.height());
    scene->setBackgroundBrush(Qt::white);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    // Exit button (top-right)
    QPushButton *exitBtn = new QPushButton(QString::fromUtf8("Exit"));
    exitBtn->setFixedSize(100, 40);
    QString exitBtnStyle =
        "QPushButton { background: rgba(150,0,0,150); color: white; font-weight: bold; border: 1px solid #222; }"
        "QPushButton:hover { color: #ffff66; }";
    exitBtn->setStyleSheet(exitBtnStyle);
    QGraphicsProxyWidget *exitProxy = scene->addWidget(exitBtn);
    QRectF srect = scene->sceneRect();
    exitProxy->setPos(srect.width() - exitBtn->width() - 10, 10);
    exitProxy->setZValue(1000);
    connect(exitBtn, SIGNAL(clicked()), qApp, SLOT(quit()));

    int button_pos_x(80), button_pos_y(80), delta_y(100), button_size_x(240), button_size_y(80);
    int button_i(0);

    lab1Btn = new QPushButton(QString::fromUtf8("Lab 1"));
    lab1Btn->setFixedSize(button_size_x, button_size_y);
    QString btnStyle =
        "QPushButton { background: rgba(0,100,200,150); color: white; font-weight: bold; border: 1px solid #ccc; border-radius: 5px; }"
        "QPushButton:hover { color: #ffffaa; background: rgba(100,200,255,180); }";
    lab1Btn->setStyleSheet(btnStyle);
    lab1Proxy = scene->addWidget(lab1Btn);
    lab1Proxy->setPos(button_pos_x, button_pos_y + delta_y * (button_i++));
    lab1Proxy->setZValue(1000);
    connect(lab1Btn, SIGNAL(clicked()), this, SLOT(onLab1ButtonClicked()));

    lab2Btn = new QPushButton(QString::fromUtf8("Lab 2"));
    lab2Btn->setFixedSize(button_size_x, button_size_y);
    lab2Btn->setStyleSheet(btnStyle);

    lab2Proxy = scene->addWidget(lab2Btn);
    lab2Proxy->setPos(button_pos_x, button_pos_y + delta_y * (button_i++));
    lab2Proxy->setZValue(1000);
    connect(lab2Btn, SIGNAL(clicked()), this, SLOT(onLab2ButtonClicked()));

    // Lab 3 button
    lab3Btn = new QPushButton(QString::fromUtf8("Lab 3"));
    lab3Btn->setFixedSize(button_size_x, button_size_y);
    lab3Btn->setStyleSheet(btnStyle);
    lab3Proxy = scene->addWidget(lab3Btn);
    lab3Proxy->setPos(button_pos_x, button_pos_y + delta_y * (button_i++));
    lab3Proxy->setZValue(1000);
    connect(lab3Btn, SIGNAL(clicked()), this, SLOT(onLab456ButtonClicked()));

    lab4Btn = new QPushButton(QString::fromUtf8("Lab 4"));
    lab4Btn->setFixedSize(button_size_x, button_size_y);
    lab4Btn->setStyleSheet(btnStyle);
    lab4Proxy = scene->addWidget(lab4Btn);
    lab4Proxy->setPos(button_pos_x, button_pos_y + delta_y * (button_i++));
    lab4Proxy->setZValue(1000);
    connect(lab4Btn, SIGNAL(clicked()), this, SLOT(onLab456ButtonClicked()));

    lab5Btn = new QPushButton(QString::fromUtf8("Lab 5"));
    lab5Btn->setFixedSize(button_size_x, button_size_y);
    lab5Btn->setStyleSheet(btnStyle);
    lab5Proxy = scene->addWidget(lab5Btn);
    lab5Proxy->setPos(button_pos_x, button_pos_y + delta_y * (button_i++));
    lab5Proxy->setZValue(1000);
    connect(lab5Btn, SIGNAL(clicked()), this, SLOT(onLab456ButtonClicked()));

    lab6Btn = new QPushButton(QString::fromUtf8("Lab 6"));
    lab6Btn->setFixedSize(button_size_x, button_size_y);
    lab6Btn->setStyleSheet(btnStyle);
    lab6Proxy = scene->addWidget(lab6Btn);
    lab6Proxy->setPos(button_pos_x, button_pos_y + delta_y * (button_i++));
    lab6Proxy->setZValue(1000);
    connect(lab6Btn, SIGNAL(clicked()), this, SLOT(onLab456ButtonClicked()));

    // Enable hover events and install event filters for all buttons
    lab1Btn->setAttribute(Qt::WA_Hover, true);
    lab1Btn->installEventFilter(this);
    lab2Btn->setAttribute(Qt::WA_Hover, true);
    lab2Btn->installEventFilter(this);
    lab3Btn->setAttribute(Qt::WA_Hover, true);
    lab3Btn->installEventFilter(this);
    lab4Btn->setAttribute(Qt::WA_Hover, true);
    lab4Btn->installEventFilter(this);
    lab5Btn->setAttribute(Qt::WA_Hover, true);
    lab5Btn->installEventFilter(this);
    lab6Btn->setAttribute(Qt::WA_Hover, true);
    lab6Btn->installEventFilter(this);

    lab1Btn->setAttribute(Qt::WA_TranslucentBackground);
    lab2Btn->setAttribute(Qt::WA_TranslucentBackground);
    lab3Btn->setAttribute(Qt::WA_TranslucentBackground);
    lab4Btn->setAttribute(Qt::WA_TranslucentBackground);
    lab5Btn->setAttribute(Qt::WA_TranslucentBackground);
    lab6Btn->setAttribute(Qt::WA_TranslucentBackground);

    loadDrumsFrames();
    loadHugsFrames();
    loadPowerFrames();
    loadLostFrames();

    const int anim_x(600), anim_y(200);
    const double scale(1.7);

    if (!drumsFrames.isEmpty()) {

        drumsAnimation = scene->addPixmap(drumsFrames.first());
        drumsAnimation->setZValue(900);
        drumsAnimation->setPos(anim_x, anim_y);  // (x, y)
        drumsAnimation->setScale(scale);
    }

    if (!hugsFrames.isEmpty()) {
        QPixmap px = hugsFrames.first().scaled(lab1Btn->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        hugsAnimation = scene->addPixmap(px);
        hugsAnimation->setZValue(1100); // above button
        hugsAnimation->setVisible(false);
        hugsAnimation->setPos(anim_x, anim_y);
        hugsAnimation->setScale(scale);
    }

    if (!powerFrames.isEmpty()) {
        QPixmap px = powerFrames.first().scaled(lab1Btn->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        powerAnimation = scene->addPixmap(px);
        powerAnimation->setZValue(1100); // above button
        powerAnimation->setVisible(false);
        powerAnimation->setPos(anim_x, anim_y);
        powerAnimation->setScale(scale);
    }

    if (!lostFrames.isEmpty()) {
        QPixmap px = lostFrames.first().scaled(lab1Btn->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        lostAnimation = scene->addPixmap(px);
        lostAnimation->setZValue(1100); // above button
        lostAnimation->setVisible(false);
        lostAnimation->setPos(anim_x, anim_y);
        lostAnimation->setScale(scale);
    }

    // Animation timer
    animationTimer = new QTimer(this);
    connect(animationTimer, SIGNAL(timeout()), this, SLOT(updateAnimation()));
    animationTimer->start(100);
}

MainWindow::~MainWindow()
{
}

void MainWindow::onLab1ButtonClicked()
{
    MessageBeep(MB_ICONERROR);
    QMessageBox::information(this,
                           "Lab Work 1",
                           "This laboratory work is only available on Windows 10 and newer versions.\n\n"
                           "Please upgrade your operating system to access this feature.");
}

void MainWindow::onLab2ButtonClicked()
{
    if (pciWindow && pciWindow->isVisible()) {
        pciWindow->activateWindow();
        pciWindow->raise();
        return;
    }

    qApp->setQuitOnLastWindowClosed(false);

    if (animationTimer && animationTimer->isActive()) {
        animationTimer->stop();
    }
    this->hide();

    pciWindow = new PCIWindow(background);
    pciWindow->setAttribute(Qt::WA_DeleteOnClose);

    connect(pciWindow, SIGNAL(destroyed()), this, SLOT(onPCIClosed()));

    pciWindow->show();
}

void MainWindow::onLab3ButtonClicked()
{
    if (storageWindow && storageWindow->isVisible()) {
        storageWindow->activateWindow();
        storageWindow->raise();
        return;
    }

    qApp->setQuitOnLastWindowClosed(false);

    if (animationTimer && animationTimer->isActive()) {
        animationTimer->stop();
    }
    this->hide();

    storageWindow = new StorageWindow(background);
    storageWindow->setAttribute(Qt::WA_DeleteOnClose);

    connect(storageWindow, SIGNAL(destroyed()), this, SLOT(onStorageClosed()));

    storageWindow->show();
}

void MainWindow::onLab456ButtonClicked()
{
    MessageBeep(MB_ICONERROR);
    QMessageBox::information(this,
                             "Lab Work 3-6",
                             "Yet to be implemented, my frosty apprentice!");
}

void MainWindow::onPCIClosed()
{
    qApp->setQuitOnLastWindowClosed(true);
    this->show();
    if (animationTimer) animationTimer->start(100);
    if (hugsAnimation) hugsAnimation->setVisible(false);
}

void MainWindow::onStorageClosed()
{
    qApp->setQuitOnLastWindowClosed(true);
    this->show();
    if (animationTimer) animationTimer->start(100);

    if (hugsAnimation) hugsAnimation->setVisible(false);
}

void MainWindow::loadDrumsFrames()
{
    drumsFrames.clear();
    for (int i = 1; i <= 21; ++i) {
        QString framePath = QString(":/drums/img/drum_%1.png").arg(i);
        QPixmap frame(framePath);

        if (frame.isNull()) {
            qDebug() << "Error: not found drums frames" << framePath;
            continue;
        }

        QPixmap scaledFrame = frame.scaled(640, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        drumsFrames.append(scaledFrame);
    }

    qDebug() << "Loaded" << drumsFrames.size() << "drums frames";
}

void MainWindow::loadHugsFrames()
{
    hugsFrames.clear();
    for (int i = 1; i <= 18; ++i) {
        QString framePath = QString(":/hugs/img/hugs_%1.png").arg(i);
        QPixmap frame(framePath);

        if (frame.isNull()) {
            qDebug() << "Error: not found hugs frame" << framePath;
            continue;
        }

        QPixmap scaledFrame = frame; // store original-ish; we'll scale dynamically for larger size
        hugsFrames.append(scaledFrame);
    }

    qDebug() << "Loaded" << hugsFrames.size() << "hugs frames";
}

void MainWindow::loadPowerFrames()
{
    powerFrames.clear();
    for (int i = 1; i <= 20; ++i) {
        QString framePath = QString(":/power/img/power_%1.png").arg(i);
        QPixmap frame(framePath);

        if (frame.isNull()) {
            qDebug() << "Error: not found power frame" << framePath;
            continue;
        }

        QPixmap scaledFrame = frame; // store original-ish; we'll scale dynamically for larger size
        powerFrames.append(scaledFrame);
    }

    qDebug() << "Loaded" << powerFrames.size() << "power frames";
}

void MainWindow::loadLostFrames()
{
    lostFrames.clear();
    for (int i = 1; i <= 64; ++i) {
        QString framePath = QString(":/lost/img/lost_%1.png").arg(i);
        QPixmap frame(framePath);

        if (frame.isNull()) {
            qDebug() << "Error: not found power frame" << framePath;
            continue;
        }

        QPixmap scaledFrame = frame; // store original-ish; we'll scale dynamically for larger size
        lostFrames.append(scaledFrame);
    }

    qDebug() << "Loaded" << lostFrames.size() << "lost frames";
}


void MainWindow::turnOffAllTheAnimations()
{
    if (drumsAnimation) drumsAnimation->setVisible(false);
    if (hugsAnimation) hugsAnimation->setVisible(false);
    if (powerAnimation) powerAnimation->setVisible(false);
    if (lostAnimation) lostAnimation->setVisible(false);
}

void MainWindow::updateAnimation()
{
    turnOffAllTheAnimations();

    const QSize fixedSize(640, 360); // fixed size for all animations

    switch (currentState)
    {
    case IdleAnimation:
        if (!drumsFrames.isEmpty() && drumsAnimation) {
            currAnimationIndex = (currAnimationIndex + 1) % drumsFrames.size();
            QPixmap px = drumsFrames[currAnimationIndex].scaled(
                fixedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            drumsAnimation->setPixmap(px);
            drumsAnimation->setVisible(true);
        }
        break;

    case HugsAnimation:
        if (!hugsFrames.isEmpty() && hugsAnimation) {
            currAnimationIndex = (currAnimationIndex + 1) % hugsFrames.size();
            QPixmap px = hugsFrames[currAnimationIndex].scaled(
                fixedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            hugsAnimation->setPixmap(px);
            hugsAnimation->setVisible(true);
        }
        break;

    case PowerAnimation:
        if (!powerFrames.isEmpty() && powerAnimation) {
            currAnimationIndex = (currAnimationIndex + 1) % powerFrames.size();
            QPixmap px = powerFrames[currAnimationIndex].scaled(
                fixedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            powerAnimation->setPixmap(px);
            powerAnimation->setVisible(true);
        }
        break;
    case LostAnimation:
        if (!lostFrames.isEmpty() && lostAnimation) {
            currAnimationIndex = (currAnimationIndex + 1) % lostFrames.size();
            QPixmap px = lostFrames[currAnimationIndex].scaled(
                fixedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            lostAnimation->setPixmap(px);
            lostAnimation->setVisible(true);
        }
        break;
    }
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Enter || event->type() == QEvent::HoverEnter) {

        if (obj == lab1Btn)
            currentState = PowerAnimation;
        else if (obj == lab2Btn)
            currentState = HugsAnimation;
        else if ((obj == lab3Btn) || (obj == lab4Btn) || (obj == lab5Btn) || (obj == lab6Btn))
            currentState = LostAnimation;
        else
            currentState = IdleAnimation;

        return false;
    }

    if (event->type() == QEvent::Leave || event->type() == QEvent::HoverLeave) {
        currentState = IdleAnimation;
        return false;
    }

    return QMainWindow::eventFilter(obj, event);
}
