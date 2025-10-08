#include "storagewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QApplication>
#include <QTableWidgetItem>
#include <QFont>
#include <QTimer>

StorageWindow::StorageWindow(QWidget *parent) : QWidget(parent), m_scanner(new StorageScanner()) {
    setupUI();
    QTimer::singleShot(100, this, &StorageWindow::scanDevices);
}

StorageWindow::~StorageWindow() {
    delete m_scanner;
}

void StorageWindow::setupUI() {
    setWindowTitle("Лабораторная работа №3 - Устройства хранения данных");
    setMinimumSize(1200, 750);
    setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #f0f4f8, stop:0.5 #e8eef5, stop:1 #f5f7fa);
            font-family: 'Segoe UI', 'Roboto', Arial, sans-serif;
        }
        QGroupBox {
            border: 2px solid #607D8B;
            border-radius: 10px;
            margin-top: 18px;
            padding-top: 18px;
            font-weight: bold;
            font-size: 13px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(255, 255, 255, 0.95), stop:1 rgba(250, 250, 252, 0.95));
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 20px;
            padding: 6px 15px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #546E7A, stop:1 #37474F);
            color: white;
            border-radius: 6px;
            font-weight: bold;
        }
        QTableWidget {
            background-color: white;
            alternate-background-color: #f8f9fa;
            gridline-color: #e0e0e0;
            border: 2px solid #90A4AE;
            border-radius: 6px;
            selection-background-color: #546E7A;
        }
        QTableWidget::item {
            padding: 10px;
            font-weight: 600;
            color: #263238;
        }
        QTableWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #546E7A, stop:1 #455A64);
            color: white;
        }
        QTableWidget::item:hover {
            background: #eceff1;
        }
        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #546E7A, stop:1 #37474F);
            color: white;
            padding: 10px;
            border: 1px solid #455A64;
            font-weight: bold;
            font-size: 12px;
            text-transform: uppercase;
        }
        QTextEdit {
            background-color: white;
            border: 2px solid #78909C;
            border-radius: 6px;
            padding: 12px;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 11px;
            color: #263238;
            line-height: 1.5;
        }
        QLabel {
            color: #37474F;
        }
    )");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(15);
    
    // Title section
    QWidget *headerWidget = new QWidget();
    headerWidget->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #546E7A, stop:1 #37474F);
            border-radius: 10px;
            padding: 15px;
        }
    )");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    
    QLabel *titleLabel = new QLabel("СКАНЕР УСТРОЙСТВ ХРАНЕНИЯ ДАННЫХ");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: white; padding: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(titleLabel);
    
    QLabel *subtitleLabel = new QLabel("Анализ накопителей HDD/SSD через WMI интерфейс Windows");
    subtitleLabel->setStyleSheet("font-size: 12px; color: #B0BEC5; padding: 0px;");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(subtitleLabel);
    
    mainLayout->addWidget(headerWidget);
    
    m_statusLabel = new QLabel("Готов к сканированию");
    m_statusLabel->setStyleSheet(R"(
        font-size: 13px; 
        color: #1B5E20; 
        font-weight: bold; 
        padding: 8px 15px;
        background: #E8F5E9;
        border-radius: 5px;
        border-left: 4px solid #4CAF50;
    )");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);
    
    QGroupBox *tableGroup = new QGroupBox("Обнаруженные накопители");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableGroup);
    tableLayout->setContentsMargins(10, 20, 10, 10);
    
    m_diskTable = new QTableWidget();
    m_diskTable->setColumnCount(8);
    m_diskTable->setHorizontalHeaderLabels(QStringList()
        << "Модель" << "Изготовитель" << "Серийный номер"
        << "Прошивка" << "Интерфейс" << "Тип" << "Объем" << "Свободно");
    
    m_diskTable->setAlternatingRowColors(true);
    m_diskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_diskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_diskTable->horizontalHeader()->setStretchLastSection(true);
    m_diskTable->setSortingEnabled(true);
    m_diskTable->verticalHeader()->setVisible(false);
    m_diskTable->setShowGrid(true);
    
    connect(m_diskTable, &QTableWidget::cellClicked, this, &StorageWindow::onTableItemClicked);
    
    tableLayout->addWidget(m_diskTable);
    mainLayout->addWidget(tableGroup, 2);
    
    QGroupBox *detailsGroup = new QGroupBox("Детальная информация об устройстве");
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);
    detailsLayout->setContentsMargins(10, 20, 10, 10);
    
    m_detailsText = new QTextEdit();
    m_detailsText->setReadOnly(true);
    m_detailsText->setPlainText("Выберите накопитель из таблицы для просмотра детальной информации...");
    detailsLayout->addWidget(m_detailsText);
    
    mainLayout->addWidget(detailsGroup, 1);
}

