#include "chatitemwidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clientmanager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->centralwidget->setEnabled(false);

    connect(ui->lnMessage, &QLineEdit::returnPressed, this, &MainWindow::on_btnSend_clicked);
    connect(ui->cmbClients, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_cmbClients_currentIndexChanged);


}

void MainWindow::on_cmbClients_currentIndexChanged(int index)
{
    if (index < 0) return;
    const int selectedId = ui->cmbClients->itemData(index).toInt();
    if (selectedId <= 0) return;

    _selectedRecipientId = selectedId;
    qDebug() << "[Recipient] Now chatting with ID:" << _selectedRecipientId;

    if (_chatTabs.contains(selectedId)) {
        ui->tabChats->setCurrentWidget(_chatTabs[selectedId]);
        return;
    }

    auto chatList = new QListWidget(this);
    _chatTabs[selectedId] = chatList;

    QString tabLabel = ui->cmbClients->itemText(index);
    ui->tabChats->addTab(chatList, tabLabel);

    const int status = _peerStatus.value(selectedId, 4);
    const int tabIndex = ui->tabChats->indexOf(chatList);
    ui->tabChats->setTabIcon(tabIndex, QIcon(iconForStatus(status)));

    ui->tabChats->setCurrentWidget(chatList);
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionConnect_triggered()
{
    _client = new ClientManager(QHostAddress::LocalHost, 4500, this);
    connect(_client, &ClientManager::connected, [this](){
        ui->centralwidget->setEnabled(true);
        _client->sendInit();
    });
    connect(_client, &ClientManager::disconnected, [this](){
        ui->centralwidget->setEnabled(false);
        ui->cmbClients->clear();

        while(ui->tabChats->count() > 0){
            QWidget* w = ui->tabChats->widget(0);
            ui->tabChats->removeTab(0);
            delete w;
        }

        _chatTabs.clear();
        _selectedRecipientId = -1;

        statusBar()->showMessage("Disconnected for server", 2000);
    });

    connect(_client, &ClientManager::clientListUpdated, this, [this](QList<QJsonObject> clients){
        const int myId = _client->clientId();
        const int prevSelectedId = _selectedRecipientId;

        std::sort(clients.begin(), clients.end(), [](const QJsonObject& a, const QJsonObject& b){
            return a.value("id").toInt() < b.value("id").toInt();
        });

        _peerStatus.clear();
        _peerNames.clear();

        for (const QJsonObject& c : clients) {
            const int id = c.value("id").toInt();
            if (id == 0 || id == myId) continue;
            _peerStatus[id] = c.value("status").toInt();
            _peerNames[id]  = c.value("name").toString();
        }

        ui->cmbClients->blockSignals(true);
        ui->cmbClients->clear();
        for (const QJsonObject& c : clients) {
            const int id = c.value("id").toInt();
            if (id == 0 || id == myId) continue;

            const QString name   = c.value("name").toString();
            const int     status = c.value("status").toInt();
            QString statusText   = statusTextFor(status);

            const QString label = QString("%1 [%2]").arg(name, statusText);
            QIcon icon(iconForStatus(status));
            ui->cmbClients->addItem(icon, label, id);
        }

        int idxToSelect = -1;
        if (prevSelectedId > 0) {
            for (int i = 0; i < ui->cmbClients->count(); ++i) {
                if (ui->cmbClients->itemData(i).toInt() == prevSelectedId) {
                    idxToSelect = i; break;
                }
            }
        }
        ui->cmbClients->setCurrentIndex(idxToSelect);
        _selectedRecipientId = (idxToSelect >= 0) ? ui->cmbClients->itemData(idxToSelect).toInt() : _selectedRecipientId;
        ui->cmbClients->blockSignals(false);

        for (auto it = _chatTabs.constBegin(); it != _chatTabs.constEnd(); ++it) {
            const int peerId = it.key();
            const int status = _peerStatus.value(peerId, 4);
            const int tabIndex = ui->tabChats->indexOf(it.value());
            if (tabIndex >= 0) {
                ui->tabChats->setTabIcon(tabIndex, QIcon(iconForStatus(status)));
                const QString name = _peerNames.value(peerId, QString("Client %1").arg(peerId));
                ui->tabChats->setTabText(tabIndex, QString("%1 [%2]").arg(name, statusTextFor(status)));
            }
        }
    });

    connect(_client, &ClientManager::isTyping, this, [this](const QString& who){
        statusBar()->showMessage(QString("%1 is typing...").arg(who), 750);
    });
    QTimer* typingTimer = new QTimer(this);
    typingTimer->setInterval(750);
    typingTimer->setSingleShot(true);

    connect(ui->lnMessage, &QLineEdit::textChanged, this, [this, typingTimer]() {
        if (!typingTimer->isActive() && _client) {
            if (!ui->lnMessage->text().trimmed().isEmpty()) {
                int recipientId = _selectedRecipientId;
                if(recipientId > 0){
                _client->sendIsTyping(recipientId);
                typingTimer->start();
                }
            }
        }
    });

    connect(_client, &ClientManager::directMessageReceived, this,
            [this](int senderId, QString senderName, QString message, int recipientId) {
                bool isMine = (senderId == _client->clientId());
                int tabId = isMine ? recipientId : senderId;

                if (!_chatTabs.contains(tabId)) {
                    auto chatList = new QListWidget(this);
                    _chatTabs[tabId] = chatList;

                    const QString nameForTab =
                        _peerNames.value(tabId,
                                         isMine ? QString("Client %1").arg(tabId) : senderName);
                    const int s = _peerStatus.value(tabId, 4);
                    const QString tabName = QString("%1 [%2]").arg(nameForTab, statusTextFor(s));

                    ui->tabChats->addTab(chatList, tabName);

                    const int tabIndex = ui->tabChats->indexOf(chatList);
                    ui->tabChats->setTabIcon(tabIndex, QIcon(iconForStatus(s)));
                }

                _selectedRecipientId = tabId;
                ui->tabChats->setCurrentWidget(_chatTabs[tabId]);

                auto messageList = _chatTabs[tabId];
                auto chatWidget = new ChatItemWidget(this);
                chatWidget->setMessage(message, isMine);

                auto listItem = new QListWidgetItem();
                listItem->setSizeHint(chatWidget->sizeHint());
                messageList->addItem(listItem);
                messageList->setItemWidget(listItem, chatWidget);
            }
            );




    _client->connectToServer();
}


void MainWindow::on_btnSend_clicked()
{
    QString message = ui->lnMessage->text().trimmed();
    if (message.isEmpty()) return;

    int recipientId = _selectedRecipientId;
    if(recipientId <= 0){
        qWarning() << "No valid recipient selected";
        return;
    }

    _client->sendMessage(message, recipientId);

    ui->lnMessage->clear();

}


void MainWindow::on_lnClientName_editingFinished()
{
    auto name= ui->lnClientName->text().trimmed();
    _client->sendName(name);
}


void MainWindow::on_cmbStatus_currentIndexChanged(int index)
{
    if (!_client) return;

    static const int statusForIndex[] = {4, 1, 2, 3};

    int sInt = 4;
    if (index >= 0 && index < int(sizeof(statusForIndex)/sizeof(statusForIndex[0])))
        sInt = statusForIndex[index];

    _client->sendStatus(static_cast<ClientManager::Status>(sInt));

}

QString MainWindow::iconForStatus(int s) const
{
    switch (s) {
    case 1: return ":/Icons/available.png";
    case 2: return ":/Icons/away.png";
    case 3: return ":/Icons/busy.png";
    default: return "";

    }
}

QString MainWindow::statusTextFor(int s) const
{
    switch (s) {
    case 1: return "Available";
    case 2: return "Away";
    case 3: return "Busy";
    default: return "Offline";
    }
}



