#include "bluetoothfilesender.h"
#include "bluetoothlogger.h"
#include "bluetoothconnection.h"
#include <QProcess>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <QThread>

BluetoothFileSender::BluetoothFileSender(BluetoothLogger *logger, QObject *parent)
    : QObject(parent)
    , logger(logger)
{
    if (logger) {
        logger->info("FileSender", "Bluetooth File Sender инициализирован");
    }
}

BluetoothFileSender::~BluetoothFileSender()
{
}

bool BluetoothFileSender::sendFile(const QString &filePath, const QString &deviceAddress, const QString &deviceName, BluetoothConnection *connection)
{
    if (!logger) return false;
    
    logger->info("FileSender", "═══════════════════════════════════════");
    logger->info("FileSender", "НАЧАЛО ОТПРАВКИ ФАЙЛА");
    logger->info("FileSender", "═══════════════════════════════════════");
    
    QFileInfo fileInfo(filePath);
    
    logger->info("FileSender", QString("Файл: %1").arg(fileInfo.fileName()));
    logger->info("FileSender", QString("Размер: %1 байт (%.2f MB)")
        .arg(fileInfo.size())
        .arg(fileInfo.size() / 1024.0 / 1024.0));
    logger->info("FileSender", QString("Устройство: %1").arg(deviceName));
    logger->info("FileSender", QString("MAC: %1").arg(deviceAddress));
    logger->info("FileSender", "");
    
    // Проверка существования файла
    if (!fileInfo.exists()) {
        logger->error("FileSender", "Файл не существует!");
        emit transferFailed("Файл не существует");
        return false;
    }
    
    emit transferStarted(fileInfo.fileName());
    
    // Проверяем наличие RFCOMM подключения
    if (!connection || !connection->isConnected()) {
        logger->error("FileSender", "═══════════════════════════════════════");
        logger->error("FileSender", "НЕТ RFCOMM ПОДКЛЮЧЕНИЯ!");
        logger->error("FileSender", "═══════════════════════════════════════");
        logger->error("FileSender", "");
        logger->warning("FileSender", "Для прямой отправки файлов нужно:");
        logger->warning("FileSender", "1. Сначала нажать 'Подключить'");
        logger->warning("FileSender", "2. Дождаться подключения");
        logger->warning("FileSender", "3. Затем отправлять файлы");
        logger->warning("FileSender", "");
        logger->error("FileSender", "Отправка отменена.");
        
        emit transferFailed("Нет RFCOMM подключения. Сначала подключитесь к устройству.");
        return false;
    }
    
    // Есть подключение - отправляем НАПРЯМУЮ!
    logger->info("FileSender", "═══════════════════════════════════════");
    logger->success("FileSender", "✓ ОБНАРУЖЕНО RFCOMM ПОДКЛЮЧЕНИЕ!");
    logger->success("FileSender", "ПРЯМАЯ ОТПРАВКА ЧЕРЕЗ СОКЕТ");
    logger->info("FileSender", "═══════════════════════════════════════");
    logger->info("FileSender", "");
    
    bool result = sendFileDirectly(connection, filePath);
    
    if (result) {
        logger->success("FileSender", "");
        logger->success("FileSender", "✓✓✓ ФАЙЛ ОТПРАВЛЕН УСПЕШНО! ✓✓✓");
        logger->info("FileSender", "");
        emit transferCompleted(fileInfo.fileName());
    } else {
        logger->error("FileSender", "");
        logger->error("FileSender", "✗ ОШИБКА ОТПРАВКИ");
        logger->error("FileSender", "");
        emit transferFailed("Ошибка прямой отправки через RFCOMM");
    }
    
    return result;
}

