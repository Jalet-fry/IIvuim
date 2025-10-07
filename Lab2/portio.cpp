#include "portio.h"

#ifdef Q_OS_WIN
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>

static bool g_portAccessEnabled = false;

// Функция для записи отладочной информации в файл
void writeToLogFile(const QString& message) {
    // Выводим в консоль Qt Creator
    qDebug() << message;
    
    // Пытаемся записать в файл в нескольких местах
    QStringList paths;
    paths << "pci_debug_log.txt";
    paths << "C:\\pci_debug_log.txt";
    paths << QDir::currentPath() + "/pci_debug_log.txt";
    
    for (const QString& path : paths) {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream out(&file);
            out << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << " - " << message << "\n";
            file.close();
            qDebug() << "Log written to:" << path;
            break;
        }
    }
    
    // Также показываем MessageBox для критических сообщений
    if (message.contains("ERROR") || message.contains("CRITICAL") || message.contains("FAILED")) {
        QMessageBox::information(nullptr, "PCI Debug", message);
    }
}

bool enablePortAccess() {
    writeToLogFile("=== ENABLING PORT ACCESS FUNCTION CALLED ===");
    
    if (g_portAccessEnabled) {
        writeToLogFile("Port access already enabled");
        return true;
    }
    
    writeToLogFile("=== ENABLING PORT ACCESS ===");
    writeToLogFile("Attempting to enable port access via HexIO driver...");
    
    // Проверяем версию Windows
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osvi)) {
        writeToLogFile(QString("Windows version: %1.%2").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion));
        if (osvi.dwMajorVersion == 5) {
            writeToLogFile("Windows XP detected - direct port access should work");
        } else {
            writeToLogFile("Not Windows XP - direct port access may not work");
        }
    } else {
        writeToLogFile("Failed to get Windows version");
    }
    
    // Проверяем права администратора
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            if (elevation.TokenIsElevated) {
                writeToLogFile("Running with administrator privileges - GOOD!");
            } else {
                writeToLogFile("NOT running with administrator privileges - BAD!");
                writeToLogFile("Please run the application as administrator");
            }
        } else {
            writeToLogFile(QString("Failed to get elevation info: %1").arg(GetLastError()));
        }
        CloseHandle(hToken);
    } else {
        writeToLogFile(QString("Failed to open process token: %1").arg(GetLastError()));
    }
    
    // Проверяем статус драйвера
    writeToLogFile("Checking HexIO driver status...");
    QString queryCmd = "sc query HexIO";
    writeToLogFile("Running command: " + queryCmd);
    int result = system(queryCmd.toLocal8Bit().data());
    writeToLogFile(QString("Command result: %1").arg(result));
    
    // Тестируем прямой доступ к портам
    writeToLogFile("Testing direct port access...");
    try {
        writeToLogFile("Writing 0x80000000 to port 0xCF8...");
        outpd(0xCF8, 0x80000000);
        writeToLogFile("Write successful");
        
        writeToLogFile("Reading from port 0xCFC...");
        unsigned long test = inpd(0xCFC);
        writeToLogFile(QString("Read value: 0x%1").arg(test, 8, 16, QChar('0')));
        
        if (test == 0xFFFFFFFF) {
            writeToLogFile("ERROR: Got 0xFFFFFFFF - no device or access denied");
        } else if (test == 0x00000000) {
            writeToLogFile("ERROR: Got 0x00000000 - no device");
        } else {
            writeToLogFile("SUCCESS: Got valid data from PCI port");
            g_portAccessEnabled = true;
            writeToLogFile("Port access test successful - direct port access working");
            return true;
        }
    } catch (...) {
        writeToLogFile("EXCEPTION: Port access test failed with exception");
    }
    
    writeToLogFile("=== PORT ACCESS FAILED ===");
    writeToLogFile("Possible reasons:");
    writeToLogFile("1. Not running as administrator");
    writeToLogFile("2. HexIO driver not running");
    writeToLogFile("3. System doesn't allow direct port access");
    writeToLogFile("4. VirtualBox restrictions");
    return false;
}

void disablePortAccess() {
    if (!g_portAccessEnabled) {
        return;
    }
    
    g_portAccessEnabled = false;
    qDebug() << "Port access disabled";
}

#else
// Реализации для не-Windows платформ
bool enablePortAccess() {
    return false;
}

void disablePortAccess() {
}

void writeToLogFile(const QString&) {
    // Заглушка для не-Windows платформ
}
#endif
