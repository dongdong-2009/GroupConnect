#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QUdpSocket>
#include <QTextFormat>
//#include "tcpclient.h"
//#include "tcpserver.h"
#include "chat.h"

namespace Ui {
class Widget;
}

class TcpServer;

class Widget : public QWidget
{
    Q_OBJECT

    
public:
    explicit Widget(QWidget *parent = 0,QString inputName="Anonymous");
    ~Widget();
    chat* privatechat;
    chat* privatechat1;

protected:
    void newParticipant(QString userName, QString localHostName, QString ipAddress);
    void participantLeft(QString localHostName);
    void sendMessage(MessageType type, QString serverAddress="");

    void hasPendingFile(QString userName, QString serverAddress, QString clientAddress, QString fileName);

    QString getIP();
    QString getUserName();
    QString getMessage();

    void closeEvent(QCloseEvent *);
    bool eventFilter(QObject *target, QEvent *event);

private:
    Ui::Widget *ui;
    QUdpSocket *udpSocket;
    qint16 port;
    qint32 bb;
    QString fileName;
    TcpServer *server;
    QColor color;
    QString inputName;

    bool saveFile(const QString& fileName);
    void showxchat(QString name, QString ip);

private slots:
    void processPendingDatagrams();
    void getFileName(QString);
    void currentFormatChanged(const QTextCharFormat &format);

    void on_sendButton_clicked();

    void on_sendToolBtn_clicked();

    void on_fontComboBox_currentFontChanged(const QFont &f);

    void on_sizeComboBox_currentIndexChanged(const QString &arg1);

    void on_boldToolBtn_clicked(bool checked);

    void on_italicToolBtn_clicked(bool checked);

    void on_underlineToolBtn_clicked(bool checked);

    void on_colorToolBtn_clicked();

    void on_clearToolBtn_clicked();

    void on_exitButton_clicked();

    void on_saveToolBtn_clicked();

    void on_userTableWidget_doubleClicked(const QModelIndex &index);
};

#endif // WIDGET_H