bool BluetoothFileSender::sendViaFsquirt(const QString &filePath)
{
    logger->info("FileSender", "Запуск fsquirt.exe...");
    logger->logApiCall("CreateProcess", "fsquirt.exe");
    
    // fsquirt.exe - встроенная утилита Windows для Bluetooth File Transfer
    // Она открывает мастер отправки файлов
    
    // Формируем команду с аргументами
    QString nativePath = QDir::toNativeSeparators(filePath);
    QString command = QString("fsquirt.exe /send \"%1\"").arg(nativePath);
    
    logger->debug("FileSender", QString("Команда: %1").arg(command));
    logger->debug("FileSender", "Запуск процесса...");
    
    // В Qt 5.5.1 используем статический метод startDetached
    bool started = QProcess::startDetached("fsquirt.exe", QStringList() << "/send" << nativePath);
    
    if (started) {
        logger->logApiResult("CreateProcess", "SUCCESS - fsquirt запущен", true);
        logger->info("FileSender", "");
        logger->success("FileSender", "✓ Мастер отправки файлов запущен!");
        logger->info("FileSender", "Откроется окно Windows Bluetooth File Transfer");
        logger->info("FileSender", "Выберите устройство в списке и нажмите 'Далее'");
        logger->info("FileSender", "");
        return true;
    } else {
        logger->logApiResult("CreateProcess", "FAILED - не удалось запустить", false);
        logger->error("FileSender", "Не удалось запустить fsquirt.exe");
        return false;
    }
}

bool BluetoothFileSender::sendViaWindowsShell(const QString &filePath, const QString & /*deviceAddress*/)
{
    logger->info("FileSender", "Попытка через Windows Shell...");
    
    // Этот метод пытается открыть контекстное меню "Отправить на Bluetooth"
    logger->warning("FileSender", "Shell API метод требует UI взаимодействия");
    logger->info("FileSender", "Пользователь должен выбрать устройство вручную");
    
    // Можно попробовать открыть проводник с файлом
    QString nativePath = QDir::toNativeSeparators(filePath);
    QString explorerCmd = QString("explorer.exe /select,\"%1\"").arg(nativePath);
    
    logger->logApiCall("ShellExecute", explorerCmd);
    
    // В Qt 5.5.1 используем статический метод
    bool started = QProcess::startDetached("explorer.exe", QStringList() << "/select," + nativePath);
    
    if (started) {
        logger->logApiResult("ShellExecute", "SUCCESS - Explorer открыт", true);
        logger->info("FileSender", "");
        logger->success("FileSender", "✓ Проводник открыт с выбранным файлом");
        logger->info("FileSender", "Далее:");
        logger->info("FileSender", "1. Правый клик на файл");
        logger->info("FileSender", "2. Отправить → Bluetooth");
        logger->info("FileSender", "3. Выберите устройство");
        logger->info("FileSender", "");
        return true;
    } else {
        logger->logApiResult("ShellExecute", "FAILED", false);
        return false;
    }
}

