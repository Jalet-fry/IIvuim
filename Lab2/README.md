# Лабораторная работа №2: Шина PCI

## Задание лабораторной работы

Вывести список всех устройств, подключенных к шине PCI, с их характеристиками (DeviceID и VendorID, состоящие из 4-х символов) в виде таблицы.

**Требования:**
- Подключение к шине производить с помощью готовых библиотек **нельзя**
- Подключение к шине реализовать с применением портов ввода-вывода

---

## Ответы на теоретические вопросы

### 1. Определение шины PCI

**PCI (Peripheral Component Interconnect)** — это локальная шина ввода-вывода, предназначенная для подключения периферийных устройств к материнской плате компьютера. Разработанная компанией Intel в 1992 году, она стала стандартом для персональных компьютеров на долгие годы, заменив более медленные шины ISA и VLB. PCI обеспечивает высокоскоростной канал обмена данными между процессором, памятью и периферийными устройствами, такими как видеокарты, сетевые адаптеры и звуковые карты.

---

### 2. Архитектура ЭВМ

**Архитектура ЭВМ** — это концептуальная структура компьютерной системы, определяющая её организацию, принципы работы и взаимодействие компонентов. 

Классическая архитектура фон Неймана включает в себя:

1. **Центральный процессор (CPU)** — выполняет арифметические и логические операции
2. **Оперативная память (RAM)** — хранит данные и инструкции для процессора
3. **Устройства ввода-вывода (I/O)** — обеспечивают взаимодействие с пользователем и другими системами (клавиатура, монитор, диски)
4. **Системная шина** — обеспечивает связь между всеми перечисленными компонентами

Шины, такие как PCI, являются ключевым элементом подсистемы ввода-вывода, соединяя высокопроизводительные устройства с остальной системой.

---

### 3. Классификация периферийных устройств

Периферийные устройства классифицируются по нескольким критериям:

#### По направлению передачи данных:
- **Устройства ввода:** клавиатура, мышь, сканер
- **Устройства вывода:** монитор, принтер, аудиосистема
- **Устройства ввода-вывода:** жесткий диск (SSD/HDD), сетевая карта, USB-флеш-накопитель

#### По способу подключения:
- **Внутренние:** подключаются к слотам на материнской плате (видеокарты, звуковые карты через PCI/PCIe)
- **Внешние:** подключаются к портам на задней панели (USB, HDMI)

#### По назначению:
- **Устройства хранения информации:** HDD, SSD, оптические диски
- **Устройства человеко-машинного интерфейса:** клавиатура, монитор
- **Коммуникационные устройства:** модем, сетевой адаптер

---

### 4. Критерии классификации периферийных устройств

Основные критерии классификации периферийных устройств:

1. **Направление передачи данных** — определяет, является ли устройство источником данных (ввод), получателем данных (вывод) или выполняет обе функции (ввод-вывод)

2. **Способ подключения** — внутреннее размещение (на материнской плате) или внешнее подключение (через порты)

3. **Функциональное назначение** — определяет роль устройства в системе (хранение, отображение, ввод, коммуникация и т.д.)

4. **Тип интерфейса подключения** — PCI, PCIe, USB, SATA, и другие стандарты

5. **Скорость передачи данных** — низкоскоростные, среднескоростные, высокоскоростные устройства

---

### 5. Определение понятия "Шина"

**Шина** (в контексте вычислительной техники) — это подсистема, которая передает данные между компонентами внутри компьютера или между компьютерами. В отличие от двухточечного соединения, шина логически соединяет несколько периферийных устройств по одному и тому же набору проводов. Каждая шина определяет свой набор разъемов для физического подключения плат, карт или кабелей.

---

### 6. Параметры, характеризующие шину

Основные параметры шины:

1. **Разрядность** — количество бит данных, которые шина может передать одновременно за один такт (например, 32-битная или 64-битная)

2. **Тактовая частота** — скорость, с которой данные передаются по шине, измеряется в мегагерцах (МГц)

3. **Пропускная способность** — максимальная скорость передачи данных, обычно измеряется в мегабайтах в секунду (МБ/с) или гигабайтах в секунду (ГБ/с). Рассчитывается как: **(Частота × Разрядность) / 8**

4. **Топология** — способ физического и логического соединения устройств (например, общая шина, звезда)

---

### 7. Иерархия шин

Современные компьютерные системы используют иерархическую структуру шин для оптимизации производительности:

