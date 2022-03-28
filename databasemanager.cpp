/*
 *@author: 缪庆瑞
 *@date:   2017.12.13
 *@brief:  操作数据库的类
 */
#include "databasemanager.h"
#include <QReadLocker>
#include <QWriteLocker>
#include <QMessageBox>
#include <QVariant>

DatabaseManager::DatabaseManager(QString connectionName, QObject *parent)
    :QObject(parent),readWriteLock(QReadWriteLock::Recursive)
{
    this->connectionName = connectionName;
}

DatabaseManager::~DatabaseManager()
{
    //对象析构时默认不会删除创建的数据库连接,需要手动关闭
    closeConnection();
}
/*
 *@brief:   连接到sqlite数据库　一个数据库只需要连接一次
 * 注:sqlite3数据库默认编码UTF-8,支持中文存取,只需要在程序中对存取的字符串
 * 采用UTF-8编码即可。
 *@author:  缪庆瑞
 *@date:    2017.12.21
 *@param:   databasname: 数据库的名字
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::createSqliteConnection(QString databaseName)
{
    //连接已创建
    if(QSqlDatabase::contains(connectionName) && db.isValid())
    {
        return true;
    }

    //目前板子的开发环境只有sqlite驱动
    db = QSqlDatabase::addDatabase("QSQLITE",connectionName);//添加数据库驱动
    //qDebug()<<db.driver()->hasFeature(QSqlDriver::Transactions);//支持事务操作
    db.setDatabaseName(databaseName);//设置连接的数据库名
    /*设置多连接并发访问busy超时时间,不设置默认是5000ms*/
    //db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    if(!db.open())//打开数据库
    {
        qDebug()<<"open database failed:"<<db.lastError();
        return false;
    }
    /*sqlite删除数据后，未使用的磁盘空间被添加到一个内在的空闲列表中，用于存储下次插入
     *的数据，数据库占用磁盘空间不会减少。要想减少磁盘空间占用,主要有两种方法。
     *1.在数据库建表前执行pragma auto_vacuum=1;(如果数据库已有表,则该语句无效。且该语句在
     *减少磁盘空间的同时会生成更多的碎片,同时效率较低，不建议使用)
     *2. 执行vacuum;清理无用空间整理碎片.(该命令会复制主数据库到一个临时文件副本，然后
     * 清空数据库，并从副本重新载入原始的数据库文件,如果数据库文件较大的话，可能就比较费时)
     *
     * sqlite3默认开启了磁盘写同步"pragma synchronous = FULL",该模式下数据非常安全，可以避免
     * 系统崩溃或异常断电造成的数据损坏.但效率比较低,一条普通的insert或update比普通的读写
     * 文件还要慢.所以如果能接受部分的数据损坏,而对效率有较高的要求时,可以关闭写同步"pragma
     *  synchronous = OFF"(经过在虚拟机和imx6上测试,一些写入操作在关闭写同步情况下的效率要比
     * 开启快10-50倍),具体要不要开启磁盘写同步,请根据项目程序的实际情况而定
     * 注:Ti am3354平台对该同步设置不敏感,FULL,NORMAL,OFF效率都差不多,针对该项目一条修改语句
     * 大约10ms,怀疑是sqlite3插件或者平台sync的问题.所以即便设置为FULL,数据也不能保证绝对安全。
    */
    QSqlQuery query(db);//创建sql语句执行对象
    //query.exec("vacuum;");//清理碎片
    if(!query.exec("pragma synchronous = OFF;"))//关闭写同步
    {
        qDebug()<<"pragma synchronous error"<<query.lastError();
    }
    return true;
}
/*
 *@brief:   关闭数据库连接
 *@author:  缪庆瑞
 *@date:    2017.12.29
 */
void DatabaseManager::closeConnection()
{
    /* 删除某个数据库连接.
     * 根据removeDatabase()方法的帮助文档,正确的方式是在删除连接之前,先析构掉
     * 使用该连接的query对象和db对象.因为db这里被定义成了类成员变量,不能以{}栈
     * 的形式自动析构,所以调用该方法会提示警告(表示删除的连接正在被使用,该连接
     * 所有的query将被强制终止),但这实际并不影响什么.
     * 此处警告的根本原因是db有一个有效的数据库连接(isValid()),虽然db在这里无法
     * 析构,但将其置为一个无效的对象也能解决警告的问题
    */
    db.close();
    db = QSqlDatabase();//将db置为一个无效的对象
    QSqlDatabase::removeDatabase(connectionName);
}
/*
 *@brief:   数据库完整性检测(结构,格式,数据记录)
 * SQLite数据库损坏的可能性很低，但是不排除某些情况下(异常断电等)因外部程序或硬件操作系统
 * 的bug而破坏数据库文件,引起"database disk image is malformed"。所以在程序初启动时可以对
 * 数据库进行完整性检测。
 *@author:  缪庆瑞
 *@date:    2019.3.5
 *@return:  返回值为布尔类型，true:完整，false:损坏
 */
