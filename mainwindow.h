#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QClipboard>
#include <QTimer>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QSound>
#include "tf2search.h"
#include "janelaaddlink.h"
#include "loghelper.h"

const int TEMPO_CONSULTA = 120; //intervalo de tempo entre as consultas da aba Compra no WH (segundos).

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    TF2Search *tf2;
    QStringList links;
    QTimer *relogio;
    QLabel *lbStatus;
    QString codigoMobile;
    LogHelper *log;
    bool modoAll,
         inicioAnalise; // criado para impedir múltiplos signals em sequência

    void abrirLinkWH(QString item);
    void mostrarNumItensEncontrados(short aba, int num);
    void preencheNumItens(short aba, int contOld);
    void inicioBusca();
    void finalBusca();
    bool pedeAutenticacaoMobile();
    void limparMarcacaoItens(short aba);
    void tocarSom();
    QStringList getItensReservados();

protected:
    void resizeEvent(QResizeEvent *e);

public slots:
    void preencherTabela(bool sucesso);
    void abrirLink(int);
    void abrirLinkCompra(int);
    void destacarItensSC(bool, QVector<int> &);
    void verResultBP(QVector<int> &);
    void destacarItensEscrowWH(bool, QVector<int> &);
    void ordemLucroSelecionada();
    void ordemPorcentSelecionada();
    void linkOutpostSelecionado();
    void linkBPClassifiedSelecionado();
    void considerarEstoqueCompraChanged();
    void mostrarPrecos();
    void atualizaStatus(QString);

private slots:
    void on_botao_refresh_clicked();
    void on_botao_sc_clicked();
    void on_botao_bp_clicked();
    void on_botao_escrow_clicked();
    void on_chkMonitorar_stateChanged();
    void on_btnNext_clicked();
    void on_botao_reservado_clicked();
};

#endif // MAINWINDOW_H
