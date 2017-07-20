#include "chat.h"
#include "ui_chat.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include <QColorDialog>

chat::chat(QString pasvusername, QString pasvuserip) : ui(new Ui::chat)
{
    ui->setupUi(this);
    ui->textEdit->setFocusPolicy(Qt::StrongFocus);
    ui->textBrowser->setFocusPolicy(Qt::NoFocus);

    ui->textEdit->setFocus();
    ui->textEdit->installEventFilter(this);

	a = 0;
	is_opened = false;
    xpasvusername = pasvusername;
    xpasvuserip = pasvuserip;

    ui->label->setText(tr("与%1聊天中   对方IP:%2").arg(xpasvusername).arg(pasvuserip));

    xchat = new QUdpSocket(this);
    xport = 55566;
    xchat->bind(xport, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(xchat, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));

    server = new TcpServer(this);
    connect(server,SIGNAL(sendFileName(QString)),this,SLOT(sentFileName(QString)));
    connect(ui->textEdit,SIGNAL(currentCharFormatChanged(QTextCharFormat)),this,SLOT(currentFormatChanged(const QTextCharFormat)));
}

chat::~chat()
{
    is_opened = false;
	delete ui;
}

bool chat::eventFilter(QObject *target, QEvent *event)
{
    if(target == ui->textEdit)
    {
        if(event->type() == QEvent::KeyPress)
        {
             QKeyEvent *k = static_cast<QKeyEvent *>(event);
             if(k->key() == Qt::Key_Return)
             {
                 on_send_clicked();
                 return true;
             }
        }
    }
    return QWidget::eventFilter(target,event);
}


void chat::participantLeft(QString userName,QString time)
{
    ui->textBrowser->setTextColor(Qt::gray);
    ui->textBrowser->setCurrentFont(QFont("宋体",9));
    ui->textBrowser->append(tr("%1 于 %2 离开！").arg(userName).arg(time));
}

QString chat::getUserName()
{
    return xpasvusername;
}

QString chat::getIP()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list)
    {
       if(address.protocol() == QAbstractSocket::IPv4Protocol)
            return address.toString();
    }
       return 0;
}

void chat::hasPendingFile(QString serverAddress,
                          QString clientAddress,QString fileName)
{
    QString ipAddress = getIP();
    if(ipAddress == clientAddress)
    {
        int btn = QMessageBox::information(this,tr("接收文件"),
                                 tr("来自设备%1的文件：%2,是否接收？")
                                 .arg(serverAddress).arg(fileName),
                                 QMessageBox::Yes,QMessageBox::No);
        if(btn == QMessageBox::Yes)
        {
            QString name = QFileDialog::getSaveFileName(0,tr("保存文件"),fileName);
            if(!name.isEmpty())
            {
                TcpClient *client = new TcpClient(this);
                client->setFileName(name);
                client->setHostAddress(QHostAddress(serverAddress));
                client->show();

            }

        }
        else{
            sendMessage(Refuse,serverAddress);
        }
    }
}