bool DatabaseManager::integrityCheck()
{
    /*SQLite提供两种完整性检测:pragma integrity_check;和quick_check;
     * integrity_check会进行彻底性的检测(包括乱序的记录,缺页,错误的记录,丢失的索引,
     * 唯一性约束,非空约束)，但相对耗时。
     * quick_check不进行约束和索引一致性检测，相对耗时较短。
     */
    QSqlQuery query(db);//创建sql语句执行对象
    //query.exec("pragma integrity_check;");
    if(!query.exec("pragma quick_check;"))
    {
        qDebug()<<"quick_check error:"<<query.lastError();
        return true;
    }
    if(query.next())
    {
        if(query.value(0).toString()=="ok")//数据库完整
        {
            return true;
        }
        else//数据库有损坏
        {
            qDebug()<<"current database is not integrity.";
            return false;
        }
    }
    return true;
}
/*
 *@brief:   建表　　该函数只在表不存在(if not exists)的情况下才会真正建表
 *@author:  缪庆瑞
 *@date:    2017.12.25
 *@param:   tableName: 表的名字
 *@param:   columnNames: 列(字段)的名字
 *@param:   columnTypes: 列(字段)的类型　此处还可能包含列级完整性约束条件
 *  例如：varchar(20)　primary key - 主键约束
 *  类型：NULL: 表示该值为NULL值　INTEGER: 无符号整型值　REAL: 浮点值
 *              TEXT: 文本字符串，存储使用的编码方式为UTF-8、UTF-16BE、UTF-16LE
 *              BLOB: 存储Blob数据，该类型数据和输入数据完全相同。
 *  sqlite 与一般关系数据库采用静态数据类型不同，它采用动态数据类型，根据存入
 *  的值确定存储类型，属于弱类型。所以建表的时候，可以没有列类型，但为了保证
 *  数据库平台的可移植性，还是按静态数据类型声明处理，且为了最大化与其它数据
 *  库引擎之间的数据类型兼容性，同样支持通用的sql类型(int ,  varchar(20),float等 )
 *  存储时根据sqlite的字段亲缘性自动处理。
 *  常见约束：NOT NULL-非空　UNIQUE-唯一　FOREIGN KEY-外键
 *                     CHECK-条件检查　DEFAULT-默认　PRIMARY KEY-主键
 *@param:   tableConstraint:表级约束条件
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::createTable(QString tableName, QList<QString> &columnNames, QList<QString> &columnTypes, QString tableConstraint)
{
    //保证字段名和类型数量一致
    if(columnNames.size() != columnTypes.size())
    {
        qDebug()<<"list names and list types don't macth(number)...";
        return false;
    }
    //列(字段)数不能为０
    int columnCount = columnNames.size();
    if(!columnCount)
    {
        qDebug()<<"The number of columns cannot be empty...";
        return false;
    }
    //执行sql建表命令
    /*sqlite 的sql语句在涉及到表名和列名时,默认是不支持表列名以数字开头或者含有特殊
     字符的(会有语法错误)，如果非要用，则可以给sql语句中的表列名加上单引号(sqlite)或
    者倒引号。这里为了表列名尽量标准化，没有进行处理*/
    QSqlQuery query(db);//创建sql语句执行对象
    QString createSql = QString("create table if not exists %1(").arg(tableName);
    for(int i=0; i<columnCount; i++)
    {
        createSql.append(columnNames.at(i)+" "+columnTypes.at(i)+",");
    }
    if(tableConstraint.isEmpty())//无表级约束
    {
        createSql.replace(createSql.size()-1,1,");");//用)取代结尾的,
    }
    else//有表级约束
    {
        createSql.append(tableConstraint);
        createSql.append(");");
    }
    /*执行建表语句时遇到一个问题:如果在当前连接(connectionName)的数据库中创建一个表，而在
     *其他连接(指向同一个数据库)中删除该表。此时在当前连接中如果不重新连接数据库或者执行其
     *他数据库操作 ,而直接创建同名表,会提示表已存在,但实际上该表确实已经被删除了。怀疑是Qt
     *数据库驱动对表是否存在的判断有问题,解决方法有两种:一是在建表语句中加上if not exists,这样
     *可以正确的判断表是否真正存在，只在不存在的情况下建表。二是建表前调用db.tables()，用该
     *方法相当于刷新当前连接的数据库中的用户表,这样在建表时就能够正常判断表是否真正存在了.
     *目前选用的是第一种方法。
     */
    //db.tables();//相当于刷新当前连接的数据库中的用户表
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec(createSql))
    {
        qDebug()<<"create table error:"<<query.lastError();
        qDebug()<<"error sql:"<<createSql;
        return false;
    }
    return true;
}
/*
 *@brief:   建表
 *@author:  缪庆瑞
 *@date:    2017.12.25
 *@param:   createSql:完整的建表语句
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::createTable(QString createSql)
{
    db.tables();//当前连接的数据库中的用户表
    QSqlQuery query(db);//创建sql语句执行对象
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec(createSql))
    {
        qDebug()<<"create table error:"<<query.lastError();
        qDebug()<<"error sql:"<<createSql;
        return false;
    }
    return true;
}
/*
 *@brief:   修改表结构 sqlite对该语句的支持有限，仅可以修改表名和添加字段，例：
 *  修改表名-ALTER  TABLE  旧表名  RENAME  TO  新表名
 *  添加字段-ALTER  TABLE  表名  ADD  COLUMN  列名  数据类型  约束条件
 *@author:  缪庆瑞
 *@date:    2017.12.25
 *@param:   alterSql:完整的修改表结构的语句
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::alterTable(QString alterSql)
{
    QSqlQuery query(db);//创建sql语句执行对象
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec(alterSql))
    {
        qDebug()<<"alter table error:"<<query.lastError();
        qDebug()<<"error sql:"<<alterSql;
        return false;
    }
    return true;
}
/*
 *@brief:   删除表结构
 *@author:  缪庆瑞
 *@date:    2017.12.25
 *@param:   tableName:表名
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::dropTable(QString tableName)
{
    //sqlite不支持使用RESTRICT和CASCADE(级联),默认级联删除
    QString dropSql = QString("drop table if exists %1;").arg(tableName);
    QSqlQuery query(db);//创建sql语句执行对象
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec(dropSql))
    {
        qDebug()<<"drop table error:"<<query.lastError();
        qDebug()<<"error sql:"<<dropSql;
        return false;
    }
    return true;
}
/*
 *@brief:   复制表(表结构＋数据)　数据库内复制
 *@author:  缪庆瑞
 *@date:    2018.8.20
 *@param:   srcTableName:源表名
 *@param:   desTableName:目的表名
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::copyTable(QString srcTableName, QString desTableName)
{
#ifdef MT_SAFE
    /*复制表的操作对于sqlite而言并不是"1"条事务操作，因为这里需要先清空数据
     * (或者新建表),然后再插入新数据。所以即便sqlite3本身是线程安全的，但也只
     * 是对1条事务线程安全，在清空数据和插入数据之间,很有可能出现其他线程对
     * 该目的表读写访问的情况，此时就有可能造成读写的数据出错。
     *
     * 所以这里利用读写锁将整个复制表的操作模拟成一个原子操作，避免被其他
     * 线程打断出问题。
     *
     * 因为这个方法中调用的函数也加了读写锁，所以在readWriteLock定义时，添加
     * 了QreadWriteLock::Recursive属性，允许对同一个线程加锁多次，否则会引起
     * 死锁。但经过测试发现Recursive属性只允许同一线程对相同的锁(读或写)加锁
     * 多次，而如果先加了写锁，再加读锁就会锁死。所以该方法中调用的函数不能加
     * 读锁，isExistTableForCopyTable()就是为了避免该问题而创建的。
    */
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    //目的表存在，则清空所有数据，但保留原结构
    if(isExistTableForCopyTable(desTableName))
    {
        if(!deleteTable(desTableName))
        {
            return false;
        }
    }
    //目的表不存在，则创建表结构
    else
    {
        //获取源表的建表语句
        QString createTableSql = getCreateTableSqlForCopyTable("sqlite_master",srcTableName);
        createTableSql.replace(srcTableName,desTableName);//替换成新表名
        if(!createTable(createTableSql))
        {
            return false;
        }
    }
    //复制源表数据到目的表
    if(!onlyCopyTable(srcTableName,desTableName))
    {
        return false;
    }
    return true;
}
/*
 *@brief:   复制表(表结构＋数据)　数据库间复制,目的数据库就是调用该方法的数据库
 *@author:  缪庆瑞
 *@date:    2018.8.21
 *@param:   srcDbName:源数据库名
 *@param:   srcTableName:源表名
 *@param:   desTableName:目的表名
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::copyTable(QString srcDbName, QString srcTableName, QString desTableName)
{
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁 　原因同上
#endif
    QString aliasName = "sourceDB";//附加数据库别名
    /*目的表如果存在则清空所有数据，但保留原结构。
     * 该操作一定放在附加数据库操作之前，因为如果基础数据库不存在目的表，
     * 那么该操作可能会删除附加数据库里的desTableName表数据，导致后面复制出错
     */
    if(isExistTableForCopyTable(desTableName))
    {
        if(!deleteTable(desTableName))
        {
            return false;
        }
        //附加数据库
        if(!attachDB(srcDbName,aliasName))
        {
            return false;
        }
    }
    //目的表不存在，则创建表结构
    else
    {
        //附加数据库
        if(!attachDB(srcDbName,aliasName))
        {
            return false;
        }
        //附加数据库的sqlite_master
        QString attachTableName = QString("%1.sqlite_master").arg(aliasName);
        //获取源表的建表语句
        QString createTableSql = getCreateTableSqlForCopyTable(attachTableName,srcTableName);
        createTableSql.replace(srcTableName,desTableName);//替换成新表名
        if(!createTable(createTableSql))
        {
            return false;
        }
    }
    //获取附加数据库的表名
    QString attachTableName = QString("%1.%2").arg(aliasName).arg(srcTableName);
    //复制源表到目的表
    if(!onlyCopyTable(attachTableName,desTableName))
    {
        detachDB(aliasName);
        return false;
    }
    //分离数据库表
    if(!detachDB(aliasName))
    {
        return false;
    }
    return true;
}
/*
 *@brief:  插入单条数据
 *@author:  缪庆瑞
 *@date:    2017.12.25
 *@param:   tableName:表名
 *@param:   rowValues:一个元组的部分或全部数据
 *@param:   columnNames:对应元组数据的列名，默认为空表示插入整个元组。注：带有默认值的参数不能用引用
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::insertTable(QString tableName, QVariantList &rowValues, QList<QString> columnNames)
{
    QString columnNamesStr;
    QString bindValuesStr;
    int rowValuesNumber = rowValues.size();
    //列名非空，则插入时value-name需要对应，否则插入整个元组
    if(!columnNames.isEmpty())
    {
        //列名和列值数量不匹配
        if(rowValuesNumber != columnNames.size())
        {
            qDebug()<<"list names and values don't macth(number)...";
            return false;
        }
        else
        {
            columnNamesStr.append("(");
            columnNamesStr.append(QStringList(columnNames).join(","));
            columnNamesStr.append(")");
        }
    }
    //为sql语句添加占位符
    for(int i=0;i<rowValuesNumber;i++)
    {
        bindValuesStr.append("?");
        if(i != rowValuesNumber-1)
        {
            bindValuesStr.append(",");
        }
    }
    QString insertSql=QString("insert into %1%2 values(%3);").arg(tableName,columnNamesStr,bindValuesStr);
    QSqlQuery query(db);//创建sql语句执行对象
    query.prepare(insertSql);
    //绑定占位符 注:mysql5 因为没有提供控制输入输出参数的API,所以不能使用占位符
    for(int i=0;i<rowValuesNumber;i++)
    {
        query.addBindValue(rowValues.at(i));
    }
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec())
    {
        qDebug()<<"insert table error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:  批量插入多条数据
 *  此处采用的是事务操作，提高批量插入的效率。sqlite本身也是支持单sql语句批量插入的，
 *  类似insert into aa values(101,'a',10),...(105,'e',50);但我们使用的Qt封装的sqlite驱动插件版本
 *  是3.7.7.1,并不支持这种语法，使用时会出错。不过这种方法的原理和事务操作本质上是相似的，效率也差不多.
 *@author:  缪庆瑞
 *@date:    2018.1.4
 *@param:   tableName:表名
 *@param:   columnValues:多个元组的部分字段或全部字段的数据,该变量的每一个元素(QVariantList)存的是
 *多个元组同一字段的内容,之所以不按行存放，是因为批量插入绑定占位符时，QVariantList列中的每一个QVariant
 *的类型必须一致，且在执行execBatch()时，QVariantList中的每一个QVariant都会被解释成一个新行的值
 *@param:   columnNames:对应元组数据的列名，默认为空表示插入整个元组QVariantList
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::insertBatchTable(QString tableName, QList<QVariantList> &columnValues, QList<QString> columnNames)
{
    QString columnNamesStr;
    QString bindValuesStr;
    int columnNumber = columnValues.size();
    //列名非空，则插入时value-name需要对应，否则插入整个元组
    if(!columnNames.isEmpty())
    {
        //列名和列数不匹配
        if(columnNumber != columnNames.size())
        {
            qDebug()<<"list names and values don't macth(number)...";
            return false;
        }
        else
        {
            columnNamesStr.append("(");
            columnNamesStr.append(QStringList(columnNames).join(","));
            columnNamesStr.append(")");
        }
    }
    //为sql语句添加占位符
    for(int i=0;i<columnNumber;i++)
    {
        bindValuesStr.append("?");
        if(i != columnNumber-1)
        {
            bindValuesStr.append(",");
        }
    }
    QString insertSql=QString("insert into %1%2 values(%3);").arg(tableName,columnNamesStr,bindValuesStr);
    QSqlQuery query(db);//创建sql语句执行对象
    query.prepare(insertSql);
    //绑定占位符
    for(int i=0;i<columnNumber;i++)
    {
        //批量插入时，允许使用QVariantList类型作为参数，一次绑定多个元组同一字段的数据
        query.addBindValue(columnValues.at(i));
    }
    /*事务属于原子性操作，同一个时刻只能存在一个，多线程时如果一个线程正在使用事务，另
     *一个线程就不能成功开启事务模式。
     *另外一旦通过transaction()成功开启事务后，必须通过commit()或者rollback()结束事务后，
     *才能再次开启成功，否则无法再次开启事务。
     */
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(db.transaction())//成功开启事务操作 原子性操作，commit()之前数据一直在内存中
    {
        qDebug()<<"transaction operation;";
        //真正的费时操作是下面这句话，取决于批量插入数据的多少
        if(!query.execBatch())
        {
            qDebug()<<"insert batch table error:"<<query.lastError();
            db.rollback();//批量插入失败，回滚到之前的状态
            return false;
        }
        //在批量插入数据时，提交事务并不费时
        if(!db.commit())//提交事务
        {
            qDebug()<<"insert batch table transaction error:"<<db.lastError();
            return false;
        }
        else
        {
            return true;
        }
    }
    else //按照普通插入方式写数据，sqlite的特点是每执行一条sql语句实则进行一次IO操作，效率低
    {
        qDebug()<<"not transaction operation";
        if(!query.execBatch())
        {
            qDebug()<<"insert batch table error:"<<query.lastError();
            return false;
        }
        return true;
    }
}
/*
 *@brief:   插入数据
 *@author:  缪庆瑞
 *@date:    2018.1.4
 *@param:   insertSql:完整的插入数据的语句
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::insertTable(QString insertSql)
{
    QSqlQuery query(db);//创建sql语句执行对象
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec(insertSql))
    {
        qDebug()<<"insert table error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:  修改多个字段的数据
 *@author:  缪庆瑞
 *@date:    2017.12.26
 *@param:   tableName:表名
 *@param:   columnNames:要修改数据的列
 *@param:   rowValues:对应列修改的值
 *@param:   whereSql:条件　例如：Sno='1001'
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::updateTable(QString tableName, QList<QString> &columnNames, QVariantList &rowValues, QString whereSql)
{
    int columnCount = columnNames.size();
    //保证修改的字段名和值数量一致
    if(columnCount != rowValues.size())
    {
        qDebug()<<"list names and types don't macth(number)...";
        return false;
    }
    QString setColumnValue("set ");
    for(int i=0;i<columnCount;i++)
    {
        setColumnValue.append(columnNames.at(i)+"=?");
        if(i != columnCount-1)
        {
            setColumnValue.append(",");
        }
    }
    QString updateSql = QString("update %1 %2").arg(tableName,setColumnValue);
    if(whereSql.isEmpty())
    {
        updateSql.append(";");
    }
    else
    {
        updateSql.append(" where "+whereSql+";");
    }
    //执行Sql命令
    QSqlQuery query(db);//创建sql语句执行对象
    query.prepare(updateSql);
    //绑定占位符
    for(int i=0;i<columnCount;i++)
    {
        query.addBindValue(rowValues.at(i));
    }
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec())
    {
        qDebug()<<"update table error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:  修改多个字段的数据　针对where条件为columnName = colnumValue;
 *@author:  缪庆瑞
 *@date:    2018.1.5
 *@param:   tableName:表名
 *@param:   columnNames:要修改数据的列
 *@param:   rowValues:对应列修改的值
 *@param:   whereColName:　where条件的列名
 *@param:   whereColValue:　where条件的列值
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::updateTable(QString tableName, QList<QString> &columnNames, QVariantList &rowValues, QString whereColName, QVariant whereColValue)
{
    int columnCount = columnNames.size();
    //保证修改的字段名和值数量一致
    if(columnCount != rowValues.size())
    {
        qDebug()<<"list names and types don't macth(number)...";
        return false;
    }
    QString setColumnValue("set ");
    for(int i=0;i<columnCount;i++)
    {
        setColumnValue.append(columnNames.at(i)+"=?");
        if(i != columnCount-1)
        {
            setColumnValue.append(",");
        }
    }
    QString updateSql = QString("update %1 %2 where %3=?;").arg(tableName,setColumnValue,whereColName);
    //执行Sql命令
    QSqlQuery query(db);//创建sql语句执行对象
    query.prepare(updateSql);
    //绑定占位符
    for(int i=0;i<columnCount;i++)
    {
        query.addBindValue(rowValues.at(i));
    }
    query.addBindValue(whereColValue);
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec())
    {
        qDebug()<<"update table error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:  修改一个字段的数据
 *@author:  缪庆瑞
 *@date:    2018.1.4
 *@param:   tableName:表名
 *@param:   columnName:要修改数据的列
 *@param:   rowValue:对应列修改的值
 *@param:   whereSql:条件　例如：Sno='1001'
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::updateTable(QString tableName, QString columnName, QVariant rowValue, QString whereSql)
{
    QString updateSql = QString("update %1 set %2 = ?").arg(tableName,columnName);
    if(whereSql.isEmpty())
    {
        updateSql.append(";");
    }
    else
    {
        updateSql.append(" where "+whereSql+";");
    }
    //执行Sql命令
    QSqlQuery query(db);//创建sql语句执行对象
    query.prepare(updateSql);
    query.addBindValue(rowValue);
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec())
    {
        qDebug()<<"update table error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:  修改一个字段的数据  针对where条件为columnName = colnumValue;
 *@author:  缪庆瑞
 *@date:    2018.1.5
 *@param:   tableName:表名
 *@param:   columnName:要修改数据的列
 *@param:   rowValue:对应列修改的值
 *@param:   whereColName:　where条件的列名
 *@param:   whereColValue:　where条件的列值
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::updateTable(QString tableName, QString columnName, QVariant rowValue, QString whereColName, QVariant whereColValue)
{
    QString updateSql = QString("update %1 set %2 = ? where %3=?;").arg(tableName,columnName,whereColName);
    //执行Sql命令
    QSqlQuery query(db);//创建sql语句执行对象
    query.prepare(updateSql);
    query.addBindValue(rowValue);
    query.addBindValue(whereColValue);
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec())
    {
        qDebug()<<"update table error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:   修改数据
 *@author:  缪庆瑞
 *@date:    2018.1.4
 *@param:   updateSql:完整的修改数据的语句
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::updateTable(QString updateSql)
{
    QSqlQuery query(db);//创建sql语句执行对象
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec(updateSql))
    {
        qDebug()<<"update table error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:  删除数据
 *@author:  缪庆瑞
 *@date:    2017.12.26
 *@param:   tableName:表名
 *@param:   whereSql:条件　例如：Sno='1001',默认无条件删除整表数据
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::deleteTable(QString tableName, QString whereSql)
{
    QSqlQuery query(db);//创建sql语句执行对象
    QString deleteSql = QString("delete from %1").arg(tableName);
    if(whereSql.isEmpty())
    {
        deleteSql.append(";");
    }
    else
    {
        deleteSql.append(" where "+whereSql+";");
    }
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec(deleteSql))
    {
        qDebug()<<"delete table error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:  删除数据  针对where条件为columnName = colnumValue;
 *@author:  缪庆瑞
 *@date:    2018.1.5
 *@param:   tableName:表名
 *@param:   whereColName:　where条件的列名
 *@param:   whereColValue:　where条件的列值
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::deleteTable(QString tableName, QString whereColName, QVariant whereColValue)
{
    QString deleteSql = QString("delete from %1 where %2=?;").arg(tableName,whereColName);
    QSqlQuery query(db);//创建sql语句执行对象
    query.prepare(deleteSql);
    query.addBindValue(whereColValue);
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec())
    {
        qDebug()<<"delete table error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:   查询单行单列的某一数据
 *@author:  缪庆瑞
 *@date:    2018.1.4
 *@param:   tableName: 表名
 *@param:   columnName:查询的列名(字段名)
 *@parma:   whereSql:条件　例如：Sno='1001'　此处为主键字段或唯一性字段
 *@return:  返回值为QVariant类型,由上层根据字段类型自行转换
 */
