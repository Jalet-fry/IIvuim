#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>

class TestStealthWindow : public QWidget
{
    Q_OBJECT

public:
    TestStealthWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Test Stealth Mode - Qt 5.5.1");
        setFixedSize(400, 300);
        
        QVBoxLayout *layout = new QVBoxLayout(this);
        
        QLabel *titleLabel = new QLabel("Тест скрытого режима", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #1976D2; }");
        layout->addWidget(titleLabel);
        
        QLabel *infoLabel = new QLabel(
            "Этот тест проверяет работу скрытого режима в Qt 5.5.1:\n\n"
            "1. Создание невидимого окна-заглушки\n"
            "2. Скрытие всех видимых окон\n"
            "3. Поддержка работы приложения\n"
            "4. Корректное восстановление окон\n\n"
            "Нажмите кнопки для тестирования:"
        );
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet("QLabel { padding: 10px; background-color: #F5F5F5; border-radius: 5px; }");
        layout->addWidget(infoLabel);
        
        QPushButton *createStealthBtn = new QPushButton("Создать невидимое окно-заглушку", this);
        connect(createStealthBtn, &QPushButton::clicked, this, &TestStealthWindow::createStealthWindow);
        layout->addWidget(createStealthBtn);
        
        QPushButton *hideMainBtn = new QPushButton("Скрыть главное окно", this);
        connect(hideMainBtn, &QPushButton::clicked, this, &TestStealthWindow::hideMainWindow);
        layout->addWidget(hideMainBtn);
        
        QPushButton *showMainBtn = new QPushButton("Показать главное окно", this);
        connect(showMainBtn, &QPushButton::clicked, this, &TestStealthWindow::showMainWindow);
        layout->addWidget(showMainBtn);
        
        QPushButton *destroyStealthBtn = new QPushButton("Уничтожить невидимое окно", this);
        connect(destroyStealthBtn, &QPushButton::clicked, this, &TestStealthWindow::destroyStealthWindow);
        layout->addWidget(destroyStealthBtn);
        
        QPushButton *testQuitBtn = new QPushButton("Тест завершения приложения", this);
        connect(testQuitBtn, &QPushButton::clicked, this, &TestStealthWindow::testQuit);
        layout->addWidget(testQuitBtn);
        
        stealthWindow = nullptr;
        isHidden = false;
    }
    
private slots:
    void createStealthWindow()
    {
        qDebug() << "=== CREATING STEALTH WINDOW ===";
        
        if (stealthWindow) {
            QMessageBox::information(this, "Ошибка", "Невидимое окно уже создано!");
            return;
        }
        
        // Создаем невидимое окно-заглушку с правильными флагами для Qt 5.5.1
        stealthWindow = new QWidget();
        
        // Устанавливаем флаги окна для невидимости, но поддержки работы приложения
        stealthWindow->setWindowFlags(
            Qt::Tool | 
            Qt::FramelessWindowHint | 
            Qt::WindowStaysOnTopHint |
            Qt::WindowDoesNotAcceptFocus
        );
        
        // Делаем окно полностью прозрачным
        stealthWindow->setAttribute(Qt::WA_TranslucentBackground, true);
        stealthWindow->setAttribute(Qt::WA_NoSystemBackground, true);
        stealthWindow->setAttribute(Qt::WA_ShowWithoutActivating, true);
        
        // Минимальный размер и позиция за экраном
        stealthWindow->setFixedSize(1, 1);
        stealthWindow->move(-1000, -1000);
        stealthWindow->setWindowTitle("Stealth Window");
        
        // КРИТИЧЕСКИ ВАЖНО: Делаем окно видимым для Qt, но невидимым для пользователя
        stealthWindow->setVisible(true);
        stealthWindow->show();
        stealthWindow->raise();
        
        qDebug() << "Stealth window created and shown";
        qDebug() << "Stealth window is visible:" << stealthWindow->isVisible();
        qDebug() << "Stealth window geometry:" << stealthWindow->geometry();
        qDebug() << "Stealth window windowFlags:" << stealthWindow->windowFlags();
        
        QMessageBox::information(this, "Успех", 
            "Невидимое окно-заглушка создано!\n\n"
            "Проверьте:\n"
            "• Окно не должно быть видимо на экране\n"
            "• Приложение должно продолжать работать\n"
            "• В диспетчере задач должно быть видно приложение"
        );
    }
    
    void hideMainWindow()
    {
        qDebug() << "=== HIDING MAIN WINDOW ===";
        
        if (isHidden) {
            QMessageBox::information(this, "Информация", "Главное окно уже скрыто!");
            return;
        }
        
        if (!stealthWindow) {
            QMessageBox::warning(this, "Предупреждение", 
                "Сначала создайте невидимое окно-заглушку!\n"
                "Без него приложение может завершиться."
            );
            return;
        }
        
        this->hide();
        isHidden = true;
        
        qDebug() << "Main window hidden";
        
        QMessageBox::information(this, "Скрыто", 
            "Главное окно скрыто!\n\n"
            "Проверьте:\n"
            "• Приложение должно продолжать работать\n"
            "• Невидимое окно-заглушка поддерживает работу\n"
            "• В диспетчере задач должно быть видно приложение"
        );
    }
    
    void showMainWindow()
    {
        qDebug() << "=== SHOWING MAIN WINDOW ===";
        
        if (!isHidden) {
            QMessageBox::information(this, "Информация", "Главное окно уже показано!");
            return;
        }
        
        this->show();
        this->raise();
        this->activateWindow();
        isHidden = false;
        
        qDebug() << "Main window shown";
        
        QMessageBox::information(this, "Показано", "Главное окно восстановлено!");
    }
    
    void destroyStealthWindow()
    {
        qDebug() << "=== DESTROYING STEALTH WINDOW ===";
        
        if (!stealthWindow) {
            QMessageBox::information(this, "Информация", "Невидимое окно не создано!");
            return;
        }
        
        // Сначала скрываем окно
        stealthWindow->hide();
        stealthWindow->setVisible(false);
        
        // Затем удаляем его
        stealthWindow->deleteLater();
        stealthWindow = nullptr;
        
        qDebug() << "Stealth window destroyed";
        
        QMessageBox::information(this, "Уничтожено", "Невидимое окно-заглушка уничтожено!");
    }
    
    void testQuit()
    {
        qDebug() << "=== TESTING QUIT ===";
        
        if (stealthWindow && isHidden) {
            QMessageBox::information(this, "Тест завершения", 
                "Приложение работает в скрытом режиме!\n\n"
                "Нажмите OK для завершения теста."
            );
        }
        
        qDebug() << "Quitting application...";
        QCoreApplication::quit();
    }

private:
    QWidget *stealthWindow;
    bool isHidden;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // ВАЖНО: Отключаем автоматическое завершение приложения при закрытии всех окон
    // Это необходимо для работы скрытого режима в Qt 5.5.1
    a.setQuitOnLastWindowClosed(false);
    qDebug() << "QuitOnLastWindowClosed set to false for stealth mode support";
    
    TestStealthWindow w;
    w.show();
    
    return a.exec();
}

#include "test_stealth_mode.moc"
