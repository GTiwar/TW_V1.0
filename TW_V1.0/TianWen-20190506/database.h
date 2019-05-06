#ifndef DATABASE_H
#define DATABASE_H
#include <QTextCodec>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTime>
#include <QSqlError>
#include <QtDebug>
#include <QSqlDriver>
#include <QSqlRecord>
#include <QVariant>
#include "app.h"
#include "ui_frmnettool.h"
#include "frmnettool.h"
#include "QtSql/qsql.h"
#include "QtSql/QsqlDatabase"
#include "QtSql/QsqlQuery"
#include "QtSql/QsqlQueryModel"
class DataBase
{
public:
    bool createConnection();  //创建一个连接
    bool createTable();       //创建数据库表
    bool insert(int ID, QString MirrorNum, float Cleanliness,
                float Hum, float Temp, QString Time);            //出入数据
    bool queryAll();          //查询所有信息
    float query(QString Name);          //查询指定信息
    bool updateById(int ID, QString MirrorNum, float Cleanliness,
                    float Hum, float Temp, QString Time);  //更新
    bool deleteById(int ID);  //删除
    bool sortById();          //排序
};
#endif // DATABASE_H
