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
    
    // Инициализация анимации Jake
    m_jakeMovie = new QMovie(":/Animation/Jake dance.gif");
    m_jakeMovie->setScaledSize(QSize(100, 100)); // Размер анимации
    m_jakeLabel->setMovie(m_jakeMovie);
    m_jakeMovie->start();
    
    QTimer::singleShot(100, this, &StorageWindow::scanDevices);
}

StorageWindow::~StorageWindow() {
    delete m_scanner;
    delete m_jakeMovie;
}

void StorageWindow::setupUI() {
    setWindowTitle("Лабораторная работа №3 - Анализатор устройств хранения данных");
    setMinimumSize(1300, 900);  // Увеличили высоту с 800 до 900
    setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0f2027, stop:0.5 #203a43, stop:1 #2c5364);
            font-family: 'Segoe UI', 'Roboto', Arial, sans-serif;
        }
        QGroupBox {
            border: 2px solid #4dd0e1;
            border-radius: 12px;
            margin-top: 20px;
            padding-top: 20px;
            font-weight: bold;
            font-size: 13px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(38, 50, 56, 0.95), stop:1 rgba(55, 71, 79, 0.95));
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 20px;
            padding: 8px 20px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00bcd4, stop:0.5 #0097a7, stop:1 #006064);
            color: white;
            border-radius: 8px;
            font-weight: bold;
            font-size: 14px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        QTableWidget {
            background-color: rgba(38, 50, 56, 0.9);
            alternate-background-color: rgba(55, 71, 79, 0.7);
            gridline-color: #4dd0e1;
            border: 2px solid #00bcd4;
            border-radius: 8px;
            selection-background-color: #0097a7;
            color: #e0f7fa;
        }
        QTableWidget::item {
            padding: 12px;
            font-weight: 600;
            color: #e0f7fa;
            border-bottom: 1px solid rgba(77, 208, 225, 0.3);
        }
        QTableWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00acc1, stop:0.5 #0097a7, stop:1 #00838f);
            color: white;
            font-weight: bold;
        }
        QTableWidget::item:hover {
            background: rgba(77, 208, 225, 0.2);
        }
        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #00bcd4, stop:0.5 #0097a7, stop:1 #006064);
            color: white;
            padding: 12px;
            border: 1px solid #0097a7;
            font-weight: bold;
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        QTextEdit {
            background-color: rgba(38, 50, 56, 0.95);
            border: 2px solid #00bcd4;
            border-radius: 8px;
            padding: 15px;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 14px;
            color: #e0f7fa;
            line-height: 1.8;
        }
        QScrollBar:vertical {
            background: rgba(55, 71, 79, 0.8);
            width: 16px;
            border-radius: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00bcd4, stop:1 #0097a7);
            border-radius: 8px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00e5ff, stop:1 #00bcd4);
        }
        QScrollBar::handle:vertical:pressed {
            background: #006064;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
        QLabel {
            color: #e0f7fa;
        }
    )");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(15);
    
    // Title section with enhanced design
    QWidget *headerWidget = new QWidget();
    headerWidget->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00bcd4, stop:0.5 #0097a7, stop:1 #006064);
            border-radius: 12px;
            padding: 20px;
            border: 2px solid #4dd0e1;
        }
    )");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    
    // Создаем вертикальный layout для текста
    QVBoxLayout *textLayout = new QVBoxLayout();
    
    QLabel *titleLabel = new QLabel("⚡ АНАЛИЗАТОР УСТРОЙСТВ ХРАНЕНИЯ ⚡");
    titleLabel->setStyleSheet(R"(
        font-size: 28px; 
        font-weight: bold; 
        color: white; 
        padding: 12px;
        text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
        letter-spacing: 2px;
    )");
    titleLabel->setAlignment(Qt::AlignCenter);
    textLayout->addWidget(titleLabel);
    
    QLabel *subtitleLabel = new QLabel("● Профессиональный анализ накопителей HDD/SSD/NVMe через WMI интерфейс Windows ●");
    subtitleLabel->setStyleSheet(R"(
        font-size: 13px; 
        color: #e0f7fa; 
        padding: 5px;
        font-weight: 600;
    )");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    textLayout->addWidget(subtitleLabel);
    
    // Добавляем Jake в заголовок
    m_jakeLabel = new QLabel();
    m_jakeLabel->setFixedSize(100, 100);
    m_jakeLabel->setAlignment(Qt::AlignCenter);
    m_jakeLabel->setStyleSheet(R"(
        QLabel {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 50px;
            border: 2px solid rgba(255, 255, 255, 0.3);
        }
    )");
    
    // Добавляем элементы в горизонтальный layout
    headerLayout->addLayout(textLayout, 1); // Текст занимает основное место
    headerLayout->addWidget(m_jakeLabel, 0, Qt::AlignCenter); // Jake справа
    
    mainLayout->addWidget(headerWidget);
    
    m_statusLabel = new QLabel("⚙ Готов к сканированию накопителей...");
    m_statusLabel->setStyleSheet(R"(
        font-size: 14px; 
        color: #00e676; 
        font-weight: bold; 
        padding: 10px 20px;
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
            stop:0 rgba(0, 150, 136, 0.3), stop:1 rgba(0, 200, 83, 0.3));
        border-radius: 6px;
        border: 2px solid #00e676;
    )");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);
    
    QGroupBox *tableGroup = new QGroupBox("Обнаруженные накопители");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableGroup);
    tableLayout->setContentsMargins(10, 20, 10, 10);
    
    // Add refresh button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *refreshButton = new QPushButton("🔄 Обновить список накопителей");
    refreshButton->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00bcd4, stop:1 #0097a7);
            color: white;
            border: 2px solid #4dd0e1;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 13px;
            font-weight: bold;
            min-width: 200px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00acc1, stop:1 #00838f);
            border: 2px solid #00e5ff;
        }
        QPushButton:pressed {
            background: #006064;
        }
    )");
    connect(refreshButton, &QPushButton::clicked, this, &StorageWindow::refreshDevices);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    tableLayout->addLayout(buttonLayout);
    
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
    
    // ВАЖНО: Добавляем вертикальную прокрутку для ТАБЛИЦЫ
    m_diskTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_diskTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Настройка размера строк для лучшей видимости
    m_diskTable->verticalHeader()->setDefaultSectionSize(50);  // Высота строки 50px (увеличено)
    
    connect(m_diskTable, &QTableWidget::cellClicked, this, &StorageWindow::onTableItemClicked);
    
    tableLayout->addWidget(m_diskTable);
    mainLayout->addWidget(tableGroup, 2);  // Таблица - БОЛЬШЕ места (было 1)
    
    QGroupBox *detailsGroup = new QGroupBox("Детальная информация об устройстве");
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);
    detailsLayout->setContentsMargins(10, 20, 10, 10);
    
    m_detailsText = new QTextEdit();
    m_detailsText->setReadOnly(true);
    
    // ВАЖНО: Настройки прокрутки
    m_detailsText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);   // Прокрутка всегда видна
    m_detailsText->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); // Горизонтальная по необходимости
    m_detailsText->setLineWrapMode(QTextEdit::NoWrap);                  // Без переноса строк
    
    m_detailsText->setPlainText("┌──────────────────────────────────────────────────────────────┐\n"
                                 "│  Выберите накопитель из таблицы для просмотра детальной      │\n"
                                 "│  информации о устройстве, включая характеристики,            │\n"
                                 "│  объем памяти и поддерживаемые режимы работы...              │\n"
                                 "└──────────────────────────────────────────────────────────────┘");
    detailsLayout->addWidget(m_detailsText);
    
    mainLayout->addWidget(detailsGroup, 1);  // Детали - МЕНЬШЕ места (было 2) - теперь 2:1!
}

