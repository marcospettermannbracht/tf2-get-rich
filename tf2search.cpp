#include "tf2search.h"

TF2Search::TF2Search(bool verEstoqueV,
                     bool verEstoqueC,
                     unsigned short tpOrdem,
                     QString codigoMobile,
                     QObject *parent) :
    QObject(parent)
{
    this->verEstoque  = verEstoqueV;
    this->verEstoqueC = verEstoqueC;
    this->tpOrdenacao = tpOrdem;
    this->codigoMobile = codigoMobile;
    PRECO_KEY = PRECO_BUD = PRECO_REF = 0;
    //connect(navegador,SIGNAL(loadFinished(bool)),this,SLOT(op_search_carregado(bool)));
    //navegador->load(QUrl("http://www.tf2outpost.com/search"));
}

void TF2Search::vamoTrabalhar()
{
    relogio = new QTime;
    navegador = new QWebView;

    //Necessário para persistir e carregar cookies entre execuções
    PersistentCookieJar *cookiePersist = new PersistentCookieJar(this);
    navegador->page()->networkAccessManager()->setCookieJar(cookiePersist);

    emit atualizarStatus("Logando na steam...");
    connect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(telaLoginCarregada()));
    navegador->load(QUrl("https://steamcommunity.com/login"));
    //navegador->show(); //para debug

    /*connect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(sc_carregado(bool)));
    navegador->load(QUrl("https://scrap.tf/items"));*/
}

TF2Search::~TF2Search()
{
    delete navegador;
    delete relogio;
}

/******************************************************************************************************
 *          LOGIN NA STEAM
 ******************************************************************************************************/

void TF2Search::telaLoginCarregada()
{
    qDebug() << "tela login ok";
    disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(telaLoginCarregada()));
    //necessário login
    if (!navegador->page()->mainFrame()->findFirstElement("input#SteamLogin").isNull() ||
        !navegador->page()->mainFrame()->findFirstElement("input#imageLogin").isNull())
    {
        if (this->codigoMobile.isNull() || this->codigoMobile.isEmpty())
            emit analisesConcluidas(false);
        else
            autenticarSteamMobile("https://steamcommunity.com/login");
    }
    //logado
    else
        conectaWH();
}

void TF2Search::loginEfetuadoClicarIniciar()
{
    QWebElement campo = navegador->page()->mainFrame()->findFirstElement("input#imageLogin");
    campo.evaluateJavaScript("this.click()");
    connect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaInicialCarregada()));
}

void TF2Search::autenticarSteamMobile(QString urlAtual)
{
    if (opcoesNavegadorAtivas())
    {
        qDebug() << "funções avançadas ok";
        QWebElement campo = navegador->page()->mainFrame()->findFirstElement("input#steamAccountName");
        campo.setAttribute("value", "chcolorado");
        campo = navegador->page()->mainFrame()->findFirstElement("input#steamPassword");
        campo.setAttribute("value", "finalseason");
        campo = navegador->page()->mainFrame()->findFirstElement("input#remember_login");
        if (!campo.isNull())
            campo.setAttribute("checked", "1");
        campo = navegador->page()->mainFrame()->findFirstElement("input#SteamLogin");
        if (campo.isNull())
            campo = navegador->page()->mainFrame()->findFirstElement("input#imageLogin");
        campo.evaluateJavaScript("this.click()");
        connect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaCodigoCarregada()));
    }
    else //recarrega a página com as funções avançadas do navegador ativas
    {
        qDebug() << "recarregando pg login com funções avançadas";
        alteraOpcoesNavegador(true);
        connect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(whCarregado()));
        navegador->load(QUrl(urlAtual));
    }
}

void TF2Search::paginaCodigoCarregada()
{
    if (navegador->page()->mainFrame()->toPlainText().contains("Olá, chcolorado!"))
    {
        disconnect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaCodigoCarregada()));
        QInputMethodEvent *input = new QInputMethodEvent();
        input->setCommitString(this->codigoMobile);
        navegador->event(input);
        navegador->event(new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));//aperta enter
        connect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaInicialCarregada()));
    }
    else
        qDebug() << "não logado";
}

void TF2Search::paginaInicialCarregada()
{
    QString urlAtual = navegador->page()->mainFrame()->url().toString();
    qDebug() << "pg inicial ok, url: " << urlAtual << urlAtual.contains("steamcommunity.com/id/marquos") << !navegador->page()->mainFrame()->findFirstElement("div.header_installsteam_btn.header_installsteam_btn_gray").isNull();

    //para selecionar elementos de classe que contém espaços (que na verdade são duas classes), usar ponto final.
    if ((urlAtual.contains("steamcommunity.com/id/marquos") && !navegador->page()->mainFrame()->findFirstElement("div.header_installsteam_btn.header_installsteam_btn_gray").isNull()) ||
        (urlAtual.contains("www.tf2wh.com") && !navegador->page()->mainFrame()->findFirstElement("div#stats").isNull()))
    {
        disconnect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaInicialCarregada()));
        conectaWH();
    }
    else if (urlAtual.contains("www.tf2wh.com") && !navegador->page()->mainFrame()->findFirstElement("div#stats").isNull())
    {
        disconnect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaInicialCarregada()));
        conectaWH();
    }
    else if (urlAtual.contains("scrap.tf") && !navegador->page()->mainFrame()->findFirstElement("li.dropdown.nav-userinfo").isNull())
    {
        disconnect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaInicialCarregada()));
        verificarBotsSC();
    }
}

/******************************************************************************************************
 *          VERIFICAÇÕES DA TABELA DO TF2WH
 ******************************************************************************************************/

void TF2Search::conectaWH()
{
    emit atualizarStatus("Carregando tabela do WH...");
    alteraOpcoesNavegador(false);
    connect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(whCarregado()));
    navegador->load(QUrl("https://www.tf2wh.com/priceguide?login"));
}

