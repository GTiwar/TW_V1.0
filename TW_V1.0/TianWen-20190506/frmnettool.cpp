#include "frmnettool.h"
#include "ui_frmnettool.h"
#include "myhelper.h"
#include "app.h"
#include "tcpserver.h"
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QTableView>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
frmNetTool::frmNetTool(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::frmNetTool)
{
    ui->setupUi(this);
    this->Ui_Config();
	this->initConfig();
    this->initForm();
	myHelper::formInCenter(this);

}

frmNetTool::~frmNetTool()
{
    if ((Capture.isOpened()))//释放资源
    {
       timer->stop();
       Capture.release();
    }

    delete timer;
    delete ui;
}



void frmNetTool::initForm()
{
    msgMaxCount = 50;
	countTcpServer = 0;
    SqlIDcount=0;
    timer = new QTimer(this);

    //数据库
    DataBase d;
    d.createConnection();
    d.createTable();
    model = new QSqlTableModel(this);
    model->setTable("RobotData");
    model->select();
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    //
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("ID"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("镜面"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("洁净度%"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("湿度值%"));
    model->setHeaderData(4,Qt::Horizontal,QObject::tr("温度值℃"));
    model->setHeaderData(5,Qt::Horizontal,QObject::tr("检测时间"));
    ui->TabViewSqlData->setModel(model);
    // TCP/IP
    QList<QHostAddress> addrs = QNetworkInterface::allAddresses();

	foreach (QHostAddress addr, addrs) {
		QString ip = addr.toString();

		if (ip.startsWith("192.168")) {
            this->setWindowTitle(QString("“天问号”巡检机器人 V1.1"));
            ui->txtTcpIPAddress->setText(ip);
			break;
		}
	}

	ui->btnSendTcpServer->setEnabled(false);
	ui->txtDataTcpServer->setReadOnly(true);

	tcpServer = new TcpServer(this);
    //tcp_connect
	connect(tcpServer, SIGNAL(clientConnect(int, QString, int)),
	        this, SLOT(clientConnect(int, QString, int)));
	connect(tcpServer, SIGNAL(clientDisConnect(int, QString, int)),
	        this, SLOT(clientDisConnect(int, QString, int)));
	connect(tcpServer, SIGNAL(clientReadData(int, QString, int, QByteArray)),
            this, SLOT(clientReadData(int, QString, int, QByteArray)));
	connect(ui->btnSendTcpServer, SIGNAL(clicked()), this, SLOT(sendDataTcpServer()));
    //Robot_connect
    connect(timer, SIGNAL(timeout()), this, SLOT(GetFrame()));
    connect(ui->btnCleanlinessCalc, SIGNAL(clicked()), this, SLOT(ImportCleanliness()));
    //Image_connect
    connect(ui->btnOpenCamera, SIGNAL(clicked()), this, SLOT(OpenCamera()));
    connect(ui->btnImgSave, SIGNAL(clicked()), this, SLOT(SaveImage()));
    //Combox show
    connect(ui->cboxMirrorNum, SIGNAL(currentIndexChanged(QString)), this, SLOT(ClickComboxShow(QString)));

 }

void frmNetTool::initConfig()
{
    App::FileConfig();
	App::ReadConfig();

	ui->frmTcpServer->setMinimumWidth(App::RightPanelWidth);
	ui->frmTcpServer->setMaximumWidth(App::RightPanelWidth);

    //Tcp server部分
	ui->ckHexSendTcpServer->setChecked(App::HexSendTcpServer);
	connect(ui->ckHexSendTcpServer, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

	ui->ckHexReceiveTcpServer->setChecked(App::HexReceiveTcpServer);
	connect(ui->ckHexReceiveTcpServer, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->txtTcpListenPort->setText(QString::number(App::TcpListenPort));
	connect(ui->txtTcpListenPort, SIGNAL(textChanged(QString)), this, SLOT(saveConfig()));

    //Robot部分
    ui->cboxMirrorNum->addItems(getMirrorName(MirrorImgFile));//读取图片文件
  }

void frmNetTool::saveConfig()
{
    App::HexSendTcpServer = ui->ckHexSendTcpServer->isChecked();//16进制发送
    App::HexReceiveTcpServer = ui->ckHexReceiveTcpServer->isChecked();//16进制接收
    App::TcpListenPort = ui->txtTcpListenPort->text().toInt();//端口号获取
	App::WriteConfig();
}



void frmNetTool::appendTcpServer(quint8 type, QString msg)
{
	if (countTcpServer > msgMaxCount) {
		ui->txtDataTcpServer->clear();
		countTcpServer = 0;
	}

	QString str;

	if (type == 0) {
		str = ">> 发送 :";
		ui->txtDataTcpServer->setTextColor(QColor("dodgerblue"));
	} else if (type == 1) {
		str = "<< 接收 :";
		ui->txtDataTcpServer->setTextColor(QColor("red"));
	}

    ui->txtDataTcpServer->append(QString("时间[%1] %2 %3").arg(TIMEMS).arg(str).arg(msg));
	countTcpServer++;
}

/************GPRS通讯************
data[0]------------0xb1：帧头
data[1]------------镜面数据：镜面1、镜面2、...
data[2]------------洁净度整数
data[3]------------洁净度小数
data[4]------------湿度整数
data[5]------------湿度小数
data[6]------------温度整数
data[7]------------温度小数
data[14]------------0xb9：帧尾
********************************/
void frmNetTool::clientReadData(int , QString ip, int port, QByteArray data)
{
	QString buffer;
    QString MirrorID;
    float CleanData, Hum=0, Temp=0;
    DataBase d;

    if(data[0]==0xb1 && data[14]==0xb9)//数据预处理
    {
        //定日镜ID
        MirrorID = tr("镜面%1").arg((int)data[1]);
        ui->lcdMirrorNum->display(MirrorID);
        //洁净度
//        CleanData = (data[2]*10+data[3])/10.0;
        CleanData = 0;
        //湿度
        Hum = (data[4]*10+data[5])/10.0;
        ui->lcdHum->display(Hum);
        //温度
        Temp = (data[6]*10+data[7])/10.0;
        ui->lcdTemp->display(Temp);

        //机器人经度
        ui->lcdJing_du->display(data[8]);//度
        ui->lcdJing_fen->display(data[9]);//分
        ui->lcdJing_miao->display(data[10]);//秒

        //机器人纬度
        ui->lcdWei_du->display(data[11]);//度
        ui->lcdWei_fen->display(data[12]);//分
        ui->lcdWei_miao->display(data[13]);//秒
        //数据库存储
        d.insert(data[1],MirrorID,CleanData,Hum,Temp,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }
	if (App::HexReceiveTcpServer) {
        buffer = myHelper::byteArrayToHexStr(data);
	} else {
		buffer = myHelper::byteArrayToAsciiStr(data);
	}

    appendTcpServer(1, QString("%1").arg(buffer));



}

void frmNetTool::clientConnect(int , QString ip, int port)
{
    appendTcpServer(1, QString("客户端[%1:%2] 上线").arg(ip).arg(port));

    ui->listTcpClient->clear();
    ui->listTcpClient->addItems(tcpServer->getClientInfo());
	ui->labTcpClientCount->setText(QString("已连接客户端共 %1 个").arg(tcpServer->getClientCount()));

    int count = ui->listTcpClient->count();

    if (count > 0) {
        ui->listTcpClient->setCurrentRow(count - 1);
    }
}

void frmNetTool::clientDisConnect(int , QString ip, int port)
{
	appendTcpServer(1, QString("客户端[%1:%2] 下线").arg(ip).arg(port));

    ui->listTcpClient->clear();
    ui->listTcpClient->addItems(tcpServer->getClientInfo());
	ui->labTcpClientCount->setText(QString("已连接客户端共 %1 个").arg(tcpServer->getClientCount()));

    int count = ui->listTcpClient->count();

    if (count > 0) {
        ui->listTcpClient->setCurrentRow(count - 1);
    }
}


void frmNetTool::sendDataTcpServer()
{
	QString data = ui->cboxSendTcpServer->currentText();
	sendDataTcpServer(data);
}

void frmNetTool::sendDataTcpServer(QString data)
{
	if (!tcpServer->isListening()) {
		return;
	}

	if (data.isEmpty()) {
		return;
	}

	bool all = ui->ckAllTcpServer->isChecked();
    QString str = ui->listTcpClient->currentIndex().data().toString();

	//没有一个连接则不用处理
	if (str.isEmpty()) {
		return;
	}

	QStringList list = str.split(":");
	QString ip = list.at(0);
	int port = list.at(1).toInt();

	QByteArray buffer;

	if (App::HexSendTcpServer) {
		buffer = myHelper::hexStrToByteArray(data);
	} else {
		buffer = myHelper::asciiStrToByteArray(data);
	}

	if (!all) {
        tcpServer->sendData(ip, port, buffer);
	} else {
		tcpServer->sendData(buffer);
	}

	appendTcpServer(0, data);
}

void frmNetTool::sendDataTcpServer(QString ip, int port, QString data)
{
	if (!tcpServer->isListening()) {
		return;
	}

	QByteArray buffer;

	if (App::HexSendTcpServer) {
		buffer = myHelper::hexStrToByteArray(data);
	} else {
		buffer = myHelper::asciiStrToByteArray(data);
	}

    tcpServer->sendData(ip, port, buffer);
	appendTcpServer(0, data);


}


void frmNetTool::on_btnTcpListen_clicked()
{
    if (ui->btnTcpListen->text() == "启动") {
//      bool ok = tcpServer->listen(QHostAddress::Any, App::TcpListenPort);
        bool ok = tcpServer->listen(QHostAddress::AnyIPv4, App::TcpListenPort);

        if (ok) {
			ui->btnTcpListen->setText("停止");
			ui->btnSendTcpServer->setEnabled(true);
			appendTcpServer(0, "监听成功");
		} else {
			appendTcpServer(1, "监听失败,请检查端口是否被占用");
		}
	} else {
        ui->listTcpClient->clear();
		tcpServer->closeAll();
        ui->btnTcpListen->setText("启动");
		ui->btnSendTcpServer->setEnabled(false);
		appendTcpServer(0, "停止监听成功");
	}
}

void frmNetTool::on_btnClearTcpServer_clicked()
{
	ui->txtDataTcpServer->clear();
	countTcpServer = 0;
}

void frmNetTool::RobotDataSend(QByteArray buffer)
{
    if (!tcpServer->isListening()) {
        return;
    }

    if (buffer.isEmpty()) {
        return;
    }

    bool all = ui->ckAllTcpServer->isChecked();
    QString str = ui->listTcpClient->currentIndex().data().toString();//获取当前连接IP

    //没有一个连接则不用处理
    if (str.isEmpty()) {
        return;
    }

    QStringList list = str.split(":");
    QString ip = list.at(0);
    int port = list.at(1).toInt();

    if (!all) {
        tcpServer->sendData(ip, port, buffer);
    } else {
        tcpServer->sendData(buffer);
    }

        QString data = QString(buffer);
        appendTcpServer(0, buffer);

}

//0x40：车体前进  0x41：车体后退  0x42：车体左转  0x43：车体右转
//0x44：检测设备向前  0x45：向后  0x46：向上  0x47：向下
//0x48：顺时针转  0x49：逆时针转
void frmNetTool::RobotControl()
{
    QByteArray Robotdata;
    Robotdata.resize(8);
    Robotdata[0]=0xa1;
    Robotdata[1]=0x11;//停止状态
    Robotdata[2]=0x00;//手动模式1，自动模式0
    Robotdata[3]=0x00;
    Robotdata[4]=0x00;
    Robotdata[5]=0x00;
    Robotdata[6]=0x00;
    Robotdata[7]=0xa9;

    if(App::Car_Mode == 0)          //Mode:0
    {
        Robotdata[1]=0x11;//停止状态
        Robotdata[3]=0x30;//车体控制
    }
    else if(App::Car_Mode>=1 && App::Car_Mode<=4)  //Mode:1-4
    {
        Robotdata[1]=0x10;//启动状态
        Robotdata[3]=0x30;//车体控制
        Robotdata[4]=0x40|(App::Car_Mode-1);
    }
    else if(App::Car_Mode>=5 && App::Car_Mode<=11) //Mode:5-11
    {
        Robotdata[1]=0x10;//启动状态
        Robotdata[3]=0x31;//机械臂控制
        Robotdata[4]=0x40|(App::Car_Mode-1);
    }
    if(App::ManualControl_Mode)
        Robotdata[2]= 0x21;//手动
    else
    {
        Robotdata[1]=0x10;
        Robotdata[2]= 0x20;//自动
    }
    RobotDataSend(Robotdata);
}

void frmNetTool::Ui_Config()
{
    QFont labFont("Microsoft YaHei", 12, 75); //第一个属性是字体（微软雅黑），第二个是大小，第三个是加粗（权重是87）
//lab控件显示初始
    ui->labTempunit->setFont(labFont);
    ui->labTemp->setFont(labFont);
    ui->labHumunit->setFont(labFont);
    ui->labHum->setFont(labFont);
    ui->labAtmosunit->setFont(labFont);
    ui->labAtmos->setFont(labFont);
    ui->labWeizhi->setFont(labFont);
    ui->labDongJing->setFont(labFont);
    ui->labJing_du->setFont(labFont);
    ui->labJing_fen->setFont(labFont);
    ui->labJing_miao->setFont(labFont);
    ui->labBeiWei->setFont(labFont);
    ui->labWei_du->setFont(labFont);
    ui->labWei_fen->setFont(labFont);
    ui->labWei_miao->setFont(labFont);
    ui->labSpeedunit->setFont(labFont);
    ui->labSpeed->setFont(labFont);
    ui->labPowerunit->setFont(labFont);
    ui->labPower->setFont(labFont);
//LCD显示初始
    ui->lcdTemp->setPalette(Qt::black);
    ui->lcdHum->setPalette(Qt::black);
    ui->lcdAtmos->setPalette(Qt::black);
    ui->lcdJing_du->setPalette(Qt::black);
    ui->lcdJing_fen->setPalette(Qt::black);
    ui->lcdJing_miao->setPalette(Qt::black);
    ui->lcdWei_du->setPalette(Qt::black);
    ui->lcdWei_fen->setPalette(Qt::black);
    ui->lcdWei_miao->setPalette(Qt::black);
    ui->lcdSpeed->setPalette(Qt::red);
    ui->lcdPower->setPalette(Qt::red);
//数据库初始：按内容调整列宽
    ui->TabViewSqlData->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
//按键初始
    ui->btnstop->setFixedSize(50,50);
    ui->btngo->setFixedSize(50,30);
    ui->btnback->setFixedSize(50,30);
    ui->btnleft->setFixedSize(50,30);
    ui->btnright->setFixedSize(50,30);
    ui->btnstop->setStyleSheet(
                           //正常状态样式
                           "QPushButton{"
                           "background-color:rgba(192,192,192,50);"//背景色（也可以设置图片）
                           "border-style:outset;"                  //边框样式（inset/outset）
                           "border-width:2px;"                     //边框宽度像素
                           "border-radius:25px;"                   //边框圆角半径像素
                           "border-color:rgba(0,0,0,0);"    //边框颜色
                           "font:bold 12px;"                       //字体，字体大小
                           "color:rgba(0,0,0,255);"          //字体颜色
                           "padding:6px;"                          //填衬
                           "}"
                           //鼠标悬停样式
                           "QPushButton:hover{"
                           "background-color:rgba(192,192,192,255);"
                           "color:rgba(0,0,0,200);"
                           "}"
                           //鼠标按下样式
                           "QPushButton:pressed{"
                           "background-color:rgba(245,100,100,150);"
                           "border-style:inset;"
                           "color:rgba(0,0,0,100);"
                           "}");
    ui->btngo->setStyleSheet(
                           //正常状态样式
                           "QPushButton{"
                           "background-color:rgba(192,192,192,255);"//背景色（也可以设置图片）
                           "font:bold 12px;"                       //字体，字体大小
                           "color:rgba(0,0,0,255);"          //字体颜色
                           "padding:6px;"                          //填衬
                           "}"

                           //鼠标悬停样式
                            "QPushButton:hover{"
                            "background-color:rgba(100,255,100,100);"
                            "border-color:rgba(0,0,0,255);"
                            "color:rgba(0,0,0,200);"
                            "}");
    ui->btnback->setStyleSheet(
                           //正常状态样式
                           "QPushButton{"
                           "background-color:rgba(192,192,192,255);"//背景色（也可以设置图片）
                           "font:bold 12px;"                       //字体，字体大小
                           "color:rgba(0,0,0,255);"          //字体颜色
                           "padding:6px;"                          //填衬
                           "}"
                           //鼠标悬停样式
                            "QPushButton:hover{"
                            "background-color:rgba(100,255,100,100);"
                            "border-color:rgba(0,0,0,255);"
                            "color:rgba(0,0,0,200);"
                            "}");
    ui->btnleft->setStyleSheet(
                           //正常状态样式
                           "QPushButton{"
                           "background-color:rgba(192,192,192,255);"//背景色（也可以设置图片）
                           "font:bold 12px;"                       //字体，字体大小
                           "color:rgba(0,0,0,255);"          //字体颜色
                           "padding:6px;"                          //填衬
                           "}"
                        //鼠标悬停样式
                           "QPushButton:hover{"
                           "background-color:rgba(100,255,100,100);"
                           "border-color:rgba(0,0,0,255);"
                           "color:rgba(0,0,0,200);"
                           "}");
    ui->btnright->setStyleSheet(
                           //正常状态样式
                           "QPushButton{"
                           "background-color:rgba(192,192,192,255);"//背景色（也可以设置图片）
                           "font:bold 12px;"                       //字体，字体大小
                           "color:rgba(0,0,0,255);"          //字体颜色
                           "padding:6px;"                          //填衬
                           "}"
                            //鼠标悬停样式
                           "QPushButton:hover{"
                           "background-color:rgba(100,255,100,100);"
                           "border-color:rgba(0,0,0,255);"
                           "color:rgba(0,0,0,200);"
                           "}");



}

void frmNetTool::on_btnstop_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_Stop;
    RobotControl();
}

void frmNetTool::on_btngo_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_Go;
    RobotControl();
}

void frmNetTool::on_btnback_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_Back;
    RobotControl();
}

void frmNetTool::on_btnleft_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_Left;
    RobotControl();
}

void frmNetTool::on_btnright_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_Right;
    RobotControl();
}

