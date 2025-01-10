#ifndef CLIENTTASK_H
#define CLIENTTASK_H

#include <QObject>
#include <QRunnable>
#include <QTcpSocket>
class ClientTask : public QObject
{
    Q_OBJECT
public:
    explicit ClientTask(QObject *parent = nullptr);

public:
    void run();

private:
    QTcpSocket *m_socket;

    QJsonObject parseMessage(const QByteArray &data);

    void handleMessage(const QJsonObject &message);
signals:
};

#endif // CLIENTTASK_H
