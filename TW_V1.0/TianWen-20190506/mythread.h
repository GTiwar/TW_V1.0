#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include "app.h"

class MyThread : public QThread
{
public:

    QString VS_program =  QString("%1/TW_H2.0.exe").arg(AppPath);
    MyThread();
    ~MyThread();
    void run();
};

#endif // MYTHREAD_H
