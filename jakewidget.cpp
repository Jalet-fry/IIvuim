#include "jakewidget.h"
#include <QPainter>
#include <QtMath>
#include <QLabel>
#include <QMovie>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>

JakeWidget::JakeWidget(QWidget *parent)
    : QWidget(parent), currentState(JakeState::FollowMouse), frame(0), t(0.0), 
      smoothFactor(0.15), m_scale(1.0), m_rotation(0.0),
      damping(0.92), acceleration(0.8), bouncePhase(0.0),
      squashAmount(0.0), stretchAmount(0.0),
      movieLabel(new QLabel(this)), movie(nullptr),
      scaleAnimation(new QPropertyAnimation(this, "scale", this)),
      rotationAnimation(new QPropertyAnimation(this, "rotation", this))
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
    
    // Улучшенный таймер анимации - 30 FPS для плавности
    animationTimer.setInterval(33);
    connect(&animationTimer, SIGNAL(timeout()), this, SLOT(onTick()));
    animationTimer.start();
    
    // Более частое обновление следования за мышкой для быстрого отклика
    mouseFollowTimer.setInterval(8); // ~120 FPS для более быстрого отклика
    connect(&mouseFollowTimer, SIGNAL(timeout()), this, SLOT(followMouse()));
    mouseFollowTimer.start();

    // Начальная позиция
    QPoint globalCursorPos = QCursor::pos();
    currentPosition = QPointF(globalCursorPos - QPoint(width()/2, height()/2));
    targetPosition = currentPosition;
    velocity = QPointF(0, 0);

    // Настройка Movie label для GIF
    movieLabel->setAlignment(Qt::AlignCenter);
    movieLabel->setScaledContents(true);
    movieLabel->raise();
    movieLabel->hide(); // Скрываем по умолчанию
    
    // Настройка анимаций
    scaleAnimation->setDuration(300);
    scaleAnimation->setEasingCurve(QEasingCurve::OutElastic);
    
    rotationAnimation->setDuration(200);
    rotationAnimation->setEasingCurve(QEasingCurve::OutBack);
    
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
}

void JakeWidget::setState(JakeState state)
{
    if (currentState == state) return;
    currentState = state;
    
    // Все состояния теперь используют Jake.gif
        ensureMovie(":/Jake.gif");
        if (movie) {
            connect(movie, SIGNAL(frameChanged(int)), this, SLOT(onMovieFrameChanged()));
        }
        if (movieLabel) movieLabel->show();
    
    if (state == JakeState::Idle) {
        if (movie) movie->setSpeed(100);
        mouseFollowTimer.stop();
        
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.0);
        scaleAnimation->start();
        
    } else if (state == JakeState::Hover) {
        if (movie) movie->setSpeed(150);
        mouseFollowTimer.stop();
        
        // Легкое увеличение при наведении
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.15);
        scaleAnimation->setDuration(200);
        scaleAnimation->start();
        
    } else if (state == JakeState::Click) {
        frame = 0;
        if (movie) movie->setSpeed(200); // Быстрее при клике
        mouseFollowTimer.stop();
        startSquashAnimation();
        
        // Более выраженное вращение при клике
        rotationAnimation->stop();
        rotationAnimation->setStartValue(0.0);
        rotationAnimation->setEndValue(720.0); // Двойное вращение
        rotationAnimation->setDuration(400); // Быстрее
        rotationAnimation->setEasingCurve(QEasingCurve::OutElastic);
        rotationAnimation->start();
        
        // Возвращаемся к следованию через 0.8 секунды
        QTimer::singleShot(800, this, [this]() {
            setState(JakeState::Excited);
        });
        QTimer::singleShot(1500, this, [this]() {
            setState(JakeState::FollowMouse);
        });
        
    } else if (state == JakeState::FollowMouse) {
        if (movie) movie->setSpeed(120); // Немного быстрее при движении
        mouseFollowTimer.start();
        
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.0);
        scaleAnimation->start();
        
        rotationAnimation->stop();
        rotationAnimation->setStartValue(m_rotation);
        rotationAnimation->setEndValue(0.0);
        rotationAnimation->start();
        
    } else if (state == JakeState::Lab1) {
        if (movie) movie->setSpeed(180); // Быстрее в Lab1
        mouseFollowTimer.stop();
        
        startBounceAnimation();
        
    } else if (state == JakeState::Excited) {
        if (movie) movie->setSpeed(250); // Очень быстро при возбуждении
        mouseFollowTimer.stop();
        bouncePhase = 0.0;
        
        // Подпрыгивание
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.2);
        scaleAnimation->setDuration(150);
        scaleAnimation->setEasingCurve(QEasingCurve::OutBounce);
        scaleAnimation->start();
    }
    update();
}

