#ifndef JAKEWIDGET_XP_H
#define JAKEWIDGET_XP_H

#include <QWidget>
#include <QTimer>
#include <QMovie>
#include <QLabel>
#include <QPainter>

// XP-совместимая версия JakeWidget без современных Qt функций
class JakeWidget_XP : public QWidget
{
    Q_OBJECT

public:
    enum JakeState {
        Idle,
        Hover,
        Click,
        FollowMouse,
        Lab1,
        Excited,
        ButtonHover
    };

    explicit JakeWidget_XP(QWidget *parent = nullptr);
    ~JakeWidget_XP();

    void setState(JakeState state);

public slots:
    void onButtonHover();
    void onButtonLeave();
    void onButtonClick();

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

private slots:
    void onTick();
    void followMouse();
    void transitionToExcited();
    void transitionToFollowMouse();

private:
    JakeState currentState;
    QTimer animationTimer;
    QTimer mouseFollowTimer;
    QMovie *movie;
    QLabel *movieLabel;
    
    // Простые переменные для анимации без QPropertyAnimation
    qreal m_scale;
    qreal m_rotation;
    qreal t;
    int frame;
    
    // Позиция и физика
    QPointF currentPosition;
    QPointF targetPosition;
    QPointF velocity;
    qreal smoothFactor;
    qreal damping;
    qreal acceleration;
    
    // Простые эффекты
    qreal bouncePhase;
    qreal squashAmount;
    qreal stretchAmount;
    
    void updatePhysics();
    void drawJakeFollowMouse(QPainter &p, const QRectF &r);
    void drawJakeExcited(QPainter &p, const QRectF &r);
    void drawJakeLab1(QPainter &p, const QRectF &r);
    void ensureMovie(const QString &path);
    void startBounceAnimation();
    void startSquashAnimation();
    
    // Простые easing функции без QEasingCurve
    qreal easeOutElastic(qreal t);
    qreal easeOutBounce(qreal t);
    QPointF calculateArmPosition(const QPointF& shoulder, const QPointF& target, qreal armLength);
};

#endif // JAKEWIDGET_XP_H