void StorageWindow::scanDevices() {
    m_statusLabel->setText("⏳ Сканирование накопителей...");
    m_statusLabel->setStyleSheet(R"(
        font-size: 14px; 
        color: #ffa726; 
        font-weight: bold; 
        padding: 10px 20px;
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
            stop:0 rgba(255, 167, 38, 0.3), stop:1 rgba(251, 140, 0, 0.3));
        border-radius: 6px;
        border: 2px solid #ffa726;
    )");
    QApplication::processEvents();
    
    if (!m_scanner->initialize()) {
        m_statusLabel->setText("❌ Ошибка инициализации WMI сканера");
        m_statusLabel->setStyleSheet(R"(
            font-size: 14px; 
            color: #ef5350; 
            font-weight: bold; 
            padding: 10px 20px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(239, 83, 80, 0.3), stop:1 rgba(229, 57, 53, 0.3));
            border-radius: 6px;
            border: 2px solid #ef5350;
        )");
        return;
    }
    
    m_disks = m_scanner->scanStorageDevices();
    
    if (m_disks.empty()) {
        m_statusLabel->setText("⚠ Накопители не обнаружены в системе");
        m_statusLabel->setStyleSheet(R"(
            font-size: 14px; 
            color: #ef5350; 
            font-weight: bold; 
            padding: 10px 20px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(239, 83, 80, 0.3), stop:1 rgba(229, 57, 53, 0.3));
            border-radius: 6px;
            border: 2px solid #ef5350;
        )");
        QMessageBox::information(this, "Информация", "Накопители не обнаружены");
    } else {
        m_statusLabel->setText(QString("✓ Обнаружено накопителей: %1 | Общий объем: %2")
            .arg(m_disks.size())
            .arg(formatSize([this]() {
                uint64_t total = 0;
                for (const auto& d : m_disks) total += d.totalSize;
                return total;
            }())));
        m_statusLabel->setStyleSheet(R"(
            font-size: 14px; 
            color: #00e676; 
            font-weight: bold; 
            padding: 10px 20px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(0, 150, 136, 0.3), stop:1 rgba(0, 200, 83, 0.3));
            border-radius: 6px;
            border: 2px solid #00e676;
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
        if (disk.driveType.contains("SSD", Qt::CaseInsensitive) || 
            disk.driveType.contains("NVMe", Qt::CaseInsensitive)) {
            typeItem->setForeground(QBrush(QColor(0, 230, 118))); // Bright green for SSD/NVMe
        } else if (disk.driveType == "HDD") {
            typeItem->setForeground(QBrush(QColor(255, 138, 101))); // Orange for HDD
        } else {
            typeItem->setForeground(QBrush(QColor(100, 181, 246))); // Blue for unknown
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
    
    // Helper function для выравнивания текста
    auto formatLine = [](const QString& label, const QString& value, int totalWidth = 60) -> QString {
        int labelWidth = 25;
        QString result = "│  " + label.leftJustified(labelWidth) + value;
        // Дополняем пробелами до нужной ширины
        int currentWidth = 3 + labelWidth + value.length(); // 3 = "│  " + последний пробел
        if (currentWidth < totalWidth) {
            result += QString(" ").repeated(totalWidth - currentWidth);
        }
        result += " │\n";
        return result;
    };
    
    QString details;
    details += "╔═══════════════════════════════════════════════════════════════╗\n";
    details += QString("║  %1%2║\n")
        .arg(disk.model.toUpper())
        .arg(QString(" ").repeated(qMax(0, 61 - disk.model.length())));
    details += "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    details += "┌─────────────────────────────────────────────────────────────┐\n";
    details += "│ ⚙️  ОСНОВНАЯ ИНФОРМАЦИЯ                                     │\n";
    details += "├─────────────────────────────────────────────────────────────┤\n";
    details += formatLine("Модель:", disk.model);
    details += formatLine("Изготовитель:", disk.manufacturer);
    details += formatLine("Серийный номер:", disk.serialNumber);
    details += formatLine("Версия прошивки:", disk.firmwareVersion);
    details += formatLine("Тип накопителя:", disk.driveType);
    
    // ВАЖНО: Выделяем тип интерфейса
    QString interfaceHighlight = disk.interfaceType;
    if (disk.interfaceType == "NVMe") {
        interfaceHighlight = "🚀 " + disk.interfaceType + " (PCIe)";
    } else if (disk.interfaceType == "SATA") {
        interfaceHighlight = "💾 " + disk.interfaceType;
    } else if (disk.interfaceType == "USB") {
        interfaceHighlight = "🔌 " + disk.interfaceType;
    }
    details += formatLine("Интерфейс подключения:", interfaceHighlight);
    details += formatLine("Системный диск:", disk.isSystemDrive ? "Да ✓" : "Нет");
    details += "└─────────────────────────────────────────────────────────────┘\n\n";
    
    details += "┌─────────────────────────────────────────────────────────────┐\n";
    details += "│ 💾 ИНФОРМАЦИЯ О ПАМЯТИ                                      │\n";
    details += "├─────────────────────────────────────────────────────────────┤\n";
    details += formatLine("Общий объем:", formatSize(disk.totalSize));
    details += formatLine("Свободно:", formatSize(disk.freeSpace));
    details += formatLine("Занято:", formatSize(disk.usedSpace));
    details += "├─────────────────────────────────────────────────────────────┤\n";
    
    double usedPercent = disk.totalSize > 0 ? 
        (double)disk.usedSpace / disk.totalSize * 100.0 : 0.0;
    double freePercent = 100.0 - usedPercent;
    
    details += QString("│  Использовано:           %1% ").arg(QString::number(usedPercent, 'f', 1), 5);
    int usedBars = (int)(usedPercent / 5);
    details += "[";
    for (int i = 0; i < 20; i++) {
        details += (i < usedBars) ? "█" : "░";
    }
    details += "]    │\n";
    details += QString("│  Свободно:               %1%                            │\n")
        .arg(QString::number(freePercent, 'f', 1).leftJustified(5));
    details += "└─────────────────────────────────────────────────────────────┘\n\n";
    
    if (!disk.supportedModes.isEmpty()) {
        details += "┌─────────────────────────────────────────────────────────────┐\n";
        details += "│ 🔧 ПОДДЕРЖИВАЕМЫЕ РЕЖИМЫ                                    │\n";
        details += "├─────────────────────────────────────────────────────────────┤\n";
        for (const QString& mode : disk.supportedModes) {
            QString modeLine = "│  ▸ " + mode;
            int padding = 62 - modeLine.length();
            if (padding > 0) {
                modeLine += QString(" ").repeated(padding);
            }
            modeLine += "│\n";
            details += modeLine;
        }
        details += "└─────────────────────────────────────────────────────────────┘\n\n";
    }
    
    details += "╔═══════════════════════════════════════════════════════════════╗\n";
    if (disk.interfaceType == "NVMe") {
        details += "║  ✅ Тип интерфейса определён через WMI MSFT_PhysicalDisk   ║\n";
    } else {
        details += "║  ℹ️  Данные получены через системные вызовы Windows API    ║\n";
    }
    details += "╚═══════════════════════════════════════════════════════════════╝";
    
    m_detailsText->setPlainText(details);
}

void StorageWindow::refreshDevices() {
    m_detailsText->setPlainText("┌──────────────────────────────────────────────────────────────┐\n"
                                 "│  Обновление списка накопителей...                            │\n"
                                 "│  Пожалуйста, подождите...                                    │\n"
                                 "└──────────────────────────────────────────────────────────────┘");
    QApplication::processEvents();
    
    // Re-scan devices
    scanDevices();
}

QString StorageWindow::formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    // Use binary (1024) conversion like Windows does
    while (size >= 1024.0 && unitIndex < 5) {
        size /= 1024.0;
        unitIndex++;
    }
    
    // Format with appropriate precision based on size
    QString sizeStr;
    if (unitIndex >= 3) { // GB, TB, PB
        sizeStr = QString::number(size, 'f', 2);
    } else if (unitIndex == 2) { // MB
        sizeStr = QString::number(size, 'f', 1);
    } else { // B, KB
        sizeStr = QString::number(size, 'f', 0);
    }
    
    return QString("%1 %2").arg(sizeStr).arg(units[unitIndex]);
}