void frmNetTool::on_btnahead_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_Ahead;
    RobotControl();
}
void frmNetTool::on_btnbehind_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_Behind;
    RobotControl();
}

void frmNetTool::on_btnup_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_Up;
    RobotControl();
}
void frmNetTool::on_btndown_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_Down;
    RobotControl();
}

void frmNetTool::on_btnclockwise_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_clockwise;
    RobotControl();
}

void frmNetTool::on_btncounterclockwise_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_counterclockwise;
    RobotControl();
}


void frmNetTool::on_btnangledetect_clicked()
{
    App::ManualControl_Mode = true;
    App::Car_Mode = App::Car_angledetect;
    RobotControl();
}

void frmNetTool::on_btnautocontrol_clicked()
{
    App::ManualControl_Mode = false;
    RobotControl();
}




void frmNetTool::OpenCamera()
{
    if (!Capture.isOpened())
    {
        Capture.open(ui->cboxCameraPath->currentText().toInt());
        Capture.set(CV_CAP_PROP_FRAME_WIDTH, CAPTURE_WIDTH);
        Capture.set(CV_CAP_PROP_FRAME_HEIGHT, CAPTURE_HEIGHT);

        if (Capture.isOpened())
        {
            timer->start(30);
            ui->btnOpenCamera->setText("关闭相机");
        }
    }
    else
    {
        timer->stop();
        Capture.release();
        ui->labVideoDisplay->clear();
        ui->btnOpenCamera->setText("打开相机");
    }
}

