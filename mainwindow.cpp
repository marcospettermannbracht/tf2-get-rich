#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->txt_nItensV->setFocusPolicy(Qt::NoFocus);
    ui->txt_nEncontradosV->setFocusPolicy(Qt::NoFocus);
    ui->txt_nItensC->setFocusPolicy(Qt::NoFocus);
    ui->botao_reservado->setVisible(false);
    ui->botao_reservado->setStyleSheet("QPushButton{ color: red; }");
    //ui->txt_nEncontradosC->setFocusPolicy(Qt::NoFocus);
    ui->chkMonitorar->setStyleSheet("QCheckBox:unchecked{ color: black; }QCheckBox:checked{ color: red; }");

    lbStatus = new QLabel();
    ui->statusBar->addWidget(lbStatus);

    lbStatus->setText(DS_CARREGANDO);

    setMinimumWidth(780);
    setMinimumHeight(650);
    setWindowIcon(QIcon(QPixmap(":/new/prefix1/moneyHeavy.png")));
    setWindowTitle("Fica Rico Poha!");

    tf2 = new TF2Search(ui->actionConsiderar_Estoque->isChecked(), ui->actionConsiderar_Estoque_C->isChecked(),
                        ui->actionLucro->isChecked() ? 0 : 1);
    modoAll = false;
    relogio = new QTimer(this);
    relogio->setInterval(TEMPO_CONSULTA*1000);
    log = new LogHelper;

    connect(tf2,SIGNAL(analisesConcluidas(bool)),this,SLOT(preencherTabela(bool)));
    connect(tf2,SIGNAL(classificadosBPAnalisados(QVector<int>&)),this,SLOT(verResultBP(QVector<int>&)));
    connect(tf2,SIGNAL(bolsasBotAnalisadas(bool,QVector<int>&)),this,SLOT(destacarItensSC(bool,QVector<int>&)));
    connect(tf2,SIGNAL(escrowWHAnalisado(bool,QVector<int>&)),this,SLOT(destacarItensEscrowWH(bool,QVector<int>&)));
    connect(tf2,SIGNAL(atualizarStatus(QString)),this,SLOT(atualizaStatus(QString)));
    connect(tf2,SIGNAL(registrarRelatorio(QString)),log,SLOT(registrar(QString)));
    connect(ui->tabela,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(abrirLink(int)));
    connect(ui->tabelaCompra,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(abrirLinkCompra(int)));
    connect(ui->actionLucro,SIGNAL(toggled(bool)),this,SLOT(ordemLucroSelecionada()));
    connect(ui->actionPercentual,SIGNAL(toggled(bool)),this,SLOT(ordemPorcentSelecionada()));
    connect(ui->actionOutpost,SIGNAL(toggled(bool)),this,SLOT(linkOutpostSelecionado()));
    connect(ui->actionBP_Classifieds,SIGNAL(toggled(bool)),this,SLOT(linkBPClassifiedSelecionado()));
    connect(ui->actionConsiderar_Estoque_C,SIGNAL(toggled(bool)),SLOT(considerarEstoqueCompraChanged()));
    connect(ui->actionPrecos_Atuais,SIGNAL(triggered(bool)),this,SLOT(mostrarPrecos()));

    log->registrar("Programa iniciado.");
    connect(relogio,SIGNAL(timeout()),this,SLOT(on_botao_escrow_clicked()));
    tf2->vamoTrabalhar();
}

MainWindow::~MainWindow()
{
    delete log;
    delete relogio;
    delete tf2;
    delete ui;
}

