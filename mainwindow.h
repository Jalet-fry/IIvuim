#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class BatteryWidget;
class JakeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

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
    void setupJake();
};

#endif // MAINWINDOW_H