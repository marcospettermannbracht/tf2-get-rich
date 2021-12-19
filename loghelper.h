#ifndef LOGHELPER_H
#define LOGHELPER_H

#include <QObject>
#include <QDir>
#include <QDateTime>
#include <QTextStream>

class LogHelper : public QObject
{
    Q_OBJECT
private:
    QString mensagem;

public:
    explicit LogHelper(QObject *parent = 0);
    ~LogHelper();

public slots:
    void registrar(QString msg);
};

#endif // LOGHELPER_H
