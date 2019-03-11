#ifndef GLOBALVAR_H
#define GLOBALVAR_H
#include <QMutex>
#include "databasemanager.h"
extern DatabaseManager *databaseManager;
//extern int number;//声明全局变量，这里是为了让多个不同线程操作全局变量，测试互斥锁的作用
/*线程使用的互斥锁，不同的线程要想使用互斥锁，必须公用同一个mutex。
因为本例的两个线程是继承只QThread，在两个类中，所以需要定义一个全局的互斥锁变量*/
//extern QMutex mutex;

#endif // GLOBALVAR_H