void TF2Search::whCarregado()
{
    QWebElement elTabela = navegador->page()->mainFrame()->findFirstElement("table#pricelist");
    if (!elTabela.isNull())
    {
        disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(whCarregado()));
        geraTabelaWH(elTabela.toPlainText());
        emit atualizarStatus("Carregando tabela do BP...");
        connect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(bpCarregado()));
        navegador->load(QUrl("http://backpack.tf/pricelist/spreadsheet"));
    }
    //necessária autenticação na steam
    else if (!navegador->page()->mainFrame()->findFirstElement("input#steamAccountName").isNull())
    {
        disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(whCarregado()));
        if (this->codigoMobile.isNull() || this->codigoMobile.isEmpty())
            emit analisesConcluidas(false);
        else
            autenticarSteamMobile("https://www.tf2wh.com/priceguide?login");
    }
    //logado, só clicar em login
    else if (!navegador->page()->mainFrame()->findFirstElement("div.OpenID_loggedInName").isNull())
    {
        disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(whCarregado()));
        loginEfetuadoClicarIniciar();
    }
    //problema na página, recarregando
    else
        navegador->load(QUrl("https://www.tf2wh.com/priceguide?login"));
}

void TF2Search::geraTabelaWH(QString tabela)
{
    int inic = tabela.indexOf("Ultimate")+9;
    QString nome, numC, num, estoq;

    /*QFile arq("D:/LetsGetItDone/tf2Projetins/log.txt");
    if(arq.open(QFile::WriteOnly | QFile::Text)){
        QTextStream out(&arq);
        out << tabela;
    }arq.close();*/

    if(itensWH.isEmpty()){
        for(int j=inic; j<tabela.size(); j++){
            nome.clear();
            num.clear();
            estoq.clear();
            //Salvar o nome do item
            for(int i=0; tabela[j].unicode()!=9; i++, j++)
                nome[i] = tabela[j];
            //Correções específicas
            if(nome == "Vintage Merryweather" || nome == "Vintage Tyrolean")
                nome = "Vintage " + nome;
            else if(nome == "Select Reserve Mann Co. Supply Crate")
                nome += " #60";
            else if(nome == "Salvaged Mann Co. Supply Crate")
                nome += " #40";
            else if(nome == "Strange Übersaw")
                nome = "Strange Ubersaw";
            else if(nome == "Bacon Grease")
                nome = "Strange Bacon Grease";
            else if(nome == "Haunted Hat")
                nome = "Haunted Haunted Hat";
            j++;
            //Salvar o estoque
            for(int i=0; tabela[j].unicode()!=9; i++, j++)
                estoq[i] = tabela[j];
            j++;
            //Salvar o preço de compra
            for(int i=0; tabela[j].unicode()!=9; i++, j++)
                numC[i] = tabela[j];
            numC.remove(',');
            if(PRECO_KEY == 0 && nome == "Mann Co. Supply Crate Key"){
                PRECO_KEY = numC.remove(',').toInt();
                LUCRO_MIN = 0.1 * PRECO_KEY;
            }
            else if(nome == "Refined Metal")
                PRECO_REF = numC.remove(',').toInt();
            else if(nome == "Earbuds")
                PRECO_BUD = numC.remove(',').toInt();
            j++;
            //Salvar o preço do item
            for(int i=0; tabela[j].unicode()!=9; i++, j++)
                num[i] = tabela[j];
            num.remove(',');
            j++;
            //Armazenamento dos dados lidos.
            itensWH << nome;
            precosWH.push_back(num.toInt());
            precosWHc.push_back(numC.toInt());
            estoquesStr << estoq;
            estoquesWH.push_back(getEstoque(estoq));
            for(; tabela[j].unicode()!=10 && j<tabela.size(); j++){}
        }
    }
}

//Converte um estoque na forma XX/YY para um número real.
float TF2Search::getEstoque(QString estoq)
{
    QString num1, num2;
    int n1, n2, j=0;
    for(int i=0; estoq[j]!='/'; i++, j++)
        num1[i] = estoq[j];
    j++;
    for(int i=0; j<estoq.size(); i++, j++)
        num2[i] = estoq[j];
    n1 = num1.toInt();
    n2 = num2.toInt();

    if(n2 == 0)
        return -1; // estoques X/0
    else
        return ((float)n1/n2);

}

/******************************************************************************************************
 *          VERIFICAÇÕES DA TABELA DO BP.TF
 ******************************************************************************************************/

void TF2Search::bpCarregado()
{
    QWebElement campo = navegador->page()->mainFrame()->findFirstElement("table#pricelist");
    if (!campo.isNull())
    {
        QString tabela = campo.toInnerXml();
        disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(bpCarregado()));
        geraTabelaBP(tabela);
        emit atualizarStatus("Comparando tabelas...");
        comparaTabelas();
    }
}

