#include "widget.h"
#include "ui_widget.h"
#include <QHostInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QNetworkInterface>
#include <QProcess>
#include <QTableView>
#include <QColor>
#include <QColorDialog>
#include <QFileDialog>
#include <QKeyEvent>

Widget::Widget(QWidget *parent,QString inputName) :
    QWidget(parent),
    ui(new Ui::Widget),inputName(inputName)
{
    ui->setupUi(this);
    udpSocket = new QUdpSocket(this);
    port = 12345;

    bb=0;
    privatechat=NULL;
    privatechat1=NULL;

    udpSocket->bind(port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
    sendMessage(NewParticipant);

    server = new TcpServer(this);
    connect(server, SIGNAL(sendFileName(QString)), this, SLOT(getFileName(QString)));
    connect(ui->messageTextEdit, SIGNAL(currentCharFormatChanged(QTextCharFormat)), this,
            SLOT(currentFormatChanged(QTextCharFormat)));

    ui->messageTextEdit->installEventFilter(this);
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::eventFilter(QObject *target, QEvent *event) {
    if(target == ui->messageTextEdit) {
        if(event->type() == QEvent::KeyPress) {
             QKeyEvent *k = static_cast<QKeyEvent *>(event);
             if(k->key() == Qt::Key_Return) {
                 sendMessage(Message);
                 return true;
             }
        }
    }
    return QWidget::eventFilter(target,event);
}

void Widget::sendMessage(MessageType type, QString serverAddress)
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    QString localHostName = QHostInfo::localHostName();
    QString address = getIP();

    out << type << getUserName() << localHostName;

    switch(type)
    {
    case Message:
        if(ui->messageTextEdit->toPlainText() == "")
        {
            QMessageBox::warning(0, tr("警告"), tr("发送内容不能为空"), QMessageBox::Ok);
            return;
        }

        out  << address << getMessage();
        ui->messageBrowser->verticalScrollBar()->setValue(ui->messageBrowser->verticalScrollBar()->maximum());
        break;

    case NewParticipant:
        out << address;
        break;

    case ParticipantLeft:
        break;

    case FileName:
    {
        int row = ui->userTableWidget->currentRow();
        QString clientAddress = ui->userTableWidget->item(row, 2)->text();
        out << address << clientAddress << fileName;
        break;
    }

    case Refuse:
        out << serverAddress;
        break;

    }

    udpSocket->writeDatagram(data, data.length(), QHostAddress::Broadcast, port);

}

void Widget::processPendingDatagrams()
{
    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());

        udpSocket->readDatagram(datagram.data(), datagram.size());

        QDataStream in(&datagram, QIODevice::ReadOnly);
        int messageType;
        in >> messageType;
        QString userName, localHostName, ipAddress, message;
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        switch(messageType)
        {
        case Message:
            in >> userName >> localHostName >> ipAddress >> message;
            if (localHostName==QHostInfo::localHostName()){
                ui->messageBrowser->setTextColor(QColor(46,128,0,255));
            }
            else {
                ui->messageBrowser->setTextColor(Qt::blue);
            }
            ui->messageBrowser->setCurrentFont(QFont("宋体", 9));
            ui->messageBrowser->append("");
            ui->messageBrowser->append("[" + userName + "]" + time);
            ui->messageBrowser->append(message);
            break;

        case NewParticipant:
            in >> userName >> localHostName >> ipAddress;
            newParticipant(userName, localHostName, ipAddress);
            break;

        case ParticipantLeft:
            in >> userName >> localHostName;
            participantLeft(localHostName);
            break;

        case FileName:
        {
            in >> userName >> localHostName >> ipAddress;
            QString clientAddress, fileName;
            in >> clientAddress >> fileName;
            hasPendingFile(userName, ipAddress, clientAddress, fileName);
            break;
        }

        case Refuse:
        {
            in >> userName >> localHostName;
            QString serverAddress;
            in >> serverAddress;
            QString ipAddress = getIP();
            if(ipAddress == serverAddress)
            {
                server->refused();
            }
            break;
        }

        case Xchat:
            {
                in >>userName >>localHostName >>ipAddress;
                showxchat(userName,ipAddress);
                break;
            }

        }
    }
}

void Widget::newParticipant(QString userName, QString localHostName, QString ipAddress)
{
    bool isEmpty = ui->userTableWidget->findItems(localHostName, Qt::MatchExactly).isEmpty();

    if(isEmpty)
    {
        QTableWidgetItem* user = new QTableWidgetItem(userName);
        QTableWidgetItem* host = new QTableWidgetItem(localHostName);
        QTableWidgetItem* ip = new QTableWidgetItem(ipAddress);

        ui->userTableWidget->insertRow(0);
        ui->userTableWidget->setItem(0, 0, user);
        ui->userTableWidget->setItem(0, 1, host);
        ui->userTableWidget->setItem(0, 2, ip);

        sendMessage(NewParticipant);
    }
}



void Widget::participantLeft(QString localHostName)
{
    int rowNum = ui->userTableWidget->findItems(localHostName, Qt::MatchExactly).first()->row();
    ui->userTableWidget->removeRow(rowNum);
}


QString Widget::getIP()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach(QHostAddress address, list)
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            return address.toString();
        }
    }
    return 0;
}

QString Widget::getUserName()
{
    return inputName;
}

QString Widget::getMessage()
{
    QString msg = ui->messageTextEdit->toHtml();
    ui->messageTextEdit->clear();
    ui->messageTextEdit->setFocus();
    return msg;
}


void Widget::on_sendButton_clicked()
{
    sendMessage(Message);
}