1. **Процессорная шина (FSB)** — самая быстрая шина, соединяет процессор с северным мостом (или напрямую с контроллером памяти)

2. **Шина памяти** — соединяет контроллер памяти с модулями ОЗУ

3. **Шины ввода-вывода (мосты)** — более медленные шины, такие как PCI и PCIe, подключаются к системной шине через специальные контроллеры-мосты (северный и южный)

Эта иерархия позволяет медленным периферийным устройствам не замедлять работу высокоскоростных компонентов.

---

### 8. Топология интерфейсов

Основные типы топологий:

1. **Общая шина (Bus)** — все устройства подключаются к одному общему каналу. Данные, отправленные одним устройством, доступны всем остальным
   - *Пример:* классическая PCI

2. **Звезда (Star)** — все устройства подключаются к центральному концентратору (хабу или коммутатору)
   - *Примеры:* USB, PCIe

3. **Кольцо (Ring)** — устройства соединены в замкнутую цепь

---

### 9. Архитектура и топология шины PCI

**PCI использует топологию общей шины.** Все устройства, подключенные к слотам PCI, разделяют один и тот же канал передачи данных.

Для управления доступом к шине используется **механизм арбитража**. Специальный контроллер (арбитр) решает, какому устройству в данный момент времени предоставить право на использование шины. Это предотвращает конфликты и обеспечивает упорядоченный обмен данными.

---

### 10. Характеристики шины PCI

Технические характеристики:

- **Разрядность:** 32 или 64 бита
- **Тактовая частота:** 33 МГц или 66 МГц
- **Пропускная способность:**
  - 32-bit, 33 MHz: **133 МБ/с**
  - 64-bit, 66 MHz: **528 МБ/с**
- **Plug and Play:** автоматическое распознавание и конфигурирование устройств операционной системой
- **Конфигурационное пространство:** каждое PCI-устройство имеет специальную область памяти (256 байт), где хранятся его идентификаторы (Vendor ID, Device ID) и настройки

---

### 11. Доступ к шине PCI и фазы транзакции

#### Доступ к конфигурационному пространству PCI

Реализуется через порты ввода-вывода. Стандартный механизм использует два 32-битных порта:

- **0xCF8 (ADDRESS_PORT)** — порт адреса. Сюда записывается адрес, состоящий из номера шины, устройства, функции и смещения регистра, который нужно прочитать
- **0xCFC (DATA_PORT)** — порт данных. После записи адреса в 0xCF8, чтение или запись данных производится через этот порт

#### Фазы транзакции на шине PCI

1. **Фаза арбитража** — устройство запрашивает доступ к шине
2. **Фаза адреса** — устройство-инициатор (Master) выставляет на шину адрес и тип транзакции (чтение/запись)
3. **Фаза данных** — происходит передача одного или нескольких слов данных

---

### 12. Режимы кэширования шины PCI

Шина PCI не имеет собственных сложных механизмов кэширования, как у процессора. Однако контроллеры шины (мосты) могут реализовывать **буферизацию транзакций записи** (write posting/write buffering).

Это позволяет процессору быстро завершить операцию записи на шину, не дожидаясь, пока медленное PCI-устройство её обработает. Данные временно сохраняются в буфере моста и передаются устройству позже.

---

### 13. Шина PCI-X

**PCI-X (Peripheral Component Interconnect eXtended)** — это расширение стандарта PCI, разработанное для повышения пропускной способности.

**Характеристики:**
- **Обратная совместимость:** устройства PCI-X могут работать в слотах PCI, и наоборот (с потерей производительности)
- **Повышенная частота:** работает на частотах 66, 100 и 133 МГц (в версии 2.0 до 533 МГц)
- **Пропускная способность:** до 1064 МБ/с (64-bit, 133 MHz)
- **Применение:** в основном в серверах и рабочих станциях для высокоскоростных сетевых карт и RAID-контроллеров

---

### 14. Шина PCI-Express

**PCI-Express (PCIe)** — это радикальное изменение архитектуры по сравнению с PCI/PCI-X.

**Ключевые особенности:**

1. **Топология "точка-точка"** — вместо общей шины PCIe использует последовательные соединения типа "звезда". Каждое устройство подключается к коммутатору (Switch) напрямую выделенным каналом. Устройства не делят пропускную способность

2. **Линии (Lanes)** — канал состоит из одной или нескольких "линий". Слот может быть x1, x4, x8, x16, что определяет количество линий и, соответственно, пропускную способность

