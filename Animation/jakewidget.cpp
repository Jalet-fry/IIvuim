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
#include <QDebug>

JakeWidget::JakeWidget(QWidget *parent)
    : QWidget(parent), currentState(JakeState::FollowMouse), frame(0), t(0.0), 
      smoothFactor(0.15), m_scale(1.0), m_rotation(0.0),
      damping(0.85), acceleration(0.3), bouncePhase(0.0),
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
    
    // Более частое обновление следования за мышкой для плавности
    mouseFollowTimer.setInterval(16); // ~60 FPS
    connect(&mouseFollowTimer, SIGNAL(timeout()), this, SLOT(followMouse()));
    mouseFollowTimer.start();
    
    // Таймер для автоматического возврата из состояния наведения
    hoverTimer.setSingleShot(true);

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
    movieLabel->setGeometry(0, 0, width(), height()); // Устанавливаем размер
    
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
    
    if (state == JakeState::Idle) {
        ensureMovie(":/Animation/Jake.gif");
        if (movie) {
            movie->setSpeed(100);
            movie->start(); // Убеждаемся что гифка запущена
            connect(movie, SIGNAL(frameChanged(int)), this, SLOT(onMovieFrameChanged()));
        }
        if (movieLabel) movieLabel->show();
        mouseFollowTimer.stop();
        
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.0);
        scaleAnimation->start();
        
    } else if (state == JakeState::Hover) {
        ensureMovie(":/Animation/Jake.gif");
        if (movie) {
            movie->setSpeed(150);
            movie->start(); // Убеждаемся что гифка запущена
        }
        if (movieLabel) movieLabel->show();
        mouseFollowTimer.stop();
        
        // Легкое увеличение при наведении
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.15);
        scaleAnimation->setDuration(200);
        scaleAnimation->start();
        
    } else if (state == JakeState::Click) {
        frame = 0;
        mouseFollowTimer.stop();
        startSquashAnimation();
        
        // Быстрое вращение при клике
        rotationAnimation->stop();
        rotationAnimation->setStartValue(0.0);
        rotationAnimation->setEndValue(360.0);
        rotationAnimation->setDuration(500);
        rotationAnimation->start();
        
        // Возвращаемся к следованию через 0.8 секунды
        QTimer::singleShot(800, this, [this]() {
            setState(JakeState::Excited);
        });
        QTimer::singleShot(1500, this, [this]() {
            setState(JakeState::FollowMouse);
        });
        
    } else if (state == JakeState::FollowMouse) {
        if (movieLabel) movieLabel->hide();
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
        if (movieLabel) movieLabel->hide();
        mouseFollowTimer.stop();
        
        startBounceAnimation();
        
    } else if (state == JakeState::Excited) {
        if (movieLabel) movieLabel->hide();
        mouseFollowTimer.stop();
        bouncePhase = 0.0;
        
        // Подпрыгивание
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.2);
        scaleAnimation->setDuration(150);
        scaleAnimation->setEasingCurve(QEasingCurve::OutBounce);
        scaleAnimation->start();
        
    } else if (state == JakeState::ButtonHover) {
        // Показываем гифку при наведении на кнопку
        ensureMovie(":/Animation/Jake.gif");
        if (movie) {
            qDebug() << "ButtonHover: Movie loaded, isValid:" << movie->isValid() << "frameCount:" << movie->frameCount();
            movie->setSpeed(120);
            movie->start(); // Убеждаемся что гифка запущена
            qDebug() << "ButtonHover: Movie started, state:" << movie->state();
            connect(movie, SIGNAL(frameChanged(int)), this, SLOT(onMovieFrameChanged()));
        } else {
            qDebug() << "ButtonHover: Movie is null!";
        }
        if (movieLabel) {
            movieLabel->show();
            qDebug() << "ButtonHover: MovieLabel shown";
        }
        mouseFollowTimer.stop();
        
        // Легкое увеличение при наведении на кнопку
        scaleAnimation->stop();
        scaleAnimation->setStartValue(m_scale);
        scaleAnimation->setEndValue(1.1);
        scaleAnimation->setDuration(200);
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
    
    // Ограничение максимальной скорости
    qreal maxSpeed = 15.0;
    qreal speed = qSqrt(velocity.x() * velocity.x() + velocity.y() * velocity.y());
    if (speed > maxSpeed) {
        velocity = velocity * (maxSpeed / speed);
    }
    
    move(qRound(currentPosition.x()), qRound(currentPosition.y()));
}

