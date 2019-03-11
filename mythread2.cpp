#include "mythread2.h"
#include "globalvar.h"
#include <QDebug>

MyThread2::MyThread2(QObject *parent) :
    QThread(parent)
{
    stopped = false;
}

void MyThread2::setMessage(const QString &message)
{
    messageStr = message;//可以在类外调用该方法，设置线程打印的值
}

void MyThread2::run()
{
    while(!stopped)
    {
        /*QString tableName = "aa";
        //单行单列
        qDebug()<<"select table start:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
        QVariant name = databaseManager->selectSingleColData(tableName,"id","name like '%a%' order by id");
        qDebug()<<"row number:"<<databaseManager->selectSingleColData(tableName,"count(*)","id != 1").toInt();
        qDebug()<<"select table end:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
        msleep(100);//休眠10ms*/

        QString tableName = "aa";
        if(databaseManager->updateTable(tableName,"id",2))
        {
            qDebug()<<"update table success;";
        }
        msleep(10);//休眠1000ms
    }
    stopped = false;
    qDebug()<<"线程2退出";
}

void MyThread2::stop()
{
    stopped = true;
}