void MainWindow::preencherTabela(bool sucesso)
{
    finalBusca();
    if (!sucesso)
    {
        if (pedeAutenticacaoMobile())
            on_botao_refresh_clicked();
        return;
    }
    QString texto;
    int tam, nItensOld[2];

    nItensOld[0] = ui->tabela->rowCount();
    nItensOld[1] = ui->tabelaCompra->rowCount();

    //Limpar valores armazenados
    ui->tabela->setRowCount(0);
    ui->tabela->clearContents();
    ui->tabelaCompra->setRowCount(0);
    ui->tabelaCompra->clearContents();
    links.clear();

    //Preenchimento das tabelas
    tam = tf2->getTamanhoResultC();
    ui->tabelaCompra->setRowCount(tam/4);
    for(int i=0; i<tam; i++){
        QTableWidgetItem *item;
        texto = tf2->getResultadoCi(i);
        item = new QTableWidgetItem(texto);
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tabelaCompra->setItem(i/4,i%4,item);
    }
    tam = tf2->getTamanhoResultV();
    ui->tabela->setRowCount(tam/6);
    for(int i=0; i<tam; i++){
        QTableWidgetItem *item;
        texto = tf2->getResultadoVi(i);
        if(texto == "SEM_LINK"){
            links << " ";
            texto = " ";
        }else if(i%6 == 5){
            links << texto;
            texto = "X";
        }
        item = new QTableWidgetItem(texto);
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tabela->setItem(i/6,i%6,item);
    }

    //Laço que destaca os itens com estoque menor que 100%
    if(modoAll){
        for(int i=0; i<ui->tabela->rowCount(); i++){
            QString estoq = ui->tabela->item(i,4)->text();
            if(estoq.section('/',0,0).toInt() < estoq.section('/',1,1).toInt()){
                for(int j=0; j<5; j++)
                    ui->tabela->item(i,j)->setBackgroundColor(QColor(Qt::green));
            }
        }
    }

    //chamada que mostra o número total de itens em cada aba
    preencheNumItens(0, nItensOld[0]);
    preencheNumItens(1, nItensOld[1]);
    //zera o número de itens encontrados por pesquisas adicionais
    mostrarNumItensEncontrados(0, 0);

    lbStatus->setText(DS_PRONTO);
    QApplication::setActiveWindow(this);
}

//Retorna true se o código for válido
bool MainWindow::pedeAutenticacaoMobile()
{
    QApplication::setActiveWindow(this);
    this->codigoMobile = QInputDialog::getText(this, "Autenticação Móvel", "Digite o código atualmente exibido no aplicativo móvel da Steam.");
    if (!this->codigoMobile.isNull() && !this->codigoMobile.isEmpty())
    {
        tf2->setCodigo(this->codigoMobile);
        return true;
    }
    return false;
}

void MainWindow::on_botao_refresh_clicked()
{
    if(lbStatus->text() == DS_PRONTO)
    {
        lbStatus->setText(DS_CARREGANDO);
        delete tf2;
        tf2 = new TF2Search(ui->actionConsiderar_Estoque->isChecked(), ui->actionConsiderar_Estoque_C->isChecked(),
                            ui->actionLucro->isChecked() ? 0 : 1, this->codigoMobile);
        this->codigoMobile.clear();
        modoAll = !ui->actionConsiderar_Estoque->isChecked();
        inicioBusca();
        connect(tf2,SIGNAL(analisesConcluidas(bool)),this,SLOT(preencherTabela(bool)));
        connect(tf2,SIGNAL(classificadosBPAnalisados(QVector<int>&)),this,SLOT(verResultBP(QVector<int>&)));
        connect(tf2,SIGNAL(bolsasBotAnalisadas(bool,QVector<int>&)),this,SLOT(destacarItensSC(bool,QVector<int>&)));
        connect(tf2,SIGNAL(escrowWHAnalisado(bool,QVector<int>&)),this,SLOT(destacarItensEscrowWH(bool,QVector<int>&)));
        connect(tf2,SIGNAL(atualizarStatus(QString)),this,SLOT(atualizaStatus(QString)));
        connect(tf2,SIGNAL(registrarRelatorio(QString)),log,SLOT(registrar(QString)));
        tf2->vamoTrabalhar();
        //connect(tf2,SIGNAL(numero_trocas_contado(uint)),this,SLOT(salvar_n_trocas(uint)));
    }
    //tf2->testeLogin();
}