QVariant DatabaseManager::selectSingleColData(QString tableName, QString columnName, QString whereSql)
{
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql = QString("select %1 from %2 where %3;").arg(columnName,tableName,whereSql);
    query.setForwardOnly(true);//设置结果集仅向前查询，内存不需要缓存结果，提高效率
#ifdef MT_SAFE
    QReadLocker locker(&readWriteLock);//读锁
#endif
    if(!query.exec(selectSql))
    {
        qDebug()<<"select table error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql;
        return QVariant();//返回无效的数据
    }
    if(query.next())//指向结果集的第一条记录
    {
        return query.value(0);
    }
    else
    {
        qDebug()<<"no select record:"<<selectSql;
        return QVariant();//返回无效的数据
    }
}
/*
 *@brief:   查询单行单列的某一数据  针对where条件为columnName = colnumValue;
 *@author:  缪庆瑞
 *@date:    2018.1.5
 *@param:   tableName: 表名
 *@param:   columnName:查询的列名(字段名)
 *@param:   whereColName:　where条件的列名　此处为主键字段或唯一性字段
 *@param:   whereColValue:　where条件的列值
 *@return:  返回值为QVariant类型,由上层根据字段类型自行转换
 */
QVariant DatabaseManager::selectSingleColData(QString tableName, QString columnName, QString whereColName, QVariant whereColValue)
{
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql = QString("select %1 from %2 where %3=?;").arg(columnName,tableName,whereColName);
    query.setForwardOnly(true);//设置结果集仅向前查询，内存不需要缓存结果，提高效率
    query.prepare(selectSql);
    query.addBindValue(whereColValue);
#ifdef MT_SAFE
    QReadLocker locker(&readWriteLock);//读锁
#endif
    if(!query.exec())
    {
        qDebug()<<"select table error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql<<whereColValue;
        return QVariant();//返回无效的数据
    }
    if(query.next())//指向结果集的第一条记录
    {
        return query.value(0);
    }
    else
    {
        qDebug()<<"no select record:"<<selectSql<<whereColValue;
        return QVariant();//返回无效的数据
    }
}
/*
 *@brief:   查询单行多列的数据
 *@author:  缪庆瑞
 *@date:    2018.1.4
 *@param:   tableName: 表名
 *@param:   columnNames:查询的多个列名(字段名)
 *@parma:   whereSql:条件　例如：Sno='1001'　此处为主键字段或唯一性字段
 *@return:  返回值为QList<QVariant>类型,由上层根据字段类型自行转换
 */
