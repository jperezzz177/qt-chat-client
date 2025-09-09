#include "clientmanager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

ClientManager::ClientManager(QHostAddress ip, ushort port, QObject *parent)
    : QObject{parent},
    _ip(ip),
    _port(port)
{
    _socket = new QTcpSocket(this);
    connect(_socket, &QTcpSocket::connected, this, &ClientManager::connected);

    connect(_socket, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << " Client disconnected (from client side)";
        emit disconnected();
    });


    connect(_socket, &QTcpSocket::readyRead, this, &ClientManager::readyRead);
    connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            [](QAbstractSocket::SocketError error){
                qDebug() << "[ClientManager] Socket error occurred:" << error;
            });

}

ClientManager::~ClientManager()
{
    qDebug() << "[ClientManager] Destructor called!";
}

void ClientManager::connectToServer()
{
    _socket->connectToHost(_ip, _port);
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
}

void ClientManager::sendInit()
{
    QJsonObject obj;
    obj["type"] = "init";
    obj["os"] = "macOS";
    obj["version"] = "1.0.0";

    QJsonDocument doc(obj);
    _socket->write(doc.toJson(QJsonDocument::Compact)+ "\n");
}

void ClientManager::sendLogin(QString username, QString password)
{
    QJsonObject obj;
    obj["type"]="login";
    obj["username"] = username;
    obj["password"] = password;

    QJsonDocument doc(obj);
    _socket->write(doc.toJson(QJsonDocument::Compact)+ "\n");
}

void ClientManager::sendMessage(QString message, int recipientId)
{

    QJsonObject obj{
        {"type", "message"},
        {"content", message},
        {"recipient", recipientId}
    };

    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
    qDebug() << "[Raw Outgoing JSON]" << payload;
    _socket->write(payload);


}

int ClientManager::clientId() const
{
    return _socket->property("id").toInt();
}

void ClientManager::sendName(QString name)
{
    QJsonObject obj;
    obj["type"] = "set_name";
    obj["name"] = name;

    QJsonDocument doc(obj);
    _socket->write(doc.toJson(QJsonDocument::Compact)+ "\n");
}

void ClientManager::sendStatus(ClientManager::Status status)
{
    QJsonObject obj;
    obj["type"] = "set_status";
    obj["status"] = static_cast<int>(status);

    QJsonDocument doc(obj);
    _socket->write(doc.toJson(QJsonDocument::Compact)+ "\n");
}

void ClientManager::sendIsTyping(int recipientId)
{
    QJsonObject obj;
    obj["type"] = "is_typing";
    obj["recipient"] = recipientId;
    _socket->write(QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n');

}

void ClientManager::readyRead()
{
    while (_socket->canReadLine()) {
        QByteArray line = _socket->readLine();
        if (line.size() > 65536) {
            qWarning() << "[Client] Dropping oversized line of size" << line.size();
            continue;
        }

        line = line.trimmed();
        if (line.isEmpty()) continue;

        qDebug() << "[Client] Received line:" << line;

        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "[Client] Invalid JSON:" << parseError.errorString();
            continue;
        }

        const QJsonObject obj = doc.object();
        const QString type = obj.value("type").toString();

        if (type == "assign_id") {
            const int assignedId = obj.value("id").toInt();
            _socket->setProperty("id", assignedId);
            qDebug() << "[ClientManager] Assigned ID from server:" << assignedId;
        } else if (type == "init_ack" && obj.value("status").toString() == "ok") {
            sendLogin("test", "test");
        } else if (type == "login_ack") {
            const QString status = obj.value("status").toString();
            if (status == "ok") qDebug() << "[Client] Login successful!";
            else qDebug() << "[Client] Login failed:" << obj.value("reason").toString();
        } else if (type == "message") {
            emit directMessageReceived(obj.value("sender_id").toInt(),
                                       obj.value("sender").toString(),
                                       obj.value("content").toString(),
                                       obj.value("recipient").toInt());
        } else if (type == "is_typing") {
            emit isTyping(obj.value("sender").toString());
        } else if (type == "set_name") {
            emit nameChanged(obj.value("name").toString());
        } else if (type == "set_status") {
            emit statusChanged(static_cast<Status>(obj.value("status").toInt()));
        } else if (type == "client_list") {
            QList<QJsonObject> clients;
            for (const auto &v : obj.value("clients").toArray())
                clients.append(v.toObject());
            emit clientListUpdated(clients);
        } else {
            qDebug() << "[Client] Unknown message type:" << type;
        }
    }

}

QTcpSocket* ClientManager::socket() const
{
    return _socket;
}

