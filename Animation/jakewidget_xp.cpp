#include "jakewidget_xp.h"
#include <QPainter>
#include <QtMath>
#include <QLabel>
#include <QMovie>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTemporaryFile>
// #include <QDesktopWidget> // Устарел, не нужен для XP
#include <QDebug>

JakeWidget_XP::JakeWidget_XP(QWidget *parent)
    : QWidget(parent), currentState(JakeState::FollowMouse),
      movie(nullptr), movieLabel(new QLabel(this)),
      m_scale(1.0), m_rotation(0.0), t(0.0), frame(0),
      smoothFactor(0.15), damping(0.85), acceleration(0.3),
      bouncePhase(0.0), squashAmount(0.0), stretchAmount(0.0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
    
    // Упрощенный таймер анимации - 20 FPS для совместимости с XP
    animationTimer.setInterval(50);
    connect(&animationTimer, SIGNAL(timeout()), this, SLOT(onTick()));
    animationTimer.start();
    
    // Упрощенное следование за мышкой
    mouseFollowTimer.setInterval(50); // 20 FPS
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
    movieLabel->setGeometry(0, 0, width(), height());
    
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
}

JakeWidget_XP::~JakeWidget_XP()
{
    if (movie) {
        movie->stop();
        delete movie;
    }
}

void JakeWidget_XP::setState(JakeState state)
{
    if (currentState == state) return;
    currentState = state;
    
    if (state == JakeState::Idle) {
        ensureMovie(":/Animation/Jake.gif");
        if (movie) {
            movie->setSpeed(100);
            movie->start();
        }
        if (movieLabel) movieLabel->show();
        mouseFollowTimer.stop();
        
        // Простая анимация масштаба без QPropertyAnimation
        m_scale = 1.0;
        
    } else if (state == JakeState::Hover) {
        ensureMovie(":/Animation/Jake.gif");
        if (movie) {
            movie->setSpeed(150);
            movie->start();
        }
        if (movieLabel) movieLabel->show();
        mouseFollowTimer.stop();
        
        // Легкое увеличение при наведении
        m_scale = 1.15;
        
    } else if (state == JakeState::Click) {
        frame = 0;
        mouseFollowTimer.stop();
        startSquashAnimation();
        
        // Простое вращение при клике
        m_rotation = 360.0;
        
        // Возвращаемся к следованию через таймеры
        QTimer::singleShot(800, this, SLOT(transitionToExcited()));
        QTimer::singleShot(1500, this, SLOT(transitionToFollowMouse()));
        
    } else if (state == JakeState::FollowMouse) {
        if (movieLabel) movieLabel->hide();
        mouseFollowTimer.start();
        
        m_scale = 1.0;
        m_rotation = 0.0;
        
    } else if (state == JakeState::Lab1) {
        if (movieLabel) movieLabel->hide();
        mouseFollowTimer.stop();
        
        startBounceAnimation();
        
    } else if (state == JakeState::Excited) {
        if (movieLabel) movieLabel->hide();
        mouseFollowTimer.stop();
        bouncePhase = 0.0;
        
        // Простое подпрыгивание
        m_scale = 1.2;
        
    } else if (state == JakeState::ButtonHover) {
        // Показываем гифку при наведении на кнопку
        ensureMovie(":/Animation/Jake.gif");
        if (movie) {
            movie->setSpeed(120);
            movie->start();
        }
        if (movieLabel) {
            movieLabel->show();
        }
        mouseFollowTimer.stop();
        
        // Легкое увеличение при наведении на кнопку
        m_scale = 1.1;
    }
    update();
}

void JakeWidget_XP::followMouse()
{
    if (currentState != JakeState::FollowMouse) return;
    
    updatePhysics();
    update();
}

void JakeWidget_XP::transitionToExcited()
{
    setState(JakeState::Excited);
}

void JakeWidget_XP::transitionToFollowMouse()
{
    setState(JakeState::FollowMouse);
}

void JakeWidget_XP::updatePhysics()
{
    QPoint globalCursorPos = QCursor::pos();
    QPointF newTarget = QPointF(globalCursorPos - QPoint(width()/2, height()/2));
    
    // Упрощенная физика движения
    QPointF direction = newTarget - currentPosition;
    qreal distance = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());
    
    // Применяем ускорение в направлении курсора
    if (distance > 1.0) {
        QPointF accel = direction * (this->acceleration / distance);
        velocity += accel;
    }
    
    // Применяем затухание
    velocity *= damping;
    
    // Обновляем позицию
    currentPosition += velocity;
    
    // Простое покачивание при движении
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