QVariantList DatabaseManager::selectMultiColData(QString tableName, QList<QString> columnNames, QString whereSql)
{
    QList<QVariant> valueList;
    QString columnNamesStr;
    //列名非空，查询对应字段数据
    if(!columnNames.isEmpty())
    {
        columnNamesStr.append(QStringList(columnNames).join(","));
    }
    else//列名为空，则查询整行数据
    {
        columnNamesStr.append("*");
    }
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql = QString("select %1 from %2 where %3;").arg(columnNamesStr,tableName,whereSql);
    query.setForwardOnly(true);//设置结果集仅向前查询，内存不需要缓存结果，提高效率
#ifdef MT_SAFE
    QReadLocker locker(&readWriteLock);//读锁
#endif
    if(!query.exec(selectSql))
    {
        qDebug()<<"select table error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql;
        return valueList;//返回无效的数据
    }
    if(query.next())//指向结果集的第一条记录
    {
        int count = query.record().count();//结果集一条记录的字段数
        for(int i=0;i<count;i++)
        {
            valueList.append(query.value(i));
        }
        return valueList;
    }
    else
    {
        qDebug()<<"no select record:"<<selectSql;
        return valueList;//返回无效的数据
    }
}
/*
 *@brief:   查询单行多列的数据   针对where条件为columnName = colnumValue;
 *@author:  缪庆瑞
 *@date:    2018.1.5
 *@param:   tableName: 表名
 *@param:   columnNames:查询的多个列名(字段名)
 *@param:   whereColName:　where条件的列名  此处为主键字段或唯一性字段
 *@param:   whereColValue:　where条件的列值
 *@return:  返回值为QList<QVariant>类型,由上层根据字段类型自行转换
 */
