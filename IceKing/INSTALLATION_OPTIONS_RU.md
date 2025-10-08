# Варианты установки и проверки компилятора для IceKing

## 🔍 Как узнать какой компилятор используется в Qt?

### Способ 1: Запустить скрипт проверки (самый простой)

```cmd
cd C:\VM-shared\Common\IIvuim\IceKing
check_qt_compiler.bat
```

Этот скрипт автоматически найдет все установленные Qt и компиляторы.

### Способ 2: Посмотреть на путь установки Qt

Откройте командную строку:
```cmd
where qmake
```

Смотрите на путь:

| Путь | Компилятор |
|------|------------|
| `C:\Qt\...\**msvc2010**\bin\qmake.exe` | ✅ MSVC 2010 |
| `C:\Qt\...\**msvc2013**\bin\qmake.exe` | ✅ MSVC 2013 |
| `C:\Qt\...\**msvc2015**\bin\qmake.exe` | ✅ MSVC 2015 |
| `C:\Qt\...\**mingw**\bin\qmake.exe` | ❌ MinGW (не подходит!) |

### Способ 3: Проверить через qmake

```cmd
# Если qmake в PATH:
qmake -query

# Или указать полный путь:
C:\Qt\Qt5.5.1\5.5\msvc2010\bin\qmake.exe -query
```

Ищите строку `QMAKE_XSPEC`:
- `win32-msvc2010` → MSVC 2010 ✅
- `win32-msvc2013` → MSVC 2013 ✅
- `win32-g++` → MinGW ❌

### Способ 4: Посмотреть в Qt Creator

1. Откройте Qt Creator
2. Tools → Options → Kits
3. Посмотрите на список Kit'ов
4. Найдите строку "Compiler:"
   - `Microsoft Visual C++ Compiler ...` → MSVC ✅
   - `MinGW ...` → MinGW ❌

### Способ 5: Проверить Makefile проекта

Если проект уже собирался, откройте `IceKing/Makefile`:

```cmd
findstr "qmake.exe" IceKing\Makefile
```

Результат покажет путь к qmake, по которому можно определить компилятор.

## 📥 Можно ли установить только компилятор без полного Qt?

### ❌ Короткий ответ: НЕТ

Qt - это не просто библиотеки, это целый фреймворк:
- Библиотеки (Qt5Core.dll, Qt5Gui.dll, и т.д.)
- Инструменты сборки (qmake, moc, uic, rcc)
- Плагины (platforms, imageformats, и т.д.)
- Заголовочные файлы

Все компоненты должны быть скомпилированы одним и тем же компилятором!

**Нельзя просто взять компилятор MSVC и использовать с Qt для MinGW.**

## 🎯 Варианты решения для вашей ситуации

### Вариант 1: Использовать существующую установку Qt

**Если у вас уже установлен Qt 5.5.1 или 5.6.x:**

1. Запустите `check_qt_compiler.bat` чтобы узнать какой компилятор
2. Если это MinGW → нужно установить Qt для MSVC
3. Если это MSVC → просто установите соответствующий Visual Studio

**Пример:**
- Есть Qt 5.5.1 MinGW → не подходит, нужен Qt для MSVC
- Есть Qt 5.5.1 MSVC 2010 → отлично! Установите только VS 2010

### Вариант 2: Установить только недостающий компонент

#### Ситуация A: Есть Qt для MSVC, нет Visual Studio

**Решение:** Установите только Visual Studio 2010

1. **Visual Studio 2010 Express** (бесплатно):
   - Размер: ~700 MB
   - Скачать: https://visualstudio.microsoft.com/vs/older-downloads/
   - Или: https://my.visualstudio.com/Downloads (требуется бесплатная регистрация)

2. **Только Windows SDK 7.1** (если нужен только компилятор):
   - Размер: ~500 MB
   - Включает компилятор MSVC 2010
   - Скачать: https://www.microsoft.com/en-us/download/details.aspx?id=8279

3. **Только Build Tools** (минимальная установка):
   - К сожалению, для VS 2010 отдельных Build Tools нет
   - Это появилось только с VS 2015+

