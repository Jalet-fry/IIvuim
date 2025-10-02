#ifndef BATTERYWIDGET_H
#define BATTERYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QCloseEvent>

class BatteryWorker;

class BatteryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BatteryWidget(QWidget *parent = 0);
    void startWorker();
    void stopWorker();
    ~BatteryWidget();

public slots:
    void onBatteryInfoUpdated(const QString &powerType, const QString &batteryChemistry,
                             int chargeLevel, const QString &powerSaveMode,
                             int batteryLifeTime, int batteryFullLifeTime);

private slots:
    void onDisplayOffClicked();  // Переименовали - только выключение дисплея
    void onHibernateClicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();

    QLabel *powerTypeLabel;
    QLabel *batteryChemistryLabel;
    QProgressBar *chargeLevelBar;
    QLabel *powerSaveLabel;
    QLabel *batteryLifeLabel;
    QLabel *batteryFullLifeLabel;
    QPushButton *displayOffButton;  // Переименовали
    QPushButton *hibernateButton;

    BatteryWorker *batteryWorker;
};

#endif // BATTERYWIDGET_H