QVariantList DatabaseManager::selectMultiColData(QString tableName, QList<QString> columnNames, QString whereColName, QVariant whereColValue)
{
    QList<QVariant> valueList;
    QString columnNamesStr;
    //列名非空，查询对应字段数据
    if(!columnNames.isEmpty())
    {
        columnNamesStr.append(QStringList(columnNames).join(","));
    }
    else//列名为空，则查询整行数据
    {
        columnNamesStr.append("*");
    }
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql = QString("select %1 from %2 where %3=?;").arg(columnNamesStr,tableName,whereColName);
    query.setForwardOnly(true);//设置结果集仅向前查询，内存不需要缓存结果，提高效率
    query.prepare(selectSql);
    query.addBindValue(whereColValue);
#ifdef MT_SAFE
    QReadLocker locker(&readWriteLock);//读锁
#endif
    if(!query.exec())
    {
        qDebug()<<"select table error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql<<whereColValue;
        return valueList;//返回无效的数据
    }
    if(query.next())//指向结果集的第一条记录
    {
        int count = query.record().count();//结果集一条记录的字段数
        for(int i=0;i<count;i++)
        {
            valueList.append(query.value(i));
        }
        return valueList;
    }
    else
    {
        qDebug()<<"no select record:"<<selectSql<<whereColValue;
        return valueList;//返回无效的数据
    }
}
/*
 *@brief:   查询多行单列的某一数据
 * sqlite3默认会对查询的主键字段按升序排列，其他情况如果不显式使用order by
 * 的话，则按照插入顺序排列。
 * 若要显式使用order by，可以将其放在该函数的whereSql函参中，如果是无条件
 * 查询且要显式排序，whereSql可以这样写："1=1 order by columName ASC/DESC"
 *@author:  缪庆瑞
 *@date:    2018.8.29
 *@param:   tableName: 表名
 *@param:   columnName:查询的列名(字段名)
 *@param:   isDistinct:是否对查询结果去重　默认true
 *@parma:   whereSql:条件　例如：Sno='1001'　为空则表示无条件查询某列
 *@return:  返回值为QVariantList类型,由上层根据字段类型自行转换
 */