#### Ситуация B: Есть Visual Studio, нет Qt для MSVC

**Решение:** Установите Qt для соответствующего MSVC

**Важно:** Версия Qt должна поддерживать вашу версию Visual Studio!

| Visual Studio | Qt 5.5.1 | Qt 5.6.3 |
|---------------|----------|----------|
| VS 2008 (VC9) | ❌ Нет | ❌ Нет |
| VS 2010 (VC10) | ✅ Да | ✅ Да |
| VS 2013 (VC12) | ✅ Да | ✅ Да |
| VS 2015 (VC14) | ❌ Нет | ✅ Да |

**Где скачать Qt 5.5.1 для MSVC 2010:**
- https://download.qt.io/archive/qt/5.5/5.5.1/
- Файл: `qt-opensource-windows-x86-msvc2010-5.5.1.exe` (~800 MB)

### Вариант 3: Установить все с нуля (если ничего нет)

#### Для Windows XP (в VirtualBox):

1. **Visual Studio 2010 Express** (~700 MB)
2. **Qt 5.5.1 MSVC 2010** (~800 MB)
3. Итого: ~1.5 GB

#### Для современной Windows (host):

Тоже самое, но можно использовать VS 2013 или 2015 с Qt 5.6.3

## 💡 Минимальная установка для компиляции

Если нужен **только компилятор** (без IDE):

### Windows SDK 7.1 (для MSVC 2010)

Это минимальный пакет с компилятором без Visual Studio IDE:

1. Скачать: https://www.microsoft.com/en-us/download/details.aspx?id=8279
2. Установить с опциями:
   - ✅ Visual C++ Compilers
   - ✅ Windows Headers and Libraries
   - ❌ Остальное можно отключить

3. После установки компилятор будет доступен через:
   - Пуск → Microsoft Windows SDK v7.1 → Windows SDK Command Prompt

**Размер:** ~500 MB вместо ~700 MB для полного VS 2010

## 🔧 Проверка после установки

### 1. Проверка Visual Studio:

Откройте **Visual Studio Command Prompt**:
```cmd
cl
```

Должно вывести:
```
Microsoft (R) C/C++ Optimizing Compiler Version 16.00.xxxxx for 80x86
```

Версия 16.x = Visual Studio 2010 ✅

### 2. Проверка Qt:

```cmd
C:\Qt\Qt5.5.1\5.5\msvc2010\bin\qmake.exe -v
```

Должно вывести:
```
QMake version 3.0
Using Qt version 5.5.1 in C:/Qt/Qt5.5.1/5.5/msvc2010/lib
```

### 3. Проверка совместимости:

Запустите скрипт проверки:
```cmd
cd C:\VM-shared\Common\IIvuim\IceKing
check_qt_compiler.bat
```

Должно показать:
```
[+] Visual Studio 2010 compiler
[+] Qt for MSVC 2010
```

## 📦 Установка на Windows XP

### Что можно установить на XP:

| Программа | XP Support | Примечание |
|-----------|------------|------------|
| Visual Studio 2010 | ✅ Да | Последняя версия VS с полной поддержкой XP |
| Visual Studio 2008 | ✅ Да | Старее, но работает |
| Visual Studio 2012+ | ❌ Нет | Требует Vista+ |
| Qt 5.5.1 | ✅ Да | Работает на XP |
| Qt 5.6.3 | ✅ Да | Последняя LTS с XP |
| Qt 5.7+ | ❌ Нет | Требует Vista+ |

### Порядок установки на XP:

**В VirtualBox с Windows XP:**

1. **Установите Visual Studio 2010 Express**
   - Скачайте установщик на host-машине
   - Перенесите в VM через shared folder
   - Запустите установку в VM
   - Выберите C++ компоненты

2. **Установите Qt 5.5.1 MSVC 2010**
   - Скачайте на host
   - Перенесите в VM
   - Запустите установку
   - Выберите компонент: `msvc2010 32-bit`

3. **Соберите проект в VM:**
   ```cmd
   cd C:\путь\к\проекту
   build_xp.bat
   ```

### Альтернатива: Сборка на host, запуск на XP