void TF2Search::geraTabelaBP(QString tabela)
{
    QString nome, nomeTemp, strAux, num;
    //unsigned inic = tabela.indexOf("<tbody>") + 7, pos, pos_fim = tabela.indexOf("Ali Baba's Wee Booties") - 5, pos_aux, n_col,
    unsigned inic = tabela.indexOf("<tbody>") + 7, nCol;
    int posAux, posFimLin, posFim = tabela.lastIndexOf("</tr>");
    //bool fim = false, fimLinha, sair;

    /*QFile arq("D:/LetsGetItDone/tf2Projetins/log.txt");
    if(arq.open(QFile::WriteOnly | QFile::Text)){
        QTextStream out(&arq);
        out << tabela;
    }arq.close();*/

    //Laço que lê a tabela toda
    for(int pos=inic; pos<posFim;)
    {
        //Laço que lê uma linha da tabela
        pos = tabela.indexOf("<td>",pos) + 4;
        nome.clear();
        for(int i=0; tabela[pos]!='<'; pos++, i++)//Lendo o nome do item
            nome[i] = tabela[pos];
        if(nome.indexOf("Part:")!=-1)//excessão para stranges part
            nome.remove(0,8);
        if(nome[nome.size()-1] != ' ')//Se não for item uncraftable
        {
            pos = tabela.indexOf("</td>",pos+2) + 5;//Ignora o tipo de item
            nCol = 0;
            posFimLin = tabela.indexOf("</tr>",pos);
            while(pos < posFimLin){
                posAux = tabela.indexOf("</a>",pos);
                if(posAux != -1 && posAux < posFimLin)
                {
                    if(tabela[posAux-1] == '>')//Se o preço possuir setas de oscilação
                        posAux = tabela.indexOf("<span ",pos);
                    strAux.clear();//String que salva a informação do número da coluna atual e do preço salvo
                    for(int i=0; pos<posAux; pos++, i++)
                    {
                        strAux[i] = tabela[pos];
                    }
                    nCol += strAux.count("<td abbr=");//Acha o número da coluna
                    //qDebug() << str_aux << n_col;
                    nomeTemp.clear();
                    nomeTemp = nome;
                    if (nCol != 3 || !strAux.contains("Strange%20Unique")) //ignorando itens Strange Unique que o tf2wh não aceita
                    {
                        switch(nCol){
                            case 1:{
                                nomeTemp.insert(0,"Genuine ");
                                break;
                            }
                            case 2:{
                                nomeTemp.insert(0,"Vintage ");
                                break;
                            }
                            case 4:{
                                nomeTemp.insert(0,"Strange ");
                                break;
                            }
                            case 5:{
                                nomeTemp.insert(0,"Haunted ");
                                break;
                            }
                        }
                        num.clear();
                        for(posAux=strAux.size()-1; strAux[posAux]!='>'; posAux--){}//Acha o preço
                        posAux++;
                        for(int i=0; posAux<strAux.size(); posAux++, i++)
                        {
                            num[i] = strAux[posAux];
                        }
                        //qDebug() << num;
                        if(nCol != 6){//Se não for item Collector's, salva as informações nas listas da classe
                            num = num.trimmed();
                            consertaFaixaValores(num);
                            precosBP.push_back(convertePrecoBP(num));
                            itensBP << nomeTemp;
                        }
                        pos = tabela.indexOf("</td>",pos) + 5;//Avança a variável contadora
                    }
                }else
                    pos = posFimLin;
            }
        }
        pos = tabela.indexOf("</tr>",pos) + 5;//Lendo até o fim dessa linha
    }
}

//Conserta casos em que o preço é dado por um intervalo de valores, mantendo apenas o menor deles. Ex.: "2.88-3 ref"->"2.88 ref"
void TF2Search::consertaFaixaValores(QString& preco)
{
    bool ehFaixa = false;

    for(int i=0; i<preco.size(); i++){
        if(preco[i].unicode() > 127)
            ehFaixa = true;
    }

    if(ehFaixa){
        int inic=0, fim=0;
        for(int i=0; i<preco.size(); i++){
            if(preco[i].unicode() > 127)
                inic = i;
            else if(preco[i] == ' ')
                fim = i;
        }preco.remove(inic,(fim-inic));
    }
}

//Converte preços do bp.tf de string para inteiros. Formato ex.: 4.6 keys -> 207000
int TF2Search::convertePrecoBP(QString txt)
{
    QString num;
    float preco=0;
    int i;

    if(txt.indexOf(' ') != -1){
        for(i=0; txt[i]!=' '; i++)
            num[i] = txt[i];

        preco = num.toFloat();

        if(txt[i+1]=='r')
            preco = preco * PRECO_REF;
        else if(txt[i+1]=='k')
            preco = preco * PRECO_KEY;
        else if(txt[i+1]=='b')
            preco = preco * PRECO_BUD;
        else
            preco = 0;
    }

    return ((int)preco);
}

/******************************************************************************************************
 *          COMPARAÇÃO ENTRE OS PREÇOS DO TF2WH E DO BP.TF
 ******************************************************************************************************/

void TF2Search::comparaTabelas()
{
    QVector<float> lucrosValidos, lucrosValidosC;
    QVector<int> posicoes, posicoesC;

    //Desativado para economizar memória - links do Outpost não estão sendo mais utilizados
    /*QFile fileLinks(QDir::currentPath()+"/links.txt");
    fileLinks.open(QIODevice::ReadOnly);
    QTextStream in(&fileLinks);
    links = in.readAll();*/

    for(int i=0; i<itensWH.size(); i++)
    {
        int ind = itensBP.indexOf(itensWH.at(i));
        if(ind==-1)
        {
            //Tentar adicionando "The" no nome.
            QString nome = itensWH.at(i);
            nome = "The " + nome;
            ind = itensBP.indexOf(nome);
        }
        if(ind!=-1)
        {
            // exceçoes para pure
            if (itensWH[i] == "Mann Co. Supply Crate Key" ||
                itensWH[i] == "Refined Metal")
            {
                lucrosValidosC.push_back(precosBP[ind]-precosWHc[i]);
                posicoesC.push_back(i);
            }
            else
            {
                // Venda no WH
                if((verEstoque && estoquesWH[i] > -1.0 && estoquesWH[i] < ESTOQUE) || !verEstoque){
                    if(precosBP[ind]>0 && ((precosWH[i]-precosBP[ind])>=LUCRO_MIN)){
                        lucrosValidos.push_back(precosWH[i]-precosBP[ind]);
                        posicoes.push_back(i);
                    }
                }
                // Compra no WH
                //Verificando itens que podem ser comprados no WH e vendidos com lucro
                if((!verEstoqueC || (verEstoqueC && estoquesWH[i] > 0.0)) &&
                   (precosBP[ind] - precosWHc[i] >= LUCRO_MIN ||
                   (precosWHc[i] >= MIN_KEYS_COMPRA * PRECO_KEY &&
                    precosBP[ind] - precosWHc[i] >= - PERDA_MAX * PRECO_KEY)))
                {
                    lucrosValidosC.push_back(precosBP[ind]-precosWHc[i]);
                    posicoesC.push_back(i);
                }
            }
        }
        /*else
            emit registrarRelatorio("Item do WH não encontrado na lista do BP: " + itensWH.at(i) + ".");*/
    }

    //MODO VENDA
    int tam = lucrosValidos.size(), pos=1;
    for(int i=0; !lucrosValidos.isEmpty(); i++){
        if((this->tpOrdenacao == 1 && i == getPosMaiorLucroPorc(lucrosValidos, posicoes)) ||
           (this->tpOrdenacao == 0 && i == getPosMaiorLucro(lucrosValidos))){
            //Salva informações iniciais
            resultadoV << QString::number(getLucroPorc(precosWH[posicoes[i]]-lucrosValidos[i], lucrosValidos[i]), 'f', 1) + "%" <<
                          getPrecoStr(lucrosValidos[i]) << itensWH.at(posicoes[i]) <<
                          getPrecoStr(precosWH[posicoes[i]]-lucrosValidos[i]) << estoquesStr.at(posicoes[i]);
            //Obtendo o link, se houver
            QString nomeValido = itensWH.at(posicoes[i]);
            nomeValido.replace("ü","u");
            nomeValido.replace("Ü","U");
            int aux = links.indexOf("-" + nomeValido + "-");
            if(aux!=-1)
                resultadoV << getLink(nomeValido,aux);
            else
                resultadoV << "SEM_LINK";
            lucrosValidos.remove(i);
            posicoes.remove(i);
            pos++;
        }else if(i == tam)
            i = -1;
    }

    //MODO COMPRA
    tam = lucrosValidosC.size();
    pos = 1;
    for(int i=0; !lucrosValidosC.isEmpty(); i++){
        if(i == getPosMaiorLucro(lucrosValidosC)){
            //Salva informações iniciais
            resultadoC << getPrecoStr(lucrosValidosC[i]) << itensWH.at(posicoesC[i])
                      << getPrecoStr(precosWHc[posicoesC[i]]+lucrosValidosC[i]) << estoquesStr.at(posicoesC[i]);
            lucrosValidosC.remove(i);
            posicoesC.remove(i);
            pos++;
        }else if(i == tam)
            i = -1;
    }
    limpaListas();

    emit analisesConcluidas(true);
}