void Widget::getFileName(QString name)
{
    fileName = name;
    sendMessage(FileName);
}


void Widget::on_sendToolBtn_clicked()
{
    if(ui->userTableWidget->selectedItems().isEmpty())
    {
        QMessageBox::warning(0, tr("选择接收者"), tr("请先从左侧用户列表选择要接收者!"), QMessageBox::Ok);
        return;
    }
    server->show();
    server->initServer();
}



void Widget::hasPendingFile(QString userName, QString serverAddress, QString clientAddress, QString fileName)
{
    QString ipAddress = getIP();
    if(ipAddress == clientAddress)
    {
        int btn = QMessageBox::information(this, tr("接收文件"), tr("来自 %1 的文件: %2, 是否接收?")
                                           .arg(userName).arg(fileName),
                                           QMessageBox::Yes, QMessageBox::No);
        if(btn == QMessageBox::Yes)
        {
            QString name = QFileDialog::getSaveFileName(0, tr("保存文件"), fileName);
            if(!name.isEmpty())
            {
                TcpClient *client = new TcpClient(this);
                client->setFileName(name);
                client->setHostAddress(QHostAddress(serverAddress));
                client->show();
            }
        }
        else
        {
            sendMessage(Refuse, serverAddress);
        }
    }
}



void Widget::on_fontComboBox_currentFontChanged(const QFont &f)
{
    ui->messageTextEdit->setCurrentFont(f);
    ui->messageTextEdit->setFocus();
}

void Widget::on_sizeComboBox_currentIndexChanged(const QString &arg1)
{
    ui->messageTextEdit->setFontPointSize(arg1.toDouble());
    ui->messageTextEdit->setFocus();
}

void Widget::on_boldToolBtn_clicked(bool checked)
{
    if(checked)
    {
        ui->messageTextEdit->setFontWeight(QFont::Bold);
    }
    else
    {
        ui->messageTextEdit->setFontWeight(QFont::Normal);
    }
    ui->messageTextEdit->setFocus();
}


void Widget::on_italicToolBtn_clicked(bool checked)
{
    ui->messageTextEdit->setFontItalic(checked);
    ui->messageTextEdit->setFocus();
}


void Widget::on_underlineToolBtn_clicked(bool checked)
{
    ui->messageTextEdit->setFontUnderline(checked);
    ui->messageTextEdit->setFocus();
}

void Widget::on_colorToolBtn_clicked()
{
    color = QColorDialog::getColor(color, this);
    if(color.isValid())
    {
        ui->messageTextEdit->setTextColor(color);
        ui->messageTextEdit->setFocus();
    }
}

void Widget::currentFormatChanged(const QTextCharFormat &format)
{
    ui->fontComboBox->setCurrentFont(format.font());
    if(format.fontPointSize() < 9)
    {
        ui->sizeComboBox->setCurrentIndex(3);
    }
    else
    {
        ui->sizeComboBox->setCurrentIndex(ui->sizeComboBox->findText(QString::number(format.fontPointSize())));
    }
    ui->boldToolBtn->setChecked(format.font().bold());
    ui->italicToolBtn->setChecked(format.font().italic());
    ui->underlineToolBtn->setChecked(format.font().underline());
    color = format.foreground().color();
}

void Widget::on_clearToolBtn_clicked()
{
    ui->messageBrowser->clear();
}

void Widget::on_exitButton_clicked()
{
    close();
}

void Widget::closeEvent(QCloseEvent *e)
{
    sendMessage(ParticipantLeft);
    QWidget::closeEvent(e);
}

void Widget::on_saveToolBtn_clicked()
{
    if(ui->messageBrowser->document()->isEmpty()){
        QMessageBox::warning(0,tr("Error！"),tr("聊天记录？不存在的！"),QMessageBox::Ok);
    }
    else {
        QString fileName=QFileDialog::getSaveFileName(this,tr("保存聊天记录"),tr("聊天记录"),tr("文本(*.txt);;All File(*.*"));
        if(!fileName.isEmpty()){
            saveFile(fileName);
        }
    }
}

bool Widget::saveFile(const QString &fileName){
    QFile file(fileName);
    if(!file.open(QFile::WriteOnly|QFile::Text)) {
        QMessageBox::warning(this,tr("保存文件"),
                             tr("无法保存文件 %1:\n %2").arg(fileName)
                             .arg(file.errorString()));
    }
    QTextStream out(&file);
    out << ui->messageBrowser->toPlainText();
    return true;
}

void Widget::showxchat(QString name, QString ip)
{
    if(!privatechat1)
    privatechat1 = new chat(name,ip);
    bb++;
}



void Widget::on_userTableWidget_doubleClicked(const QModelIndex &index)
{
    if(ui->userTableWidget->item(index.row(),0)->text() == getUserName() &&
        ui->userTableWidget->item(index.row(),2)->text() == getIP())
    {
        QMessageBox::warning(0,tr("Error!"),tr("和自己聊天是孤独的最高境界。"),QMessageBox::Ok);
    }
    else
    {
        if(!privatechat){
        privatechat = new chat(ui->userTableWidget->item(index.row(),0)->text(),
                               ui->userTableWidget->item(index.row(),2)->text()) ;
        }
        QByteArray data;
        QDataStream out(&data,QIODevice::WriteOnly);
        QString localHostName = QHostInfo::localHostName();
        QString address = getIP();
        out << Xchat << getUserName() << localHostName << address;
        QHostAddress address1 (ui->userTableWidget->item(index.row(),2)->text());
        udpSocket->writeDatagram(data, data.length(), address1, port);
        privatechat->remoteUserName=getUserName();
        privatechat->show();
        privatechat->is_opened = true;
    }

}
