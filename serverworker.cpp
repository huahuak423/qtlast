#include "serverworker.h"
#include <QDataStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
ServerWorker::ServerWorker(QObject *parent)
    : QObject(parent)
{
    m_serverSocket= new QTcpSocket(this);
    connect(m_serverSocket,&QTcpSocket::readyRead,this,&ServerWorker::onReadyRead);
    connect(m_serverSocket,&QTcpSocket::disconnected,this,&ServerWorker::disconnectedFromClient);
    admin=false;
}
bool ServerWorker::setSocketDescriptor(qintptr socketDesciptor){

    return m_serverSocket->setSocketDescriptor(socketDesciptor);
}
void ServerWorker::onReadyRead(){
    QByteArray jsonDate;
    QDataStream socketStream(m_serverSocket);
    socketStream.setVersion(QDataStream::Qt_6_7);
    for(;;){
        socketStream.startTransaction();
        socketStream>>jsonDate;
        if(socketStream.commitTransaction()){
 //           emit logMessage(QString::fromUtf8(jsonDate));
 //           sendMessage("I recieved message");


            QJsonParseError parseError;
            const QJsonDocument jsonDoc =QJsonDocument::fromJson(jsonDate,&parseError);
            if(parseError.error==QJsonParseError::NoError){
                if(jsonDoc.isObject()){
                    emit logMessage(QJsonDocument(jsonDoc).toJson(QJsonDocument::Compact));
                    emit jsonReceived(this,jsonDoc.object());
                }
            }
        }else{
            break;
        }
    }
}

void ServerWorker::sendMessage(const QString &text,const QString &type){

    if(m_serverSocket->state()!=QAbstractSocket::ConnectedState)
        return;


    if(!text.isEmpty()){
        QDataStream serverStream(m_serverSocket);
        serverStream.setVersion(QDataStream::Qt_6_7);

        QJsonObject message;
        message["type"]=type;
        message["text"]=text;

        serverStream<<QJsonDocument(message).toJson();
    }


}
void ServerWorker::extracted(QDataStream &serverStream, QJsonObject &message)
{
    // 提供一个空实现，避免链接错误
    // 你可以在这里处理数据流和 JSON 对象的逻辑
    qDebug() << "extracted method called";
}
QString ServerWorker::userName(){
    return m_userName;
}
void ServerWorker::setUserName(QString name){
    m_userName=name;
}
void ServerWorker::sendJson(const QJsonObject &json)
{
    const QByteArray jsonData =QJsonDocument(json).toJson(QJsonDocument::Compact);
    emit logMessage(QLatin1String("Sending to ")+userName()+QLatin1String(" - ")+
                    QString::fromUtf8(jsonData));
    QDataStream socketStream(m_serverSocket);
    socketStream.setVersion(QDataStream::Qt_6_7);
    socketStream<<jsonData;
}
void ServerWorker::setAdmin(bool isAdmin) {
    admin = isAdmin;
}

bool ServerWorker::isAdmin() const {
    return admin;
}
void ServerWorker::disconnectFromClient()
{
    if (m_serverSocket) { // 检查套接字是否有效
        m_serverSocket->disconnectFromHost(); // 请求断开连接
        if (m_serverSocket->state() != QAbstractSocket::UnconnectedState) {
            // 如果未立即断开，强制关闭
            m_serverSocket->close();
        }
        qDebug() << "Disconnected client:" << userName(); // 调试输出断开的用户
    }
}
