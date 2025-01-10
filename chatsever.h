#ifndef CHATSEVER_H
#define CHATSEVER_H

#include "serverworker.h"
#include <QObject>
#include <QTcpServer>
#include <QQueue>
class Chatsever : public QTcpServer
{
    Q_OBJECT
public:
    explicit Chatsever(QObject *parent = nullptr);
    QQueue<QString> messageHistory;  // 用于保存历史消息
    const int maxHistorySize = 100; // 设置最大历史消息数量

signals:
    void logMessage(const QString &msg);  // 信号声明

protected:
    void incomingConnection(qintptr socketDescripotor) override;
    QVector<ServerWorker*>m_clients;
    void broadcast(const QJsonObject &message, ServerWorker *exclude);

private:

public slots:
    void stopServer();
    void jsonReceived(ServerWorker *sender,const QJsonObject &docObj);
    void userDisconnected(ServerWorker *sender);
    void startServer();
    void saveMessageToMemory(const QString &sender, const QString &receiver, const QString &content);
    QList<QString> getHistory();
    void handleHistoryRequest(ServerWorker *client);
    void processKickRequest(ServerWorker *sender, const QJsonObject &docObj);

};

#endif // CHATSEVER_H