3. **Последовательная передача** — в отличие от параллельной шины PCI, данные передаются последовательно, что позволяет достигать более высоких частот и упрощает разводку платы

4. **Высокая пропускная способность:**
   - Одна линия PCIe 3.0: ~1 ГБ/с
   - Слот x16: ~16 ГБ/с
   - PCIe 4.0 и 5.0 удваивают эти значения

**На сегодняшний день PCIe является стандартом де-факто для подключения всех высокопроизводительных внутренних устройств.**

---

---

## Реализация задания

### Общая архитектура проекта

Проект разделен на два основных компонента:

1. **PciScannerGiveIO** (`pciscanner_giveio.h/cpp`) — отвечает за низкоуровневую работу с PCI:
   - Инициализация драйвера GiveIO
   - Чтение/запись в порты ввода-вывода
   - Сканирование PCI шины
   - Декодирование идентификаторов устройств

2. **PCIWidget_GiveIO** (`pciwidget_giveio.h/cpp`) — отвечает за пользовательский интерфейс:
   - Отображение таблицы устройств
   - Логирование операций
   - Сохранение отчетов

---

## Детальное описание реализации

### 1. Работа с драйвером GiveIO (Windows XP)

На Windows XP прямой доступ к портам ввода-вывода из пользовательского приложения запрещён. Для обхода этого ограничения используется драйвер **GiveIO**.

#### Инициализация драйвера

```cpp
bool PciScannerGiveIO::giveioInitialize()
{
    giveioHandle = CreateFileA("\\\\.\\giveio",
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    
    if (giveioHandle == INVALID_HANDLE_VALUE) {
        return false;  // Драйвер не установлен
    }
    
    giveioInitialized = true;
    return true;
}
```

**Как это работает:**
- Драйвер GiveIO открывает доступ к портам ввода-вывода для вызывающего процесса
- После успешного открытия устройства `\\\\.\\giveio` можно использовать инструкции `IN` и `OUT`
- Доступ работает только с правами администратора

---

### 2. Низкоуровневая работа с портами (Inline Assembly)

Для работы с портами используется встроенный ассемблер (inline assembly):

#### Запись в порт (OUT)

```cpp
void PciScannerGiveIO::giveioOutPortDword(WORD port, DWORD value)
{
    #if defined(_MSC_VER)
        __asm {
            mov dx, port      // Адрес порта в DX
            mov eax, value    // Значение в EAX
            out dx, eax       // Записываем 32-битное значение
        }
    #else
        asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
    #endif
}
```

#### Чтение из порта (IN)

```cpp
DWORD PciScannerGiveIO::giveioInPortDword(WORD port)
{
    #if defined(_MSC_VER)
        DWORD result;
        __asm {
            mov dx, port      // Адрес порта в DX
            in eax, dx        // Читаем 32-битное значение в EAX
            mov result, eax   // Сохраняем результат
        }
        return result;
    #else
        DWORD result;
        asm volatile ("inl %1, %0" : "=a"(result) : "Nd"(port));
        return result;
    #endif
}
```

**Пояснение инструкций:**
- `mov dx, port` — помещает номер порта в регистр DX
- `out dx, eax` — выводит 32-битное значение из EAX в порт DX
- `in eax, dx` — читает 32-битное значение из порта DX в регистр EAX

---

### 3. Доступ к конфигурационному пространству PCI

PCI использует два порта для доступа к конфигурационному пространству:

- **0xCF8 (CONFIG_ADDRESS)** — порт адреса
- **0xCFC (CONFIG_DATA)** — порт данных

#### Формат адреса CONFIG_ADDRESS

```
Биты 31-24: [Enable bit: 1] [Reserved: 7 bits]
Биты 23-16: Bus Number (0-255)
Биты 15-11: Device Number (0-31)
Биты 10-8:  Function Number (0-7)
Биты 7-2:   Register Offset (выровнен на 4 байта)
Биты 1-0:   Always 0
```

#### Реализация чтения регистра

```cpp
DWORD PciScannerGiveIO::readPCIConfigDword(quint8 bus, quint8 device, 
                                           quint8 function, quint8 offset)
{
    // Формируем адрес
    DWORD address = 0x80000000 |          // Enable bit
                    (bus << 16) |          // Bus
                    (device << 11) |       // Device
                    (function << 8) |      // Function
                    (offset & 0xFC);       // Register offset (DWORD aligned)
    
    // Записываем адрес в порт 0xCF8
    writePortDword(0x0CF8, address);
    
    // Читаем данные из порта 0xCFC
    return readPortDword(0x0CFC);
}
```