void JakeWidget::followMouse()
{
    if (currentState != JakeState::FollowMouse) return;
    
    updatePhysics();
    update();
}

void JakeWidget::updatePhysics()
{
    QPoint globalCursorPos = QCursor::pos();
    QPointF newTarget = QPointF(globalCursorPos - QPoint(width()/2, height()/2));
    
    // Улучшенная физика движения с использованием скорости и затухания
    QPointF direction = newTarget - currentPosition;
    qreal distance = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());
    
    // Применяем ускорение в направлении курсора
    if (distance > 1.0) {
        QPointF acceleration = direction * (this->acceleration / distance);
        velocity += acceleration;
    }
    
    // Применяем затухание
    velocity *= damping;
    
    // Обновляем позицию
    currentPosition += velocity;
    
    // Небольшое покачивание при движении
    qreal velocityMag = qSqrt(velocity.x() * velocity.x() + velocity.y() * velocity.y());
    if (velocityMag > 0.5) {
        qreal wobble = qSin(t * 15.0) * velocityMag * 0.5;
        m_rotation = wobble;
    } else {
        m_rotation *= 0.9; // Плавно возвращаемся к нулю
    }
    
    // Ограничение максимальной скорости (увеличено для более быстрого движения)
    qreal maxSpeed = 35.0;
    qreal speed = qSqrt(velocity.x() * velocity.x() + velocity.y() * velocity.y());
    if (speed > maxSpeed) {
        velocity = velocity * (maxSpeed / speed);
    }
    
    move(qRound(currentPosition.x()), qRound(currentPosition.y()));
}

void JakeWidget::onButtonHover()
{
    if (currentState != JakeState::FollowMouse) return;
    
    setState(JakeState::Hover);
    QTimer::singleShot(500, this, [this]() {
        if (currentState == JakeState::Hover) {
            setState(JakeState::FollowMouse);
        }
    });
}

void JakeWidget::onButtonClick()
{
    setState(JakeState::Click);
}

void JakeWidget::onTick()
{
    frame = (frame + 1) % 1000000;
    t += 0.05;
    
    // Обновляем фазу подпрыгивания
    if (currentState == JakeState::Excited || currentState == JakeState::Lab1) {
        bouncePhase += 0.15;
    }
    
    // Плавное затухание squash/stretch эффектов (быстрее для более отзывчивого эффекта)
    squashAmount *= 0.88;
    stretchAmount *= 0.88;
    
    update();
}

void JakeWidget::onMovieFrameChanged()
{
    // Добавляем легкое покачивание к GIF анимации
    if (currentState == JakeState::Idle || currentState == JakeState::Hover) {
        update();
    }
}

void JakeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    // Теперь всегда показываем GIF через movieLabel
    // Дополнительные эффекты применяются через изменение геометрии и стилей movieLabel
    if (movieLabel && movieLabel->isVisible()) {
        QRect movieRect = rect();
        
        // Применяем масштабирование через изменение размера
        if (m_scale != 1.0) {
            int scaledWidth = qRound(rect().width() * m_scale);
            int scaledHeight = qRound(rect().height() * m_scale);
            movieRect.setSize(QSize(scaledWidth, scaledHeight));
            movieRect.moveCenter(rect().center());
        }
        
        // Дополнительные эффекты для разных состояний
        if (currentState == JakeState::Click) {
            // Более выраженный squash effect при клике
            qreal sq = 1.0 + squashAmount;
            movieRect.setHeight(qRound(movieRect.height() * sq));
            movieRect.setWidth(qRound(movieRect.width() / sq));
            movieRect.moveCenter(rect().center());
            
            // Добавляем небольшое покачивание при клике
            qreal shake = qSin(t * 20.0) * 3.0;
            movieRect.translate(qRound(shake), 0);
        } else if (currentState == JakeState::Excited) {
            // Подпрыгивание
            qreal bounce = qAbs(qSin(bouncePhase)) * 15.0;
            movieRect.translate(0, -qRound(bounce));
    } else if (currentState == JakeState::Lab1) {
            // Покачивание
        qreal bob = qSin(t * 3.0) * 5.0;
            movieRect.translate(0, qRound(bob));
        }
        
        // Применяем поворот через CSS трансформацию (если поддерживается)
        if (m_rotation != 0.0) {
            QString style = QString("QLabel { transform: rotate(%1deg); }").arg(m_rotation);
            movieLabel->setStyleSheet(style);
    } else {
            movieLabel->setStyleSheet("");
        }
        
        movieLabel->setGeometry(movieRect);
    }
}

void JakeWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (movieLabel) movieLabel->setGeometry(rect());
}

void JakeWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        onButtonClick();
    }
    QWidget::mousePressEvent(event);
}

void JakeWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

void JakeWidget::enterEvent(QEvent *event)
{
    if (currentState == JakeState::FollowMouse) {
        // Небольшое увеличение при наведении
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.08);
        scaleAnimation->setDuration(150);
        scaleAnimation->start();
    }
    QWidget::enterEvent(event);
}

void JakeWidget::leaveEvent(QEvent *event)
{
    if (currentState == JakeState::FollowMouse) {
        // Возвращаемся к нормальному размеру
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.0);
        scaleAnimation->setDuration(150);
        scaleAnimation->start();
    }
    QWidget::leaveEvent(event);
}

void JakeWidget::startBounceAnimation()
{
    bouncePhase = 0.0;
    scaleAnimation->stop();
    scaleAnimation->setStartValue(m_scale);
    scaleAnimation->setEndValue(1.1);
    scaleAnimation->setDuration(400);
    scaleAnimation->setEasingCurve(QEasingCurve::OutBounce);
    scaleAnimation->start();
}

void JakeWidget::startSquashAnimation()
{
    squashAmount = 0.6; // Увеличен squash эффект
    stretchAmount = 0.0;
}

qreal JakeWidget::easeOutElastic(qreal t)
{
    if (t == 0.0 || t == 1.0) return t;
    qreal p = 0.3;
    return qPow(2.0, -10.0 * t) * qSin((t - p / 4.0) * (2.0 * M_PI) / p) + 1.0;
}

qreal JakeWidget::easeOutBounce(qreal t)
{
    if (t < (1.0 / 2.75)) {
        return 7.5625 * t * t;
    } else if (t < (2.0 / 2.75)) {
        t -= (1.5 / 2.75);
        return 7.5625 * t * t + 0.75;
    } else if (t < (2.5 / 2.75)) {
        t -= (2.25 / 2.75);
        return 7.5625 * t * t + 0.9375;
    } else {
        t -= (2.625 / 2.75);
        return 7.5625 * t * t + 0.984375;
    }
}

QPointF JakeWidget::calculateArmPosition(const QPointF& shoulder, const QPointF& target, qreal armLength)
{
    QPointF direction = target - shoulder;
    qreal distance = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());
    
    if (distance <= armLength) {
        return target;
    } else {
        direction = direction * (armLength / distance);
        return shoulder + direction;
    }
}




void JakeWidget::ensureMovie(const QString &path)
{
    if (!movieLabel) return;
    if (movie && movie->fileName() == path) return;
    if (movie) {
        movie->stop();
        delete movie;
        movie = nullptr;
    }
    movie = new QMovie(path);
    if (!movie->isValid()) {
        delete movie;
        movie = nullptr;
        movieLabel->clear();
        return;
    }
    movieLabel->setMovie(movie);
    movie->start();
}