void MainWindow::abrirLink(int lin)
{
    //Deixando o nome do item disponível no clipboard do windows (mesmo efeito de copiar com CTRL+C)
    QClipboard *ctrl = QApplication::clipboard();
    ctrl->setText(ui->tabela->item(lin,2)->text());

    //Se a opção de abrir links de busca do OP estiver ativa
    if(ui->actionOutpost->isChecked()){
        if(links[lin] != " ")
            QDesktopServices::openUrl(links[lin]);
        else{
            QDesktopServices::openUrl(QUrl("http://www.tf2outpost.com/search"));
            JanelaAddLink w;
            w.set_nome_item(ui->tabela->item(lin,2)->text());
            w.exec();
            if(w.link_foi_digitado()){
                QFile arq(QDir::currentPath()+"/links.txt");
                if(arq.open(QFile::Append | QFile::Text)){
                    QTextStream out(&arq);
                    out << "-" + ui->tabela->item(lin,2)->text() + "-" + w.get_link() + "\n";
                    arq.close();
                }
            }
        }
    }
    //Se a opção de abrir links de busca do BP estiver ativa
    else
        QDesktopServices::openUrl(QUrl(tf2->geraLinkBPCl(ui->tabela->item(lin,2)->text())));
}

void MainWindow::abrirLinkCompra(int lin)
{
    //Deixando o nome do item disponível no clipboard do windows (mesmo efeito de copiar com CTRL+C)
    QClipboard *ctrl = QApplication::clipboard();
    ctrl->setText(ui->tabelaCompra->item(lin,1)->text());

    QDesktopServices::openUrl(QUrl(tf2->geraLinkBPCl(ui->tabelaCompra->item(lin,1)->text())));
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    //QSize tam_botao(width()/7,25);
    int largValida = width()*0.85; //y_atual;

    //ui->tabela->setGeometry(0,0,width(),height()*0.8);
    for(int i=0; i<ui->tabela->columnCount(); i++){
        if(i == 2)
            ui->tabela->setColumnWidth(i,largValida/3);
        else
            ui->tabela->setColumnWidth(i,largValida*2/15);
    }
    for(int i=0; i<ui->tabelaCompra->columnCount(); i++){
        if(i == 1)
            ui->tabelaCompra->setColumnWidth(i,largValida/2);
        else
            ui->tabelaCompra->setColumnWidth(i,largValida/6);
    }
}

void MainWindow::on_botao_sc_clicked()
{
    if(lbStatus->text() == DS_PRONTO && !modoAll)
    {
        lbStatus->setText(DS_CARREGANDO_SC);
        inicioBusca();
        tf2->verificarBotsSC();
    }
}

void MainWindow::destacarItensSC(bool sucesso, QVector<int>& inds)
{
    if (inicioAnalise)
    {
        finalBusca();
        if (!sucesso) //Necessária autenticação na steam
        {
            if (pedeAutenticacaoMobile())
                on_botao_sc_clicked();
            return;
        }
        //Limpa as marcações de itens
        limparMarcacaoItens(0);
        //Destaca itens encontrados
        for(int i=0; i<inds.size(); i++)
            for(int j=0; j<ui->tabela->columnCount(); j++)
                ui->tabela->item(inds.at(i),j)->setBackgroundColor(QColor(Qt::green));
        mostrarNumItensEncontrados(0, inds.count());
    }
}

void MainWindow::on_botao_bp_clicked()
{
    if(lbStatus->text() == DS_PRONTO && !modoAll)
    {
        lbStatus->setText(DS_CARREGANDO_BP);
        inicioBusca();
        tf2->verificarListasBP();
    }
}

void MainWindow::verResultBP(QVector<int>& inds)
{
    if (inicioAnalise)
    {
        finalBusca();
        //Se algum item foi encontrado no Bp.tf, muda o padrão de abertura de links
        if(!inds.isEmpty()){
            ui->actionBP_Classifieds->setChecked(true);
            ui->actionOutpost->setChecked(false);//agora, o padrão será abrir links de busca do BP classifieds.
        }
        //Limpa as marcações de itens
        limparMarcacaoItens(0);
        //Destaca itens encontrados
        for(int i=0; i<inds.size(); i++)
            for(int j=0; j<ui->tabela->columnCount(); j++)
                ui->tabela->item(inds.at(i),j)->setBackgroundColor(QColor(Qt::yellow));
        mostrarNumItensEncontrados(0, inds.count());
    }
}

void MainWindow::limparMarcacaoItens(short aba)
{
    if (aba == 0)
        for(int i=0; i<ui->tabela->rowCount(); i++)
            for(int j=0; j<ui->tabela->columnCount(); j++)
                ui->tabela->item(i,j)->setBackgroundColor(QColor(Qt::white));
    else
        for(int i=0; i<ui->tabelaCompra->rowCount(); i++)
            for(int j=0; j<ui->tabelaCompra->columnCount(); j++)
                ui->tabelaCompra->item(i,j)->setBackgroundColor(QColor(Qt::white));
}