**Пример:**
Чтение Vendor ID и Device ID устройства на Bus=0, Device=0, Function=0:

```cpp
DWORD address = 0x80000000;  // Enable bit установлен, все остальные = 0
writePortDword(0x0CF8, address);
DWORD data = readPortDword(0x0CFC);

quint16 vendorID = data & 0xFFFF;           // Младшие 16 бит
quint16 deviceID = (data >> 16) & 0xFFFF;   // Старшие 16 бит
```

---

### 4. Алгоритм сканирования PCI шины

Алгоритм последовательно проверяет все возможные комбинации Bus/Device/Function:

```cpp
bool PciScannerGiveIO::scanInternal()
{
    m_devices.clear();
    
    // Перебираем все 256 шин
    for (DWORD bus = 0; bus < 256; bus++) {
        // На каждой шине до 32 устройств
        for (DWORD device = 0; device < 32; device++) {
            // У каждого устройства до 8 функций
            for (DWORD function = 0; function < 8; function++) {
                
                // Формируем адрес для чтения Vendor ID / Device ID
                DWORD address = 0x80000000 | (bus << 16) | 
                               (device << 11) | (function << 8);
                
                writePortDword(0x0CF8, address);
                DWORD vendorDevice = readPortDword(0x0CFC);
                
                DWORD vendorID = vendorDevice & 0xFFFF;
                DWORD deviceID = (vendorDevice >> 16) & 0xFFFF;
                
                // Проверяем валидность
                if (vendorID != 0xFFFF && vendorID != 0x0000) {
                    // УСТРОЙСТВО НАЙДЕНО!
                    PCI_Device_GiveIO dev;
                    dev.vendorID = vendorID;
                    dev.deviceID = deviceID;
                    dev.bus = bus;
                    dev.device = device;
                    dev.function = function;
                    
                    // Читаем дополнительные регистры
                    readDeviceDetails(dev);
                    
                    m_devices.append(dev);
                }
            }
        }
    }
    
    return (m_devices.size() > 0);
}
```

**Почему проверяем 0xFFFF и 0x0000?**
- `0xFFFF` — возвращается, когда устройство отсутствует (шина не отвечает)
- `0x0000` — невалидный Vendor ID (нет такого производителя)

---

### 5. Чтение конфигурационного пространства

#### Структура конфигурационного заголовка PCI

```
Offset  Size  Description
------  ----  -----------
0x00    16    Vendor ID          ─┐
0x02    16    Device ID          ─┤ Идентификация
                                  ─┘
0x04    16    Command Register   ─┐
0x06    16    Status Register    ─┤ Управление
                                  ─┘
0x08    8     Revision ID        ─┐
0x09    8     Prog IF            │
0x0A    8     SubClass           ├─ Классификация
0x0B    8     Class Code         ─┘

0x0C    8     Cache Line Size    ─┐
0x0D    8     Latency Timer      │
0x0E    8     Header Type        ├─ Заголовок
0x0F    8     BIST               ─┘

0x10-0x27     Base Address Registers (BARs)

0x2C    16    Subsystem Vendor ID ─┐ Только для
0x2E    16    Subsystem ID        ─┘ Header Type 0
```

#### Чтение классификации устройства

```cpp
// Читаем регистр 0x08
DWORD classReg = readPCIConfigDword(bus, device, function, 0x08);

dev.revisionID = classReg & 0xFF;           // Биты 7-0
dev.progIF = (classReg >> 8) & 0xFF;        // Биты 15-8
dev.subClass = (classReg >> 16) & 0xFF;     // Биты 23-16
dev.classCode = (classReg >> 24) & 0xFF;    // Биты 31-24
```

#### Чтение типа заголовка

```cpp
// Читаем регистр 0x0C
DWORD headerReg = readPCIConfigDword(bus, device, function, 0x0C);

dev.headerType = (headerReg >> 16) & 0xFF;  // Байт offset 0x0E
```

**Типы заголовков:**
- `0x00` — Стандартное PCI устройство
- `0x01` — PCI-to-PCI мост
- `0x02` — CardBus мост
- `0x80` (бит 7) — Multifunction device

#### Чтение Subsystem ID (только для Header Type 0)