void frmNetTool::GetFrame()//槽函数
{
    timer->stop();

    //获取原图并预处理
    Capture >> Frame;
    imgOringal=Frame.clone();    //深拷贝

    if(ui->ckRecognition->isChecked())
        FaceRecognition();
    else
        ShowVideo();

    timer->start();
}

void frmNetTool::ShowVideo()
{

    Mat Frame_Orign = Mat::zeros(Size(CAPTURE_WIDTH,CAPTURE_HEIGHT),CV_8UC3);
    Frame_Orign=imgOringal.clone();

    cv::resize(Frame_Orign,Frame_Orign,Size(ui->labVideoDisplay->width(), ui->labVideoDisplay->height()));
    cvtColor(Frame_Orign,Frame_Orign,CV_BGR2RGB);
    getImg = QImage((const uchar*)(Frame_Orign.data),Frame_Orign.cols,Frame_Orign.rows,Frame_Orign.cols*Frame_Orign.channels(),
                   QImage::Format_RGB888).scaled(ui->labVideoDisplay->width(),ui->labVideoDisplay->height());
    ui->labVideoDisplay->setPixmap(QPixmap::fromImage(getImg));
}

void frmNetTool::FaceRecognition()
{
    Mat Frame_Orign = Mat::zeros(Size(CAPTURE_WIDTH,CAPTURE_HEIGHT),CV_8UC3);
    Mat Frame_gray = Mat::zeros(Size(CAPTURE_WIDTH,CAPTURE_HEIGHT),CV_8UC3);
    Frame_Orign=imgOringal.clone();
    Frame_Orign.copyTo(Frame_gray);
    cvtColor(Frame_gray, Frame_gray, CV_BGR2GRAY);//转为灰度图
    cvtColor(Frame_Orign,Frame_Orign,CV_BGR2RGB);
    equalizeHist(Frame_gray, Frame_gray);//直方图均衡化，增加对比度方便处理

    CascadeClassifier eye_Classifier;  //载入分类器
    CascadeClassifier face_cascade;    //载入分类器

    //加载分类训练器，OpenCv官方文档提供的xml文档，可以直接调用
    //xml文档路径  opencv\sources\data\haarcascades
    if (!eye_Classifier.load("haarcascade_eye.xml"))  //需要将xml文档放在自己指定的路径下
    {
        cout << "Load haarcascade_eye.xml failed!" << endl;

    }

    if (!face_cascade.load("haarcascade_frontalface_alt.xml"))
    {
        cout << "Load haarcascade_frontalface_alt failed!" << endl;

    }

    //vector 是个类模板 需要提供明确的模板实参 vector<Rect>则是个确定的类 模板的实例化
    vector<Rect> eyeRect;
    vector<Rect> faceRect;

    //检测关于眼睛部位位置
    eye_Classifier.detectMultiScale(Frame_gray, eyeRect, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));//检测
    for (size_t eyeIdx = 0; eyeIdx < eyeRect.size(); eyeIdx++)
    {
        rectangle(Frame_Orign, eyeRect[eyeIdx], Scalar(0, 0, 255));   //用矩形画出检测到的位置
    }

    //检测关于脸部位置
    face_cascade.detectMultiScale(Frame_gray, faceRect, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(50, 50));//检测
    for (size_t i = 0; i < faceRect.size(); i++)
    {
        rectangle(Frame_Orign, faceRect[i], Scalar(0, 0, 255));      //用矩形画出检测到的位置
    }

    cv::resize(Frame_Orign,Frame_Orign,Size(ui->labVideoDisplay->width(), ui->labVideoDisplay->height()));
    getImg = QImage((const uchar*)(Frame_Orign.data),Frame_Orign.cols,Frame_Orign.rows,Frame_Orign.cols*Frame_Orign.channels(),
                   QImage::Format_RGB888).scaled(ui->labVideoDisplay->width(),ui->labVideoDisplay->height());
    ui->labVideoDisplay->setPixmap(QPixmap::fromImage(getImg));

}

