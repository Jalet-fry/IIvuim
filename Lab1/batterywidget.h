#ifndef BATTERYWIDGET_H
#define BATTERYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QCloseEvent>
#include "batteryworker.h"

class BatteryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BatteryWidget(QWidget *parent = 0);
    void startWorker();
    void stopWorker();
    ~BatteryWidget();

    // Метод для показа окна с запуском мониторинга
    void showAndStart();

public slots:
    void onBatteryInfoUpdated(const BatteryWorker::BatteryInfo &info);

private slots:
    void onDisplayOffClicked();
    void onHibernateClicked();

protected:
    void closeEvent(QCloseEvent *event);

private:
    void setupUI();

    QLabel *powerTypeLabel;
    QLabel *batteryChemistryLabel;
    QProgressBar *chargeLevelBar;
    QLabel *powerSaveLabel;
    QLabel *batteryLifeLabel;
    QLabel *batteryFullLifeLabel;
    QPushButton *displayOffButton;
    QPushButton *hibernateButton;

    BatteryWorker *batteryWorker;
};

#endif // BATTERYWIDGET_H