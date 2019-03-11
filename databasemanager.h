/*
 *@file:   databasemanager.h
 *@author: 缪庆瑞
 *@date:   2017.12.13
 *@brief:  该组件提供SQLite数据库的管理操作，包括表的定义以及数据的CRUD操作。经过对比
 * 考虑，该组件暂时主要针对单数据库的管理，对于多数据库管理，需要定义多个对象实现。
 *
 * 另外通过帮助文档“Thread-Support in Qt Modules”可以了解到Qt 封装的SQL Module针对同
 * 一个数据库连接并不支持多线程，而该组件的设计初衷是“全局共用一个数据库连接操作一个
 * 数据库”,所以并不建议在多线程里操作同一个数据库。
 *
 * 不过经过测试发现,使用Qt自带的SQLite3驱动,多线程使用同一连接操作同一数据库并没有出现
 * 错误,这是因为SQLite3本身支持多种线程模式(单线程/多线程/串行),Qt在编译插件时默认使用
 * 了串行模式,这种情况下SQLite内部通过加锁实现操作序列化,保证线程安全,但同时因为无法并
 * 行处理也会在一定程度上降低性能。如果需要使用SQLite3的其他线程模式,可以重新编译生成
 * 指定线程模式的SQLite3插件,又或者是不使用Qt Sql Module,直接将SQLite3编译成库,通过c函
 * 数接口完成线程模式的修改及数据库的操作。
 *
 * 我们的项目目前使用的是串行模式的SQLite3插件,经过测试性能可以满足我们的需求。如果要换
 * 用其他模式,则要注意现有的接口将不再是线程安全的,需要我们主动加上一些锁操作,可以打开
 * MT_SAFE宏的定义，通过读写锁实现线程的同步。
 */
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlDriver>
#include <QReadWriteLock>
#include <QString>
#include <QList>
#include <QDebug>
#include <QTime>
/* SQLite3只支持一写多读，在数据库本身是非线程安全的情况下，则可以打开该宏
 * 进行线程同步(读写锁)
 * 因为我们的项目使用的sqlite3插件是串行模式，数据库内部接口已经加了锁，理论上
 * 就不需要我们在外边再加了。但是由于该组件的copyTable()函数的内部处理是由
 * 数据库的多条事务完成的，在其之间有可能被其他线程的数据库访问打断，造成数据
 * 出错(我们的项目刚好存在这种情况)。所以需要对copyTable()加写锁模拟成一个原子
 * 操作，并对其他可能同时访问的数据库处理也加锁。这里为了方便，直接打开该宏，
 * 相当于对数据库多线程安全的双重保护，当然也会有一定的性能损失，但实际上很小。
*/
#define MT_SAFE

class DatabaseManager : public QObject
{
public:
    DatabaseManager(QString connectionName,QObject *parent = 0);
    ~DatabaseManager();

    bool createSqliteConnection(QString databaseName);//创建sqlite连接
    bool integrityCheck();//数据库完整性检测
    /*****数据定义*******/
    //建表
    bool createTable(QString tableName,QList<QString> &columnNames,QList<QString> &columnTypes,QString tableConstraint=QString());//建表
    bool createTable(QString createSql);
    //修改表结构 sqlite对alter支持有限，仅可以修改表名和添加字段
    bool alterTable(QString alterSql);
    //删除表
    bool dropTable(QString tableName);
    //复制表
    bool copyTable(QString srcTableName,QString desTableName);//数据库内复制
    bool copyTable(QString srcDbName,QString srcTableName,QString desTableName);//数据库间复制

    /******数据更新*********/
    //插入数据
    bool insertTable(QString tableName,QVariantList &rowValues,QList<QString> columnNames=QList<QString>());
    bool insertBatchTable(QString tableName, QList<QVariantList> &columnValues, QList<QString> columnNames=QList<QString>());
    bool insertTable(QString insertSql);
    //修改数据
    bool updateTable(QString tableName,QList<QString> &columnNames,QVariantList &rowValues,QString whereSql=QString());
    bool updateTable(QString tableName,QList<QString> &columnNames,QVariantList &rowValues,QString whereColName,QVariant whereColValue);
    bool updateTable(QString tableName, QString columnName, QVariant rowValue, QString whereSql=QString());
    bool updateTable(QString tableName, QString columnName, QVariant rowValue, QString whereColName,QVariant whereColValue);
    bool updateTable(QString updateSql);
    //删除数据
    bool deleteTable(QString tableName, QString whereSql=QString());
    bool deleteTable(QString tableName, QString whereColName,QVariant whereColValue);

    /******数据查询**********/
    //查询单行数据记录
    QVariant selectSingleColData(QString tableName,QString columnName,QString whereSql);//单行单列
    QVariant selectSingleColData(QString tableName,QString columnName,QString whereColName,QVariant whereColValue);
    QVariantList selectMultiColData(QString tableName,QList<QString> columnNames,QString whereSql);//单行多列
    QVariantList selectMultiColData(QString tableName,QList<QString> columnNames,QString whereColName,QVariant whereColValue);
    //查询多行数据记录
    QVariantList selectSingleColDatas(QString tableName,QString columnName,bool isDistinct=true,QString whereSql=QString());//多行单列
    QVariantList selectSingleColDatas(QString tableName,QString columnName,QString whereColName,QVariant whereColValue,bool isDistinct=true);
    //查询数据表行数
    int selectRowCount(QString tableName,QString whereSql=QString());
    //查询表是否存在
    bool isExistTable(QString tableName);

    void closeConnection();//断开连接

private:
    //附加数据库与分离数据库
    bool attachDB(QString attachDbName,QString aliasName);//附加数据库
    bool detachDB(QString aliasName);//分离数据库
    //复制表 复制数据库表　仅执行insert的sql语句
    bool onlyCopyTable(QString srcTableName,QString desTableName);
    bool isExistTableForCopyTable(QString tableName);//查询表是否存在
    QString getCreateTableSql(QString masterTableName,QString tableName);//获取表的创建语句

    QSqlDatabase db;//描述数据库连接的对象 全局共用一个数据库连接
    QString connectionName;//连接名，通过连接名可以在全局找到对应的数据库
    //读写锁 在构造函数内显式或隐式初始化,目前只在多线程安全条件下会用到
    QReadWriteLock readWriteLock;
};

#endif // DATABASEMANAGER_H
