#ifndef JAKECAMERAWARNING_H
#define JAKECAMERAWARNING_H

#include <QWidget>
#include <QLabel>
#include <QMovie>
#include <QTimer>
#include <QPropertyAnimation>
#include <QVBoxLayout>

class JakeCameraWarning : public QWidget
{
    Q_OBJECT

public:
    enum WarningType {
        CAMERA_STARTED,      // Камера включилась
        RECORDING_STARTED,   // Запись началась
        STEALTH_MODE,        // Скрытый режим!
        PHOTO_TAKEN,         // Фото сделано
        KEYWORD_DETECTED,    // Обнаружено ключевое слово
        STEALTH_DAEMON,      // Скрытый демон активен
        FORBIDDEN_WORD       // Запрещенное слово обнаружено!
    };

    explicit JakeCameraWarning(QWidget *parent = nullptr);
    ~JakeCameraWarning();

public slots:
    void showWarning(WarningType type);
    void showForbiddenWordWarning(const QString &word);
    void hideWarning();

private:
    void setupUI();
    void loadGif(const QString &gifPath, const QString &message);
    void startSlideAnimation();

    QLabel *jakeLabel;         // GIF анимация Jake
    QLabel *warningLabel;      // Текст предупреждения
    QMovie *currentMovie;      // Текущая анимация
    QTimer *autoHideTimer;     // Таймер автоскрытия
    QPropertyAnimation *slideAnimation;  // Анимация появления
    
    WarningType currentType;
    bool isPersistent;         // Не скрывать автоматически (для записи)
};

#endif // JAKECAMERAWARNING_H

