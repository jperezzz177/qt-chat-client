#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QListWidget>
#include <QMainWindow>
#include "clientmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void on_cmbClients_currentIndexChanged(int index);

private slots:
    void on_actionConnect_triggered();

    void on_btnSend_clicked();

    void on_lnClientName_editingFinished();

    void on_cmbStatus_currentIndexChanged(int index);


private:
    Ui::MainWindow *ui;
    ClientManager *_client;
    QMap<int, QListWidget*> _chatTabs;
    int _selectedRecipientId = -1;
    QHash<int, int> _peerStatus;
    QHash<int, QString> _peerNames;
    QString iconForStatus(int s) const;
    QString statusTextFor(int s) const;

};
#endif // MAINWINDOW_H