//Código para converter o preço em número (WH) para keys e metal
QString TF2Search::getPrecoStr(int precoInt)
{
    QString precoStr;
    int nKeys;
    float nRefs;

    nKeys = precoInt / PRECO_KEY;
    nRefs = (float)(precoInt % PRECO_KEY) / PRECO_REF;
    if(nKeys == 0)
        precoStr = QString::number(nRefs, 'f', 2) + "ref";
    else if(nRefs == 0)
        precoStr = QString::number(nKeys) + "key";
    else
        precoStr = QString::number(nKeys) + "key " + QString::number(nRefs, 'f', 2) + "ref";
    return precoStr;
}

int TF2Search::getPosMaiorLucro(QVector<float>& lucros)
{
    float maior = - 2 * PERDA_MAX * PRECO_KEY;
    int pos = -1;
    for(int i=0; i<lucros.size(); i++){
        if(lucros[i] > maior){
            pos = i;
            maior = lucros[i];
        }
    }

    return pos;
}

int TF2Search::getPosMaiorLucroPorc(QVector<float>& lucros, QVector<int>& posicoes)
{
    float maior = -1;
    int pos = -1;
    for(int i=0; i<lucros.size(); i++){
        if(getLucroPorc(precosWH[posicoes[i]]-lucros[i], lucros[i]) > maior){
            pos = i;
            maior = getLucroPorc(precosWH[posicoes[i]]-lucros[i], lucros[i]);
        }
    }

    return pos;
}

float TF2Search::getLucroPorc(float base, float lucro)
{
    return lucro / base * 100;
}

/******************************************************************************************************
 *          VERIFICAÇÕES DOS INVENTÁRIOS DOS BOTS DO SCRAP.TF
 ******************************************************************************************************/

void TF2Search::verificarBotsSC()
{
    indicesSearch.clear();
    indice = 46;
    alteraOpcoesNavegador(true);
    connect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(botBagCarregado()));
    navegador->load(QUrl("https://scrap.tf/stranges/" + QString::number(indice)));
}

void TF2Search::botBagCarregado()
{
    if (!navegador->page()->mainFrame()->findFirstElement("div.item.hoverable.quality11.app440").isNull())
    {
        disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(botBagCarregado()));
        comparaTabelasSCeWH();
        emit atualizarStatus(DS_CARREGANDO_SC + QString::number((indice - 45) * 20) + "%");
        indice++;
        if (indice > 50)
        {
            alteraOpcoesNavegador(false);
            emit bolsasBotAnalisadas(true, indicesSearch);
        }
        else
        {
            connect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(botBagCarregado()));
            navegador->load(QUrl("https://scrap.tf/stranges/" + QString::number(indice)));
        }
    }
    //necessária autenticação na steam
    else if (!navegador->page()->mainFrame()->findFirstElement("input#steamAccountName").isNull())
    {
        disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(botBagCarregado()));
        if (this->codigoMobile.isNull() || this->codigoMobile.isEmpty())
            emit analisesConcluidas(false);
        else
            autenticarSteamMobile("https://scrap.tf/stranges/" + QString::number(indice));
    }
    //logado, só clicar em login
    else if (!navegador->page()->mainFrame()->findFirstElement("div.OpenID_loggedInName").isNull())
    {
        disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(botBagCarregado()));
        loginEfetuadoClicarIniciar();
    }
}

void TF2Search::comparaTabelasSCeWH()
{
    //Lista de itens presentes na bag desse bot
    QStringList itens = getNomesItensBagsSC(navegador->page()->mainFrame()->findAllElements("div.item.hoverable.quality11.app440"));
    for (int i = 2, lin = 0; i < resultadoV.count(); i += 6, lin++)
        if (itens.contains(resultadoV[i]) && !indicesSearch.contains(lin))
            indicesSearch.append(lin);
}

//Retorna uma lista de itens presentes na bag desse bot
QStringList TF2Search::getNomesItensBagsSC(QWebElementCollection elementos)
{
    QStringList nomes;
    for (int i = 0; i < elementos.count(); i++)
    {
        QString nome = ajustaNomeItemBagSC(elementos[i].attribute("data-title"));
        if (!nomes.contains(nome))
            nomes.append(nome);
    }
    return nomes;
}

//Remove as tags XML em volta do nome do item no atributo data-title
QString TF2Search::ajustaNomeItemBagSC(QString item)
{
    QString itemOk;
    int pos = item.indexOf('>') + 1;
    itemOk = item.remove(0, pos);
    pos = itemOk.indexOf('<');
    itemOk = itemOk.remove(pos, itemOk.count());
    itemOk = itemOk.replace("&apos;","\'");//corrigindo apóstrofes
    return itemOk;
}