bool BluetoothFileSender::sendFileDirectly(BluetoothConnection *connection, const QString &filePath)
{
    logger->info("FileSender", "═══════════════════════════════════════");
    logger->info("FileSender", "ПРЯМАЯ ОТПРАВКА ЧЕРЕЗ RFCOMM СОКЕТ");
    logger->info("FileSender", "═══════════════════════════════════════");
    logger->info("FileSender", QString("Файл: %1").arg(filePath));
    logger->info("FileSender", "");
    
    // Открываем файл
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        logger->error("FileSender", QString("Не удалось открыть файл: %1").arg(file.errorString()));
        return false;
    }
    
    qint64 fileSize = file.size();
    QFileInfo fileInfo(filePath);
    
    logger->info("FileSender", QString("Размер файла: %1 байт (%2 MB)")
        .arg(fileSize)
        .arg(fileSize / 1024.0 / 1024.0, 0, 'f', 2));
    logger->info("FileSender", "");
    
    // Шаг 1: Отправляем заголовок (имя файла + размер)
    logger->info("FileSender", "ШАГ 1: Отправка заголовка");
    
    QByteArray header;
    QDataStream headerStream(&header, QIODevice::WriteOnly);
    headerStream.setVersion(QDataStream::Qt_5_5);
    
    headerStream << fileInfo.fileName();  // Имя файла
    headerStream << fileSize;              // Размер
    
    logger->debug("FileSender", QString("Заголовок: Имя='%1', Размер=%2").arg(fileInfo.fileName()).arg(fileSize));
    logger->debug("FileSender", QString("Размер заголовка: %1 байт").arg(header.size()));
    
    // Отправляем длину заголовка
    quint32 headerSize = header.size();
    QByteArray headerSizeData;
    QDataStream(&headerSizeData, QIODevice::WriteOnly) << headerSize;
    
    qint64 sent = connection->sendData(headerSizeData);
    if (sent != headerSizeData.size()) {
        logger->error("FileSender", "Ошибка отправки размера заголовка");
        file.close();
        return false;
    }
    
    // Отправляем сам заголовок
    sent = connection->sendData(header);
    if (sent != header.size()) {
        logger->error("FileSender", "Ошибка отправки заголовка");
        file.close();
        return false;
    }
    
    logger->success("FileSender", "✓ Заголовок отправлен");
    logger->info("FileSender", "");
    
    // Шаг 2: Отправляем файл по частям
    logger->info("FileSender", "ШАГ 2: Отправка данных файла");
    
    const qint64 chunkSize = 4096;  // 4 KB chunks
    qint64 totalSent = 0;
    int chunkNumber = 0;
    
    while (!file.atEnd()) {
        QByteArray chunk = file.read(chunkSize);
        qint64 chunkOffset = 0;
        
        // Отправляем chunk, обрабатывая частичную отправку
        while (chunkOffset < chunk.size()) {
            QByteArray remaining = chunk.mid(chunkOffset);
            sent = connection->sendData(remaining);
            
            if (sent == -1) {
                // Реальная ошибка отправки
                logger->error("FileSender", QString("Фатальная ошибка отправки блока %1").arg(chunkNumber));
                file.close();
                return false;
            }
            
            if (sent == 0) {
                // WSAEWOULDBLOCK - буфер полон, нужно подождать
                logger->debug("FileSender", QString("Буфер полон, ожидание... (блок %1)").arg(chunkNumber));
                QThread::msleep(10);  // Ждем 10ms
                continue;  // Повторяем попытку
            }
            
            // Успешно отправлена часть или весь chunk
            chunkOffset += sent;
            totalSent += sent;
        }
        
        chunkNumber++;
        
        // Логируем прогресс каждые 10 блоков
        if (chunkNumber % 10 == 0) {
            int progress = (totalSent * 100) / fileSize;
            logger->debug("FileSender", QString("Отправлено: %1/%2 байт (%3%)").arg(totalSent).arg(fileSize).arg(progress));
            emit transferProgress(totalSent, fileSize);
        }
    }
    
    file.close();
    
    logger->success("FileSender", "");
    logger->success("FileSender", "✓✓✓ ФАЙЛ ОТПРАВЛЕН ПОЛНОСТЬЮ! ✓✓✓");
    logger->success("FileSender", QString("Отправлено блоков: %1").arg(chunkNumber));
    logger->success("FileSender", QString("Всего байт: %1").arg(totalSent));
    logger->info("FileSender", "");
    
    return true;
}

QString BluetoothFileSender::formatMacAddress(const QString &address)
{
    // Форматируем MAC адрес
    QString formatted = address;
    formatted.remove(":");
    formatted.remove("-");
    return formatted.toUpper();
}

bool BluetoothFileSender::isObexSupported()
{
    // Проверяем наличие fsquirt.exe
    logger->info("FileSender", "Проверка поддержки OBEX...");
    logger->info("FileSender", "Поиск fsquirt.exe в системе...");
    
    QProcess process;
    process.start("where", QStringList() << "fsquirt.exe");
    process.waitForFinished(1000);
    
    bool found = (process.exitCode() == 0);
    
    if (found) {
        QString path = process.readAllStandardOutput().trimmed();
        logger->success("FileSender", QString("✓ fsquirt.exe найден: %1").arg(path));
        logger->info("FileSender", "OBEX File Transfer поддерживается");
        return true;
    } else {
        logger->error("FileSender", "✗ fsquirt.exe не найден");
        logger->warning("FileSender", "OBEX File Transfer может не работать");
        return false;
    }
}