```cpp
if ((dev.headerType & 0x7F) == 0x00) {
    // Читаем регистр 0x2C
    DWORD subsysReg = readPCIConfigDword(bus, device, function, 0x2C);
    
    dev.subsysVendorID = subsysReg & 0xFFFF;
    dev.subsysID = (subsysReg >> 16) & 0xFFFF;
}
```

---

### 6. База данных PCI ID

Для декодирования Vendor ID и Device ID используется встроенная база данных PCI:

```cpp
// Структуры базы данных
struct PCI_VENTABLE {
    unsigned short VenId;
    const char *VenShort;    // Короткое имя
    const char *VenFull;     // Полное имя
};

struct PCI_DEVTABLE {
    unsigned short VenId;
    unsigned short DevId;
    const char *Chip;        // Название чипа
    const char *ChipDesc;    // Описание
};
```

#### Поиск производителя

```cpp
QString PCIDatabase::findVendorName(unsigned short vendorId)
{
    size_t tableSize = getPciVenTableSize();
    
    for (size_t i = 0; i < tableSize; i++) {
        if (PciVenTable[i].VenId == vendorId) {
            QString shortName = QString::fromLocal8Bit(PciVenTable[i].VenShort);
            
            if (!shortName.isEmpty()) {
                return shortName;
            }
        }
    }
    
    // Не найдено в базе
    return QString("Vendor 0x%1").arg(vendorId, 4, 16, QChar('0')).toUpper();
}
```

#### Примеры записей

```cpp
{0x8086, "Intel", "Intel Corporation"},
{0x1022, "AMD", "Advanced Micro Devices"},
{0x10DE, "NVIDIA", "NVIDIA Corporation"},
{0x80EE, "InnoTek", "InnoTek Systemberatung GmbH (VirtualBox)"}
```

---

### 7. Декодирование Class Code и SubClass

#### Class Code (основной класс устройства)

```cpp
QString PciScannerGiveIO::getClassString(quint8 classCode) const
{
    switch (classCode) {
        case 0x00: return "Pre-2.0 PCI Specification Device";
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Device";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge Device";
        case 0x07: return "Simple Communications Controller";
        case 0x08: return "Base Systems Peripheral";
        case 0x09: return "Input Device";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        // ...
        default: return QString("Class 0x%1").arg(classCode, 2, 16, QChar('0'));
    }
}
```

#### SubClass (подкласс)

Подкласс детализирует тип устройства. Например, для Class 0x01 (Mass Storage):

```cpp
case 0x01: // Mass Storage
    switch (subClass) {
        case 0x00: return "SCSI";
        case 0x01: return "IDE";
        case 0x02: return "Floppy";
        case 0x04: return "RAID";
        case 0x05: return "ATA Controller";
        case 0x06: return "Serial ATA Controller";
        case 0x08: return "Non-Volatile Memory (NVMe)";
        // ...
    }
```

#### Programming Interface (Prog IF)

Prog IF уточняет интерфейс устройства. Например, для USB контроллеров (Class 0x0C, SubClass 0x03):

```cpp
if (classCode == 0x0C && subClass == 0x03) {
    switch (progIF) {
        case 0x00: return "USB 1.1 UHCI";
        case 0x10: return "USB 1.1 OHCI";
        case 0x20: return "USB 2.0 EHCI";
        case 0x30: return "USB 3.0 XHCI";
        // ...
    }
}
```

---

### 8. Проверка прав администратора (Windows XP)

```cpp
bool PciScannerGiveIO::isRunningAsAdmin() const
{
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    
    // Открываем токен процесса
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return false;
    }
    
    // Создаем SID для группы администраторов
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup = NULL;
    
    if (!AllocateAndInitializeSid(&NtAuthority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &AdministratorsGroup)) {
        CloseHandle(hToken);
        return false;
    }
    
    // Проверяем членство в группе
    CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin);
    
    // Освобождаем ресурсы
    FreeSid(AdministratorsGroup);
    CloseHandle(hToken);
    
    return (isAdmin == TRUE);
}
```

---

### 9. Пользовательский интерфейс

#### Структура UI

- **Кнопка "Сканировать PCI устройства"** — запускает сканирование
- **Таблица устройств** — отображает найденные устройства с колонками:
  - Bus, Device, Function
  - VendorID, DeviceID
  - Vendor Name, Device Name
  - Class Code, SubClass, Prog IF, Header Type
- **Лог операций** — показывает детальный лог всех операций
- **Прогресс-бар** — отображает прогресс сканирования
- **Анимация Jake** — показывается во время сканирования

