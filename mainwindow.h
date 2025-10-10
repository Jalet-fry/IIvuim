#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QTimer>
#include <QStringList>
#include <QDir>

class BatteryWidget;
class JakeWidget;
class StorageWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void openLab1();
    void openLab2();
    void openLab3();
    void openLab4();
    void openLab5();
    void openLab6();

private:
    BatteryWidget *batteryWidget;
    JakeWidget *jakeWidget;
    StorageWindow *storageWindow;
    void setupJake();
    bool isHoveringButton;
    QPixmap backgroundPixmap;
    QTimer *backgroundTimer;
    QStringList backgroundImages;
    int currentBackgroundIndex;
    
    void changeBackground();
    void loadRandomBackground();
    void scanBackgroundImages();
};

#endif // MAINWINDOW_H