void frmNetTool::SaveImage()
{
    static int CountImg=1;

    if(!getImg.isNull())
    {
        getImg.save(QString("%1/镜面%2.jpg").arg(MirrorImgFile).arg(CountImg));
        ui->labImgName->setText(QString("镜面%1.jpg").arg(++CountImg));

    }

}

//镜面图像显示+洁净度显示
void frmNetTool::ClickComboxShow(const QString &arg1)
{
    Mat img;
    QImage image;
    QString ImgFile;
    QString FileName =arg1;
    DataBase d;
    int index;
//    static QString MirrorNum;    //镜面编号：镜面1、镜面2...

//    MirrorNum = ui->cboxMirrorNum->currentText();
    ImgFile=QString("%1/%2").arg(MirrorImgFile).arg(arg1);//arg1为当前索引内容，等同于currentText();
    img = cv::imread(ImgFile.toLocal8Bit().data());
    cvtColor(img,img,CV_BGR2RGB);
    image = QImage((const uchar*)(img.data),img.cols,img.rows,img.cols*img.channels(),
                   QImage::Format_RGB888).scaled(ui->labImgDisplay->width(),ui->labImgDisplay->height());
    ui->labImgDisplay->setPixmap(QPixmap::fromImage(image));

    index = FileName.lastIndexOf(".");//去除后缀
    FileName.truncate(index);
    ui->lcdCleanliness->display(QString("%1").arg(d.query(FileName)));
}

