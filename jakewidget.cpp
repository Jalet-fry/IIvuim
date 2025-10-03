#include "jakewidget.h"
#include <QPainter>
#include <QtMath>
#include <QLabel>
#include <QMovie>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QDesktopWidget>

JakeWidget::JakeWidget(QWidget *parent)
    : QWidget(parent), currentState(JakeState::FollowMouse), frame(0), t(0.0), 
      smoothFactor(0.1), movieLabel(new QLabel(this)), movie(nullptr)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
    
    // Таймер анимации
    animationTimer.setInterval(50); // 20 FPS
    connect(&animationTimer, SIGNAL(timeout()), this, SLOT(onTick()));
    animationTimer.start();
    
    // Таймер следования за мышкой
    mouseFollowTimer.setInterval(30);
    connect(&mouseFollowTimer, SIGNAL(timeout()), this, SLOT(followMouse()));
    mouseFollowTimer.start();

    // Начальная позиция
    QPoint globalCursorPos = QCursor::pos();
    currentPosition = globalCursorPos - QPoint(width()/2, height()/2);
    targetPosition = currentPosition;

    movieLabel->setAlignment(Qt::AlignCenter);
    movieLabel->setScaledContents(false);
    movieLabel->raise();
    // ensureMovie(":/assets/jake_base.gif"); // Закомментировано, так как используем отрисовку
    
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
}

void JakeWidget::setState(JakeState state)
{
    if (currentState == state) return;
    currentState = state;
    
    if (state == JakeState::Idle) {
        // ensureMovie(":/assets/jake_base.gif");
        // if (movie) movie->setSpeed(100);
        // if (movieLabel) movieLabel->show();
        mouseFollowTimer.stop();
    } else if (state == JakeState::Hover) {
        // ensureMovie(":/assets/jake_base.gif");
        // if (movie) movie->setSpeed(120);
        // if (movieLabel) movieLabel->show();
        mouseFollowTimer.stop();
    } else if (state == JakeState::Click) {
        // ensureMovie(":/assets/jake_base.gif");
        // if (movie) movie->setSpeed(180);
        // if (movieLabel) movieLabel->show();
        frame = 0;
        mouseFollowTimer.stop();
        
        // Возвращаемся к следования через 1 секунду
        QTimer::singleShot(1000, this, [this]() {
            setState(JakeState::FollowMouse);
        });
    } else if (state == JakeState::FollowMouse) {
        // if (movieLabel) movieLabel->hide();
        mouseFollowTimer.start();
    } else if (state == JakeState::Lab1) {
        // if (movieLabel) movieLabel->hide();
        mouseFollowTimer.stop();
    }
    update();
}

void JakeWidget::followMouse()
{
    if (currentState != JakeState::FollowMouse) return;
    
    QPoint globalCursorPos = QCursor::pos();
    QPoint newTarget = globalCursorPos - QPoint(width()/2, height()/2);
    
    // Плавное движение
    targetPosition = newTarget;
    currentPosition = currentPosition + (targetPosition - currentPosition) * smoothFactor;
    
    // ИСПРАВЛЕНИЕ: используем QPointF для move()
    move(currentPosition.x(), currentPosition.y());
    update();
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
    update();
}

void JakeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    if (currentState == JakeState::Lab1) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRectF bounds = rect();
        QRectF r = QRectF(0, 0, qMin(bounds.width(), 420.0), qMin(bounds.height(), 340.0));
        r.moveCenter(bounds.center());
        qreal bob = qSin(t * 3.0) * 3.0;
        r.translate(0, bob);
        drawJakeLab1(p, r);
    } else if (currentState == JakeState::FollowMouse) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRectF bounds = rect();
        QRectF r = QRectF(0, 0, qMin(bounds.width(), 200.0), qMin(bounds.height(), 160.0));
        r.moveCenter(bounds.center());
        drawJakeFollowMouse(p, r);
    } else {
        // Для других состояний используем стандартную отрисовку
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRectF bounds = rect();
        QRectF r = QRectF(0, 0, qMin(bounds.width(), 200.0), qMin(bounds.height(), 160.0));
        r.moveCenter(bounds.center());
        drawJakeFollowMouse(p, r);
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
    // Colors
    QColor bodyColor(255, 199, 64);
    QColor outline(60, 40, 10);
    QColor eyeWhite(255, 255, 255);
    QColor eyeBlack(20, 20, 20);
    QColor mouthColor(120, 60, 20);

    p.save();
    p.translate(r.center());

    qreal baseW = r.width();
    qreal baseH = r.height();

    // Получаем позицию курсора относительно Джейка
    QPoint globalCursorPos = QCursor::pos();
    QPoint jakeGlobalPos = mapToGlobal(rect().center());
    QPointF cursorRelative = globalCursorPos - jakeGlobalPos;
    
    // Нормализуем позицию для анимации
    qreal cursorX = qBound(-1.0, cursorRelative.x() / (baseW * 0.5), 1.0);
    qreal cursorY = qBound(-1.0, cursorRelative.y() / (baseH * 0.5), 1.0);

    // Тело (округлое)
    QRectF bodyRect(-baseW * 0.25, -baseH * 0.30, baseW * 0.5, baseH * 0.6);
    QPainterPath bodyPath;
    bodyPath.addRoundedRect(bodyRect, 40, 40);
    p.setPen(QPen(outline, 2));
    p.setBrush(bodyColor);
    p.drawPath(bodyPath);

    // Уши
    QRectF earL(bodyRect.left() - 8, bodyRect.top() + 15, 16, 14);
    QRectF earR(bodyRect.right() - 8, bodyRect.top() + 15, 16, 14);
    p.drawEllipse(earL);
    p.drawEllipse(earR);

    // Глаза следят за курсором
    QPointF eyeCenterL(bodyRect.center().x() - 15, bodyRect.center().y() - 10);
    QPointF eyeCenterR(bodyRect.center().x() + 15, bodyRect.center().y() - 10);
    qreal eyeRadius = 12;
    p.setBrush(eyeWhite);
    p.drawEllipse(eyeCenterL, eyeRadius, eyeRadius);
    p.drawEllipse(eyeCenterR, eyeRadius, eyeRadius);

    // Зрачки следят за курсором
    p.setBrush(eyeBlack);
    QPointF pupilOffset(cursorX * 4, cursorY * 4);
    p.drawEllipse(eyeCenterL + pupilOffset, 4, 4);
    p.drawEllipse(eyeCenterR + pupilOffset, 4, 4);

    // Нос
    QRectF muzzle(bodyRect.center().x() - 12, bodyRect.center().y() - 2, 24, 12);
    p.setBrush(bodyColor.darker(105));
    p.drawRoundedRect(muzzle, 6, 6);

    // Улыбка
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(mouthColor, 3));
    qreal smile = 0.8 + 0.2 * qSin(t * 4.0);
    QRectF mouthRect(bodyRect.center().x() - 15, bodyRect.center().y() + 5, 30, 15);
    int startAngle = 200 * 16;
    int spanAngle = static_cast<int>(smile * 140 * 16);
    p.drawArc(mouthRect, startAngle, spanAngle);

    // Руки тянутся к курсору
    p.setPen(QPen(outline, 4));
    qreal armLen = baseW * 0.3;
    
    // Левая рука
    QPointF shoulderL(bodyRect.left() + 4, bodyRect.center().y() - 8);
    QPointF targetHandL = calculateArmPosition(shoulderL, cursorRelative * 0.5, armLen);
    p.drawLine(shoulderL, targetHandL);
    
    // Правая рука
    QPointF shoulderR(bodyRect.right() - 4, bodyRect.center().y() - 8);
    QPointF targetHandR = calculateArmPosition(shoulderR, cursorRelative * 0.5, armLen);
    p.drawLine(shoulderR, targetHandR);

    p.restore();
}

