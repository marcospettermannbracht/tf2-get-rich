#include "loghelper.h"

LogHelper::LogHelper(QObject *parent) : QObject(parent)
{

}

void LogHelper::registrar(QString msg)
{
    mensagem.append(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss") + " - " + msg + "\n");
}

LogHelper::~LogHelper()
{
    QFile arq(QDir::currentPath() + "/log.txt");
    if(arq.open(QFile::Append | QFile::Text))
    {
        QTextStream out(&arq);
        out << mensagem;
        arq.close();
    }
}