/*          VErsão MONITOR SCRAP
******************************************************************************************************

//Função chamada depois que o scrap.tf tiver sido carregado
void TF2Search::scCarregado()
{
   QString tabela = navegador->page()->mainFrame()->findFirstElement("table#itembanking-list").toInnerXml();
   disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(scCarregado()));
   geraTabelaSC(tabela);
   emit analiseSCConcluida();
}

//Isola e salva as informações presentes na tabela de preços do scrap.tf
void TF2Search::geraTabelaSC(QString tabela)
{
   int inic, cont=0;
   QString nome, num, estoq;
   float estoqF;

   tabela.remove("<i18n>");
   tabela.remove("</i18n>");
   inic = tabela.indexOf("<tr");
   //Cada passagem pelo laço analisa as informações de um item
   for(int i=inic; i>-1; cont++){
       nome.clear();
       num.clear();
       estoq.clear();
       i = tabela.indexOf("</td>",i);//ignora o ícone
       i = tabela.indexOf("<td>",i)+4;//início do nome
       for(int j=0; tabela[i]!='<'; i++, j++)//salvando o nome
           nome[j] = tabela[i];
       if(nome.indexOf("(Uncraftable)")==-1){//se o item for craftable
           //correções específicas
           if(itens_bp.indexOf(nome)==-1)//se o nome do item estiver errado, chama a função de correção
               corrigeNomeSC(nome);
           //---------------------------------
           i = tabela.indexOf("<td>",i)+4;//início do preço de venda do scrap.tf
           for(int j=0; tabela[i]!='<'; i++, j++)//salvando o preço de venda
               num[j] = tabela[i];
           num = num.section(QChar(10),0,0);//removendo caracteres de espaçamento no final da string
           /*i = tabela.indexOf("<td>",i)+4;//início do preço de compra do scrap.tf
           for(int j=0; tabela[i]!='<'; i++, j++)//salvando o preço de compra
               numC[j] = tabela[i];
           numC = numC.section(QChar(10),0,0);//removendo caracteres de espaçamento no final da string
           i = tabela.indexOf("div rel",i)+25;//início do estoque
           for(int j=0; tabela[i]!='"'; i++, j++)//salvando o estoque
               estoq[j] = tabela[i];
           //salvando os dados lidos nas listas da classe
           estoqF = getEstoqueF(estoq);
           if(estoqF>0.0){//se houver ao menos um item em estoque
               itens_sc.push_back(nome);
               precos_sc.push_back(convertePrecoSC(num));
               estoques_sc.push_back(estoqF);
           }
       }
       i = tabela.indexOf("<tr",i);
   }
}

//Função que corrige os nomes de itens escritos de maneira errada na tabela do Scrap.tf
void TF2Search::corrigeNomeSC(QString &nome)
{
   int indBP;

   if(nome.indexOf("Australium")!=-1){//no caso de australiums, insere strange na frente
       nome.insert(0,"Strange ");
       nome.replace("-a","-A");//force-a-nature
   }else if(nome.indexOf("Shred")!=-1)//(genuine) taunt shred alert
       nome.replace("Taunt: ","");
   else if(nome.indexOf("Conc")!=-1)//crate 93
       nome = "The Concealed Killer Weapons Case #93";
   else if(nome.indexOf("Power")!=-1)//crate 94
       nome = "The Powerhouse Weapons Case #94";
   else if(nome.indexOf("Level 1 o")!=-1)//max head
       nome = "Max's Severed Head";
   else{
       nome.replace("Bills","Bill's");//(vintage) bills hat
       nome.replace("ttin A","ttin' a");//taunt battin thousand
       nome.replace("ppin","ppin'");//taunt flippin
       nome.replace("Five","Five!");//taunt high 5
       nome.replace("Rock Paper Scissors","Rock, Paper, Scissors");//taunt jackenpo
       nome.replace("anns","ann's");//manns mint, gentlemanns business
       nome.replace("ters","ter's");//noble hatters violet
       nome.replace("tors","tor's");//operators overalls
       nome.replace("Zephaniahs","Zepheniah's");//zepheniahs greed
       nome.replace("Horace","Genuine Horace");
       nome.replace("Salvaged","Salvaged Mann Co. Supply");//crates
       nome.replace("Ube","Übe");//parts uber
   }indBP = itens_bp.indexOf(nome);//verificando se achou o item
   if(indBP==-1){//se ainda não achou
       //no caso de taunts, tentar inserindo the
       if(nome.indexOf("Taunt:")!=-1){
           int pos;
           for(pos=0; nome.at(pos)!=':'; pos++){}
           nome.insert(pos+2,"The ");
       }
       indBP = itens_bp.indexOf(nome);
       if(indBP==-1){//se ainda não achou
           //tentar tirando o 'The'
           nome.replace("The ","");
           indBP = itens_bp.indexOf(nome);
           //if(indBP==-1)//se ainda não achou
               //qDebug() << nome;
       }
   }
}

//Converte preços do sc.tf de string para inteiros. Formato ex.: "2 keys, 8 refined" -> 110000
int TF2Search::convertePrecoSC(QString txt)
{
   float nKeys=0.0, nRefs=0.0;

   if(txt.indexOf(',') != -1){//preço expresso em keys e refs
       nKeys = txt.section(',',0,0).section(' ',0,0).toFloat();
       nRefs = txt.section(',',1,1).section(' ',1,1).toFloat();
   }else if(txt.indexOf("key") != -1)//preço expresso somente em keys
       nKeys = txt.section(' ',0,0).toFloat();
   else if(txt.indexOf("refined") != -1)//preço expresso somente em refs
       nRefs = txt.section(' ',0,0).toFloat();

   return (int)(nKeys*PRECO_KEY+nRefs*PRECO_REF);
}

//Converte um estoque na forma XX/YY para um número real.
float TF2Search::getEstoqueF(QString estoq)
{
   QString num1, num2;
   int n1, n2, j=0;
   for(int i=0; estoq[j]!='/'; i++, j++)
       num1[i] = estoq[j];
   j++;
   for(int i=0; j<estoq.size(); i++, j++)
       num2[i] = estoq[j];
   n1 = num1.toInt();
   n2 = num2.toInt();

   if(n2 == 0)
       return 1;
   else
       return ((float)n1/n2);
}

// Compara os preços salvos na seção 'itens' do scrap.tf com aqueles aplicados pelo wh
void TF2Search::comparaTabelasItensSC()
{

}*/