void JakeWidget::onButtonHover()
{
    qDebug() << "onButtonHover called, current state:" << static_cast<int>(currentState);
    // Переходим в состояние наведения на кнопку
    setState(JakeState::ButtonHover);
}

void JakeWidget::onButtonLeave()
{
    // Возвращаемся к следованию за мышью когда мышь уходит с кнопки
    if (currentState == JakeState::ButtonHover) {
        setState(JakeState::FollowMouse);
    }
}

void JakeWidget::onButtonClick()
{
    // Сразу скрываем гифку и показываем анимацию нажатия
    if (movieLabel) movieLabel->hide();
    if (movie) {
        movie->stop();
    }
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
    
    // Плавное затухание squash/stretch эффектов
    squashAmount *= 0.95;
    stretchAmount *= 0.95;
    
    update();
}

void JakeWidget::onMovieFrameChanged()
{
    // Добавляем легкое покачивание к GIF анимации
    if (currentState == JakeState::Idle || currentState == JakeState::Hover || currentState == JakeState::ButtonHover) {
        update();
    }
}

void JakeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    // Применяем глобальные трансформации
    p.translate(rect().center());
    p.scale(m_scale, m_scale);
    p.rotate(m_rotation);
    p.translate(-rect().center());
    
    if (currentState == JakeState::Idle || currentState == JakeState::Hover || currentState == JakeState::ButtonHover) {
        // Показываем GIF через movieLabel
        if (movieLabel && movieLabel->isVisible()) {
            // GIF отрисовывается через QLabel
        }
    } else if (currentState == JakeState::Lab1) {
        QRectF bounds = rect();
        QRectF r = QRectF(0, 0, qMin(bounds.width(), 420.0), qMin(bounds.height(), 340.0));
        r.moveCenter(bounds.center());
        qreal bob = qSin(t * 3.0) * 5.0;
        r.translate(0, bob);
        drawJakeLab1(p, r);
    } else if (currentState == JakeState::Excited) {
        QRectF bounds = rect();
        QRectF r = QRectF(0, 0, qMin(bounds.width(), 240.0), qMin(bounds.height(), 200.0));
        r.moveCenter(bounds.center());
        qreal bounce = qAbs(qSin(bouncePhase)) * 15.0;
        r.translate(0, -bounce);
        drawJakeExcited(p, r);
    } else if (currentState == JakeState::Click) {
        QRectF bounds = rect();
        QRectF r = QRectF(0, 0, qMin(bounds.width(), 200.0), qMin(bounds.height(), 160.0));
        r.moveCenter(bounds.center());
        // Squash effect при клике
        p.save();
        p.translate(r.center());
        qreal sq = 1.0 + squashAmount;
        p.scale(1.0 / sq, sq);
        p.translate(-r.center());
        drawJakeFollowMouse(p, r);
        p.restore();
    } else if (currentState == JakeState::FollowMouse) {
        QRectF bounds = rect();
        QRectF r = QRectF(0, 0, qMin(bounds.width(), 200.0), qMin(bounds.height(), 160.0));
        r.moveCenter(bounds.center());
        drawJakeFollowMouse(p, r);
    } else {
        QRectF bounds = rect();
        QRectF r = QRectF(0, 0, qMin(bounds.width(), 200.0), qMin(bounds.height(), 160.0));
        r.moveCenter(bounds.center());
        drawJakeFollowMouse(p, r);
    }
}

void JakeWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (movieLabel) {
        movieLabel->setGeometry(0, 0, width(), height());
        qDebug() << "ResizeEvent: MovieLabel geometry set to" << movieLabel->geometry();
    }
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
    squashAmount = 0.3;
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

