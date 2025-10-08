#include <windows.h>
#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"

bool isRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    DWORD dwError = ERROR_SUCCESS;

    // Пытаемся открыть токен процесса
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        dwError = GetLastError();
        return false;
    }

    // Для Windows XP используем старый способ проверки прав
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup = NULL;

    // Создаем SID для группы администраторов
    if (!AllocateAndInitializeSid(&NtAuthority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &AdministratorsGroup)) {
        if (hToken) CloseHandle(hToken);
        return false;
    }

    // Проверяем, есть ли у токена группа администраторов
    if (!CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin)) {
        isAdmin = FALSE;
    }

    // Освобождаем ресурсы
    if (AdministratorsGroup) FreeSid(AdministratorsGroup);
    if (hToken) CloseHandle(hToken);

    return (isAdmin == TRUE);
}

bool restartAsAdmin()
{
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(NULL, modulePath, MAX_PATH);

    // Используем ShellExecuteW вместо ShellExecuteEx для лучшей совместимости
    HINSTANCE result = ShellExecuteW(
        NULL,                   // hwnd
        L"runas",               // operation - запрос прав администратора
        modulePath,             // file to execute
        NULL,                   // parameters
        NULL,                   // directory
        SW_SHOWNORMAL           // show command
    );

    // ShellExecute возвращает значение > 32 при успехе
    return ((INT_PTR)result > 32);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Проверяем права ДО создания главного окна
    if (!isRunningAsAdmin()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(NULL,
            "Требуются права администратора",
            "Для работы с PCI устройствами программа должна быть запущена от имени администратора.\n\n"
            "Перезапустить программу с повышенными правами?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes  // По умолчанию "Да"
        );

        if (reply == QMessageBox::Yes) {
            if (restartAsAdmin()) {
                return 0; // Завершаем текущий экземпляр
            } else {
                QMessageBox::warning(NULL,
                    "Ошибка",
                    "Не удалось перезапустить программу с правами администратора.\n"
                    "Запустите программу вручную от имени администратора."
                );
            }
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
