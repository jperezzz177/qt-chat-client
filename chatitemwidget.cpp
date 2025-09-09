#include "chatitemwidget.h"
#include "ui_chatitemwidget.h"

ChatItemWidget::ChatItemWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatItemWidget)
{
    ui->setupUi(this);
    ui->lblMessage->setWordWrap(true);
    ui->lblMessage->setTextInteractionFlags(Qt::TextSelectableByMouse);

}

ChatItemWidget::~ChatItemWidget()
{
    delete ui;
}

void ChatItemWidget::setMessage(QString message, bool isMyMesssage)
{
    if(message.isEmpty()){
        message = "[Empty message]";
    }
    ui->lblMessage->setText(message);
    ui->lblTime->setText(QDateTime::currentDateTime().toString("HH:mm"));

    if(isMyMesssage){
        ui->lblMessage->setAlignment(Qt::AlignRight);
        ui->lblMessage->setStyleSheet("background-color: #3874f2; color: white; padding: 5px; border-radius: 5px;");
        ui->lblTime->setAlignment(Qt::AlignRight);
    }else{
        ui->lblMessage->setAlignment(Qt::AlignLeft);
        ui->lblMessage->setStyleSheet("background-color: green; padding: 5px; border-radius: 5px;");
        ui->lblTime->setAlignment(Qt::AlignLeft);
    }

    qDebug() << "[Client UI] Safely set message" << message;

}
