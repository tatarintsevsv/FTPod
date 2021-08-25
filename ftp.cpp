#include "ftp.h"
#include "ftpparse.c"
#define beVerbose 1L
QByteArray data;
FTP* ftpClient;
CURL *curl;
CURLcode res;

static int curl_trace(CURL *handle, curl_infotype type, char *data, size_t size,void *userp)
{
    Q_UNUSED(userp)Q_UNUSED(handle)
    ftpClient->curl_debug(type,data,size);
    return 0;
}
int FTP::curl_debug(curl_infotype type, char *data, size_t size)
{
    QString text;
    switch((int)type){
    case CURLINFO_TEXT:
      text = QString("* %1").arg(QString::fromUtf8(data,size));
      break;
    case CURLINFO_HEADER_OUT:
      text = QString("> %1").arg(QString::fromUtf8(data,size));
      break;
    case CURLINFO_HEADER_IN:
      text = QString("< %1").arg(QString::fromUtf8(data,size));
      break;
    //case CURLINFO_DATA_OUT:
    //  text = "=> Send data";
    //  break;
    //case CURLINFO_SSL_DATA_OUT:
    //  text = "=> Send SSL data";
    //  break;
    //case CURLINFO_DATA_IN:
    //  text = "<= Recv data";
    //  break;
    //case CURLINFO_SSL_DATA_IN:
    //  text = "<= Recv SSL data";
    //  break;
    }
    if(!text.trimmed().isEmpty())
        emit dump(text.trimmed());
    return 0;
}

static size_t curl_data(void *ptr, size_t size, size_t nmemb, void* userdata){    
    Q_UNUSED(userdata)
    QByteArray x = QByteArray((char*)ptr,size*nmemb);
    data.append(x);    
    return size*nmemb;
}
static size_t curl_download(void *buffer, size_t size, size_t nmemb, QFile *file)
{
    return file->write((char*)buffer,size*nmemb);
}
static size_t progress_callback(void *clientp,   double dltotal,   double dlnow,   double ultotal,   double ulnow){
    return ftpClient->curl_progress(clientp, dltotal,dlnow, ultotal, ulnow);;
}

static size_t curl_uploaddata(char *ptr, size_t size, size_t nmemb, void *stream)
{
    QFile* f = (QFile*)stream;
    size_t retcode = f->read(ptr, size*nmemb);
    return retcode;
}


int FTP::curl_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
{
    Q_UNUSED(p)
    qApp->processEvents();
    if(dltotal==0&&ultotal==0)
        return;
    if(dltotal){
        emit progress(dltotal,dlnow);
    }else{
        if(ultotal){
            emit progress(ultotal,ulnow);
        }
    }
    qApp->processEvents();
    return 0;
}



FTP::FTP(QObject *parent) : QObject(parent)
{
    username=password=url="";
#ifdef QT_DEBUG
    connect(this,SIGNAL(log(QString)),this,SLOT(logSlot(QString)));
    connect(this,SIGNAL(error(QString)),this,SLOT(logSlot(QString)));
    connect(this,SIGNAL(dump(QString)),this,SLOT(logSlot(QString)));
#endif
}

void FTP::setUrl(QString ftpUrl)
{
    url=ftpUrl;
    if(!url.startsWith("ftp://",Qt::CaseInsensitive)&&!url.startsWith("sftp://",Qt::CaseInsensitive))
        url = "ftp://"+url;
    if(!url.endsWith("/",Qt::CaseInsensitive))
        url+="/";
}

ftpList FTP::getFileList(QString dir)
{
    ftpList list;
    CURL *curl;
    CURLcode res;
    if(!dir.endsWith("/"))dir+="/";
    curl = curl_easy_init();
    if(curl) {
        data.clear();
        if(!username.isEmpty()){
            curl_easy_setopt(curl, CURLOPT_USERNAME, username.toUtf8().data());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password.toUtf8().data());
        }
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_trace);
        curl_easy_setopt(curl, CURLOPT_VERBOSE,beVerbose);
        curl_easy_setopt(curl, CURLOPT_URL, QString("%1%2").arg(url).arg(dir).toUtf8().data());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,curl_data);
        emit log(QString::fromLocal8Bit("Получение списка файлов в %1").arg(dir));
        res = curl_easy_perform(curl);
        emit progress(0,0);
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &returnCode);
        if(res != CURLE_OK)
          emit error(QString(curl_easy_strerror(res)));
        curl_easy_cleanup(curl);
        struct ftpparse fp;
        foreach (QByteArray str, data.split('\n')) {
            int r =  ftpparse(&fp,str.data(),str.length());
            if(r==1){
                ftpFile f;
                f.filename = QString((char*)fp.name).trimmed();
                f.isDir = fp.flagtrycwd;
                f.size=fp.size;
                f.date= QDateTime::fromTime_t(fp.mtime);
                list.append(f);
            };
        };
    }else{
        emit error("Init error");

    }
    return list;

}

