#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);

private slots:
    void openLab1();
    void openLab2();
    void openLab3();
    void openLab4();
    void openLab5();
    void openLab6();
};

#endif // MAINWINDOW_H
