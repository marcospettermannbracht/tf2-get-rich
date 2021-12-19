#include "janelaaddlink.h"
#include "ui_janelaaddlink.h"

JanelaAddLink::JanelaAddLink(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JanelaAddLink)
{
    ui->setupUi(this);
    setWindowTitle("Add Link");
}

JanelaAddLink::~JanelaAddLink()
{
    delete ui;
}

void JanelaAddLink::set_nome_item(QString nome)
{
    ui->nome_item->setText(nome);
}

QString JanelaAddLink::get_link()
{
    return ui->link->text();
}

bool JanelaAddLink::link_foi_digitado()
{
    if(ui->link->text().indexOf("http://www.tf2outpost.com/search/") != -1)
        return true;
    return false;
}

void JanelaAddLink::on_buttonBox_rejected()
{
    close();
}

void JanelaAddLink::on_buttonBox_accepted()
{
    close();
}
