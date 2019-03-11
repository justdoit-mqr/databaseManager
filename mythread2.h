/*
 *@file:   mythread.h
 *@author: 缪庆瑞
 *@date:   2016.9.30
 *@brief:  子类化QThread的头文件 线程2
 */
#ifndef MYTHREAD2_H
#define MYTHREAD2_H

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QTime>

class MyThread2 : public QThread
{
    Q_OBJECT
public:
    explicit MyThread2(QObject *parent = 0);

    void setMessage(const QString &message);
    void stop();

protected:
    void run();//线程开启后执行的函数，该函数退出线程结束

signals:

public slots:

private:
    volatile bool stopped;//使用标记变量控制线程的退出
    QString messageStr;
    //QMutex mutex;//互斥量 一般用来保护一个变量或者一段代码，分为加锁和解锁
    //QMutexLocker locker;

};

#endif // MYTHREAD2_H
