#ifndef JAKEWIDGET_H
#define JAKEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPoint>
#include <QCursor>
#include <QPropertyAnimation>

class QLabel;
class QMovie;

class JakeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)
    
public:
    enum JakeState { Idle, Hover, Click, FollowMouse, Lab1, Excited, Squash, ButtonHover };

    explicit JakeWidget(QWidget *parent = nullptr);
    QSize minimumSizeHint() const { return QSize(300, 260); }
    QSize sizeHint() const { return QSize(360, 300); }

    qreal scale() const { return m_scale; }
    void setScale(qreal s) { m_scale = s; update(); }
    qreal rotation() const { return m_rotation; }
    void setRotation(qreal r) { m_rotation = r; update(); }

public slots:
    void setState(JakeState state);
    void followMouse();
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
    void onMovieFrameChanged();

private:
    JakeState currentState;
    QTimer animationTimer;
    QTimer mouseFollowTimer;
    QTimer hoverTimer;
    int frame;
    qreal t;
    QPointF targetPosition;
    QPointF currentPosition;
    QPointF velocity;
    qreal smoothFactor;
    qreal m_scale;
    qreal m_rotation;

    // Physics parameters for better motion
    qreal damping;
    qreal acceleration;
    
    // Animation properties
    qreal bouncePhase;
    qreal squashAmount;
    qreal stretchAmount;
    
    // Movie-based animation
    QLabel *movieLabel;
    QMovie *movie;
    void ensureMovie(const QString &path);
    
    // Property animations for smooth transitions
    QPropertyAnimation *scaleAnimation;
    QPropertyAnimation *rotationAnimation;
    
    // Enhanced animation helpers
    void startBounceAnimation();
    void startSquashAnimation();
    void updatePhysics();
    qreal easeOutElastic(qreal t);
    qreal easeOutBounce(qreal t);

    // Helpers for drawing
    QPointF calculateArmPosition(const QPointF& shoulder, const QPointF& target, qreal armLength);
    void drawJakeLab1(QPainter &p, const QRectF &r);
    void drawJakeExcited(QPainter &p, const QRectF &r);
    void drawJakeFollowMouse(QPainter &p, const QRectF &r);
};

#endif // JAKEWIDGET_H