#ifndef SERVERWORKER_H
#define SERVERWORKER_H

#include <QObject>
#include<qtcpsocket.h>
class ServerWorker : public QObject
{
    Q_OBJECT
public:
    explicit ServerWorker(QObject *parent = nullptr);
    virtual bool setSocketDescriptor(qintptr socketDesciptor);
    QString userName();
    void setUserName(QString name);
signals:
    void logMessage(const QString &msg);  // 信号声明
    void jsonReceived(ServerWorker *sender,const QJsonObject &docObj);
    void disconnectedFromClient();

private:
    QTcpSocket *m_serverSocket;
    QString m_userName;
    bool admin;

public slots:
    void onReadyRead();
    void extracted(QDataStream &serverStream, QJsonObject &message);
    void sendMessage(const QString &text, const QString &type = "message");
    void sendJson(const QJsonObject &json);
    void setAdmin(bool isAdmin);
    bool isAdmin() const;
    void disconnectFromClient();
};

#endif // SERVERWORKER_H
