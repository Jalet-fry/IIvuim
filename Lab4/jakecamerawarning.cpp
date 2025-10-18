#include "jakecamerawarning.h"
#include <QDebug>

JakeCameraWarning::JakeCameraWarning(QWidget *parent)
    : QWidget(parent),
      currentMovie(nullptr),
      isPersistent(false)
{
    setupUI();
    
    // Таймер автоскрытия
    autoHideTimer = new QTimer(this);
    autoHideTimer->setSingleShot(true);
    connect(autoHideTimer, &QTimer::timeout, this, &JakeCameraWarning::hideWarning);
    
    // Анимация появления (slide-in снизу)
    slideAnimation = new QPropertyAnimation(this, "pos");
    slideAnimation->setDuration(500);
    slideAnimation->setEasingCurve(QEasingCurve::OutBounce);
    
    hide();
}

JakeCameraWarning::~JakeCameraWarning()
{
    if (currentMovie) {
        currentMovie->stop();
        delete currentMovie;
    }
}

void JakeCameraWarning::setupUI()
{
    // Настройка окна
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFixedSize(250, 200);
    
    // Стиль окна
    setStyleSheet(
        "QWidget { "
        "  background-color: rgba(255, 255, 255, 240); "
        "  border: 3px solid #FF5722; "
        "  border-radius: 10px; "
        "}"
    );
    
    // Layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(5);
    
    // Jake GIF
    jakeLabel = new QLabel(this);
    jakeLabel->setAlignment(Qt::AlignCenter);
    jakeLabel->setFixedSize(200, 130);
    jakeLabel->setScaledContents(true);
    layout->addWidget(jakeLabel);
    
    // Текст предупреждения
    warningLabel = new QLabel(this);
    warningLabel->setAlignment(Qt::AlignCenter);
    warningLabel->setWordWrap(true);
    warningLabel->setStyleSheet(
        "QLabel { "
        "  font-size: 12px; "
        "  font-weight: bold; "
        "  color: #FF5722; "
        "  background: transparent; "
        "  border: none; "
        "}"
    );
    layout->addWidget(warningLabel);
}

void JakeCameraWarning::loadGif(const QString &gifPath, const QString &message)
{
    qDebug() << "=== LOADING GIF ===";
    qDebug() << "GIF path:" << gifPath;
    qDebug() << "Message:" << message;
    
    // Останавливаем предыдущую анимацию
    if (currentMovie) {
        qDebug() << "Stopping previous movie";
        currentMovie->stop();
        delete currentMovie;
        currentMovie = nullptr;
    }
    
    // Загружаем новую
    qDebug() << "Creating new QMovie with path:" << gifPath;
    currentMovie = new QMovie(gifPath);
    
    qDebug() << "Movie created, isValid:" << currentMovie->isValid();
    qDebug() << "Frame count:" << currentMovie->frameCount();
    
    if (!currentMovie->isValid()) {
        qDebug() << "❌ FAILED to load GIF:" << gifPath;
        warningLabel->setText("⚠️ Jake недоступен");
        return;
    }
    
    qDebug() << "✅ GIF loaded successfully";
    jakeLabel->setMovie(currentMovie);
    currentMovie->start();
    
    qDebug() << "Movie started, state:" << currentMovie->state();
    
    warningLabel->setText(message);
    qDebug() << "✅ Loaded GIF:" << gifPath << "with message:" << message;
}

void JakeCameraWarning::startSlideAnimation()
{
    // Позиционируем в правом нижнем углу родителя
    if (parentWidget()) {
        QPoint startPos = parentWidget()->rect().bottomRight() - QPoint(width() + 10, -10);
        QPoint endPos = parentWidget()->rect().bottomRight() - QPoint(width() + 10, height() + 10);
        
        setGeometry(startPos.x(), startPos.y(), width(), height());
        
        slideAnimation->setStartValue(startPos);
        slideAnimation->setEndValue(endPos);
        slideAnimation->start();
    }
}

