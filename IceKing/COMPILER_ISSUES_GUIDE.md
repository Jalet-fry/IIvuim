# Проблема совместимости компиляторов для Windows XP

## Проблема

Проект `IceKing` требует использование библиотеки `hexiosupp.lib` для доступа к портам ввода/вывода через драйвер HexIO.sys. Возникает конфликт совместимости компиляторов:

- **hexiosupp.lib** - скомпилирована с MSVC (Microsoft Visual C++)
- **Qt** часто используется с MinGW компилятором
- Библиотеки (.lib файлы) **НЕ совместимы** между MSVC и MinGW!

## Что такое MinGW и MSVC?

### MinGW (Minimalist GNU for Windows)
- **Бесплатный** компилятор на базе GCC (GNU Compiler Collection)
- Использует GNU-стандартные библиотеки
- Формат библиотек: `.a` (libhexio.a) или `.lib` в формате GNU
- Генерирует код совместимый с GNU/Linux-подобным окружением
- **Плюсы**: бесплатный, open-source, хорошая совместимость с Unix-кодом
- **Минусы**: меньшая оптимизация под Windows, проблемы с проприетарными Windows-библиотеками

### MSVC (Microsoft Visual C++)
- **Коммерческий** компилятор от Microsoft (входит в Visual Studio)
- Использует Microsoft C Runtime (MSVCRT)
- Формат библиотек: `.lib` в формате COFF
- Лучшая интеграция с Windows API
- **Плюсы**: лучшая оптимизация под Windows, отладка, совместимость с Windows-библиотеками
- **Минусы**: платный (хотя есть Community версия), закрытый код

### Почему библиотеки несовместимы?

1. **Разный формат объектных файлов**:
   - MinGW использует PE/COFF формат GNU-стиля
   - MSVC использует Microsoft COFF формат

2. **Разные соглашения о вызовах (calling conventions)**:
   - Name mangling для C++ функций отличается
   - Разная обработка исключений (SEH vs Dwarf)

3. **Разные C Runtime библиотеки**:
   - MinGW связывается с `msvcrt.dll` или `libgcc`
   - MSVC связывается с `msvcrXXX.dll` (например, `msvcr100.dll` для VS2010)

## Текущее состояние проекта

В файле `Makefile` (строка 6) видно:
```makefile
# Command: C:\Qt\Qt5.5.1\5.5\msvc2010\bin\qmake.exe "CONFIG+=release" -o Makefile IceKing.pro
```

**Проект уже настроен на MSVC2010!** Это правильное решение.

В папке `release/` присутствуют:
- `msvcp100.dll` - C++ Standard Library для MSVC 2010
- `msvcr100.dll` - C Runtime Library для MSVC 2010

## Версии Qt для Windows XP

### Последние версии Qt с поддержкой Windows XP:

| Qt Версия | Компилятор | Статус поддержки XP | Примечания |
|-----------|-----------|---------------------|------------|
| **Qt 5.5.1** | MSVC 2010, 2012, 2013 | ✅ Полная поддержка | **Используется в проекте** |
| Qt 5.6.x | MSVC 2010, 2013, 2015 | ✅ Полная поддержка | Последняя версия LTS с XP |
| Qt 5.7+ | MSVC 2015+ | ❌ НЕТ поддержки | Требует Windows Vista+ |
| Qt 4.8.x | MSVC 2008, 2010, MinGW | ✅ Полная поддержка | Устаревшая, но работает |

### Рекомендации для Windows XP:

**Оптимальный выбор**: **Qt 5.5.1 с MSVC 2010** (как в текущем проекте)

## Решение проблемы

### ✅ Решение 1: Использовать Qt с MSVC 2010 (РЕКОМЕНДУЕТСЯ)

Проект уже настроен правильно! Но нужно собирать именно через MSVC.

#### Шаги на host-машине:

1. **Установить Visual Studio 2010** (или Visual Studio 2010 Express):
   - Можно скачать бесплатную версию: Visual Studio 2010 Express
   - Или использовать полную версию Visual Studio 2010

2. **Проверить переменные окружения**:
   ```cmd
   set PATH=C:\Qt\Qt5.5.1\5.5\msvc2010\bin;%PATH%
   ```

3. **Открыть "Visual Studio Command Prompt 2010"**:
   - Пуск → Visual Studio 2010 → Visual Studio Command Prompt 2010
   - Это настроит все пути для MSVC компилятора

4. **В командной строке VS перейти в папку проекта**:
   ```cmd
   cd C:\VM-shared\Common\IIvuim\IceKing
   ```

