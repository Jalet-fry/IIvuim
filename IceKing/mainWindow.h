#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPushButton>
#include <QTimer>
#include <QPixmap>

class PCIWindow;
class StorageWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum AnimationState {
        IdleAnimation,   // default = drums
        PowerAnimation,  // Lab 1 hover
        HugsAnimation,    // Lab 2 hover
        LostAnimation //lab 3-6
    };

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void onLab1ButtonClicked();
    void onLab2ButtonClicked();
    void onLab3ButtonClicked();
    void onLab456ButtonClicked();
    void updateAnimation();
    void onPCIClosed();
    void onStorageClosed();

private:
    void loadDrumsFrames();
    void loadHugsFrames();
    void loadPowerFrames();
    void loadLostFrames();

    void turnOffAllTheAnimations();

    QGraphicsScene *scene;
    QGraphicsView *view;
    QPixmap background;

    QPushButton *lab1Btn;
    QPushButton *lab2Btn;
    QPushButton *lab3Btn;
    QPushButton *lab4Btn;
    QPushButton *lab5Btn;
    QPushButton *lab6Btn;

    QGraphicsProxyWidget *lab1Proxy;
    QGraphicsProxyWidget *lab2Proxy;
    QGraphicsProxyWidget *lab3Proxy;
    QGraphicsProxyWidget *lab4Proxy;
    QGraphicsProxyWidget *lab5Proxy;
    QGraphicsProxyWidget *lab6Proxy;

    QPushButton *currentHoverButton;
    QGraphicsProxyWidget *currentHoverButtonProxy;

    QTimer *animationTimer;
    QList<QPixmap> drumsFrames;
    QList<QPixmap> hugsFrames;
    QList<QPixmap> powerFrames;
    QList<QPixmap> lostFrames;

    int currAnimationIndex;

    bool hoverActive;

    QGraphicsPixmapItem *drumsAnimation;
    QGraphicsPixmapItem *hugsAnimation;
    QGraphicsPixmapItem *powerAnimation;
    QGraphicsPixmapItem *lostAnimation;

    PCIWindow *pciWindow;
    StorageWindow *storageWindow;

    AnimationState currentState;
};

#endif // MAINWINDOW_H
