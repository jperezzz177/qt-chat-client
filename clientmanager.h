#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QHostAddress>


class ClientManager : public QObject
{
    Q_OBJECT
public:
    explicit ClientManager(QHostAddress ip= QHostAddress::LocalHost, ushort port = 4500, QObject *parent = nullptr);
    ~ClientManager();
    void connectToServer();
    void sendInit();
    void sendLogin(QString username, QString password);
    void sendMessage(QString message, int recipientId);
    int clientId() const;

    void sendName(QString name);
    enum class Status {
        Available = 1,
        Away = 2,
        Busy = 3,
        Offline = 4
    };

    void sendStatus(Status status);
    QTcpSocket* socket() const;

public:
    void sendIsTyping(int recipientId);

signals:
    void connected();
    void disconnected();
    void isTyping(QString from);
    void nameChanged(QString name);
    void statusChanged(Status status);
    void clientListUpdated(QList<QJsonObject> clients);
    void directMessageReceived(int senderId, QString senderName, QString message, int recipientId);



private slots:
    void readyRead();

private:
    QTcpSocket *_socket;
    QHostAddress _ip;
    ushort _port;

};

#endif // CLIENTMANAGER_H
