#ifndef JAKEWIDGET_H
#define JAKEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPoint>
#include <QCursor>

class QLabel;
class QMovie;

class JakeWidget : public QWidget
{
    Q_OBJECT
public:
    enum class JakeState { Idle, Hover, Click, FollowMouse, Lab1 };

    explicit JakeWidget(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override { return QSize(300, 260); }
    QSize sizeHint() const override { return QSize(360, 300); }

public slots:
    void setState(JakeState state);
    void followMouse();
    void onButtonHover();
    void onButtonClick();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onTick();

private:
    JakeState currentState;
    QTimer animationTimer;
    QTimer mouseFollowTimer;
    int frame;
    qreal t;
    QPoint targetPosition;
    QPoint currentPosition;
    qreal smoothFactor;

    // Movie-based animation for non-Lab1 states
    QLabel *movieLabel;
    QMovie *movie;
    void ensureMovie(const QString &path);

    // Helpers for drawing
    void drawJakeLab1(QPainter &p, const QRectF &r);
    void drawJakeFollowMouse(QPainter &p, const QRectF &r);
    QPointF calculateArmPosition(const QPointF& shoulder, const QPointF& target, qreal armLength);
};

#endif // JAKEWIDGET_H