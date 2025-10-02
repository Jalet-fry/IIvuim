#ifndef JAKEWIDGET_H
#define JAKEWIDGET_H

#include <QWidget>
#include <QTimer>
class QLabel;
class QMovie;

class JakeWidget : public QWidget
{
    Q_OBJECT
public:
    enum class JakeState { Idle, Hover, Click, Lab1 };

    explicit JakeWidget(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override { return QSize(300, 260); }
    QSize sizeHint() const override { return QSize(360, 300); }

public slots:
    void setState(JakeState state);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTick();

private:
    JakeState currentState;
    QTimer animationTimer;
    int frame;
    qreal t;

    // Movie-based animation for non-Lab1 states
    QLabel *movieLabel;
    QMovie *movie;
    void ensureMovie(const QString &path);

    // Helpers for drawing
    void drawJakeLab1(QPainter &p, const QRectF &r);
};

#endif // JAKEWIDGET_H