void MainWindow::on_botao_escrow_clicked()
{
    if(lbStatus->text() == DS_PRONTO)
    {
        lbStatus->setText(DS_CARREGANDO_WH);
        inicioBusca();
        tf2->verificarEscrowWH(getItensReservados());
    }
}

void MainWindow::destacarItensEscrowWH(bool reservado, QVector<int> &inds)
{
    if (inicioAnalise)
    {
        log->registrar("Fim da análise escrow.");
        finalBusca();
        //Destaca itens encontrados
        for(int i=0; i<inds.size(); i++)
            for(int j=0; j<ui->tabelaCompra->columnCount(); j++)
                ui->tabelaCompra->item(inds.at(i),j)->setBackgroundColor(QColor(Qt::green));
        //mostrarNumItensEncontrados(1, inds.count());
        ui->botao_reservado->setVisible(ui->botao_reservado->isVisible() ? true : reservado);
        if (ui->botao_reservado->isVisible())
        {
            QApplication::setActiveWindow(this);
            tocarSom();
        }
        //log->registrar("Análise de itens à venda no WH finalizada.");
    }
}

void MainWindow::inicioBusca()
{
    inicioAnalise = true;
    ui->botao_refresh->setEnabled(false);
    ui->botao_bp->setEnabled(false);
    ui->botao_sc->setEnabled(false);
    ui->botao_escrow->setEnabled(false);
    ui->chkMonitorar->setEnabled(false);
}

void MainWindow::finalBusca()
{
    inicioAnalise = false;
    lbStatus->setText(DS_PRONTO);
    ui->botao_refresh->setEnabled(true);
    ui->botao_bp->setEnabled(true);
    ui->botao_sc->setEnabled(true);
    ui->botao_escrow->setEnabled(true);
    ui->chkMonitorar->setEnabled(true);
}

void MainWindow::preencheNumItens(short aba, int contOld)
{
    QPalette palette;
    QFont fonte;
    QTableWidget *tabela;
    QLineEdit *campo;

    if (aba == 0)
    {
        tabela = ui->tabela;
        campo = ui->txt_nItensV;
    }
    else
    {
        tabela = ui->tabelaCompra;
        campo = ui->txt_nItensC;
    }

    fonte = campo->font();
    if (tabela->rowCount() > contOld)
    {
        fonte.setBold(true);
        palette.setColor(QPalette::Base,Qt::green);
    }
    else if (tabela->rowCount() < contOld)
    {
        fonte.setBold(true);
        palette.setColor(QPalette::Base,Qt::red);
    }
    else
    {
        fonte.setBold(false);
        palette.setColor(QPalette::Base,Qt::white);
    }
    palette.setColor(QPalette::Text,Qt::black);
    campo->setPalette(palette);
    campo->setFont(fonte);
    campo->setText(QString::number(tabela->rowCount()));
}

void MainWindow::mostrarNumItensEncontrados(short aba, int num)
{
    QLineEdit *campo;
    campo = ui->txt_nEncontradosV;
    //Preenchendo campo contendo o número de registros destacados
    QPalette palette;
    QFont fonte = campo->font();
    campo->setText(QString::number(num));
    palette.setColor(QPalette::Base,Qt::white);
    //Se algum item foi encontrado, faz esta janela piscar na barra de tarefas e muda a cor do campo de número de itens encontrados.
    if(num > 0)
    {
        palette.setColor(QPalette::Base,Qt::green);
        QApplication::setActiveWindow(this);
    }
    campo->setPalette(palette);
    campo->setFont(fonte);
}

void MainWindow::on_chkMonitorar_stateChanged()
{
    if (ui->chkMonitorar->isChecked())
    {
        QFont fonte = ui->chkMonitorar->font();
        fonte.setBold(true);
        ui->chkMonitorar->setFont(fonte);
        ui->chkMonitorar->setText("Monitorando");
        this->on_botao_escrow_clicked();
        relogio->start();
    }
    else
    {
        QFont fonte = ui->chkMonitorar->font();
        fonte.setBold(false);
        ui->chkMonitorar->setFont(fonte);
        ui->chkMonitorar->setText("Monitorar");
        relogio->stop();
    }
}