QVariantList DatabaseManager::selectSingleColDatas(QString tableName, QString columnName, bool isDistinct, QString whereSql)
{
    QList<QVariant> recordList;
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql;
    if(isDistinct)//去重
    {
        selectSql = QString("select distinct %1 from %2").arg(columnName,tableName);
    }
    else
    {
        selectSql = QString("select %1 from %2").arg(columnName,tableName);
    }
    if(whereSql.isEmpty())
    {
        selectSql.append(";");
    }
    else
    {
        selectSql.append(" where "+whereSql+";");
    }
    query.setForwardOnly(true);//设置结果集仅向前查询，内存不需要缓存结果，提高效率
#ifdef MT_SAFE
    QReadLocker locker(&readWriteLock);//读锁
#endif
    if(!query.exec(selectSql))
    {
        qDebug()<<"select table error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql;
        return recordList;//返回空的列表
    }
    while(query.next())//指向结果集的第一条记录
    {
        recordList.append(query.value(0));
    }
    return recordList;
}
/*
 *@brief:   查询多行单列的某一数据  针对where条件为columnName = colnumValue;
 * sqlite3默认会对查询的主键字段按升序排列，其他情况如果不显式使用order by
 * 的话，则按照插入顺序排列。不过该函数未提供order by的接口，要显式使用order by
 * 请调用上一个重载函数
 *@author:  缪庆瑞
 *@date:    2018.8.29
 *@param:   tableName: 表名
 *@param:   columnName:查询的列名(字段名)
 *@param:   whereColName:　where条件的列名
 *@param:   whereColValue:　where条件的列值
 *@param:   isDistinct: 是否对查询结果去重　默认true
 *@return:  返回值为QVariantList类型,由上层根据字段类型自行转换
 */
