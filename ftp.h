#ifndef FTP_H
#define FTP_H

#include <QObject>
#include <QtCore>
#include <QDebug>
#include <curl/curl.h>

struct ftpFile{
    bool isDir;
    QString filename;
    int size;
    QDateTime date;
};

typedef QVector<ftpFile> ftpList;

class FTP : public QObject
{
    Q_OBJECT
private:
    QString username;
    QString password;
    QString url;
    QString proxyurl;

public:
    explicit FTP(QObject *parent = 0);
    void setUrl(QString ftpUrl);
    void setCreditinals(QString user="",QString pass=""){username=user;password=pass;}
    long returnCode;
    int curl_progress(void *p,double dltotal, double dlnow, double ultotal, double ulnow);
    int curl_debug(curl_infotype type, char *data, size_t size);

    ftpList getFileList(QString dir);
    int downloadFile(QString path, QString saveTo, bool reuse=false);
    int downloadDir(QString path, QString saveTo, ftpList fileList);
    int uploadFile(QString path, QString file, bool reuse=false);
    int uploadDir(QString path, QString dir, ftpList fileList);
    int deleteFile(QString path);
signals:
    void log(QString text);
    void error(QString text);
    void dump(QString text);
    void test();
    void progress(int total,int current);
private slots:
    void logSlot(QString text){qDebug()<<text;}
public slots:

};

#endif // FTP_H
