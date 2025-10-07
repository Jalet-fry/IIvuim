#ifndef PORTIO_H
#define PORTIO_H

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <conio.h>
    #include "hexioctrl_original.h"
    
    // Для Windows XP совместимости
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0501
    #endif
    
    // Функции для работы с портами ввода-вывода через HexIO драйвер
    inline void outpd(unsigned short port, unsigned long value) {
        // Прямой доступ к портам через inline ассемблер
        #ifdef Q_OS_WIN
            __asm__ __volatile__(
                "outl %%eax, %%dx"
                :
                : "a" (value), "d" (port)
            );
        #endif
    }
    
    inline unsigned long inpd(unsigned short port) {
        // Прямой доступ к портам через inline ассемблер
        unsigned long value = 0;
        #ifdef Q_OS_WIN
            __asm__ __volatile__(
                "inl %%dx, %%eax"
                : "=a" (value)
                : "d" (port)
            );
        #endif
        return value;
    }
    
    // Функции для разрешения доступа к портам (объявления)
    bool enablePortAccess();
    void disablePortAccess();
    
    // Функция для записи отладочной информации в файл
    class QString; // forward declaration to avoid including Qt headers here
    void writeToLogFile(const QString& message);
    
#else
    // Для других ОС - заглушки
    inline void outpd(unsigned short, unsigned long) {}
    inline unsigned long inpd(unsigned short) { return 0; }
    // Функции для разрешения доступа к портам (заглушки)
    class QString; // forward declaration for non-Windows path as well
    bool enablePortAccess();
    void disablePortAccess();
    void writeToLogFile(const QString&);
#endif

#endif // PORTIO_H
