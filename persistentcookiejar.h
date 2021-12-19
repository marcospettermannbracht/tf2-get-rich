#ifndef PERSISTENTCOOKIEJAR_H
#define PERSISTENTCOOKIEJAR_H

#include <QNetworkCookieJar>
#include <QMutexLocker>
#include <QNetworkCookie>
#include <QSettings>

class PersistentCookieJar : public QNetworkCookieJar {
public:
    PersistentCookieJar(QObject *parent) : QNetworkCookieJar(parent)
    {
        load();
    }

    ~PersistentCookieJar()
    {
        //Comentado para que o tamanho dos cookies não aumente excessivamente
        save();
    }

    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const
    {
        QMutexLocker lock(&mutex);
        return QNetworkCookieJar::cookiesForUrl(url);
    }

    virtual bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
    {
        QMutexLocker lock(&mutex);
        return QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
    }

private:
    //Comentado para que o tamanho dos cookies não aumente excessivamente
    void save()
    {
        QMutexLocker lock(&mutex);
        QList<QNetworkCookie> list = allCookies();
        QByteArray data;
        foreach (QNetworkCookie cookie, list) {
            if (!cookie.isSessionCookie()) {
                data.append(cookie.toRawForm());
                data.append("\n");
            }
        }
        QSettings settings("Marcos", "Fica Rico");
        settings.setValue("Cookies",data);
        settings.sync();
        qDebug() << "entrou no save" << data.count() << settings.contains("Cookies");
    }

    void load()
    {
        QMutexLocker lock(&mutex);
        QSettings settings("Marcos", "Fica Rico");
        QByteArray data = settings.value("Cookies").toByteArray();
        qDebug() << "entrou no load" << settings.contains("Cookies") << data.count();
        setAllCookies(QNetworkCookie::parseCookies(data));
    }

    mutable QMutex mutex;
};

#endif // PERSISTENTCOOKIEJAR_H
