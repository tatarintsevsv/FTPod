#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ftpClient = new FTP();

#ifdef QT_DEBUG
    ui->url->setText("ftp://192.168.22.70/epvv");
    ui->username->setText("arise");
    ui->pasword->setText("****");
#endif

    connect(ftpClient,SIGNAL(log(QString)),this,SLOT(logSlot(QString)));
    connect(ftpClient,SIGNAL(error(QString)),this,SLOT(logSlot(QString)));
    connect(ftpClient,SIGNAL(dump(QString)),this,SLOT(logSlot(QString)));
    connect(ftpClient,SIGNAL(progress(int,int)),SLOT(progressSlot(uint,uint)));
    model = new FTPListModel();
    ui->list->setModel(model);
    ui->list->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->progressBar->setMaximum(0);
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(false);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    ftpClient->setUrl(QUrl(ui->url->text()).host());
    ftpClient->setCreditinals(ui->username->text(),ui->pasword->text());
    model->setData(ftpClient->getFileList(QUrl(ui->url->text()).path().mid(1)));
    ui->list->resizeColumnsToContents();
    ui->list->resizeRowsToContents();


}

void MainWindow::on_list_doubleClicked(const QModelIndex &index)
{
    QString fn = model->data(model->index(index.row(),0),Qt::DisplayRole).toString();
    QString isdir = model->data(model->index(index.row(),3),Qt::DisplayRole).toString();
    if(isdir!="DIR"){
        QString dir = QFileDialog::getExistingDirectory(this,"save file to...", "");
        if(!dir.isEmpty())
            ftpClient->downloadFile(QUrl(ui->url->text()).path().mid(1)+"/"+fn,dir+"/"+fn);
        return;
    }
    ui->url->setText(ui->url->text()+"/"+fn);
    model->setData(ftpClient->getFileList(QUrl(ui->url->text()).path().mid(1)));
    ui->list->resizeColumnsToContents();
    ui->list->resizeRowsToContents();
}

void MainWindow::logSlot(QString text)
{
    ui->log->appendPlainText(text+"\n");
}

void MainWindow::progressSlot(uint max, uint val)
{
    ui->progressBar->setMaximum(max);
    ui->progressBar->setValue(val);
    ui->progressBar->setVisible(max);

}

