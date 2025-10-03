#include "batterywidget.h"
#include "batteryworker.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QDebug>
#include <QMessageBox>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <powrprof.h>
#endif

BatteryWidget::BatteryWidget(QWidget *parent) : QWidget(parent)
{
    setupUI();
    batteryWorker = new BatteryWorker(this);
    connect(batteryWorker, &BatteryWorker::batteryInfoUpdated,
            this, &BatteryWidget::onBatteryInfoUpdated);
    
    // Устанавливаем свойства окна
    setWindowTitle("Лабораторная работа 1 - Информация о батарее");
    setFixedSize(500, 400);
}

BatteryWidget::~BatteryWidget()
{
    stopWorker();
}

void BatteryWidget::showAndStart()
{
    startWorker(); // Запускаем мониторинг
    show();        // Показываем окно
    raise();       // Поднимаем на передний план
    activateWindow(); // Активируем окно
}

void BatteryWidget::closeEvent(QCloseEvent* event) {
    stopWorker();
    event->accept();
}

void BatteryWidget::setupUI()
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    QGroupBox *infoGroup = new QGroupBox("Информация о питании");
    infoGroup->setStyleSheet("QGroupBox { font-weight: bold; color: blue; }");
    QGridLayout *infoLayout = new QGridLayout(infoGroup);

    int row = 0;
    infoLayout->addWidget(new QLabel("Тип питания:"), row, 0);
    powerTypeLabel = new QLabel("Неизвестно");
    powerTypeLabel->setStyleSheet("font-weight: bold; color: green;");
    infoLayout->addWidget(powerTypeLabel, row++, 1);

    infoLayout->addWidget(new QLabel("Тип батареи:"), row, 0);
    batteryChemistryLabel = new QLabel("Неизвестно");
    infoLayout->addWidget(batteryChemistryLabel, row++, 1);

    infoLayout->addWidget(new QLabel("Уровень заряда:"), row, 0);
    chargeLevelBar = new QProgressBar;
    chargeLevelBar->setRange(0, 100);
    chargeLevelBar->setValue(0);
    chargeLevelBar->setStyleSheet("QProgressBar::chunk { background-color: #5cb85c; }");
    infoLayout->addWidget(chargeLevelBar, row++, 1);

    infoLayout->addWidget(new QLabel("Режим энергосбережения:"), row, 0);
    powerSaveLabel = new QLabel("Неизвестно");
    infoLayout->addWidget(powerSaveLabel, row++, 1);

    infoLayout->addWidget(new QLabel("Оставшееся время:"), row, 0);
    batteryLifeLabel = new QLabel("Неизвестно");
    infoLayout->addWidget(batteryLifeLabel, row++, 1);

    infoLayout->addWidget(new QLabel("Полное время работы:"), row, 0);
    batteryFullLifeLabel = new QLabel("Неизвестно");
    infoLayout->addWidget(batteryFullLifeLabel, row++, 1);

    layout->addWidget(infoGroup, 0, 0, 1, 2);

    displayOffButton = new QPushButton("Выключить экран");
    hibernateButton = new QPushButton("Гибернация");
    displayOffButton->setStyleSheet("QPushButton { background-color: #f0ad4e; color: white; padding: 5px; }");
    hibernateButton->setStyleSheet("QPushButton { background-color: #d9534f; color: white; padding: 5px; }");

    layout->addWidget(displayOffButton, 1, 0);
    layout->addWidget(hibernateButton, 1, 1);

    connect(displayOffButton, &QPushButton::clicked, this, &BatteryWidget::onDisplayOffClicked);
    connect(hibernateButton, &QPushButton::clicked, this, &BatteryWidget::onHibernateClicked);
}

void BatteryWidget::onBatteryInfoUpdated(const QString &powerType, const QString &batteryChemistry,
                                        int chargeLevel, const QString &powerSaveMode,
                                        int batteryLifeTime, int batteryFullLifeTime)
{
    powerTypeLabel->setText(powerType);
    batteryChemistryLabel->setText(batteryChemistry);
    chargeLevelBar->setValue(chargeLevel);
    powerSaveLabel->setText(powerSaveMode);

    if(powerType == "AC supply") {
        batteryLifeLabel->setText("Не применимо (питание от сети)");
        batteryFullLifeLabel->setText("Не применимо (питание от сети)");
        return;
    } else {
        if (batteryLifeTime == -1) {
            batteryLifeLabel->setText("Вычисляется...");
        } else {
            int hours = batteryLifeTime / 3600;
            int minutes = (batteryLifeTime % 3600) / 60;
            batteryLifeLabel->setText(QString("%1 ч %2 м").arg(hours).arg(minutes));
        }

        if (batteryFullLifeTime == -1) {
            batteryFullLifeLabel->setText("Вычисляется...");
        } else {
            int hours = batteryFullLifeTime / 3600;
            int minutes = (batteryFullLifeTime % 3600) / 60;
            batteryFullLifeLabel->setText(QString("%1 ч %2 м").arg(hours).arg(minutes));
        }
    }
}

void BatteryWidget::onDisplayOffClicked()
{
    qDebug() << "Выключение экрана";

#ifdef Q_OS_WIN
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM) 2);
    QMessageBox::information(this, "Экран", "Экран выключен. Нажмите любую клавишу или пошевелите мышкой для включения.");
#else
    QMessageBox::information(this, "Экран", "Функция выключения экрана доступна только в Windows");
#endif
}

void BatteryWidget::onHibernateClicked()
{
    qDebug() << "Активация гибернации";
    
    int ret = QMessageBox::question(this, "Гибернация",
        "Вы уверены, что хотите перевести компьютер в режим гибернации?\n"
        "Все несохраненные данные будут сохранены на диск.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
#ifdef Q_OS_WIN
        HANDLE hToken;
        TOKEN_PRIVILEGES tp;
        LUID luid;
        
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            if (LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME, &luid)) {
                tp.PrivilegeCount = 1;
                tp.Privileges[0].Luid = luid;
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                
                AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
                
                if (GetLastError() == ERROR_SUCCESS) {
                    SetSuspendState(TRUE, TRUE, FALSE);
                } else {
                    QMessageBox::warning(this, "Ошибка", "Не удалось получить права для гибернации");
                }
            }
            CloseHandle(hToken);
        }
#else
        QMessageBox::information(this, "Гибернация", "Функция гибернации доступна только в Windows");
#endif
    }
}

void BatteryWidget::startWorker() {
    batteryWorker->startMonitoring();
}

void BatteryWidget::stopWorker() {
    batteryWorker->stopMonitoring();
}