void StorageWindow::scanDevices() {
    m_statusLabel->setText("Сканирование накопителей...");
    m_statusLabel->setStyleSheet(R"(
        font-size: 13px; 
        color: #E65100; 
        font-weight: bold; 
        padding: 8px 15px;
        background: #FFF3E0;
        border-radius: 5px;
        border-left: 4px solid #FF9800;
    )");
    QApplication::processEvents();
    
    if (!m_scanner->initialize()) {
        m_statusLabel->setText("Ошибка инициализации сканера");
        m_statusLabel->setStyleSheet(R"(
            font-size: 13px; 
            color: #C62828; 
            font-weight: bold; 
            padding: 8px 15px;
            background: #FFEBEE;
            border-radius: 5px;
            border-left: 4px solid #F44336;
        )");
        return;
    }
    
    m_disks = m_scanner->scanStorageDevices();
    
    if (m_disks.empty()) {
        m_statusLabel->setText("Накопители не обнаружены");
        m_statusLabel->setStyleSheet(R"(
            font-size: 13px; 
            color: #C62828; 
            font-weight: bold; 
            padding: 8px 15px;
            background: #FFEBEE;
            border-radius: 5px;
            border-left: 4px solid #F44336;
        )");
        QMessageBox::information(this, "Информация", "Накопители не обнаружены");
    } else {
        m_statusLabel->setText(QString("Обнаружено накопителей: %1 | Общий объем: %2")
            .arg(m_disks.size())
            .arg(formatSize([this]() {
                uint64_t total = 0;
                for (const auto& d : m_disks) total += d.totalSize;
                return total;
            }())));
        m_statusLabel->setStyleSheet(R"(
            font-size: 13px; 
            color: #1B5E20; 
            font-weight: bold; 
            padding: 8px 15px;
            background: #E8F5E9;
            border-radius: 5px;
            border-left: 4px solid #4CAF50;
        )");
        populateTable(m_disks);
    }
}

void StorageWindow::populateTable(const std::vector<StorageDevice>& disks) {
    m_diskTable->setRowCount(0);
    
    for (const auto& disk : disks) {
        int row = m_diskTable->rowCount();
        m_diskTable->insertRow(row);
        
        auto addItem = [this, row](int col, const QString& text, bool bold = false) {
            QTableWidgetItem *item = new QTableWidgetItem(text);
            if (bold) item->setFont(QFont("Arial", 10, QFont::Bold));
            item->setTextAlignment(Qt::AlignCenter);
            m_diskTable->setItem(row, col, item);
        };
        
        addItem(0, disk.model, true);
        addItem(1, disk.manufacturer);
        addItem(2, disk.serialNumber);
        addItem(3, disk.firmwareVersion);
        addItem(4, disk.interfaceType);
        
        QTableWidgetItem *typeItem = new QTableWidgetItem(disk.driveType);
        typeItem->setFont(QFont("Arial", 10, QFont::Bold));
        typeItem->setTextAlignment(Qt::AlignCenter);
        if (disk.driveType == "SSD") {
            typeItem->setForeground(QBrush(QColor(0, 150, 136)));
        } else {
            typeItem->setForeground(QBrush(QColor(255, 87, 34)));
        }
        m_diskTable->setItem(row, 5, typeItem);
        
        addItem(6, formatSize(disk.totalSize));
        
        QTableWidgetItem *freeItem = new QTableWidgetItem(formatSize(disk.freeSpace));
        freeItem->setTextAlignment(Qt::AlignCenter);
        freeItem->setForeground(QBrush(QColor(76, 175, 80)));
        freeItem->setFont(QFont("Arial", 10, QFont::Bold));
        m_diskTable->setItem(row, 7, freeItem);
    }
    
    m_diskTable->resizeColumnsToContents();
}

void StorageWindow::onTableItemClicked(int row, int column) {
    Q_UNUSED(column);
    
    if (row < 0 || row >= static_cast<int>(m_disks.size())) return;
    
    const StorageDevice& disk = m_disks[row];
    
    QString details;
    details += "═══════════════════════════════════════════════════════════════\n";
    details += QString("  %1\n").arg(disk.model.toUpper());
    details += "═══════════════════════════════════════════════════════════════\n\n";
    
    details += "[ ОСНОВНАЯ ИНФОРМАЦИЯ ]\n";
    details += "───────────────────────────────────────────────────────────────\n";
    details += QString("  Модель:                 %1\n").arg(disk.model);
    details += QString("  Изготовитель:           %1\n").arg(disk.manufacturer);
    details += QString("  Серийный номер:         %1\n").arg(disk.serialNumber);
    details += QString("  Версия прошивки:        %1\n").arg(disk.firmwareVersion);
    details += QString("  Тип накопителя:         %1\n").arg(disk.driveType);
    details += QString("  Интерфейс подключения:  %1\n").arg(disk.interfaceType);
    details += QString("  Системный диск:         %1\n").arg(disk.isSystemDrive ? "Да" : "Нет");
    details += "\n";
    
    details += "[ ИНФОРМАЦИЯ О ПАМЯТИ ]\n";
    details += "───────────────────────────────────────────────────────────────\n";
    details += QString("  Общий объем:            %1\n").arg(formatSize(disk.totalSize));
    details += QString("  Свободно:               %1\n").arg(formatSize(disk.freeSpace));
    details += QString("  Занято:                 %1\n").arg(formatSize(disk.usedSpace));
    
    double usedPercent = disk.totalSize > 0 ? 
        (double)disk.usedSpace / disk.totalSize * 100.0 : 0.0;
    double freePercent = 100.0 - usedPercent;
    
    details += QString("  Использовано:           %1% ").arg(QString::number(usedPercent, 'f', 1));
    int usedBars = (int)(usedPercent / 5);
    details += "[";
    for (int i = 0; i < 20; i++) {
        details += (i < usedBars) ? "█" : "░";
    }
    details += "]\n";
    details += QString("  Свободно:               %1%\n").arg(QString::number(freePercent, 'f', 1));
    details += "\n";
    
    if (!disk.supportedModes.isEmpty()) {
        details += "[ ПОДДЕРЖИВАЕМЫЕ РЕЖИМЫ ]\n";
        details += "───────────────────────────────────────────────────────────────\n";
        for (const QString& mode : disk.supportedModes) {
            details += QString("  > %1\n").arg(mode);
        }
        details += "\n";
    }
    
    details += "═══════════════════════════════════════════════════════════════";
    
    m_detailsText->setPlainText(details);
}

QString StorageWindow::formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    return QString("%1 %2").arg(QString::number(size, 'f', 2)).arg(units[unitIndex]);
}

