#include "database.h"


//建立一个数据库连接
bool DataBase::createConnection()
{
    //以后就可以用"sqlite1"与数据库进行连接了
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(QString("%1/%2/TW_sql1.db").arg(AppPath).arg(App::FileName));
    if( !db.open())
    {
        QMessageBox::critical(0, "Cannot open database",
            "Unable to establish a database connection.", QMessageBox::Cancel);
        return false;
    }
    return true;
}

//创建数据库表
bool DataBase::createTable()
{
    QSqlDatabase db; //建立数据库连接
    QSqlQuery query(db);
    bool success = query.exec("create table RobotData("
                              "ID int primary key,"     //ID
                              "MirrorNum varchar(20),"  //MirrorNum
                              "Cleanliness float,"        //Cleanliness
                              "Hum float,"                //Hum
                              "Temp float,"               //Temp
                              "Time varchar(20))");     //Time
    if(success)
    {
        qDebug() << QObject::tr("数据库表创建成功！\n");
        return true;
    }
    else
    {
        qDebug() << QObject::tr("数据库表创建失败！\n");
        return false;
    }
}

//向数据库中插入记录
bool DataBase::insert(int ID, QString MirrorNum, float Cleanliness,
                      float Hum, float Temp, QString Time)
{
    QSqlDatabase db = QSqlDatabase::database("sqlite1"); //建立数据库连接
    QSqlQuery query(db);
    query.prepare("insert into RobotData values(?, ?, ?, ?, ?, ?)");

    query.bindValue(0, ID);
    query.bindValue(1, MirrorNum);
    query.bindValue(2, Cleanliness);
    query.bindValue(3, Hum);
    query.bindValue(4, Temp);
    query.bindValue(5, Time);

    bool success=query.exec();
    if(!success)
    {
        QSqlError lastError = query.lastError();
        qDebug() << lastError.driverText() << QString(QObject::tr("插入失败"));
        return false;
    }
    return true;
}

//查询所有信息
bool DataBase::queryAll()
{
    QSqlDatabase db = QSqlDatabase::database("sqlite1"); //建立数据库连接
    QSqlQuery query(db);
    query.exec("select * from RobotData");
    QSqlRecord rec = query.record();
    qDebug() << QObject::tr("RobotData表字段数：" ) << rec.count();

    while(query.next())
    {
        qDebug() << query.value(0).toInt()
                 << query.value(1).toString()
                 << query.value(2).toFloat()
                 << query.value(3).toFloat()
                 << query.value(4).toFloat()
                 << query.value(5).toString();
    }
}

//查询指定信息--洁净度查询
float DataBase::query(QString Name)
{
    QSqlDatabase db = QSqlDatabase::database("sqlite1"); //建立数据库连接
    QSqlQuery query(db);
    query.exec(QString("select * from RobotData where MirrorNum='%1'").arg(Name));

    while(query.next())
    {
        return query.value(2).toFloat();
//        qDebug() << query.value(0).toInt()
//                 << query.value(1).toString()
//                 << query.value(2).toFloat()
//                 << query.value(3).toFloat()
//                 << query.value(4).toFloat()
//                 << query.value(5).toString();
    }

}
//根据ID删除记录
bool DataBase::deleteById(int ID)
{
    QSqlDatabase db = QSqlDatabase::database("sqlite1"); //建立数据库连接
    QSqlQuery query(db);
    query.prepare(QString("delete from RobotData where ID = %1").arg(ID));
    if(!query.exec())
    {
        qDebug() << "删除记录失败！";
        return false;
    }
    return true;
}

//根据ID更新记录
bool DataBase::updateById(int ID, QString MirrorNum, float Cleanliness,
                          float Hum, float Temp, QString Time)
{
    QSqlDatabase db = QSqlDatabase::database("sqlite1"); //建立数据库连接
    QSqlQuery query(db);
    query.prepare(QString("update RobotData set MirrorNum=?,Cleanliness=?,"
                             "Hum=?, Temp=?,"
                             "Time=? where ID=%1").arg(ID));

    query.bindValue(0, ID);
    query.bindValue(1, MirrorNum);
    query.bindValue(2, Cleanliness);
    query.bindValue(3, Hum);
    query.bindValue(4, Temp);
    query.bindValue(5, Time);


     bool success=query.exec();
     if(!success)
     {
          QSqlError lastError = query.lastError();
          qDebug() << lastError.driverText() << QString(QObject::tr("更新失败"));
     }
    return true;
}

//排序
bool DataBase::sortById()
{
    QSqlDatabase db = QSqlDatabase::database("sqlite1"); //建立数据库连接
    QSqlQuery query(db);
    bool success=query.exec("select * from RobotData order by ID desc");
    if(success)
    {
        qDebug() << QObject::tr("排序成功");
        return true;
    }
    else
    {
        qDebug() << QObject::tr("排序失败！");
        return false;
    }
}

