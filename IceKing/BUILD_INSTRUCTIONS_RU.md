# Инструкция по сборке проекта IceKing для Windows XP

## Кратко о проблеме

**hexiosupp.lib требует MSVC, а не MinGW!**

- **MSVC** (Microsoft Visual C++) - компилятор от Microsoft
- **MinGW** (Minimalist GNU for Windows) - бесплатный компилятор на базе GCC
- Библиотеки .lib от MSVC **НЕ совместимы** с MinGW!

## Что нужно установить на хост-машину

1. **Visual Studio 2010** (любая версия: Express, Professional, Ultimate)
   - Скачать: https://visualstudio.microsoft.com/vs/older-downloads/
   - Или Visual Studio 2010 Express (бесплатно)

2. **Qt 5.5.1 для MSVC 2010**
   - Скачать: https://download.qt.io/archive/qt/5.5/5.5.1/
   - Файл: `qt-opensource-windows-x86-msvc2010-5.5.1.exe`
   - Установить в `C:\Qt\Qt5.5.1\`

## Как собрать проект

### Вариант 1: Через Visual Studio Command Prompt (рекомендуется)

1. Откройте **"Visual Studio Command Prompt 2010"**:
   - Пуск → All Programs → Microsoft Visual Studio 2010 → Visual Studio Tools → Visual Studio Command Prompt (2010)

2. Перейдите в папку проекта:
   ```cmd
   cd C:\VM-shared\Common\IIvuim\IceKing
   ```

3. Запустите qmake для генерации Makefile:
   ```cmd
   C:\Qt\Qt5.5.1\5.5\msvc2010\bin\qmake.exe IceKing.pro
   ```

4. Соберите проект:
   ```cmd
   nmake release
   ```

5. Исполняемый файл будет в папке `release\IceKing.exe`

### Вариант 2: Через батник (автоматически)

1. Используйте `build_xp.bat` из папки проекта:
   ```cmd
   build_xp.bat
   ```

2. Скрипт автоматически:
   - Настроит окружение MSVC
   - Запустит qmake
   - Соберет проект
   - Скопирует необходимые DLL

### Вариант 3: Через Qt Creator

1. Откройте Qt Creator из комплекта Qt 5.5.1 MSVC 2010
2. File → Open File or Project → выберите `IceKing.pro`
3. В настройках Kit выберите **"Desktop Qt 5.5.1 MSVC2010 32bit"**
4. Build → Build Project "IceKing"

## Что копировать на Windows XP VM

После сборки скопируйте в одну папку:

### Обязательные файлы:
```
IceKing.exe           (из release/)
HexIO.sys             (драйвер для доступа к портам)
msvcp100.dll          (уже есть в release/)
msvcr100.dll          (уже есть в release/)
```

### Qt DLL (из C:\Qt\Qt5.5.1\5.5\msvc2010\bin\):
```
Qt5Core.dll
Qt5Gui.dll
Qt5Widgets.dll
```

### Qt плагины (из C:\Qt\Qt5.5.1\5.5\msvc2010\plugins\):
```
platforms\qwindows.dll    (создайте папку platforms\ рядом с exe)
```

### Ресурсы (если нужны иконки):
```
img\                      (вся папка)
```

### Итоговая структура на XP:
```
C:\IceKing\
├── IceKing.exe
├── HexIO.sys
├── msvcp100.dll
├── msvcr100.dll
├── Qt5Core.dll
├── Qt5Gui.dll
├── Qt5Widgets.dll
├── platforms\
│   └── qwindows.dll
└── img\
    └── ik-ic.ico
```

## Установка драйвера HexIO на Windows XP

1. Скопируйте `HexIO.sys` в `C:\Windows\System32\drivers\`

2. Откройте командную строку **от имени администратора**

3. Установите драйвер:
   ```cmd
   sc create HexIO binPath= C:\Windows\System32\drivers\HexIO.sys type= kernel start= demand
   sc start HexIO
   ```

4. Проверьте статус:
   ```cmd
   sc query HexIO
   ```

## Проверка правильности сборки

### На host-машине проверьте:

1. **Какой компилятор использовался**:
   - Откройте `release\IceKing.exe.embed.manifest`
   - Должна быть строка: `<assemblyIdentity type="win32" name="Microsoft.VC100.CRT"`
   - VC100 = Visual C++ 2010 ✅

2. **Какие DLL нужны**:
   ```cmd
   dumpbin /DEPENDENTS release\IceKing.exe
   ```
   Должны быть: MSVCR100.dll, MSVCP100.dll, Qt5Core.dll, Qt5Gui.dll, Qt5Widgets.dll

3. **32-bit или 64-bit**:
   ```cmd
   dumpbin /HEADERS release\IceKing.exe | findstr "machine"
   ```
   Должно быть: `14C machine (x86)` ✅ (32-bit)

## Устранение проблем

### Проблема: "nmake: command not found"
**Причина**: Не запущен Visual Studio Command Prompt  
**Решение**: Используйте именно Visual Studio Command Prompt 2010, а не обычную cmd

### Проблема: "error LNK2019: unresolved external symbol"
**Причина**: Пытаетесь собрать с MinGW вместо MSVC  
**Решение**: Убедитесь, что используете Qt MSVC 2010, а не MinGW версию

### Проблема: "inline assembler not supported in 64-bit"
**Причина**: Пытаетесь собрать 64-bit версию  
**Решение**: Используйте 32-bit (x86) MSVC 2010

### Проблема: На XP не запускается - "ordinal not found"
**Причина**: Используется Qt версии выше 5.6  
**Решение**: Используйте Qt 5.5.1 или 5.6.x (последние версии с XP)

### Проблема: На XP не запускается - "MSVCP100.dll not found"
**Причина**: Отсутствует Visual C++ 2010 Runtime  
**Решение**: 
- Скопируйте msvcp100.dll и msvcr100.dll в папку с exe
- ИЛИ установите "Microsoft Visual C++ 2010 Redistributable Package (x86)" на XP

## Дополнительная информация

Подробное объяснение различий между MinGW и MSVC, совместимости компиляторов и версий Qt для XP смотрите в файле: **COMPILER_ISSUES_GUIDE.md**

## Контрольный чек-лист перед сборкой

- [ ] Установлен Visual Studio 2010
- [ ] Установлен Qt 5.5.1 для MSVC 2010 (32-bit)
- [ ] Используется Visual Studio Command Prompt 2010
- [ ] Путь к qmake: `C:\Qt\Qt5.5.1\5.5\msvc2010\bin\qmake.exe`
- [ ] Компилятор: MSVC 2010 (x86/32-bit)
- [ ] Библиотека: hexiosupp.lib (MSVC-совместимая) присутствует

## Быстрая проверка окружения

В Visual Studio Command Prompt 2010 выполните:
```cmd
echo %VSINSTALLDIR%
echo %FrameworkDir%
cl
```

Должны увидеть пути Visual Studio и информацию о компиляторе Microsoft C/C++ версии 16.x (VS 2010).
