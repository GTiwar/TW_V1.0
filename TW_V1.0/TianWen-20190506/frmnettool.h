
#ifndef FRMNETTOOL_H
#define FRMNETTOOL_H

#include <QWidget>
#include <QMainWindow>
#include <QString>
#include <iostream>
#include <QTimer>
#include <QDebug>
#include <vector>
#include <math.h>
#include <qmath.h>
#include "opencv2/opencv.hpp"
#include <opencv2/calib3d/calib3d.hpp>
#include <QProcess>
#include "app.h"
#include "database.h"
#include "mythread.h"

#define CAPTURE_WIDTH       320 //摄像头捕获分辨率：长
#define CAPTURE_HEIGHT      240 //摄像头捕获分辨率：宽

class QTcpSocket;
class TcpServer;
class QSqlTableModel;
using namespace cv;
using namespace std;

namespace Ui
{
class frmNetTool;
}

class frmNetTool : public QWidget
{
    Q_OBJECT

public:
    explicit frmNetTool(QWidget *parent = 0);
    ~frmNetTool();

    QString FileAddress = QString("%1/%2").arg(AppPath).arg(App::FileName);//文件根地址
    QString MirrorImgFile = QString("%1/%2/%3").arg(AppPath).arg(App::FileName).arg(App::ImgFileName);//图片保存路径
    MyThread thread;

//void on_txtTcpIPAddress_textChanged(const QString &arg1);
    void ShowVideo();
    void FaceRecognition();
    QStringList getMirrorName(const QString &path);

private:
    Ui::frmNetTool *ui;

    int msgMaxCount;
    int countTcpServer;
    int SqlIDcount;
    TcpServer *tcpServer;

    QTimer *timer;
    VideoCapture Capture;
    Mat Frame,FrameResized,FrameGray,FrameBinary;
    Mat imgOringal, imgGray, imgBinaryzation, imgCalibration;
    QImage getImg;//获取图片格式视频流
    QSqlTableModel *model;

private slots:
    void Ui_Config();
    void initForm();
    void initConfig();
    void saveConfig();

    void sendDataTcpServer();
    void sendDataTcpServer(QString data);
    void sendDataTcpServer(QString ip, int port, QString data);
    void appendTcpServer(quint8 type, QString msg);//数据挂载至界面

private slots:
    void RobotDataSend(QByteArray data);//控制数据发送
    void RobotControl();//机器人控制

private slots:

    void clientReadData(int, QString ip, int port, QByteArray data);
    void clientConnect(int, QString ip, int port);
    void clientDisConnect(int, QString ip, int port);


private slots:
    void on_btnTcpListen_clicked();
    void on_btnClearTcpServer_clicked();

private slots:
    void on_btngo_clicked();
    void on_btnstop_clicked();
    void on_btnleft_clicked();
    void on_btnback_clicked();
    void on_btnright_clicked();
    void on_btnahead_clicked();
    void on_btnbehind_clicked();
    void on_btnup_clicked();
    void on_btndown_clicked();
    void on_btnclockwise_clicked();
    void on_btncounterclockwise_clicked();
    void on_btnangledetect_clicked();

private slots:
    void GetFrame();
    void OpenCamera();
    void SaveImage();
    void ClickComboxShow(const QString &arg1);
    void ImportCleanliness();
    void on_btnsubmitchanges_clicked();
    void on_btncancelchanges_clicked();
    void on_btnsqlseek_clicked();
    void on_btnsqlseekall_clicked();
    void on_btnascendingsort_clicked();
    void on_btndescendingsort_clicked();
    void on_btnaddrecord_clicked();
    void on_btndeleteline_clicked();

    void on_btnautocontrol_clicked();
};

#endif // FRMNETTOOL_H