void JakeWidget::drawJakeFollowMouse(QPainter &p, const QRectF &r)
{
    // Более яркие цвета как у оригинального Джейка
    QColor bodyColor(255, 212, 102);
    QColor outline(50, 35, 10);
    QColor eyeWhite(255, 255, 255);
    QColor eyeBlack(20, 20, 20);
    QColor mouthColor(80, 50, 20);

    p.save();
    p.translate(r.center());

    qreal baseW = r.width();
    qreal baseH = r.height();

    // Получаем позицию курсора относительно Джейка
    QPoint globalCursorPos = QCursor::pos();
    QPoint jakeGlobalPos = mapToGlobal(rect().center());
    QPointF cursorRelative = globalCursorPos - jakeGlobalPos;
    
    // Нормализуем позицию для анимации
    qreal cursorX = qBound(-1.0, cursorRelative.x() / (baseW * 0.8), 1.0);
    qreal cursorY = qBound(-1.0, cursorRelative.y() / (baseH * 0.8), 1.0);

    // Легкое дыхание
    qreal breathe = 1.0 + 0.015 * qSin(t * 2.5);
    
    // Тело (более округлое и мягкое)
    QRectF bodyRect(-baseW * 0.28 * breathe, -baseH * 0.32, baseW * 0.56 * breathe, baseH * 0.64 * breathe);
    QPainterPath bodyPath;
    bodyPath.addRoundedRect(bodyRect, 45, 45);
    
    // Добавляем градиент для объема
    QLinearGradient bodyGrad(bodyRect.topLeft(), bodyRect.bottomRight());
    bodyGrad.setColorAt(0.0, bodyColor.lighter(105));
    bodyGrad.setColorAt(1.0, bodyColor.darker(105));
    
    p.setPen(QPen(outline, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(bodyGrad);
    p.drawPath(bodyPath);

    // Уши (побольше и более округлые)
    QRectF earL(bodyRect.left() - 10, bodyRect.top() + 18, 20, 18);
    QRectF earR(bodyRect.right() - 10, bodyRect.top() + 18, 20, 18);
    p.setBrush(bodyColor);
    p.drawEllipse(earL);
    p.drawEllipse(earR);

    // Глаза следят за курсором (побольше)
    QPointF eyeCenterL(bodyRect.center().x() - 18, bodyRect.center().y() - 12);
    QPointF eyeCenterR(bodyRect.center().x() + 18, bodyRect.center().y() - 12);
    qreal eyeRadius = 14;
    
    // Моргание
    qreal blinkPhase = qSin(t * 0.8);
    qreal eyeOpen = blinkPhase > 0.95 ? qMax(0.0, 1.0 - (blinkPhase - 0.95) * 20.0) : 1.0;
    
    p.setBrush(eyeWhite);
    p.drawEllipse(eyeCenterL, eyeRadius, eyeRadius * eyeOpen);
    p.drawEllipse(eyeCenterR, eyeRadius, eyeRadius * eyeOpen);

    // Зрачки следят за курсором
    if (eyeOpen > 0.3) {
        p.setBrush(eyeBlack);
        QPointF pupilOffset(cursorX * 5, cursorY * 5);
        qreal pupilSize = 5;
        p.drawEllipse(eyeCenterL + pupilOffset, pupilSize, pupilSize);
        p.drawEllipse(eyeCenterR + pupilOffset, pupilSize, pupilSize);
    }

    // Мордочка/нос
    QRectF muzzle(bodyRect.center().x() - 14, bodyRect.center().y() + 0, 28, 14);
    p.setBrush(bodyColor.darker(108));
    p.drawRoundedRect(muzzle, 7, 7);
    
    // Точка носа
    p.setBrush(outline);
    QPointF nosePos(muzzle.center().x(), muzzle.top() + 4);
    p.drawEllipse(nosePos, 2.5, 2.5);

    // Улыбка (более выразительная)
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(mouthColor, 2.5, Qt::SolidLine, Qt::RoundCap));
    qreal smile = 0.85 + 0.15 * qSin(t * 3.5);
    QRectF mouthRect(bodyRect.center().x() - 18, bodyRect.center().y() + 8, 36, 18);
    int startAngle = 200 * 16;
    int spanAngle = static_cast<int>(smile * 140 * 16);
    p.drawArc(mouthRect, startAngle, spanAngle);

    // Руки тянутся к курсору (более плавно)
    p.setPen(QPen(outline, 5, Qt::SolidLine, Qt::RoundCap));
    qreal armLen = baseW * 0.35;
    qreal armWave = qSin(t * 4.0) * 0.05;
    
    // Левая рука
    QPointF shoulderL(bodyRect.left() + 6, bodyRect.center().y() - 5);
    QPointF targetHandL = calculateArmPosition(shoulderL, cursorRelative * (0.6 + armWave), armLen);
    p.drawLine(shoulderL, targetHandL);
    // Ладошка
    p.setBrush(bodyColor);
    p.drawEllipse(targetHandL, 6, 6);
    
    // Правая рука
    QPointF shoulderR(bodyRect.right() - 6, bodyRect.center().y() - 5);
    QPointF targetHandR = calculateArmPosition(shoulderR, cursorRelative * (0.6 - armWave), armLen);
    p.drawLine(shoulderR, targetHandR);
    // Ладошка
    p.drawEllipse(targetHandR, 6, 6);
    
    // Ножки
    p.setPen(QPen(outline, 5, Qt::SolidLine, Qt::RoundCap));
    QPointF legL(bodyRect.center().x() - 15, bodyRect.bottom() + 2);
    QPointF legR(bodyRect.center().x() + 15, bodyRect.bottom() + 2);
    QPointF footL(legL.x() - 4, legL.y() + 8);
    QPointF footR(legR.x() + 4, legR.y() + 8);
    p.drawLine(legL, footL);
    p.drawLine(legR, footR);
    p.setBrush(bodyColor);
    p.drawEllipse(footL, 5, 5);
    p.drawEllipse(footR, 5, 5);

    p.restore();
}

void JakeWidget::drawJakeExcited(QPainter &p, const QRectF &r)
{
    // Яркие цвета для возбужденного состояния
    QColor bodyColor(255, 220, 110);
    QColor outline(50, 35, 10);
    QColor eyeWhite(255, 255, 255);
    QColor eyeBlack(20, 20, 20);
    QColor mouthColor(200, 80, 30);

    p.save();
    p.translate(r.center());

    qreal baseW = r.width();
    qreal baseH = r.height();

    // Энергичное покачивание
    qreal wiggle = qSin(bouncePhase * 4.0) * 5.0;
    p.rotate(wiggle);

    // Тело
    QRectF bodyRect(-baseW * 0.30, -baseH * 0.33, baseW * 0.60, baseH * 0.66);
    QPainterPath bodyPath;
    bodyPath.addRoundedRect(bodyRect, 48, 48);
    
    QRadialGradient bodyGrad(bodyRect.center(), baseW * 0.4);
    bodyGrad.setColorAt(0.0, bodyColor.lighter(108));
    bodyGrad.setColorAt(1.0, bodyColor);
    
    p.setPen(QPen(outline, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(bodyGrad);
    p.drawPath(bodyPath);

    // Уши подпрыгивают
    qreal earBounce = qAbs(qSin(bouncePhase * 2.0)) * 8.0;
    QRectF earL(bodyRect.left() - 12, bodyRect.top() + 20 - earBounce, 22, 20);
    QRectF earR(bodyRect.right() - 10, bodyRect.top() + 20 - earBounce, 22, 20);
    p.setBrush(bodyColor);
    p.drawEllipse(earL);
    p.drawEllipse(earR);

    // Огромные глаза от восторга!
    QPointF eyeCenterL(bodyRect.center().x() - 22, bodyRect.center().y() - 15);
    QPointF eyeCenterR(bodyRect.center().x() + 22, bodyRect.center().y() - 15);
    qreal eyeRadius = 18;
    p.setBrush(eyeWhite);
    p.drawEllipse(eyeCenterL, eyeRadius, eyeRadius);
    p.drawEllipse(eyeCenterR, eyeRadius, eyeRadius);

    // Огромные зрачки
    p.setBrush(eyeBlack);
    qreal pupilSize = 8;
    p.drawEllipse(eyeCenterL, pupilSize, pupilSize);
    p.drawEllipse(eyeCenterR, pupilSize, pupilSize);
    
    // Блики в глазах
    p.setBrush(eyeWhite);
    p.drawEllipse(eyeCenterL + QPointF(-3, -3), 3, 3);
    p.drawEllipse(eyeCenterR + QPointF(-3, -3), 3, 3);

    // Большая улыбка
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(mouthColor, 4, Qt::SolidLine, Qt::RoundCap));
    QRectF mouthRect(bodyRect.center().x() - 22, bodyRect.center().y() + 5, 44, 25);
    p.drawArc(mouthRect, 180 * 16, 180 * 16);
    
    // Язык
    QPainterPath tongue;
    tongue.moveTo(mouthRect.center().x() - 6, mouthRect.center().y() + 8);
    tongue.cubicTo(mouthRect.center().x() - 4, mouthRect.center().y() + 12,
                   mouthRect.center().x() + 4, mouthRect.center().y() + 12,
                   mouthRect.center().x() + 6, mouthRect.center().y() + 8);
    p.setBrush(QColor(220, 100, 100));
    p.setPen(QPen(outline, 2));
    p.drawPath(tongue);

    // Руки вверх от радости!
    p.setPen(QPen(outline, 5, Qt::SolidLine, Qt::RoundCap));
    qreal armWave = qSin(bouncePhase * 6.0) * 15.0;
    
    QPointF shoulderL(bodyRect.left() + 8, bodyRect.center().y() - 10);
    QPointF handL = shoulderL + QPointF(-30 + armWave, -45);
    p.drawLine(shoulderL, handL);
    p.setBrush(bodyColor);
    p.drawEllipse(handL, 7, 7);
    
    QPointF shoulderR(bodyRect.right() - 8, bodyRect.center().y() - 10);
    QPointF handR = shoulderR + QPointF(30 - armWave, -45);
    p.drawLine(shoulderR, handR);
    p.drawEllipse(handR, 7, 7);

    // Прыгающие ножки
    p.setPen(QPen(outline, 5, Qt::SolidLine, Qt::RoundCap));
    qreal legKick = qSin(bouncePhase * 4.0) * 10.0;
    QPointF legL(bodyRect.center().x() - 18, bodyRect.bottom() + 3);
    QPointF legR(bodyRect.center().x() + 18, bodyRect.bottom() + 3);
    QPointF footL(legL.x() - 6 - legKick, legL.y() + 10);
    QPointF footR(legR.x() + 6 + legKick, legR.y() + 10);
    p.drawLine(legL, footL);
    p.drawLine(legR, footR);
    p.drawEllipse(footL, 6, 6);
    p.drawEllipse(footR, 6, 6);

    p.restore();
}

void JakeWidget::drawJakeLab1(QPainter &p, const QRectF &r)
{
    // Colors
    QColor bodyColor(255, 212, 102);
    QColor outline(50, 35, 10);
    QColor eyeWhite(255, 255, 255);
    QColor eyeBlack(20, 20, 20);
    QColor mouthColor(100, 60, 25);

    p.save();
    p.translate(r.center());

    qreal baseW = r.width();
    qreal baseH = r.height();

    // Покачивание в Lab1
    qreal rot = qSin(t * 4.0) * 8.0;
    p.rotate(rot);

    // Body
    QRectF bodyRect(-baseW * 0.28, -baseH * 0.31, baseW * 0.56, baseH * 0.62);
    QPainterPath bodyPath;
    bodyPath.addRoundedRect(bodyRect, 52, 52);
    
    QLinearGradient bodyGrad(bodyRect.topLeft(), bodyRect.bottomRight());
    bodyGrad.setColorAt(0.0, bodyColor.lighter(106));
    bodyGrad.setColorAt(1.0, bodyColor.darker(104));
    
    p.setPen(QPen(outline, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(bodyGrad);
    p.drawPath(bodyPath);

    // Ears
    QRectF earL(bodyRect.left() - 16, bodyRect.top() + 28, 26, 22);
    QRectF earR(bodyRect.right() - 10, bodyRect.top() + 28, 26, 22);
    p.setBrush(bodyColor);
    p.drawEllipse(earL);
    p.drawEllipse(earR);

    // Eyes
    QPointF eyeCenterL(bodyRect.center().x() - 30, bodyRect.center().y() - 18);
    QPointF eyeCenterR(bodyRect.center().x() + 30, bodyRect.center().y() - 18);
    qreal eyeRadius = 22;
    p.setBrush(eyeWhite);
    p.drawEllipse(eyeCenterL, eyeRadius, eyeRadius);
    p.drawEllipse(eyeCenterR, eyeRadius, eyeRadius);

    // Pupils делают орбиту
    p.setBrush(eyeBlack);
    qreal orbit = 7.0;
    QPointF pupilOffset(qCos(t * 3.0) * orbit, qSin(t * 3.0) * orbit);
    p.drawEllipse(eyeCenterL + pupilOffset, 7, 7);
    p.drawEllipse(eyeCenterR + pupilOffset, 7, 7);

    // Nose/muzzle
    QRectF muzzle(bodyRect.center().x() - 28, bodyRect.center().y() - 3, 56, 28);
    p.setBrush(bodyColor.darker(108));
    p.drawRoundedRect(muzzle, 9, 9);

    // Mouth (большая улыбка)
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(mouthColor, 4, Qt::SolidLine, Qt::RoundCap));
    qreal smile = 0.75 + 0.20 * qSin(t * 5.0);
    QRectF mouthRect(bodyRect.center().x() - 32, bodyRect.center().y() + 10, 64, 32);
    int startAngle = 195 * 16;
    int spanAngle = static_cast<int>(smile * 150 * 16);
    p.drawArc(mouthRect, startAngle, spanAngle);

    // Arms машут в Lab1
    p.setPen(QPen(outline, 6, Qt::SolidLine, Qt::RoundCap));
    qreal armLen = baseW * 0.26;
    qreal wave = qSin(t * 7.0);
    
    // Left arm машет
    QPointF shoulderL(bodyRect.left() + 8, bodyRect.center().y() - 12);
    QPointF elbowL = shoulderL + QPointF(-armLen * 0.6, -armLen * (0.2 + 0.5 * qAbs(wave)));
    QPointF handL = elbowL + QPointF(-armLen * 0.4 * (1.0 + wave * 0.3), armLen * (0.1 - 0.3 * wave));
    p.drawLine(shoulderL, elbowL);
    p.drawLine(elbowL, handL);
    p.setBrush(bodyColor);
    p.drawEllipse(handL, 8, 8);
    
    // Right arm тоже машет
    QPointF shoulderR(bodyRect.right() - 8, bodyRect.center().y() - 12);
    QPointF elbowR = shoulderR + QPointF(armLen * 0.6, -armLen * (0.3 + 0.4 * qAbs(wave)));
    QPointF handR = elbowR + QPointF(armLen * 0.4 * (1.0 - wave * 0.3), armLen * (0.1 + 0.3 * wave));
    p.drawLine(shoulderR, elbowR);
    p.drawLine(elbowR, handR);
    p.drawEllipse(handR, 8, 8);

    p.restore();
}

void JakeWidget::ensureMovie(const QString &path)
{
    qDebug() << "ensureMovie called with path:" << path;
    if (!movieLabel) {
        qDebug() << "MovieLabel is null!";
        return;
    }
    if (movie && movie->fileName() == path) {
        qDebug() << "Movie already loaded with same path";
        return;
    }
    if (movie) {
        qDebug() << "Stopping and deleting existing movie";
        movie->stop();
        delete movie;
        movie = nullptr;
    }
    qDebug() << "Creating new movie with path:" << path;
    movie = new QMovie(path);
    if (!movie->isValid()) {
        qDebug() << "Movie is not valid!";
        delete movie;
        movie = nullptr;
        movieLabel->clear();
        return;
    }
    qDebug() << "Movie is valid, frameCount:" << movie->frameCount();
    movieLabel->setMovie(movie);
    movie->start();
    qDebug() << "Movie started, state:" << movie->state();
}