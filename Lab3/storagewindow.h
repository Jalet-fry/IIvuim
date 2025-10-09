#ifndef STORAGEWINDOW_H
#define STORAGEWINDOW_H

#include <QWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include "storagescanner.h"

class StorageWindow : public QWidget {
    Q_OBJECT

public:
    explicit StorageWindow(QWidget *parent = nullptr);
    ~StorageWindow();

private slots:
    void scanDevices();
    void onTableItemClicked(int row, int column);
    void refreshDevices();

private:
    void setupUI();
    void populateTable(const std::vector<StorageDevice>& disks);
    QString formatSize(uint64_t bytes);

    QTableWidget *m_diskTable;
    QTextEdit *m_detailsText;
    QLabel *m_statusLabel;
    StorageScanner *m_scanner;
    std::vector<StorageDevice> m_disks;
};

#endif // STORAGEWINDOW_H