void JakeCameraWarning::showWarning(WarningType type)
{
    currentType = type;
    isPersistent = false;
    
    QString gifPath;
    QString message;
    int hideDelay = 3000; // По умолчанию 3 секунды
    
    switch (type) {
        case CAMERA_STARTED:
            gifPath = "Animation/Jake vig eyes.gif";
            message = "👁️ Эй! Камера включилась!";
            hideDelay = 3000;
            setStyleSheet("QWidget { background-color: rgba(255, 255, 255, 240); border: 3px solid #2196F3; border-radius: 10px; }");
            break;
            
        case RECORDING_STARTED:
            gifPath = "Animation/Jake dance.gif";
            message = "⏺️ ЗАПИСЬ ИДЕТ!";
            isPersistent = true; // Не скрывать автоматически
            setStyleSheet("QWidget { background-color: rgba(255, 220, 220, 240); border: 3px solid #FF0000; border-radius: 10px; }");
            break;
            
        case STEALTH_MODE:
            gifPath = "Animation/Jake laugh.gif";
            message = "🕵️ Скрытая камера активна!";
            hideDelay = 5000;
            setStyleSheet("QWidget { background-color: rgba(255, 200, 200, 240); border: 3px solid #9C27B0; border-radius: 10px; }");
            break;
            
        case PHOTO_TAKEN:
            gifPath = "Animation/jake with cicrle.gif";
            message = "📸 КЛИК! Фото!";
            hideDelay = 2000;
            setStyleSheet("QWidget { background-color: rgba(220, 255, 220, 240); border: 3px solid #4CAF50; border-radius: 10px; }");
            break;
            
        case KEYWORD_DETECTED:
            gifPath = "Animation/Jake laugh.gif";
            message = "🔍 Ключевое слово!";
            hideDelay = 3000;
            setStyleSheet("QWidget { background-color: rgba(255, 235, 59, 240); border: 3px solid #FF9800; border-radius: 10px; }");
            break;
            
        case STEALTH_DAEMON:
            gifPath = "Animation/Jake vig eyes.gif";
            message = "🕵️ Демон активен!";
            hideDelay = 4000;
            setStyleSheet("QWidget { background-color: rgba(255, 200, 200, 240); border: 3px solid #E91E63; border-radius: 10px; }");
            break;
    }
    
    loadGif(gifPath, message);
    
    show();
    raise();
    startSlideAnimation();
    
    // Автоскрытие (если не постоянное)
    if (!isPersistent && hideDelay > 0) {
        autoHideTimer->start(hideDelay);
    }
    
    qDebug() << "JakeCameraWarning shown, type:" << type;
}

void JakeCameraWarning::hideWarning()
{
    // Анимация исчезновения
    QPropertyAnimation *fadeOut = new QPropertyAnimation(this, "windowOpacity");
    fadeOut->setDuration(300);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    connect(fadeOut, &QPropertyAnimation::finished, this, [this, fadeOut]() {
        hide();
        setWindowOpacity(1.0);
        fadeOut->deleteLater();
        
        // Останавливаем анимацию
        if (currentMovie) {
            currentMovie->stop();
        }
    });
    fadeOut->start();
    
    qDebug() << "JakeCameraWarning hiding";
}

void JakeCameraWarning::showForbiddenWordWarning(const QString &word)
{
    qDebug() << "=== SHOWING FORBIDDEN WORD WARNING ===";
    qDebug() << "Word:" << word;
    
    // Используем анимацию 009.gif для запрещенных слов
    QString gifPath = "Animation/009.gif";
    QString message = QString("⚠️ ЗАПРЕЩЕННОЕ СЛОВО!\n'%1'").arg(word);
    
    qDebug() << "GIF path:" << gifPath;
    qDebug() << "Message:" << message;
    
    // Стиль для запрещенных слов - красный и тревожный
    setStyleSheet("QWidget { background-color: rgba(255, 0, 0, 240); border: 4px solid #D32F2F; border-radius: 10px; }");
    
    // Загружаем GIF
    loadGif(gifPath, message);
    
    // Показываем окно
    show();
    raise();
    startSlideAnimation();
    
    // Показываем дольше для запрещенных слов
    autoHideTimer->start(5000);
    
    qDebug() << "Forbidden word warning displayed successfully for:" << word;
}

