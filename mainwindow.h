#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include "chatsever.h"
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

public slots:
    void logMessage(const QString &msg);

private slots:
    void on_startSever_clicked();

private:
    Ui::MainWindow *ui;
    Chatsever *m_chatServer;
};
#endif // MAINWINDOW_H