QVariantList DatabaseManager::selectSingleColDatas(QString tableName, QString columnName, QString whereColName, QVariant whereColValue, bool isDistinct)
{
    QList<QVariant> recordList;
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql;
    if(isDistinct)//去重
    {
        selectSql = QString("select distinct %1 from %2 where %3=?;").arg(columnName,tableName,whereColName);
    }
    else
    {
        selectSql = QString("select %1 from %2 where %3=?;").arg(columnName,tableName,whereColName);
    }
    query.setForwardOnly(true);//设置结果集仅向前查询，内存不需要缓存结果，提高效率
    query.prepare(selectSql);
    query.addBindValue(whereColValue);
#ifdef MT_SAFE
    QReadLocker locker(&readWriteLock);//读锁
#endif
    if(!query.exec())
    {
        qDebug()<<"select table error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql<<whereColValue;
        return recordList;//返回空的列表
    }
    while(query.next())//指向结果集的第一条记录
    {
        recordList.append(query.value(0));
    }
    return recordList;
}
/*
 *@brief:   查询数据库表的行数
 *@author:  缪庆瑞
 *@date:    2018.8.29
 *@param:   tableName: 表名
 *@param:   whereSql:　条件，默认无条件查询
 *@return:  返回值int类型 无效返回-1
 */
int DatabaseManager::selectRowCount(QString tableName, QString whereSql)
{
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql = QString("select count(0) from %1").arg(tableName);
    if(whereSql.isEmpty())
    {
        selectSql.append(";");
    }
    else
    {
        selectSql.append(" where "+whereSql+";");
    }
#ifdef MT_SAFE
    QReadLocker locker(&readWriteLock);//读锁
#endif
    if(!query.exec(selectSql))
    {
        qDebug()<<"select table row error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql;
        return -1;
    }
    if(query.next())
    {
        return query.value(0).toInt();
    }
    else
    {
        return -1;
    }
}
/*
 *@brief:   查询表是否存在
 *@author:  缪庆瑞
 *@date:    2018.8.22
 *@param:   tableName: 表名
 *@return:  bool:true=存在　　false=不存在
 */