//Função chamada depois que o scrap.tf tiver sido carregado
/*void TF2Search::sc_carregado()
{
    QString tabela = navegador->page()->mainFrame()->findFirstElement("table#itembanking-list").toInnerXml();
    gera_tabela_sc(tabela);
    disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(sc_carregado()));
    emit bolsas_bot_analisadas(indices_search);
}

void TF2Search::gera_tabela_sc(QString tabela)
{
    int inic = tabela.indexOf("<tr"), indWH, sucessos=0;
    QString nome, num, estoq;

    qDebug() << "inicio";
    //Cada passagem pelo laço analisa as informações de um item
    for(int i=inic; i>-1;){
        nome.clear();
        num.clear();
        estoq.clear();
        i = tabela.indexOf("</td>",i);//ignora o ícone
        i = tabela.indexOf("<td>",i)+4;//início do nome
        for(int j=0; tabela[i]!='<'; i++, j++)//salvando o nome
            nome[j] = tabela[i];
        if(nome.indexOf("(Uncraftable)")==-1 && nome.indexOf("Australium")==-1){//se o item for craftable
            //correções específicas
            if(nome.indexOf("Part:")!=-1)//strange (cosmetic) parts
                nome.replace("Strange ","");
            nome.replace("Bills","Bill's");//(vintage) bills hat
            nome.replace("ttin A","ttin' a");//taunt battin thousand
            nome.replace("ppin","ppin'");//taunt flippin
            nome.replace("Five","Five!");//taunt high 5
            nome.replace("Rock Paper Scissors","Rock, Paper, Scissors");//taunt high 5
            nome.replace("anns","ann's");//manns mint, gentlemanns business
            nome.replace("ters","ter's");//noble hatters violet
            nome.replace("tors","tor's");//operators overalls
            nome.replace("Zephaniahs","Zepheniah's");//zepheniahs greed
            nome.replace("Horace","Genuine Horace");
            nome.replace("Ube","Übe");//parts uber
            nome.replace("Salvaged","Salvaged Mann Co. Supply");//crates
            if(nome.indexOf("Shred")!=-1)//(genuine) taunt shred alert
                nome.replace("Taunt: ","");

            i = tabela.indexOf("<td>",i)+4;//início do preço de venda do scrap.tf
            for(int j=0; tabela[i]!='<'; i++, j++)//salvando o preço de venda
                num[j] = tabela[i];
            num = num.section(QChar(10),0,0);//removendo caracteres de espaçamento no final da string de preço
            i = tabela.indexOf("<td>",i)+4;//início do preço de compra do scrap.tf
            i = tabela.indexOf("</td>",i)+4;//igonora o preço de compra
            i = tabela.indexOf("title",i)+7;//início do estoque
            for(int j=0; tabela[i]!='"'; i++, j++)//salvando o estoque
                estoq[j] = tabela[i];
            indWH = itens_bp.indexOf(nome);
            if(indWH==-1){
                //no caso de taunts, tentar inserindo the
                if(nome.indexOf("Taunt:")!=-1){
                    int pos;
                    for(pos=0; nome.at(pos)!=':'; pos++){}
                    nome.insert(pos+2,"The ");
                }
                indWH = itens_bp.indexOf(nome);
                if(indWH==-1){
                    //tentar tirando o 'The'
                    nome.replace("The ","");
                    indWH = itens_bp.indexOf(nome);
                    if(indWH==-1)
                        qDebug() << nome;
                }
            }else
                sucessos++;
            indWH = itens_wh.indexOf(nome);
            //qDebug() << nome << num << converte_preco_sc(num) << indWH;
            if(estoques_wh.at(indWH) < 1.0 &&//se o estoque do wh não estiver cheio
               get_estoque(estoq) > 0.0 &&//se o scrap.tf tiver ao menos um exemplar do item
               precos_wh.at(indWH)-converte_preco_sc(num)>=LUCRO_MIN){//se o preço do scrap fornecer lucro
                int ind = resultadoV.indexOf(nome);
                if(ind!=-1 && indices_search.indexOf(ind/5)==-1)//se o índice do item já nao tiver sido salvo
                    indices_search.push_back(ind/5);//salva o índice da linha deste item na tabela
            }
        }
        i = tabela.indexOf("<tr",i);
    }qDebug() << "sucessos:" << sucessos;
}

//Converte preços do sc.tf de string para inteiros. Formato ex.: "2 keys, 8 refined" -> 110000
int TF2Search::converte_preco_sc(QString txt)
{
    float nKeys=0.0, nRefs=0.0;

    if(txt.indexOf(',') != -1){//preço expresso em keys e refs
        nKeys = txt.section(',',0,0).section(' ',0,0).toFloat();
        nRefs = txt.section(',',1,1).section(' ',1,1).toFloat();
    }else if(txt.indexOf("keys") != -1)//preço expresso somente em keys
        nKeys = txt.section(' ',0,0).toFloat();
    else if(txt.indexOf("refined") != -1)//preço expresso somente em refs
        nRefs = txt.section(' ',0,0).toFloat();

    return (int)(nKeys*PRECO_KEY+nRefs*PRECO_REF);
}*/

/******************************************************************************************************
 *          VERIFICAÇÕES DOS CLASSIFICADOS DO BP.TF
 ******************************************************************************************************/

void TF2Search::verificarListasBP()
{
    indice = 2;
    indicesSearch.clear();

    if (resultadoV.count() > 2) {
        connect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(bpClCarregado()));
        navegador->load(QUrl(geraLinkBPCl(resultadoV.at(indice))));
    } else
        emit classificadosBPAnalisados(indicesSearch);
}

