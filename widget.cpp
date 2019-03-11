#include "widget.h"
#include "ui_widget.h"

DatabaseManager *databaseManager;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    databaseManager = new DatabaseManager("test");
    thread = new MyThread();
    thread2 = new MyThread2();

}

Widget::~Widget()
{
    delete ui;
}
//连接
void Widget::on_pushButton_clicked()
{
    QString databaseName = ui->lineEdit->text();
    if(databaseManager->createSqliteConnection(databaseName))
    {
        ui->recordLabel->setText("create sqlite connection success;");
    }
    qDebug()<<"integrity_check:"<<databaseManager->integrityCheck();
}
//建表
void Widget::on_pushButton_2_clicked()
{
    QList <QString> columnNames;
    QList <QString> columnTypes;
    QString tableName = ui->lineEdit_2->text();
    columnNames<<"id"<<"name"<<"score"<<"age";
    columnTypes<<"int "<<"varchar(20)"<<"float"<<"int";
    if(databaseManager->createTable(tableName,columnNames,columnTypes))
    {
        ui->recordLabel->setText("create table success;");
    }

}
//删表
void Widget::on_pushButton_3_clicked()
{
    QString tableName = ui->lineEdit_3->text();
    if(databaseManager->dropTable(tableName))
    {
        ui->recordLabel->setText("drop table success;");
    }
}
//插数(单条)
void Widget::on_pushButton_4_clicked()
{
    QString tableName = ui->lineEdit_4->text();
    QVariantList list;
    list<<101<<QString::fromUtf8("狗剩")<<90.5<<18;
    qDebug()<<"insert table start:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
    if(databaseManager->insertTable(tableName,list))
    {
        ui->recordLabel->setText("insert table success;");
    }
    QList<QString> columnNames;
    columnNames<<"id"<<"score";
    list.clear();
    list<<102<<91.5;
    if(databaseManager->insertTable(tableName,list,columnNames))
    {
        ui->recordLabel->setText("insert table success;");
    }
    qDebug()<<"insert table end:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
}
//插数(批量 事务操作)  在板子上测试１s时间大约能插入一万多条数据
void Widget::on_pushButton_5_clicked()
{
    QString tableName = ui->lineEdit_5->text();
    QVariantList idList;
    QVariantList nameList;
    QVariantList scoreList;
    QVariantList ageList;
    QList<QVariantList> list;
    QStringList name;
    name<<"aabd"<<"abae"<<"csde"<<"sedad"<<"eadf"<<"afaf"<<"eddg"<<"aeh"<<"iesa"<<"jdae"<<"dek"<<"eal"<<"adm"<<"edan";
    int nameNum = name.size();
    for(int i =0;i<100000;i++)
    {
        idList<<i;
        nameList<<name.at(i%nameNum);
        scoreList<<qrand()%10+85;
        ageList<<qrand()%5+16;
    }
    list<<idList<<nameList<<scoreList<<ageList;
    qDebug()<<"insert  batch table start:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
    if(databaseManager->insertBatchTable(tableName,list))
    {
        ui->recordLabel->setText("insert  batch table success;");
    }
    qDebug()<<"insert batch table end:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
}
//插数(批量　单条sql长语句) 经测试发现貌似Qt针对sqlite的驱动不支持这种语法
void Widget::on_pushButton_6_clicked()
{
    QString tableName = ui->lineEdit_6->text();
    QString insertSql = QString("insert into %1 values").arg(tableName);
    insertSql.append("(1,'a',10,18),(2,'b',20,19),(3,'c',30,19),(4,'d',40,20);");
    qDebug()<<"insert  batch table start:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
    if(databaseManager->insertTable(insertSql))
    {
        ui->recordLabel->setText("insert  batch table success;");
    }
    qDebug()<<"insert  batch table end:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
}
//改数(单条)
void Widget::on_pushButton_7_clicked()
{
    QString tableName = ui->lineEdit_7->text();
    //修改一个字段的数据
    qDebug()<<"start:"<<QTime::currentTime().toString("hh:mm:ss:zzz");
    if(databaseManager->updateTable(tableName,"name",QVariant("haha"),"id",QVariant(101)))
    {
        qDebug()<<"update table success;";
        qDebug()<<databaseManager->selectSingleColData(tableName,"name","id = 101").toString();
    }
    qDebug()<<"end:"<<QTime::currentTime().toString("hh:mm:ss:zzz");
    if(databaseManager->updateTable(tableName,"name",QVariant("hehe"),"id = 102"))
    {
        qDebug()<<"update table success;";
    }
    //修改多个字段的数据
    QList<QString> columnNames;
    QVariantList rowValues;
    columnNames<<"name"<<"score";
    rowValues<<"heihei"<<30.1;
    if(databaseManager->updateTable(tableName,columnNames,rowValues,"name",QVariant("c")))
    {
        qDebug()<<"update table success;";
    }
    if(databaseManager->updateTable(tableName,columnNames,rowValues,"name = 'd'"))
    {
        qDebug()<<"update table success;";
    }
}
//删数(单条)
void Widget::on_pushButton_8_clicked()
{
    QString tableName = ui->lineEdit_8->text();
    int idValue = 109;
    if(databaseManager->deleteTable(tableName,"id",idValue))
    {
        qDebug()<<"delete table success;";
    }
    if(databaseManager->deleteTable(tableName,"id = 108"))
    {
        qDebug()<<"delete table success;";
    }
}
//删数(整表)
void Widget::on_pushButton_9_clicked()
{
    QString tableName = ui->lineEdit_9->text();
    if(databaseManager->deleteTable(tableName))
    {
        ui->recordLabel->setText("delete table success;");
    }
}
//查表
void Widget::on_pushButton_10_clicked()
{
    QString tableName = ui->lineEdit_10->text();
    /*//单行单列
    qDebug()<<"select table start:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
    QVariant name = databaseManager->selectSingleColData(tableName,"id","name like '%a%' order by id");
    qDebug()<<"select table end:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
    ui->label->setText(name.toString());*/
    int countNum1 = databaseManager->selectSingleColData(tableName,"count(id)","id = 2").toInt();
    int countNum2 = databaseManager->selectSingleColData(tableName,"count(id)","id != 2").toInt();
    qDebug()<<"id = 2 row number:"<<countNum1;
    qDebug()<<"id != 2 row number:"<<countNum2;
    ui->recordLabel->setText(QString::number(countNum1)+"   "+QString::number(countNum2));
    //单行多列
    /*QList<QString> columnNames;
    columnNames<<"id"<<"name";
    QVariantList list;
    list = databaseManager->selectMultiData(tableName,columnNames,"id",QVariant(103));
    qDebug()<<list.at(0).toInt()<<list.at(1).toString();
    list.clear();
    list = databaseManager->selectMultiData(tableName,columnNames,"id > 103");
    qDebug()<<list.at(0).toInt()<<list.at(1).toString();*/

}
//改数(整表)
void Widget::on_pushButton_11_clicked()
{
    QString tableName = ui->lineEdit_11->text();
    //修改一个字段的所有数据
    if(databaseManager->updateTable(tableName,"score",100))
    {
        qDebug()<<"update table success;";
    }
    //修改多个字段的所有数据
    QList<QString> columnNames;
    QVariantList rowValues;
    columnNames<<"id"<<"name";
    rowValues<<1<<"gousheng";
    if(databaseManager->updateTable(tableName,columnNames,rowValues))
    {
        qDebug()<<"update table success;";
    }
}
//关闭连接
void Widget::on_pushButton_12_clicked()
{
    databaseManager->closeConnection();
    /*qDebug()<<"11:"<<QSqlDatabase::connectionNames();
    {
    DatabaseManager databaseManager2("connection2");
    databaseManager2.createSqliteConnection("test.db");
    qDebug()<<"12:"<<QSqlDatabase::connectionNames();
    }
    qDebug()<<"13:"<<QSqlDatabase::connectionNames();*/

}
//开启子线程
void Widget::on_pushButton_13_clicked()
{
    thread->start();
}
//开启子线程2
void Widget::on_pushButton_14_clicked()
{
    thread2->start();
}
//关闭子线程1
void Widget::on_pushButton_15_clicked()
{
    thread->stop();
}
//关闭子线程2
void Widget::on_pushButton_16_clicked()
{
    thread2->stop();
}
//复制表 数据库内
void Widget::on_pushButton_17_clicked()
{
    qDebug()<<"copy table start:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
    QString desTableName = ui->lineEdit_12->text();
    databaseManager->copyTable("aa",desTableName);
    qDebug()<<"copy table stop:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
}
//复制表　数据库间
void Widget::on_pushButton_18_clicked()
{
    qDebug()<<"copy2 table start:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
    QString desTableName = ui->lineEdit_13->text();
    databaseManager->copyTable("./test2.db","aa",desTableName);
    qDebug()<<"copy2 table stop:"<<QTime::currentTime().toString("HH:mm:ss:zzz");
}
//查询表是否存在
void Widget::on_pushButton_19_clicked()
{
    QString tableName = ui->lineEdit_14->text();
    bool isExist = databaseManager->isExistTable(tableName);
    qDebug()<<"the table isExist is "<<isExist;
}
