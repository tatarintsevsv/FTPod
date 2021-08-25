#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QTableView>
#include <QAbstractTableModel>
#include <QFileDialog>
#include "ftp.h"
extern FTP* ftpClient;

namespace Ui {
class MainWindow;
}

class FTPListModel : public QAbstractTableModel
 {
     Q_OBJECT

 public:
    //FTPListModel( QObject* parent = 0 );
     //StringListModel(const QStringList &strings, QObject *parent = 0)         : QAbstractListModel(parent), stringList(strings) {}
     void setData(ftpList list){
         beginResetModel();
         files = list;
         endResetModel();
     }
     int rowCount(const QModelIndex &parent = QModelIndex()) const{return files.count();}
     int columnCount(const QModelIndex &parent = QModelIndex()) const{return 4;}
     QVariant data(const QModelIndex &index, int role) const{
         if(role==Qt::DisplayRole){
             ftpFile file = files.at(index.row());
             switch (index.column()) {
               case 0: return file.filename; break;
               case 1: return file.size; break;
               case 2: return file.date; break;
               case 3: return file.isDir?"DIR":""; break;
             }
         }
         return QVariant();
     };
     QVariant headerData( int section, Qt::Orientation orientation, int role ) const {
         if( role != Qt::DisplayRole ) {
             return QVariant();
         }

         if( orientation == Qt::Vertical ) {
             return section;
         }

         switch( section ) {
         case 0: return "filename"; break;
         case 1: return "size"; break;
         case 2: return "date"; break;
         case 3: return "isDir"; break;
         }

         return QVariant();
     }

     ftpList files;

 private:
     QStringList stringList;
 };

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    FTPListModel* model;
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_list_doubleClicked(const QModelIndex &index);
    void logSlot(QString text);
    void progressSlot(uint max,uint val);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
