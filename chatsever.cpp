#include "chatsever.h"
#include "clienttask.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "serverworker.h"
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QThreadPool>

Chatsever::Chatsever(QObject *parent):
    QTcpServer(parent)
{

}
void Chatsever::incomingConnection(qintptr socketDescripotor){

    ServerWorker *worker =new ServerWorker(this);
    if(!worker->setSocketDescriptor(socketDescripotor)){
        worker->deleteLater();
        return;
    }
    connect(worker,&ServerWorker::logMessage,this,&Chatsever::logMessage);
    connect(worker,&ServerWorker::jsonReceived,this,&Chatsever::jsonReceived);
    connect(worker,&ServerWorker::disconnectedFromClient,this,
            std::bind(&Chatsever::userDisconnected,this,worker));
    m_clients.append(worker);
    emit logMessage("新用户链接上了");

}
void Chatsever::stopServer(){
    close();
}
void Chatsever::jsonReceived(ServerWorker *sender,const QJsonObject &docObj){
    qDebug() << "Received JSON:" << QJsonDocument(docObj).toJson(QJsonDocument::Compact); // 调试输出
    const QJsonValue typeVal= docObj.value("type");
    if(typeVal.isNull()||!typeVal.isString())
        return;
    if (typeVal.toString().compare("message", Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = docObj.value("text");
        if (textVal.isNull() || !textVal.isString())
            return;
        const QString text = textVal.toString().trimmed();
        if (text.isEmpty())
            return;

        QJsonObject message;
        message["type"] = "message";
        message["text"] = text;
        message["sender"] = sender->userName();

        broadcast(message, nullptr);
    }else if (typeVal.toString().compare("login", Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = docObj.value("text");
        if (usernameVal.isNull() || !usernameVal.isString())
            return;

        QString username = usernameVal.toString();
        sender->setUserName(username);

        // 判断是否为管理员登录
        bool isAdmin = (username == "admin");

        // 广播新用户加入（管理员登录可以选择不广播给普通用户）
        if (!isAdmin) {
            QJsonObject connectedMessage;
            connectedMessage["type"] = "newuser";
            connectedMessage["username"] = username;
            broadcast(connectedMessage, sender);
        }

        // 发送当前用户列表给新用户（包括管理员）
        QJsonObject userListMessage;
        userListMessage["type"] = "userlist";
        QJsonArray userListArray;
        for (ServerWorker *worker : m_clients) {
            if (!worker->userName().isEmpty()) {
                userListArray.append(worker->userName());
            }
        }
        userListMessage["userlist"] = userListArray;
        sender->sendJson(userListMessage);

        qDebug() << (isAdmin ? "Admin" : "User") << "logged in:" << username;
    }
    else if (typeVal.toString().compare("userlist_request", Qt::CaseInsensitive) == 0) {
        // 构建在线用户列表消息
        QJsonObject userListMessage;
        userListMessage["type"] = "userlist";
        QJsonArray userListArray;
        for (ServerWorker *worker : m_clients) {
            if (!worker->userName().isEmpty()) {
                userListArray.append(worker->userName());
            }
        }
        userListMessage["userlist"] = userListArray;

        // 仅发送给请求的客户端（管理员）
        sender->sendJson(userListMessage);
    }

    else if (typeVal.toString().compare("history_request", Qt::CaseInsensitive) == 0) {
        // 调用处理历史信息的函数
        handleHistoryRequest(sender);
    }
    else if (typeVal.toString().compare("private_message", Qt::CaseInsensitive) == 0) {
        const QJsonValue receiverVal = docObj.value("receiver");
        const QJsonValue textVal = docObj.value("text");

        if (!receiverVal.isString() || !textVal.isString())
            return;

        QString receiver = receiverVal.toString();
        QString text = textVal.toString();

        // 查找接收者
        for (ServerWorker *worker : m_clients) {
            if (worker->userName() == receiver) {
                QJsonObject message;
                message["type"] = "private_message";
                message["sender"] = sender->userName();
                message["text"] = text;

                worker->sendJson(message); // 将私聊消息发送给接收者
                break;
            }
        }
    }
    else if (typeVal.toString().compare("kick", Qt::CaseInsensitive) == 0) {
        // 调用处理踢人的函数
        processKickRequest(sender, docObj);
    }
}
void Chatsever::broadcast(const QJsonObject &message, ServerWorker *exclude) {
    QString sender = message.value("sender").toString();  // 从 JSON 消息中获取发送者
    QString content = message.value("text").toString();  // 从 JSON 消息中获取消息内容
    saveMessageToMemory(sender, QString(), content);
    for (ServerWorker *worker : m_clients) {
        if (worker != exclude) {
            worker->sendJson(message);
        }
    }
}
void Chatsever::userDisconnected(ServerWorker *sender) {
    m_clients.removeAll(sender);
    const QString userName = sender->userName();
    if (!userName.isEmpty()) {
        QJsonObject disconnectedMessage;
        disconnectedMessage["type"] = "userdisconnected";
        disconnectedMessage["username"] = userName;
        broadcast(disconnectedMessage, nullptr);
        emit logMessage(userName + " disconnected");
    }
    sender->deleteLater();
}
void Chatsever::startServer() {
    QThreadPool::globalInstance()->setMaxThreadCount(10); // 设置最大线程数
}
void Chatsever::saveMessageToMemory(const QString &sender, const QString &receiver, const QString &content) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString formattedMessage = QString("[%1] %2 -> %3: %4")
                                   .arg(timestamp)
                                   .arg(sender)
                                   .arg(receiver.isEmpty() ? "ALL" : receiver)
                                   .arg(content);

    messageHistory.enqueue(formattedMessage);
    qDebug() << "Message saved to history:" << formattedMessage; // 调试输出
    // 如果历史记录超过最大限制，删除最早的记录
    if (messageHistory.size() > maxHistorySize) {
        messageHistory.dequeue();
    }
}
QList<QString> Chatsever::getHistory()  {
    return messageHistory.toList();
}
void Chatsever::handleHistoryRequest(ServerWorker *requestingClient) {
    QList<QString> history = getHistory();

    QJsonArray historyArray;
    for (const QString &entry : history) {
        historyArray.append(entry);
    }

    QJsonObject response;
    response["type"] = "history";
    response["data"] = historyArray;
    qDebug() << "Sending history to client:" << QJsonDocument(response).toJson(QJsonDocument::Compact); // 调试输出
    // 使用 ServerWorker 的 sendJson 发送消息
    requestingClient->sendJson(response);
}
void Chatsever::processKickRequest(ServerWorker *sender, const QJsonObject &docObj) {
    const QJsonValue usernameVal = docObj.value("username");
    if (usernameVal.isNull() || !usernameVal.isString()) {
        qDebug() << "Invalid kick request: missing or invalid username.";
        return;
    }

    QString usernameToKick = usernameVal.toString();
    qDebug() << "Kick request received for user:" << usernameToKick;

    // 查找目标用户
    ServerWorker *workerToKick = nullptr;
    for (ServerWorker *worker : m_clients) {
        if (worker->userName() == usernameToKick) {
            workerToKick = worker;
            break;
        }
    }

    if (!workerToKick) {
        qDebug() << "User not found:" << usernameToKick;
        return; // 没有找到用户
    }

    // 调用断开连接方法
    workerToKick->disconnectFromClient();

    // 从客户端列表中移除
    m_clients.removeAll(workerToKick);

    // 通知其他用户该用户被踢出
    QJsonObject userLeftMessage;
    userLeftMessage["type"] = "user_left";
    userLeftMessage["username"] = usernameToKick;
    broadcast(userLeftMessage, nullptr);

    // 释放资源
    workerToKick->deleteLater();

    // 向管理员反馈踢人操作成功
    QJsonObject kickResponse;
    kickResponse["type"] = "kick_response";
    kickResponse["message"] = QString("User %1 has been kicked out.").arg(usernameToKick);
    sender->sendJson(kickResponse);

    qDebug() << "User kicked successfully:" << usernameToKick;
}
