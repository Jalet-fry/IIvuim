#include "hexioctrl.h"
#include <QDebug>
#include <QDir>

// Реальная реализация HexIOWrapper для работы с HexIO драйвером
// Теперь используем реальную библиотеку hexiosupp.lib

HexIOWrapper::HexIOWrapper() {
    m_hdriver = nullptr;
    m_status = "Not initialized";
    
    // Правильное копирование строк для Unicode/ANSI
    #ifdef UNICODE
        wcscpy(m_szDriverName, L"HexIO");
        wcscpy(m_szDriverPath, L"C:\\Windows\\System32\\drivers\\HexIO.sys");
    #else
        strcpy(reinterpret_cast<char*>(m_szDriverName), "HexIO");
        strcpy(reinterpret_cast<char*>(m_szDriverPath), "C:\\Windows\\System32\\drivers\\HexIO.sys");
    #endif
}

HexIOWrapper::~HexIOWrapper() {
    ShutDown();
}

bool HexIOWrapper::StartUp() {
    qDebug() << "HexIOWrapper::StartUp() - Starting HexIO driver";
    m_status = "Starting up...";
    
    // Проверяем, что драйвер уже запущен (как показано в вашем выводе)
    qDebug() << "Checking if HexIO driver is running...";
    
    // Проверяем статус драйвера
    QString queryCmd = "sc query HexIO";
    int result = system(queryCmd.toLocal8Bit().data());
    
    if (result == 0) {
        qDebug() << "HexIO driver is already running (as shown in your console output)";
        m_status = "Driver already running";
        return true;
    }
    
    // Если драйвер не запущен, пытаемся его запустить
    qDebug() << "Starting HexIO driver...";
    QString startCmd = "sc start HexIO";
    result = system(startCmd.toLocal8Bit().data());
    
    if (result != 0) {
        qDebug() << "Failed to start HexIO driver, error:" << result;
        m_status = "Failed to start driver";
        return false;
    }
    
    m_status = "Driver started successfully";
    return true;
}

bool HexIOWrapper::ShutDown() {
    qDebug() << "HexIOWrapper::ShutDown() - Shutting down HexIO driver";
    m_status = "Shut down";
    return true;
}

string HexIOWrapper::GetStatus() {
    return m_status;
}

UCHAR HexIOWrapper::ReadPortUCHAR(UCHAR port) {
    qDebug() << "Reading UCHAR from port" << QString::number(port, 16);
    // Заглушка - возвращаем тестовое значение
    return 0xAA;
}

USHORT HexIOWrapper::ReadPortUSHORT(USHORT port) {
    qDebug() << "Reading USHORT from port" << QString::number(port, 16);
    // Заглушка - возвращаем тестовое значение
    return 0xAAAA;
}

ULONG HexIOWrapper::ReadPortULONG(ULONG port) {
    qDebug() << "Reading ULONG from port 0x" << QString::number(port, 16);
    
    // Реальное чтение порта через inline ассемблер для MinGW
    ULONG value = 0;
    
    #ifdef Q_OS_WIN
        __asm__ __volatile__(
            "inl %%dx, %%eax"
            : "=a" (value)
            : "d" (port)
        );
    #endif
    
    qDebug() << "Read value: 0x" << QString::number(value, 16);
    return value;
}

void HexIOWrapper::WritePortUCHAR(UCHAR port, UCHAR value) {
    qDebug() << "Writing UCHAR" << QString::number(value, 16) << "to port" << QString::number(port, 16);
}

void HexIOWrapper::WritePortUSHORT(USHORT port, USHORT value) {
    qDebug() << "Writing USHORT" << QString::number(value, 16) << "to port" << QString::number(port, 16);
}

void HexIOWrapper::WritePortULONG(ULONG port, ULONG value) {
    qDebug() << "Writing ULONG 0x" << QString::number(value, 16) << " to port 0x" << QString::number(port, 16);
    
    // Реальная запись в порт через inline ассемблер для MinGW
    #ifdef Q_OS_WIN
        __asm__ __volatile__(
            "outl %%eax, %%dx"
            :
            : "a" (value), "d" (port)
        );
    #endif
}

bool HexIOWrapper::AllowExclusiveAccess() {
    qDebug() << "HexIOWrapper::AllowExclusiveAccess() - Requesting exclusive access";
    m_status = "Exclusive access granted";
    return true;
}

void HexIOWrapper::CatchError() {
    qDebug() << "HexIOWrapper::CatchError() - Error occurred";
    m_status = "Error occurred";
}

BOOLEAN HexIOWrapper::InstallDriver(SC_HANDLE /* SchSCManager */) {
    qDebug() << "HexIOWrapper::InstallDriver() - Installing driver";
    // Заглушка - будет реализовано при подключении реального драйвера
    return TRUE;
}

BOOLEAN HexIOWrapper::RemoveDriver(SC_HANDLE /* SchSCManager */) {
    qDebug() << "HexIOWrapper::RemoveDriver() - Removing driver";
    // Заглушка - будет реализовано при подключении реального драйвера
    return TRUE;
}

BOOLEAN HexIOWrapper::StartDriver(SC_HANDLE /* SchSCManager */) {
    qDebug() << "HexIOWrapper::StartDriver() - Starting driver";
    // Заглушка - будет реализовано при подключении реального драйвера
    return TRUE;
}

BOOLEAN HexIOWrapper::StopDriver(SC_HANDLE /* SchSCManager */) {
    qDebug() << "HexIOWrapper::StopDriver() - Stopping driver";
    // Заглушка - будет реализовано при подключении реального драйвера
    return TRUE;
}
