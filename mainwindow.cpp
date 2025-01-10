#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_chatServer= new Chatsever(this);

    connect(m_chatServer,&Chatsever::logMessage,this,&MainWindow::logMessage);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startSever_clicked()
{
    if(m_chatServer->isListening()){
        m_chatServer->stopServer();
        ui->startSever->setText("启动服务器");
        logMessage("服务器已停止");
    }else{
        if(!m_chatServer->listen(QHostAddress::Any,1967)){
            QMessageBox::critical(this,"错误","无法启动服务器");
            return;
        }
        logMessage("服务器已启动");
        ui->startSever->setText("停止服务器");
    }

}
void MainWindow::logMessage(const QString &msg){
    ui->logEdit->appendPlainText(msg);
}

