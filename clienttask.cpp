#include "clienttask.h"
#include "qjsondocument.h"
#include "qjsonobject.h"

ClientTask::ClientTask(QObject *parent)
    : QObject{parent}
{}
void ClientTask::run()  {
}
QJsonObject ClientTask::parseMessage(const QByteArray &data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    return doc.object();
}
