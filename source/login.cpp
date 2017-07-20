#include "login.h"
#include "ui_login.h"
#include "widget.h"

Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
}

Login::~Login()
{
    delete ui;
}

void Login::on_pushButton_clicked()
{
    QString inputName;
    if(ui->nameEdit->text().isEmpty()){
        inputName="Anonymous";
    }
    else {
        inputName=ui->nameEdit->text();
    }
    w=new Widget(0,inputName);
    this->close();
    w->show();
}