#### Сохранение отчета

```cpp
void PCIWidget_GiveIO::onSaveToFileClicked()
{
    // Генерируем имя файла с датой
    QString defaultFileName = QString("PCI_Scan_Report_%1.txt")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Save PCI Scan Report", defaultFileName, "Text Files (*.txt)");
    
    // Записываем отчет
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        
        // Заголовок
        out << "PCI DEVICES SCAN REPORT (GiveIO)\n";
        out << "Scan Date: " << QDateTime::currentDateTime().toString() << "\n";
        
        // Список устройств
        foreach (const PCI_Device_GiveIO &dev, pciDevices) {
            out << QString("Bus %1, Device %2, Function %3\n")
                   .arg(dev.bus).arg(dev.device).arg(dev.function);
            out << QString("  VendorID: 0x%1 - %2\n")
                   .arg(dev.vendorID, 4, 16, QChar('0')).arg(dev.vendorName);
            // ...
        }
        
        file.close();
    }
}
```

---

## Инструкция по установке GiveIO на Windows XP

### Шаг 1: Копирование драйвера

```cmd
copy giveio.sys C:\Windows\System32\drivers\
```

### Шаг 2: Установка службы

```cmd
sc create giveio binPath= "C:\Windows\System32\drivers\giveio.sys" type= kernel
```

### Шаг 3: Запуск службы

```cmd
sc start giveio
```

### Шаг 4: Проверка статуса

```cmd
sc query giveio
```

Должно показать: **STATE: RUNNING**

---

## Примеры результатов

### Типичные устройства на реальном железе

```
Bus 0, Device 0, Function 0
  VendorID: 0x8086 (Intel)
  DeviceID: 0x2E30
  Class: 0x06 - Bridge Device
  SubClass: 0x00 - Host/PCI
  
Bus 0, Device 2, Function 0
  VendorID: 0x8086 (Intel)
  DeviceID: 0x2E32
  Class: 0x03 - Display Controller
  SubClass: 0x00 - VGA Compatible
  
Bus 0, Device 31, Function 2
  VendorID: 0x8086 (Intel)
  DeviceID: 0x2825
  Class: 0x01 - Mass Storage Controller
  SubClass: 0x06 - Serial ATA Controller
  Prog IF: 0x01 - AHCI 1.0
```

### На виртуальной машине VirtualBox

```
Bus 0, Device 0, Function 0
  VendorID: 0x8086 (Intel)
  DeviceID: 0x1237
  Device Name: Intel 440FX PCIset
  Class: 0x06 - Bridge Device
  
Bus 0, Device 2, Function 0
  VendorID: 0x80EE (InnoTek - VirtualBox)
  DeviceID: 0xBEEF
  Device Name: VirtualBox Graphics Adapter
  Class: 0x03 - Display Controller
```

---

## Возможные проблемы и решения

### ❌ Ошибка: "GiveIO open error"

**Причина:** Драйвер не установлен или не запущен

**Решение:**
```cmd
sc query giveio
sc start giveio
```

### ❌ Ошибка: "Administrator rights required"

**Причина:** Программа запущена без прав администратора

**Решение:** 
- Закройте программу
- ПКМ на файл → "Запуск от имени администратора"

### ❌ Устройства не найдены (0 devices)

**Возможные причины:**
1. Драйвер не работает: `sc query giveio` → должен быть `RUNNING`
2. Недостаточно прав: запустите от администратора
3. Порты недоступны на данной системе

### ❌ Программа зависает при сканировании

**Причина:** Сканируется 256 × 32 × 8 = 65,536 адресов — это может занять 1-2 минуты на медленных системах

**Решение:** Дождитесь завершения (анимация Jake показывает, что процесс идет)

---

## Заключение

Данная реализация:
- ✅ **Полностью соответствует спецификации PCI 2.x/3.0**
- ✅ **Не использует готовые библиотеки** — весь доступ через порты I/O
- ✅ **Работает на Windows XP** с драйвером GiveIO
- ✅ **Читает все необходимые характеристики** (Vendor ID, Device ID, Class, SubClass и др.)
- ✅ **Выводит результаты в виде таблицы** с возможностью сохранения в файл
- ✅ **Использует встроенный ассемблер** для прямого доступа к портам
- ✅ **Включает базу данных PCI ID** для декодирования производителей и устройств

**Важно:** Программа требует прав администратора и установленного драйвера GiveIO для работы на Windows XP.