void TF2Search::bpClCarregado()
{
    //Como a lista é ordenada em ordem crescente de preços, precisamos analisar apenas o primeiro anúncio. No código-fonte,
    //aparecem primeiro todos os anúncios de venda, seguidos de todos os de compra. Então, antes de tudo, verificamos
    //se a página contém algum anúncio de venda.
    qDebug() << "carregou bpcl do item " << resultadoV.at(indice);
    if(navegador->page()->mainFrame()->findAllElements("i.fa.fa-arrow-right.listing-intent-sell").count()>0)
    {
        qDebug() << "achou anúncio venda do item " << resultadoV.at(indice);
        //QString primPrecoStr = navegador->page()->mainFrame()->findFirstElement("span.equipped").toPlainText();
        QString primPrecoStr = navegador->page()->mainFrame()->findFirstElement("div.tag.bottom-right").toPlainText();
        int precoDesejado, ind, primPreco;
        ind = itensWH.indexOf(resultadoV.at(indice));//indice do item procurado na lista de itens do tf2wh
        precoDesejado = precosWH.at(ind) - LUCRO_MIN;//preço máximo desejado é a quantia paga pelo WH menos o lucro mínimo requerido.
        primPrecoStr.remove(0,1);//Remove o espaço inicial
        primPreco = convertePrecoBP(primPrecoStr);
        qDebug() << "preços:" << primPrecoStr << QString::number(primPreco) << QString::number(precoDesejado);
        //Preço deve ser menor ou igual que o desejado e algum anúncio tem que ter sido encontrado
        if(primPreco <= precoDesejado && !primPrecoStr.isNull())
        {
            qDebug() << "preço desejado encontrado, item " << resultadoV.at(indice);
            indicesSearch.push_back(indice/6);
        }
    }
    indice += 6;
    qDebug() << "divisao:" << QString::number(indice) << QString::number(resultadoV.size()) << QString::number(((float)indice / resultadoV.size() * 100));
    emit atualizarStatus(DS_CARREGANDO_BP + QString::number(((float)indice / resultadoV.size() * 100)) + "%");
    if(indice <= resultadoV.size())
        navegador->load(QUrl(geraLinkBPCl(resultadoV.at(indice))));
    else
    {
        disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(bpClCarregado()));
        emit classificadosBPAnalisados(indicesSearch);
    }
}

QString TF2Search::geraLinkBPCl(QString item)
{
    QString link;
    unsigned short codeTipo = getCodigoBP(item);
    getNomeItemPuro(item);
    if(item.indexOf("Crate #")!=-1){
        int ind=item.indexOf(" #");
        item.remove(ind,4);
    }
    link = "http://backpack.tf/classifieds?tradable=1&craftable=1&item=" + item;
    //Se não for item do tipo Unique, adiciona-se um sufixo no link
    if(codeTipo != 0)
        link += "&quality=" + QString::number(codeTipo);
    return link;
}

//Função que altera uma string, salvando apenas o nome do item sem o seu tipo.
void TF2Search::getNomeItemPuro(QString &item)
{
    item.remove("Strange ");
    item.remove("Haunted ");
    item.remove("Vintage ");
    item.remove("Genuine ");
}

unsigned short TF2Search::getCodigoBP(QString item)
{
    if(item.indexOf("Strange")!=-1)
        return 11;
    if(item.indexOf("Genuine")!=-1)
        return 1;
    if(item.indexOf("Haunted")!=-1)
        return 13;
    if(item.indexOf("Vintage")!=-1)
        return 3;
    return 0;
}

/******************************************************************************************************
 *          VERIFICAÇÕES DE ITENS EM ESCROW NO WH
 ******************************************************************************************************/

void TF2Search::verificarEscrowWH(QStringList itensReservados)
{
    qDebug() << "Itens reservados: " << itensReservados.count();
    indicesSearch.clear();
    linksWH.clear();
    indice = 0;
    this->itensReservados = itensReservados;
    itemReservado = false;
    connect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(escrowPageWHCarregada()));
    navegador->load(QUrl("https://www.tf2wh.com/allitems"));
}

void TF2Search::escrowPageWHCarregada()
{
    disconnect(navegador->page()->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(escrowPageWHCarregada()));

    for (int i = 1, j = 0; i < resultadoC.length(); i += 4, j++)
        if (navegador->page()->mainFrame()->findAllElements("div[data-name=\""+resultadoC[i]+"\"]").count() > 0)
            indicesSearch.push_back(j);

    if (indicesSearch.count() > 0)
    {
        indice = 0;
        salvarLinksWH();
        if (QString::number(indicesSearch.count()) != QString::number(linksWH.count()))
            registrarLog("Itens: " + QString::number(indicesSearch.count()) + " Links: " + QString::number(linksWH.count()));
        if (linksWH.count() > 0)
        {
            alteraOpcoesNavegador(true);
            carregarPaginaItem(indice);
        }
    }
    else
        emit escrowWHAnalisado(false, indicesSearch);

    //debug
    /*alteraOpcoesNavegador(true);
    carregarPaginaItem(0);*/
}

void TF2Search::salvarLinksWH()
{
    QWebElementCollection els = navegador->page()->mainFrame()->findAllElements("a.allitemsitename");
    for (int i = 0; i < els.count(); i++)
        for (int j = 0; j < indicesSearch.count(); j++)
            if (resultadoC[indicesSearch[j] * 4 + 1] == els[i].toPlainText())
                linksWH.append(els[i].attribute("href"));
}

void TF2Search::carregarPaginaItem(int pos)
{
    QString itemAtual = resultadoC[indicesSearch[pos] * 4 + 1];
    qDebug() << "itemAtual:" << itemAtual;
    if (itensReservados.contains(itemAtual))
    {
        qDebug() << "Item " + itemAtual + " já reservado.";
        registrarLog("Item " + itemAtual + " já reservado.");
        acessaProximoLink();
        return;
    }
    else
        registrarLog("Item encontrado: " + itemAtual + ".");
    relogio->start();
    connect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaItemWHCarregada()));
    navegador->load(QUrl(INICIO_LINK_WH + linksWH[indice]));
    //debug
    //navegador->load(QUrl("https://www.tf2wh.com/item/5002/6/430d648fb618d66f3dad945e2ea509bd"));
    //navegador->load(QUrl("https://www.tf2wh.com/item/5859/6/389d99f203d372a5f160f53fd6d3bc6f/mann-co-supply-munition"));
}