**Проще и быстрее:**

1. На host-машине (Windows 10):
   - Установите VS 2010
   - Установите Qt 5.5.1 MSVC 2010
   - Соберите проект

2. Перенесите в XP VM только:
   - IceKing.exe
   - DLL файлы
   - HexIO.sys

**Это намного быстрее!** VS 2010 на XP работает медленно.

## 📊 Сравнение вариантов установки

| Вариант | Размер | Время установки | Сложность |
|---------|--------|-----------------|-----------|
| VS 2010 Express | ~700 MB | 20-30 мин | Легко |
| VS 2010 Professional | ~3 GB | 40-60 мин | Легко |
| Windows SDK 7.1 | ~500 MB | 15-20 мин | Средне |
| Qt 5.5.1 MSVC | ~800 MB | 10-15 мин | Легко |

## 🎓 Рекомендация

### Для начинающих:

**Самый простой путь:**

1. Скачайте и установите **Visual Studio 2010 Express**
2. Скачайте и установите **Qt 5.5.1 for MSVC 2010**
3. Используйте `build_xp.bat` для сборки

**Итого:** 2 установки, ~1.5 GB, ~30-40 минут

### Для опытных:

**Минимальный путь:**

1. Установите Windows SDK 7.1 (~500 MB)
2. Установите Qt 5.5.1 for MSVC 2010 (~800 MB)
3. Соберите вручную через SDK Command Prompt

**Итого:** 2 установки, ~1.3 GB, ~25-30 минут

## 🔗 Ссылки для скачивания

### Visual Studio 2010

**Visual Studio 2010 Express (бесплатно):**
- Требуется регистрация на Microsoft
- https://visualstudio.microsoft.com/vs/older-downloads/
- https://my.visualstudio.com/Downloads

**Windows SDK 7.1 (только компилятор):**
- https://www.microsoft.com/en-us/download/details.aspx?id=8279
- Прямая ссылка: GRMSDKX_EN_DVD.iso

### Qt

**Qt 5.5.1:**
- https://download.qt.io/archive/qt/5.5/5.5.1/
- Файл для MSVC 2010: `qt-opensource-windows-x86-msvc2010-5.5.1.exe`

**Qt 5.6.3 (если нужна LTS):**
- https://download.qt.io/archive/qt/5.6/5.6.3/
- Файл для MSVC 2010: `qt-opensource-windows-x86-msvc2013-5.6.3.exe`

## ❓ Частые вопросы

### Q: Можно ли использовать MinGW вместо MSVC?
**A:** НЕТ! hexiosupp.lib скомпилирован с MSVC и несовместим с MinGW.

### Q: Можно ли собрать на современной Windows и запустить на XP?
**A:** ДА! Если используете MSVC 2010 и Qt 5.5.1/5.6.x. Это даже удобнее.

### Q: Нужно ли удалять Qt для MinGW перед установкой Qt для MSVC?
**A:** НЕТ! Можно иметь обе версии одновременно. Они устанавливаются в разные папки.

### Q: Какая минимальная установка для сборки проекта?
**A:** Windows SDK 7.1 + Qt 5.5.1 MSVC 2010 (~1.3 GB)

### Q: Можно ли использовать Visual Studio 2013 или 2015?
**A:** Да, но нужна соответствующая версия Qt (5.6.x для VS 2015). Для XP лучше VS 2010.

## ✅ Итоговая рекомендация

**Для вашего случая (IceKing на Windows XP):**

### На хост-машине (Windows 10):
1. ✅ Установите Visual Studio 2010 Express
2. ✅ Установите Qt 5.5.1 для MSVC 2010
3. ✅ Соберите проект через `build_xp.bat`
4. ✅ Скопируйте результат в XP VM

### В XP VM (только для запуска):
1. ✅ Скопируйте IceKing.exe и DLL
2. ✅ Установите драйвер HexIO.sys
3. ✅ Запустите программу

**Не нужно ставить VS и Qt в саму XP VM!** Это будет очень медленно.

---

**Дополнительная помощь:** Запустите `check_qt_compiler.bat` чтобы увидеть что у вас уже установлено!
