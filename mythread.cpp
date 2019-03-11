/*
 *@file:   mythread.cpp
 *@author: 缪庆瑞
 *@date:   2016.9.28
 *@brief:  子类化QThread线程类
 */
#include "mythread.h"
#include "globalvar.h"
#include <QDebug>

MyThread::MyThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;//初始化状态变量，通过变量控制线程的退出
}

void MyThread::setMessage(const QString &message)
{
    messageStr = message;//可以在类外调用该方法，设置线程打印的值
}

void MyThread::stop()//公有方法操作私有变量
{
    stopped = true;
}

//线程启动后进入该函数，该函数退出则线程退出
void MyThread::run()
{
    while(!stopped)
    {
        QString tableName = "aa";
        QVariantList idList;
        QVariantList nameList;
        QVariantList scoreList;
        QVariantList ageList;
        QList<QVariantList> list;
        QStringList name;
        name<<"a"<<"b"<<"c"<<"d"<<"e"<<"f"<<"g"<<"h"<<"i"<<"j"<<"k"<<"l"<<"m"<<"n";
        int nameNum = name.size();
        for(int i =0;i<100;i++)
        {
            idList<<i;
            nameList<<name.at(i%nameNum);
            scoreList<<qrand()%10+85;
            ageList<<qrand()%5+16;
        }
        list<<idList<<nameList<<scoreList<<ageList;
        qDebug()<<"thread1 start:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
        if(databaseManager->insertBatchTable(tableName,list))
        {
            qDebug()<<"insert  batch table success;";
        }
        qDebug()<<"thread1 end:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
        msleep(100);//休眠1000ms
    }
    stopped = false;
    qDebug()<<"线程退出";
}
