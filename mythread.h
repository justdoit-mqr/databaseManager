/*
 *@file:   mythread.h
 *@author: 缪庆瑞
 *@date:   2016.9.28
 *@brief:  子类化QThread的头文件 线程1
 */
#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QTime>
#include "globalvar.h"

class MyThread : public QThread
{
    Q_OBJECT
public:
    explicit MyThread(QObject *parent = 0);

    void setMessage(const QString &message);
    void stop();

protected:
    void run();

signals:

public slots:

private:
    QString messageStr;
    volatile bool stopped;//易失性变量，要时刻保持最新值，防止编译器优化
    //QMutex mutex;//互斥量（互斥锁） 在线程之间提供一个串行化通道
    //mutex可以使用lock()和unlock()对一个线程加锁或解锁，实现线程的互斥。
    //一个线程一旦加锁，而另一个线程如果请求加锁则阻塞直到线程解锁，
    //但如果另一个线程并没有使用互斥锁加锁，则另一个线程照常运行。注意：则加锁和解锁之间的语句不能有休眠即sleep语句，
    //一旦线程休眠，则另一个阻塞的线程会自动获得资源开始运行,互斥量就没有作用了

    //在一些复杂函数和抛出C++异常的函数中锁定和解锁互斥量，非常容易发生错误
    //于是可以使用QMutexLocker类简化处理 它的构造函数接受QMutex做参数并且将其加锁，析构函数则进行解锁
    //所以在需要加锁解锁的区域使用{QMutexLocker locker(&mutex);......}进行加锁解锁，因为locker的作用域是该大括号
    //跳出大括号便析构 解锁
    //QMutexLocker locker(&mutex);

};

#endif // MYTHREAD_H
