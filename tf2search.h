#ifndef TF2SEARCH_H
#define TF2SEARCH_H

#include <QObject>
#include <QWebView>
#include <QWebFrame>
#include <QWebPage>
#include <QWebElement>
#include <QDesktopServices>
#include <QXmlStreamReader>
#include <QKeyEvent>
#include <QTime>
#include <QDir>
#include "persistentcookiejar.h"

const QString DS_PRONTO = "Análise Concluída.",
              DS_CARREGANDO = "Carregando...",
              DS_CARREGANDO_SC = "Carregando bp dos bots... ",
              DS_CARREGANDO_BP = "Checando classificados do bp.tf... ",
              DS_CARREGANDO_WH = "Verificando bloqueios por Escrow no WH... ";
//const int PRECO_REF = 2500, PRECO_KEY = 45000, PRECO_BUD = 200000;
//const float FATOR = 0.93;
const float ESTOQUE = 1.00;
//const int LUCRO_MIN = 0.1*PRECO_KEY;//lucro mínimo que o item deve apresentar para aparecer na lista
const int PRECO_BASE = 100000;//preço sobre o qual é necessário ter-se, no mínimo, o lucro salvo em "DIFERENCA".
const int MIN_KEYS_COMPRA = 10;//se o preço do item for maior que este valor, ele será mostrado na página de compra mesmo que não gere lucro
const int PERDA_MAX = 1;//perda máxima, em keys, que os itens caros podem gerar para aparecerem no programa
const QString INICIO_LINK_WH = "https://www.tf2wh.com",
              ITEM_UNCRAFTABLE_WH = "( Not Usable in Crafting )";

//const int PRECO_MIN_COMPRA = 50000;
//const int FATOR_COMPRA = 1.25;
//const int CAPITAL_WH = 312000;

class TF2Search : public QObject
{
    Q_OBJECT
private:
    QWebView *navegador;
    QStringList itensWH, itensBP, estoquesStr, resultadoV, resultadoC, linksWH, itensReservados;
    /*stringlist venda resultado contém os textos que serão mostrados na tabela final:
     * lucro % - lucro - item - preço bp - estoque wh - link*/
    /*stringlist compra:
     * lucro - item - preço venda - estoque wh*/
    QVector<int> precosWH, precosWHc, precosBP, indicesSearch;
    QVector<float> estoquesWH;
    QString links, codigoMobile;
    QTime *relogio;
    bool verEstoque, verEstoqueC, itemReservado;
    unsigned short indice, tpOrdenacao;
    int PRECO_REF, PRECO_KEY, PRECO_BUD, LUCRO_MIN;

    void loginEfetuadoClicarIniciar();
    void autenticarSteamMobile(QString urlAtual);
    void conectaWH();
    QString getPrecoStr(int precoInt);
    float getLucroPorc(float base, float lucro);
    void alteraOpcoesNavegador(bool ativado);
    bool opcoesNavegadorAtivas();
    void limpaListas();

    void carregarPaginaItem(int pos);
    bool reservarItem();

    void salvarLinksWH();
    bool verificaItemUncraftableWH();
    void finalizaAnaliseEscrow();
    void acessaProximoLink();

    void registrarLog(QString msg);

public:
    explicit TF2Search(bool verEstoqueV = true, bool verEstoqueC = false, unsigned short tpOrdem = 0,
                       QString codigoMobile = "", QObject *parent = 0);
    ~TF2Search();
    void vamoTrabalhar();
    QString getTipo(QString&);
    int convertePrecoBP(QString);
    //int converte_preco_sc(QString);
    void geraTabelaWH(QString);
    void geraTabelaBP(QString);
    //void gera_tabela_sc(QString);
    void consertaFaixaValores(QString&);
    void comparaTabelas();
    float getEstoque(QString);
    int getPosMaiorLucro(QVector<float>& lucros);
    int getPosMaiorLucroPorc(QVector<float>& lucros, QVector<int>& posicoes);
    QString getLink(QString,int);
    bool tarefasConcluidas();

    int getTamanhoResultV();
    QString getResultadoVi(unsigned);
    int getTamanhoResultC();
    QString getResultadoCi(unsigned);
    void setCodigo(QString codigo);
    QString getLinkWH(QString item);

    void considerarEstoque(bool);
    void considerarEstoqueC(bool);
    void getPrecos(int &, int &, int &);
    void getNomeItemPuro(QString&);
    unsigned short getCodigoBP(QString);

    //Pesquisa nas bags dos bots do Scrap
    void verificarBotsSC();
    QStringList getNomesItensBagsSC(QWebElementCollection elementos);
    QString ajustaNomeItemBagSC(QString item);
    void comparaTabelasSCeWH();
    //bool tipoItemOk(QString);

    void verificarListasBP();
    QString geraLinkBPCl(QString);

    void verificarEscrowWH(QStringList itensReservados);

signals:
    void analisesConcluidas(bool);
    void bolsasBotAnalisadas(bool, QVector<int>&);
    void classificadosBPAnalisados(QVector<int>&);
    void escrowWHAnalisado(bool, QVector<int>&);
    void registrarRelatorio(QString);
    void atualizarStatus(QString);

public slots:
    void bpCarregado();
    void whCarregado();
    void telaLoginCarregada();
    void paginaCodigoCarregada();
    void paginaInicialCarregada();
    void bpClCarregado();
    void botBagCarregado();
    /*void sc_carregado();
    void botBagCarregadoLink1();
    void botBagCarregadoLink2();
    void botBagCarregadoLink3();*/
    void escrowPageWHCarregada();
    void paginaItemWHCarregada();
};

#endif // TF2SEARCH_H