void MainWindow::abrirLinkWH(QString item)
{
    QDesktopServices::openUrl(QUrl(tf2->getLinkWH(item)));
}

void MainWindow::considerarEstoqueCompraChanged()
{
    ui->btnNext->setVisible(!ui->actionConsiderar_Estoque_C->isChecked());
}

void MainWindow::on_btnNext_clicked()
{
    int inic = ui->tabelaCompra->currentRow();
    qDebug() << "atual" << inic;
    for (int i = inic + 1; i < ui->tabelaCompra->rowCount(); i++)
    {
        if (ui->tabelaCompra->item(i, 0)->backgroundColor() == Qt::green)
        {
            ui->tabelaCompra->scrollToItem(ui->tabelaCompra->item(i, 0));
            return;
        }
    }
    if (inic > 0)
    {
        for (int i = 0; i < inic; i++)
        {
            if (ui->tabelaCompra->item(i, 0)->backgroundColor() == Qt::green)
            {
                ui->tabelaCompra->scrollToItem(ui->tabelaCompra->item(i, 0));
                return;
            }
        }
    }
}

//Impedindo que os dois tipos de ordenação sejam selecionados.
void MainWindow::ordemLucroSelecionada()
{
    if (ui->actionLucro->isChecked() && ui->actionPercentual->isChecked())
        ui->actionPercentual->setChecked(false);
    else if (!ui->actionLucro->isChecked() && !ui->actionPercentual->isChecked())
        ui->actionLucro->setChecked(true);
}
void MainWindow::ordemPorcentSelecionada()
{
    if (ui->actionLucro->isChecked() && ui->actionPercentual->isChecked())
        ui->actionLucro->setChecked(false);
    else if (!ui->actionLucro->isChecked() && !ui->actionPercentual->isChecked())
        ui->actionPercentual->setChecked(true);
}

//Impedindo que os dois tipos de link sejam selecionados.
void MainWindow::linkOutpostSelecionado()
{
    if (ui->actionOutpost->isChecked() && ui->actionBP_Classifieds->isChecked())
        ui->actionBP_Classifieds->setChecked(false);
    else if (!ui->actionOutpost->isChecked() && !ui->actionBP_Classifieds->isChecked())
        ui->actionOutpost->setChecked(true);
}
void MainWindow::linkBPClassifiedSelecionado()
{
    if (ui->actionOutpost->isChecked() && ui->actionBP_Classifieds->isChecked())
        ui->actionOutpost->setChecked(false);
    else if (!ui->actionOutpost->isChecked() && !ui->actionBP_Classifieds->isChecked())
        ui->actionBP_Classifieds->setChecked(true);
}

void MainWindow::mostrarPrecos()
{
    if(lbStatus->text() == DS_PRONTO)
    {
        int ref, key, bud;
        tf2->getPrecos(ref,key,bud);

        QMessageBox::information(this, "Preços", "REF: " + QString::number(ref) + " - KEY: " + QString::number(key) + " - BUD: " + QString::number(bud) +
                                       "\n1 KEY = " + QString::number((float)key/ref,'f',2) + " REFS" +
                                       "\n1 BUD = " + QString::number((float)bud/key,'f',2) + " KEYS", QMessageBox::Ok);
    }
}

void MainWindow::on_botao_reservado_clicked()
{
    limparMarcacaoItens(1);
    ui->botao_reservado->setVisible(false);
    QDesktopServices::openUrl(QUrl("https://www.tf2wh.com"));
}

void MainWindow::atualizaStatus(QString msg)
{
    lbStatus->setText(msg);
}

void MainWindow::tocarSom()
{
    QSound::play(":/new/prefix1/beep.wav");
}

QStringList MainWindow::getItensReservados()
{
    QStringList nomes;
    QColor verde(Qt::green);
    for (int i = 0; i < this->ui->tabelaCompra->rowCount(); i++)
    {
        QTableWidgetItem *item = this->ui->tabelaCompra->item(i, 1);
        if (item->backgroundColor() == verde)
        {
            log->registrar("Item já reservado: " + item->text());
            nomes.append(item->text());
        }
    }
    return nomes;
}
