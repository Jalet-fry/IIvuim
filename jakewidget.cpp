#include "jakewidget.h"

#include <QPainter>
#include <QtMath>
#include <QLabel>
#include <QMovie>
#include <QResizeEvent>

JakeWidget::JakeWidget(QWidget *parent)
    : QWidget(parent), currentState(JakeState::Idle), frame(0), t(0.0), movieLabel(new QLabel(this)), movie(nullptr)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    animationTimer.setInterval(50); // 20 FPS (used for Lab1 drawing only)
    connect(&animationTimer, SIGNAL(timeout()), this, SLOT(onTick()));
    animationTimer.start();

    movieLabel->setAlignment(Qt::AlignCenter);
    movieLabel->setScaledContents(false); // Keep aspect
    movieLabel->raise();
    ensureMovie(":/assets/jake_base.gif");
}

void JakeWidget::setState(JakeState state)
{
    if (currentState == state) return;
    currentState = state;
    // Adjust movie speed / visibility; Lab1 uses custom drawing
    if (state == JakeState::Idle) {
        ensureMovie(":/assets/jake_base.gif");
        if (movie) movie->setSpeed(100);
        if (movieLabel) movieLabel->show();
    } else if (state == JakeState::Hover) {
        ensureMovie(":/assets/jake_base.gif");
        if (movie) movie->setSpeed(120);
        if (movieLabel) movieLabel->show();
    } else if (state == JakeState::Click) {
        ensureMovie(":/assets/jake_base.gif");
        if (movie) movie->setSpeed(180);
        if (movieLabel) movieLabel->show();
        frame = 0;
    } else if (state == JakeState::Lab1) {
        if (movieLabel) movieLabel->hide();
    }
    update();
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
    } else {
        // movie draws via QLabel; paintEvent only ensures layout
        if (movieLabel) movieLabel->setGeometry(rect());
    }
}

void JakeWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (movieLabel) movieLabel->setGeometry(rect());
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
        rot = qSin(t * 8.0) * 6.0; // веселый мах
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