void JakeWidget::drawJakeLab1(QPainter &p, const QRectF &r)
{
    // Colors
    QColor bodyColor(255, 199, 64);
    QColor outline(60, 40, 10);
    QColor eyeWhite(255, 255, 255);
    QColor eyeBlack(20, 20, 20);
    QColor mouthColor(120, 60, 20);

    p.save();
    p.translate(r.center());

    qreal baseW = r.width();
    qreal baseH = r.height();

    // Squash & stretch depending on state
    qreal sx = 1.0, sy = 1.0, rot = 0.0;
    if (currentState == JakeState::Idle) {
        sx = 1.0 + 0.02 * qSin(t * 2.0);
        sy = 1.0 - 0.02 * qSin(t * 2.0);
    } else if (currentState == JakeState::Hover) {
        sx = 1.05; sy = 1.05; rot = qSin(t * 3.0) * 3.0;
    } else if (currentState == JakeState::Click) {
        qreal pulse = 1.0 + 0.12 * qExp(-0.3 * frame) * qSin(frame * 0.9);
        sx = pulse; sy = pulse; rot = qSin(frame * 0.6) * 2.0;
    } else if (currentState == JakeState::Lab1) {
        rot = qSin(t * 8.0) * 6.0;
    }

    p.rotate(rot);
    p.scale(sx, sy);

    // Body (more similar proportions)
    QRectF bodyRect(-baseW * 0.27, -baseH * 0.30, baseW * 0.54, baseH * 0.60);
    QPainterPath bodyPath;
    bodyPath.addRoundedRect(bodyRect, 50, 50);
    p.setPen(QPen(outline, 3));
    p.setBrush(bodyColor);
    p.drawPath(bodyPath);

    // Ears
    QRectF earL(bodyRect.left() - 16, bodyRect.top() + 28, 26, 22);
    QRectF earR(bodyRect.right() - 10, bodyRect.top() + 28, 26, 22);
    p.drawEllipse(earL);
    p.drawEllipse(earR);

    // Eyes
    QPointF eyeCenterL(bodyRect.center().x() - 30, bodyRect.center().y() - 16);
    QPointF eyeCenterR(bodyRect.center().x() + 30, bodyRect.center().y() - 16);
    qreal eyeRadius = 20;
    p.setBrush(eyeWhite);
    p.drawEllipse(eyeCenterL, eyeRadius, eyeRadius);
    p.drawEllipse(eyeCenterR, eyeRadius, eyeRadius);

    // Pupils follow a tiny orbit depending on state
    p.setBrush(eyeBlack);
    qreal orbit = 5.0;
    QPointF pupilOffset(qCos(t * 2.0) * orbit, qSin(t * 2.0) * orbit);
    p.drawEllipse(eyeCenterL + pupilOffset, 6, 6);
    p.drawEllipse(eyeCenterR + pupilOffset, 6, 6);

    // Nose/muzzle
    QRectF muzzle(bodyRect.center().x() - 26, bodyRect.center().y() - 4, 52, 26);
    p.setBrush(bodyColor.darker(105));
    p.drawRoundedRect(muzzle, 8, 8);

    // Mouth (smile animates)
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(mouthColor, 4));
    qreal smile = 0.7 + 0.25 * qSin(t * 6.0);
    QRectF mouthRect(bodyRect.center().x() - 30, bodyRect.center().y() + 8, 60, 30);
    int startAngle = 200 * 16;
    int spanAngle = static_cast<int>(smile * 140 * 16);
    p.drawArc(mouthRect, startAngle, spanAngle);

    // Arms waving in Lab1
    p.setPen(QPen(outline, 6));
    qreal armLen = baseW * 0.24;
    qreal wave = qSin(t * 8.0) * 0.6;
    // Left arm up waving
    QPointF shoulderL(bodyRect.left() + 6, bodyRect.center().y() - 10);
    QPointF elbowL = shoulderL + QPointF(-armLen * 0.6, -armLen * (0.1 + 0.4 * (wave + 0.5)));
    QPointF handL = elbowL + QPointF(-armLen * 0.4, armLen * (0.2 - 0.2 * wave));
    p.drawLine(shoulderL, elbowL);
    p.drawLine(elbowL, handL);
    // Right arm sideways
    QPointF shoulderR(bodyRect.right() - 6, bodyRect.center().y() - 10);
    QPointF handR = shoulderR + QPointF(armLen * (1.0 - 0.2 * wave), -armLen * (0.15 + 0.2 * (wave + 0.5)));
    p.drawLine(shoulderR, handR);

    p.restore();
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