bool DatabaseManager::isExistTable(QString tableName)
{
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql = QString("select count(type) from sqlite_master where type='table' and "
                                "name='%1';").arg(tableName);
    query.setForwardOnly(true);//设置结果集仅向前查询，内存不需要缓存结果，提高效率
#ifdef MT_SAFE
    QReadLocker locker(&readWriteLock);//读锁
#endif
    if(!query.exec(selectSql))
    {
        qDebug()<<"select table error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql;
        return false;
    }
    if(query.next())//指向结果集的第一条记录
    {
        return query.value(0).toBool();
    }
    else
    {
        qDebug()<<"no select record..."<<selectSql;
        return false;
    }
}
/*
 *@brief:   附加数据库　实现在一个数据库里使用另一个数据库的数据
 *@author:  缪庆瑞
 *@date:    2018.8.21
 *@param:   attachDbName:附加数据库名
 *@param:   aliasName:附加数据库的别名
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::attachDB(QString attachDbName, QString aliasName)
{
    QString attachSql = QString("attach database '%1' as '%2';").arg(attachDbName).arg(aliasName);
    QSqlQuery query(db);//创建sql语句执行对象
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec(attachSql))
    {
        qDebug()<<"attach database error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:   分离数据库　分离attach附加数据库的别名
 *@author:  缪庆瑞
 *@date:    2018.8.21
 *@param:   aliasName:附加数据库的别名
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::detachDB(QString aliasName)
{
    QString detachSql = QString("detach database '%1';").arg(aliasName);
    QSqlQuery query(db);//创建sql语句执行对象
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    if(!query.exec(detachSql))
    {
        qDebug()<<"detach database error:"<<query.lastError();
        return false;
    }
    return true;
}
/*
 *@brief:   复制数据库表　仅执行insert的sql语句
 *@author:  缪庆瑞
 *@date:    2018.8.21
 *@param:   srcTableName:源表名
 *@param:   desTableName:目的表名
 *@return:  返回值为布尔类型，true:成功，false:失败
 */
bool DatabaseManager::onlyCopyTable(QString srcTableName, QString desTableName)
{
    /*sqlite数据内复制表主要有两种方式:
     * 1.使用"create table new_table as select * from old_table;"直接创建一个相同结构的新表，
     * 并复制所有的数据。但该方式存在的问题是表的主键属性不能被复制，且字段类型复制后会
     * 变成动态数据类型。
     * 2.先手动创建一个相同结构的表new_table，包括主键属性。然后使用"insert into new_table
     * select * from old_table;"将旧表中的所有数据插入到新表
     */
    //QString copySql = QString("create table %1 as select * from %2;").arg(desTableName).arg(srcTableName);
    QString copySql = QString("insert into %1 select * from %2;").arg(desTableName).arg(srcTableName);
    QSqlQuery query(db);//创建sql语句执行对象
#ifdef MT_SAFE
    QWriteLocker locker(&readWriteLock);//写锁
#endif
    //qDebug()<<"onlyCopyTable start time:"<<QTime::currentTime().toString("hh:mm:ss:zzz");
    if(!query.exec(copySql))
    {
        qDebug()<<"copy table error:"<<query.lastError();
        return false;
    }
    //qDebug()<<"onlyCopyTable end time:"<<QTime::currentTime().toString("hh:mm:ss:zzz");
    return true;
}
/*
 *@brief:   查询表是否存在 该方法主要用在内部的copyTable()函数内
 * 无论MT_SAFE宏是否定义，该方法都不会加读锁，避免同一个线程在copyTable()
 * 函数内加写锁，然后再加读锁造成死锁
 *@author:  缪庆瑞
 *@date:    2018.12.3
 *@param:   tableName: 表名
 *@return:  bool:true=存在　　false=不存在
 */
bool DatabaseManager::isExistTableForCopyTable(QString tableName)
{
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql = QString("select count(type) from sqlite_master where type='table' and "
                                "name='%1';").arg(tableName);
    query.setForwardOnly(true);//设置结果集仅向前查询，内存不需要缓存结果，提高效率
    if(!query.exec(selectSql))
    {
        qDebug()<<"select table error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql;
        return false;
    }
    if(query.next())//指向结果集的第一条记录
    {
        return query.value(0).toBool();
    }
    else
    {
        qDebug()<<"no select record..."<<selectSql;
        return false;
    }
}
/*
 *@brief:   获取表的创建语句 该方法主要用在内部的copyTable()函数内
 * 无论MT_SAFE宏是否定义，该方法都不会加读锁，避免同一个线程在copyTable()
 * 函数内加写锁，然后再加读锁造成死锁
 *@author:  缪庆瑞
 *@date:    2018.12.3
 *@param:   masterTableName:主表名(sqlite_master)
 *@param:   tableName: 要查的表名
 *@return:  QString:表结构的创建语句
 */
QString DatabaseManager::getCreateTableSqlForCopyTable(QString masterTableName, QString tableName)
{
    QSqlQuery query(db);//创建sql语句执行对象
    QString selectSql = QString("select sql from %1 where type='table' and name='%2';").arg(masterTableName,tableName);
    query.setForwardOnly(true);//设置结果集仅向前查询，内存不需要缓存结果，提高效率
    if(!query.exec(selectSql))
    {
        qDebug()<<"select table error:"<<query.lastError();
        qDebug()<<"error sql:"<<selectSql;
        return QString();//返回无效的数据
    }
    if(query.next())//指向结果集的第一条记录
    {
        return query.value(0).toString();
    }
    else
    {
        qDebug()<<"no select record:"<<selectSql;
        return QString();//返回无效的数据
    }
}