void chat::processPendingDatagrams()
{
    while(xchat->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(xchat->pendingDatagramSize());
        xchat->readDatagram(datagram.data(),datagram.size());
        QDataStream in(&datagram,QIODevice::ReadOnly);
        int messageType;
        in >> messageType;
        QString userName,localHostName,ipAddress,messagestr;
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch(messageType)
        {
			case Xchat:
			{
				break;
			}
            case Message:
                {
                    in >>userName >>localHostName >>ipAddress >>messagestr;
                    remoteUserName=userName;
                    ui->textBrowser->setTextColor(Qt::blue);
                    ui->textBrowser->setCurrentFont(QFont("宋体",9));
                    ui->textBrowser->append("");
                    ui->textBrowser->append("[" +getUserName()+"]"+ time);
                    ui->textBrowser->append(messagestr);
					{
						this->show();////解决bug1.收到私聊消息后才显示
						is_opened = true;
					}
                    break;
                }
        case FileName:
            {
                in >>userName >>localHostName >> ipAddress;
                QString clientAddress,fileName;
                in >> clientAddress >> fileName;
                hasPendingFile(ipAddress,clientAddress,fileName);
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
        case ParticipantLeft:
            {
                in >>userName >>localHostName;
                participantLeft(userName,time);
                QMessageBox::information(0,tr("本次对话关闭"),tr("对方结束了对话"),QMessageBox::Ok);
				a = 1;
				ui->textBrowser->clear();
				ui->~chat();
                close();
                break;
            }
        }
    }
}

void chat::sentFileName(QString fileName)
{
    this->fileName = fileName;
    sendMessage(FileName);
}

QString chat::getMessage()
{
    QString msg = ui->textEdit->toHtml();
    qDebug()<<msg;
    ui->textEdit->clear();
    ui->textEdit->setFocus();
    return msg;
}

void chat::sendMessage(MessageType type , QString serverAddress)
{
    QByteArray data;
    QDataStream out(&data,QIODevice::WriteOnly);
    QString localHostName = QHostInfo::localHostName();
    QString address = getIP();
    out << type << getUserName() << localHostName;


    switch(type)
    {
    case ParticipantLeft:
        {
            break;
        }
    case Message :
        {
            if(ui->textEdit->toPlainText() == "")
            {
                QMessageBox::warning(0,tr("警告"),tr("发送内容不能为空"),QMessageBox::Ok);
                return;
            }
            message = getMessage();
            out << address << message;
            ui->textBrowser->verticalScrollBar()->setValue(ui->textBrowser->verticalScrollBar()->maximum());
            break;
        }
    case FileName:
            {
                QString clientAddress = xpasvuserip;
                out << address << clientAddress << fileName;
                break;
            }
    case Refuse:
            {
                out << serverAddress;
                break;
            }
    }
    QHostAddress address2 (xpasvuserip);
    xchat->writeDatagram(data, data.length(), address2, 55566);
}

void chat::currentFormatChanged(const QTextCharFormat &format)
{
    ui->fontComboBox->setCurrentFont(format.font());

    if(format.fontPointSize()<9)
    {
        ui->fontsizecomboBox->setCurrentIndex(3);
    }
    else
    {
        ui->fontsizecomboBox->setCurrentIndex(ui->fontsizecomboBox->findText(QString::number(format.fontPointSize())));

    }

    ui->textbold->setChecked(format.font().bold());
    ui->textitalic->setChecked(format.font().italic());
    ui->textUnderline->setChecked(format.font().underline());
    color = format.foreground().color();
}

void chat::on_fontComboBox_currentFontChanged(QFont f)
{
    ui->textEdit->setCurrentFont(f);
    ui->textEdit->setFocus();
}

void chat::on_fontsizecomboBox_currentIndexChanged(QString size)
{
   ui->textEdit->setFontPointSize(size.toDouble());
   ui->textEdit->setFocus();
}

void chat::on_textbold_clicked(bool checked)
{
    if(checked)
        ui->textEdit->setFontWeight(QFont::Bold);
    else
        ui->textEdit->setFontWeight(QFont::Normal);
    ui->textEdit->setFocus();
}

void chat::on_textitalic_clicked(bool checked)
{
    ui->textEdit->setFontItalic(checked);
    ui->textEdit->setFocus();
}

void chat::on_save_clicked()
{
    if(ui->textBrowser->document()->isEmpty())
        QMessageBox::warning(0,tr("警告"),tr("聊天记录为空，无法保存！"),QMessageBox::Ok);
    else
    {
       QString fileName = QFileDialog::getSaveFileName(this,tr("保存聊天记录"),tr("聊天记录"),tr("文本(*.txt);;All File(*.*)"));
       if(!fileName.isEmpty())
           saveFile(fileName);
    }
}

void chat::on_clear_clicked()
{
    ui->textBrowser->clear();
}

bool chat::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if(!file.open(QFile::WriteOnly | QFile::Text))

    {
        QMessageBox::warning(this,tr("保存文件"),
        tr("无法保存文件 %1:\n %2").arg(fileName)
        .arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out << ui->textBrowser->toPlainText();

    return true;
}

void chat::on_textUnderline_clicked(bool checked)
{
    ui->textEdit->setFontUnderline(checked);
    ui->textEdit->setFocus();
}

void chat::on_textcolor_clicked()
{
    color = QColorDialog::getColor(color,this);
    if(color.isValid())
    {
        ui->textEdit->setTextColor(color);
        ui->textEdit->setFocus();
    }
}



void chat::on_close_clicked()
{
    sendMessage(ParticipantLeft);
	a = 1;
	ui->textBrowser->clear();
    close();
	ui->~chat();	
}

void chat::on_send_clicked()
{
    sendMessage(Message);
	QString localHostName = QHostInfo::localHostName();
    QString time = QDateTime::currentDateTime().toString(" yyyy-MM-dd hh:mm:ss");
    ui->textBrowser->setTextColor(QColor(46,128,0,255));
    ui->textBrowser->setCurrentFont(QFont("宋体",9));
    ui->textBrowser->append("");
    ui->textBrowser->append("[" +remoteUserName+"]"+ time);
    ui->textBrowser->append(message);
}

void chat::on_sendfile_clicked()
{
    server->show();
    server->initServer();
}