void TF2Search::paginaItemWHCarregada()
{
    if (relogio->elapsed() > 15000)
    {
        disconnect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaItemWHCarregada()));
        registrarLog("Pagina com problema, salvando html.");
        QFile arq(QDir::currentPath() + "/page.html");
        if(arq.open(QFile::WriteOnly | QFile::Text))
        {
            QTextStream out(&arq);
            out << navegador->page()->mainFrame()->toHtml();
            arq.close();
        }
        emit escrowWHAnalisado(false, indicesSearch);
        finalizaAnaliseEscrow();
        return;
    }

    if (relogio->elapsed() > 5000 &&
       (!navegador->page()->mainFrame()->findFirstElement("img#buybutton").isNull() ||
        !navegador->page()->mainFrame()->findFirstElement("div.notice").isNull() ||
        !navegador->page()->mainFrame()->findFirstElement("div.error.item").isNull()))
    {
        disconnect(navegador->page()->networkAccessManager(),SIGNAL(finished(QNetworkReply*)),this,SLOT(paginaItemWHCarregada()));
        itemReservado = reservarItem() ? true : itemReservado;
        registrarLog("Item reservado? " + QString::number(itemReservado));
        acessaProximoLink();
    }
}

void TF2Search::acessaProximoLink()
{
    indice++;
    qDebug() << "Indice: " + QString::number(indice);
    registrarLog("Indice: " + QString::number(indice));
    if (indice < linksWH.count())
        carregarPaginaItem(indice);
    else
    {
        qDebug() << "Todos os links foram acessados.";
        registrarLog("Todos os links foram acessados.");
        emit escrowWHAnalisado(itemReservado, indicesSearch);
        finalizaAnaliseEscrow();
    }
}

//Reservar item cuja página está carregada no navegador. Retorna true se reservou.
bool TF2Search::reservarItem()
{
    if (verificaItemUncraftableWH())
    {
        registrarLog("Item uncraftable, reserva não será efetuada.");
        return false;
    }
    QWebElement campoBulk = navegador->page()->mainFrame()->findFirstElement("input#bulkinput");
    QWebElement campoBotao = navegador->page()->mainFrame()->findFirstElement("img#buybutton");
    if (!campoBulk.isNull())
    {
        registrarLog("Efetuando bulk buy...");
        campoBulk.setAttribute("value", "10");
        campoBotao.setAttribute("quantity", "10");
    }
    if (!campoBotao.isNull())
    {
        registrarLog("Reserva efetuada.");
        campoBotao.evaluateJavaScript("this.click()");
        return true;
    }
    return false;
}

bool TF2Search::verificaItemUncraftableWH()
{
    QWebElementCollection campos = navegador->page()->mainFrame()->findAllElements("h2");
    for (int i = 0; i < campos.count(); i++)
        if (campos[i].toPlainText() == ITEM_UNCRAFTABLE_WH)
            return true;
    return false;
}

void TF2Search::finalizaAnaliseEscrow()
{
    indicesSearch.clear();
    linksWH.clear();
    itensReservados.clear();
    alteraOpcoesNavegador(false);
}

/******************************************************************************************************
 *          GETTERS E SETTERS
 ******************************************************************************************************/

QString TF2Search::getLink(QString item, int pos)
{
    QString link;

    for(int i=pos+item.size()+2, j=0; links[i+1].unicode()!=10; i++, j++)
        link[j] = links[i];

    return link;
}

int TF2Search::getTamanhoResultV()
{
    return resultadoV.size();
}

QString TF2Search::getResultadoVi(unsigned pos)
{
    return resultadoV.at(pos);
}

int TF2Search::getTamanhoResultC()
{
    return resultadoC.size();
}

QString TF2Search::getResultadoCi(unsigned pos)
{
    return resultadoC.at(pos);
}

void TF2Search::considerarEstoque(bool op)
{
    verEstoque = op;
}

void TF2Search::considerarEstoqueC(bool op)
{
    verEstoqueC = op;
}

void TF2Search::getPrecos(int &r, int &k, int &b)
{
    r = PRECO_REF;
    k = PRECO_KEY;
    b = PRECO_BUD;
}

void TF2Search::setCodigo(QString codigo)
{
    this->codigoMobile = codigo;
}

QString TF2Search::getLinkWH(QString item)
{
    return ("https://www.tf2wh.com/items?search=" + item.replace(" ","+"));
}

/******************************************************************************************************
 *          UTILITÁRIOS
 ******************************************************************************************************/

//Listas que podem ser limpas após a comparação entre tabelas WH e BP (análise principal).
void TF2Search::limpaListas()
{
    itensBP.clear();
    estoquesStr.clear();
    precosWHc.clear();
    precosBP.clear();
    estoquesWH.clear();
}

void TF2Search::alteraOpcoesNavegador(bool ativado)
{
    QWebSettings *globalSettings = navegador->settings();
    globalSettings->setAttribute(QWebSettings::AutoLoadImages, ativado);
    globalSettings->setAttribute(QWebSettings::JavascriptEnabled, ativado);
    globalSettings->setAttribute(QWebSettings::JavaEnabled, ativado);
    globalSettings->setAttribute(QWebSettings::PluginsEnabled, ativado);
    globalSettings->setAttribute(QWebSettings::JavascriptCanOpenWindows, ativado);
    globalSettings->setAttribute(QWebSettings::JavascriptCanCloseWindows, ativado);
    globalSettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, ativado);
    globalSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, ativado);
    globalSettings->setAttribute(QWebSettings::SpatialNavigationEnabled, ativado);
    globalSettings->setAttribute(QWebSettings::PrintElementBackgrounds, ativado);
}

bool TF2Search::opcoesNavegadorAtivas()
{
    return navegador->settings()->testAttribute(QWebSettings::AutoLoadImages);
}

void TF2Search::registrarLog(QString msg)
{
    qDebug() << msg;
    emit registrarRelatorio(msg);
    emit atualizarStatus(msg);
}
