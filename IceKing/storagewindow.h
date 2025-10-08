#ifndef STORAGEWINDOW_H
#define STORAGEWINDOW_H

#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QTextStream>

class QTextEdit;
class QPushButton;

class StorageWindow : public QWidget
{
    Q_OBJECT

public:
    explicit StorageWindow(const QPixmap &background, QWidget *parent = 0);

private slots:
    void goBack();  // ????????? ?????? ???? ????

private:
    void setupUI();
    void getATAStorageInfo();
    QString getDeviceInfo(int channel, int device);
    void printToConsole(const QString &text);
    QString getManufacturerFromATA();
    QString getDiskUsageInfo();
    QString getDriveTypeFromATA();

    QTextEdit *textEdit;
    QPixmap m_background;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // STORAGEWINDOW_H