//导入洁净度数据
void frmNetTool::ImportCleanliness()
{
    thread.start();
    disconnect(ui->cboxMirrorNum, SIGNAL(currentIndexChanged(QString)), this, SLOT(ClickComboxShow(QString)));
    ui->cboxMirrorNum->clear();//更新图片目录
    ui->cboxMirrorNum->addItems(getMirrorName(MirrorImgFile));
    connect(ui->cboxMirrorNum, SIGNAL(currentIndexChanged(QString)), this, SLOT(ClickComboxShow(QString)));
    //程序间交互：打开洁净度数据处理程序

}
QStringList frmNetTool::getMirrorName(const QString &path)
{
    QDir dir(path);
    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.png";
    QStringList files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
    return files;
}

void frmNetTool::on_btnsubmitchanges_clicked()
{
    //开始事务操作
    model->database().transaction();
    if(model->submitAll())
    {
        model->database().commit();//提交

    }
    else
    {
        model->database().rollback();//回滚
        QMessageBox::warning(this,tr("tableModel"),
                             tr("数据库错误：%1").arg(model->lastError().text()));
    }

}

void frmNetTool::on_btncancelchanges_clicked()
{
    model->revertAll();
}

void frmNetTool::on_btnsqlseek_clicked()
{
    QString MirrorNum = ui->txtsqlseek->text();

    model->setFilter(QString("MirrorNum='%1'").arg(MirrorNum));
    model->select();
}