void JakeWidget_XP::onButtonHover()
{
    // Переходим в состояние наведения на кнопку
    setState(JakeState::ButtonHover);
}

void JakeWidget_XP::onButtonLeave()
{
    // Возвращаемся к следованию за мышью когда мышь уходит с кнопки
    if (currentState == JakeState::ButtonHover) {
        setState(JakeState::FollowMouse);
    }
}

void JakeWidget_XP::onButtonClick()
{
    // Сразу скрываем гифку и показываем анимацию нажатия
    if (movieLabel) movieLabel->hide();
    if (movie) {
        movie->stop();
    }
    setState(JakeState::Click);
}

void JakeWidget_XP::onTick()
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
    
    // Простая анимация масштаба
    if (m_scale > 1.0) {
        m_scale *= 0.98;
        if (m_scale < 1.01) m_scale = 1.0;
    }
    
    // Простая анимация вращения
    if (m_rotation > 0.1) {
        m_rotation *= 0.95;
        if (m_rotation < 0.1) m_rotation = 0.0;
    }
    
    update();
}

void JakeWidget_XP::paintEvent(QPaintEvent *event)
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
            // GIF отрисовывается через QLabel - ничего дополнительного не нужно
            return; // Выходим, так как QLabel сам отрисовывает GIF
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

void JakeWidget_XP::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (movieLabel) {
        movieLabel->setGeometry(0, 0, width(), height());
    }
}

void JakeWidget_XP::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        onButtonClick();
    }
    QWidget::mousePressEvent(event);
}

void JakeWidget_XP::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

void JakeWidget_XP::enterEvent(QEvent *event)
{
    if (currentState == JakeState::FollowMouse) {
        // Небольшое увеличение при наведении
        m_scale = 1.08;
    }
    QWidget::enterEvent(event);
}

void JakeWidget_XP::leaveEvent(QEvent *event)
{
    if (currentState == JakeState::FollowMouse) {
        // Возвращаемся к нормальному размеру
        m_scale = 1.0;
    }
    QWidget::leaveEvent(event);
}

void JakeWidget_XP::startBounceAnimation()
{
    bouncePhase = 0.0;
    m_scale = 1.1;
}

void JakeWidget_XP::startSquashAnimation()
{
    squashAmount = 0.3;
    stretchAmount = 0.0;
}

qreal JakeWidget_XP::easeOutElastic(qreal t)
{
    if (t == 0.0 || t == 1.0) return t;
    qreal p = 0.3;
    return qPow(2.0, -10.0 * t) * qSin((t - p / 4.0) * (2.0 * M_PI) / p) + 1.0;
}

qreal JakeWidget_XP::easeOutBounce(qreal t)
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

QPointF JakeWidget_XP::calculateArmPosition(const QPointF& shoulder, const QPointF& target, qreal armLength)
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

void JakeWidget_XP::drawJakeFollowMouse(QPainter &p, const QRectF &r)
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

void JakeWidget_XP::drawJakeExcited(QPainter &p, const QRectF &r)
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

void JakeWidget_XP::drawJakeLab1(QPainter &p, const QRectF &r)
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

void JakeWidget_XP::ensureMovie(const QString &path)
{
    if (!movieLabel) {
        return;
    }
    if (movie && movie->fileName() == path) {
        return;
    }
    if (movie) {
        movie->stop();
        delete movie;
        movie = nullptr;
    }
    
    // Попытка 1: Загрузить напрямую из ресурсов
    movie = new QMovie(path);
    if (movie->isValid()) {
        movieLabel->setMovie(movie);
        movie->start();
        return;
    }
    
    // Попытка 2: Для Windows XP - копируем из ресурсов во временный файл
    delete movie;
    movie = nullptr;
    
    QFile resourceFile(path);
    if (!resourceFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open resource file:" << path;
        movieLabel->clear();
        return;
    }
    
    QByteArray gifData = resourceFile.readAll();
    resourceFile.close();
    
    QString tempPath = QDir::tempPath() + "/jake_temp.gif";
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to create temp file:" << tempPath;
        movieLabel->clear();
        return;
    }
    
    tempFile.write(gifData);
    tempFile.close();
    
    // Попытка загрузить из временного файла
    movie = new QMovie(tempPath);
    if (!movie->isValid()) {
        delete movie;
        movie = nullptr;
        movieLabel->clear();
        qDebug() << "Failed to load GIF from temp file:" << tempPath << "- Qt GIF plugin may be missing on XP";
        return;
    }
    
    movieLabel->setMovie(movie);
    movie->start();
}