int FTP::downloadFile(QString path, QString saveTo,bool reuse)
{
    if(!reuse)
        curl = curl_easy_init();
    if(curl) {
        QFile f(saveTo);
        if(!f.open(QIODevice::WriteOnly)){
            emit error(QString("Error opening file for write '%1': %2").arg(saveTo).arg(f.errorString()));
            curl_easy_cleanup(curl);
            return 0;
        }
        curl_easy_setopt(curl, CURLOPT_URL,QString("%1%2").arg(url).arg(path).toUtf8().data());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_download);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &f);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, beVerbose);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_trace);
        if(!username.isEmpty()){
            curl_easy_setopt(curl, CURLOPT_USERNAME, username.toUtf8().data());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password.toUtf8().data());
        }
        curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);
        res = curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
        qDebug()<<QString(curl_easy_strerror(res));
        emit log(QString::fromLocal8Bit("Скачивание файла: %1").arg(path));
        res = curl_easy_perform(curl);
        emit progress(0,0);
        double speed_download, total_time;        
        curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &speed_download);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &returnCode);
        emit log(QString::fromLocal8Bit("Обработка запроса завершена. Скорость %1 байт/сек в течение %2 сек. Результат: %3")
                 .arg(speed_download)
                 .arg(total_time)
                 .arg(returnCode));

        if(!reuse)
            curl_easy_cleanup(curl);
        f.close();
        if(CURLE_OK != res) {
            f.remove();
            emit error(QString(curl_easy_strerror(res)));
            return 0;
        }

        return returnCode;
    }
    emit error("Init error");
    return 0;
}

int FTP::downloadDir(QString path, QString saveTo,ftpList fileList)
{
    if(fileList.isEmpty())
        fileList = getFileList(path);
    if(!path.endsWith("/"))path+="/";
    int ret = 1;
    curl = curl_easy_init();
    foreach (ftpFile f, fileList) {
        if(!f.isDir)
            ret &= downloadFile(QString("%1%2").arg(path).arg(f.filename),QString("%1/%2").arg(saveTo).arg(f.filename),true);
    }
    curl_easy_cleanup(curl);
    return ret;
}

int FTP::uploadFile(QString path, QString file, bool reuse)
{
    QFile fi(file);
    if(!fi.open(QIODevice::ReadOnly)){
        emit error(QString::fromLocal8Bit("Ошибка открытия файла %1: %2").arg(file).arg(fi.errorString()));
        return 1;
    }
    QFileInfo finf(fi);
    if(!reuse)
        curl = curl_easy_init();
    if(curl) {
      curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_uploaddata);
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
      curl_easy_setopt(curl, CURLOPT_URL, QString("%1%2%3").arg(url).arg(path).arg(finf.fileName()).toUtf8().data());
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_READDATA, &fi);
      curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)finf.size());
      curl_easy_setopt(curl, CURLOPT_VERBOSE, beVerbose);
      curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_trace);
      if(!username.isEmpty()){
          curl_easy_setopt(curl, CURLOPT_USERNAME, username.toUtf8().data());
          curl_easy_setopt(curl, CURLOPT_PASSWORD, password.toUtf8().data());
      }
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS,0L);
      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
      emit log(QString::fromLocal8Bit("Отправка файла: %1").arg(path));
      res = curl_easy_perform(curl);
      emit progress(0,0);
      fi.close();
      double speed_download, total_time;
      curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_download);
      curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
      curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &returnCode);
      emit log(QString::fromLocal8Bit("Обработка запроса завершена. Скорость %1 байт/сек в течение %2 сек. Результат: %3")
               .arg(speed_download)
               .arg(total_time)
               .arg(returnCode));
      if(res != CURLE_OK){
          emit error(QString(curl_easy_strerror(res)));
          return 0;
      }

      if(!reuse)
          curl_easy_cleanup(curl);
    }
    return returnCode;

}
int FTP::uploadDir(QString path, QString dir,ftpList fileList)
{
    int ret = 1;
    curl = curl_easy_init();
    foreach (ftpFile f, fileList) {
        if(!f.isDir)
            ret &= uploadFile(path,QString("%1/%2").arg(dir).arg(f.filename),true);
    }
    curl_easy_cleanup(curl);
    return ret;
}
int FTP::deleteFile(QString path){
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist *cmdlist = NULL;
        cmdlist = curl_slist_append(cmdlist, QString("DELE %1").arg(path).toUtf8().data());
      curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().data());
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, beVerbose);
      curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_trace);
      if(!username.isEmpty()){
          curl_easy_setopt(curl, CURLOPT_USERNAME, username.toUtf8().data());
          curl_easy_setopt(curl, CURLOPT_PASSWORD, password.toUtf8().data());
      }
      curl_easy_setopt(curl, CURLOPT_QUOTE, cmdlist);
      curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &returnCode);
      res = curl_easy_perform(curl);
      emit progress(0,0);
      curl_easy_cleanup(curl);
      if(res != CURLE_OK){
          emit error(QString(curl_easy_strerror(res)));
          return 0;
      }
    }
    return returnCode;
}