void frmNetTool::on_btnsqlseekall_clicked()//显示全部数据
{
    model->setTable("RobotData");
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("ID"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("镜面"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("洁净度%"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("湿度值%"));
    model->setHeaderData(4,Qt::Horizontal,QObject::tr("温度值℃"));
    model->setHeaderData(5,Qt::Horizontal,QObject::tr("检测时间"));
    model->select();
}


void frmNetTool::on_btnascendingsort_clicked()//升序排列
{
    model->setSort(0,Qt::AscendingOrder);
    model->select();
}



void frmNetTool::on_btndescendingsort_clicked()//降序排列
{
    model->setSort(0,Qt::DescendingOrder);
    model->select();
}

void frmNetTool::on_btnaddrecord_clicked()
{
    //获得表的行数
    int rowNum = model->rowCount();
    int id = 10;

    //添加一行
    model->insertRow(rowNum);
    model->setData(model->index(rowNum,0),id);

    //也可以直接提交
//    model->submitAll();
}

void frmNetTool::on_btndeleteline_clicked()
{
    //获取选中的行
    int curRow = ui->TabViewSqlData->currentIndex().row();

    //删除该行
    model->removeRow(curRow);
    int ok = QMessageBox::warning(this,tr("删除当前行!"),
            tr("你确定删除当前行吗?"),QMessageBox::Yes,QMessageBox::No);
    if(ok == QMessageBox::No)
    {
        model->revertAll();//如果不删除则撤销
    }
    else
    {
        model->submitAll();//否则提交，在数据库中删除改行
    }
}

