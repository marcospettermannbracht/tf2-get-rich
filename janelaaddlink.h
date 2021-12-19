#ifndef JANELAADDLINK_H
#define JANELAADDLINK_H

#include <QDialog>

namespace Ui {
class JanelaAddLink;
}

class JanelaAddLink : public QDialog
{
    Q_OBJECT

public:
    explicit JanelaAddLink(QWidget *parent = 0);
    ~JanelaAddLink();

    void set_nome_item(QString);
    bool link_foi_digitado();
    QString get_link();

private slots:
    void on_buttonBox_rejected();

    void on_buttonBox_accepted();

private:
    Ui::JanelaAddLink *ui;
};

#endif // JANELAADDLINK_H