5. **Сгенерировать Makefile и собрать**:
   ```cmd
   C:\Qt\Qt5.5.1\5.5\msvc2010\bin\qmake.exe IceKing.pro
   nmake release
   ```

#### Проблемы которые могут возникнуть:

- **Inline assembly**: В `pciwindow.cpp` используется `__asm` блоки:
  ```cpp
  __asm
  {
      mov eax, configAddress
      mov dx, 0CF8h
      out dx, eax
      mov dx, 0CFCh
      in eax, dx
      mov regData, eax
  }
  ```
  
  **Важно**: Это **НЕ работает на 64-bit** компиляторах! MSVC x64 не поддерживает inline assembly.
  
  **Решение**: Использовать **32-bit компилятор** (x86) MSVC 2010.

### ❌ Решение 2: Создать обёртку для MinGW (НЕ рекомендуется)

Технически можно создать C-обёртку (без C++) для hexiosupp.lib и скомпилировать её как DLL с MSVC, затем загружать динамически через LoadLibrary в MinGW-коде. Но это сложно и не имеет смысла.

### ❌ Решение 3: Использовать альтернативную библиотеку (проблемы с XP)

Можно использовать альтернативы типа:
- **giveio.sys** (старый драйвер для портов I/O)
- **inpout32.dll/inpoutx64.dll**

Но hexiosupp.lib уже есть и работает!

## Сборка для VirtualBox с Windows XP

### На host-машине (Windows 10):

1. Установить Visual Studio 2010
2. Установить Qt 5.5.1 для MSVC 2010
3. Собрать проект как описано выше (x86/32-bit)
4. Скопировать в VM нужные файлы:
   - `IceKing.exe`
   - `HexIO.sys` (драйвер)
   - `msvcp100.dll`
   - `msvcr100.dll`
   - Qt DLL (Qt5Core.dll, Qt5Gui.dll, Qt5Widgets.dll) из `C:\Qt\Qt5.5.1\5.5\msvc2010\bin\`
   - Папку `platforms\` с `qwindows.dll` плагином

### В VirtualBox Windows XP:

1. Установить драйвер HexIO.sys (требуются права администратора)
2. Скопировать все DLL в папку с IceKing.exe
3. Запустить IceKing.exe

## Текущая структура проекта - ОК!

```
IceKing/
├── IceKing.pro          ← использует msvc2010 spec
├── hexiosupp.lib        ← MSVC библиотека
├── HexIO.sys            ← драйвер
├── msvcp100.dll         ← MSVC 2010 C++ runtime
├── msvcr100.dll         ← MSVC 2010 C runtime
└── release/
    └── IceKing.exe      ← собран с MSVC 2010
```

## Проверка версии компилятора

Посмотрите в `release\IceKing.exe.embed.manifest`:
```xml
<assemblyIdentity type="win32" name="Microsoft.VC100.CRT" ...>
```
Это подтверждает, что используется Visual C++ 2010 (VC100).

## Итоговые рекомендации

1. ✅ **Используйте MSVC 2010** - проект уже настроен правильно
2. ✅ **Qt 5.5.1 для MSVC 2010** - идеально для Windows XP
3. ✅ **32-bit (x86) компиляция** - обязательно из-за inline assembly
4. ❌ **НЕ используйте MinGW** - несовместимо с hexiosupp.lib
5. ❌ **НЕ используйте 64-bit компилятор** - inline assembly не поддерживается
6. ✅ **Собирайте в Visual Studio Command Prompt** - правильное окружение

## Дополнительные ссылки

- Visual Studio 2010 Express: https://visualstudio.microsoft.com/vs/older-downloads/
- Qt 5.5.1 Downloads: https://download.qt.io/archive/qt/5.5/5.5.1/
- Qt для Windows XP: https://wiki.qt.io/Qt_5.6_Tools_and_Versions#Supported_platforms

## Часто встречающиеся ошибки

### Ошибка: "unresolved external symbol" при сборке
**Причина**: Пытаетесь использовать MinGW с MSVC библиотекой
**Решение**: Используйте MSVC компилятор

### Ошибка: "inline assembler not supported in 64-bit"
**Причина**: Пытаетесь собрать 64-bit версию
**Решение**: Используйте 32-bit MSVC компилятор (x86)

### Ошибка: "MSVCP100.dll not found" на Windows XP
**Причина**: Отсутствует Visual C++ 2010 Runtime
**Решение**: Скопируйте msvcp100.dll и msvcr100.dll в папку с exe, или установите "Microsoft Visual C++ 2010 Redistributable Package (x86)"
