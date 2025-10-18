#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>

class TestWindow : public QWidget
{
    Q_OBJECT

public:
    TestWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Test Stealth Window");
        setFixedSize(300, 200);
        
        QVBoxLayout *layout = new QVBoxLayout(this);
        
        QLabel *label = new QLabel("Test Stealth Window\nClick to create stealth window", this);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
        
        QPushButton *createBtn = new QPushButton("Create Stealth Window", this);
        connect(createBtn, &QPushButton::clicked, this, &TestWindow::createStealthWindow);
        layout->addWidget(createBtn);
        
        QPushButton *destroyBtn = new QPushButton("Destroy Stealth Window", this);
        connect(destroyBtn, &QPushButton::clicked, this, &TestWindow::destroyStealthWindow);
        layout->addWidget(destroyBtn);
        
        QPushButton *hideBtn = new QPushButton("Hide Main Window", this);
        connect(hideBtn, &QPushButton::clicked, this, &TestWindow::hideMainWindow);
        layout->addWidget(hideBtn);
        
        QPushButton *showBtn = new QPushButton("Show Main Window", this);
        connect(showBtn, &QPushButton::clicked, this, &TestWindow::showMainWindow);
        layout->addWidget(showBtn);
        
        stealthWindow = nullptr;
    }
    
private slots:
    void createStealthWindow()
    {
        qDebug() << "Creating stealth window...";
        
        if (stealthWindow) {
            qDebug() << "Stealth window already exists!";
            return;
        }
        
        // Создаем видимое окно-заглушку для тестирования
        stealthWindow = new QWidget();
        stealthWindow->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        stealthWindow->setFixedSize(200, 100); // Больший размер для тестирования
        stealthWindow->move(100, 100); // Позиция в центре экрана для тестирования
        stealthWindow->setWindowTitle("Stealth Window - TEST MODE");
        
        // Добавляем простой текст для видимости
        QLabel *label = new QLabel("Stealth Mode\nActive", stealthWindow);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("QLabel { background-color: red; color: white; font-weight: bold; }");
        label->setGeometry(0, 0, 200, 100);
        
        stealthWindow->show();
        
        qDebug() << "Stealth window created and shown (TEST MODE)";
    }
    
    void destroyStealthWindow()
    {
        qDebug() << "Destroying stealth window...";
        
        if (stealthWindow) {
            stealthWindow->hide();
            stealthWindow->deleteLater();
            stealthWindow = nullptr;
            qDebug() << "Stealth window destroyed";
        } else {
            qDebug() << "No stealth window to destroy";
        }
    }
    
    void hideMainWindow()
    {
        qDebug() << "Hiding main window...";
        this->hide();
        qDebug() << "Main window hidden";
    }
    
    void showMainWindow()
    {
        qDebug() << "Showing main window...";
        this->show();
        qDebug() << "Main window shown";
    }

private:
    QWidget *stealthWindow;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    TestWindow w;
    w.show();
    
    return a.exec();
}

#include "test_stealth_window.moc"
