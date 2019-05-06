#include "app.h"
#include "myhelper.h"
#include <QDir>

#ifdef __arm__
int App::LeftPanelWidth = 150;
int App::RightPanelWidth = 200;
QString App::PlatForm = "A9";
#else
int App::LeftPanelWidth = 120;
int App::RightPanelWidth = 170;
QString App::PlatForm = "WIN";
#endif
QString App::FileName = "TianWen";
QString App::ImgFileName = "TW_Img";
QString App::SendFileName = "send.txt";
QString App::DeviceFileName = "device.txt";
QString App::TcpIPAddress = "192.168.0.0";

bool App::HexSendTcpServer = false;
bool App::HexReceiveTcpServer = false;
int App::TcpListenPort = 1234;

//0x40：车体前进  0x41：车体后退  0x42：车体左转  0x43：车体右转
//0x44：检测设备向前  0x45：向后  0x46：向上  0x47：向下
//0x48：顺时针转  0x49：逆时针转
//bool Start_Mode = false;       //启0停1 状态
bool App::ManualControl_Mode = true;       //自动0/手动1 控制
//bool Target_Mode = false;       //车体0/机械臂1控制
int App::Car_Mode = 0;          //控制模式


void App::FileConfig()
{
    QDir dir;//创建临时文件夹
    dir.cd(AppPath);  //进入根文件夹
    if(!dir.exists(FileName))//判断需要创建的文件夹是否存在
    {
        dir.mkdir(FileName); //创建文件夹
        dir.cd(FileName);  //进入子文件夹
        if(!dir.exists(ImgFileName))//判断需要创建的文件夹是否存在
        {
            dir.mkdir(ImgFileName); //创建文件夹
        }
    }

}
void App::ReadConfig()
{
	if (!CheckConfig()) {
		return;
	}

    QString fileName = QString("%1/%2/%3_Config.ini").arg(AppPath).arg(FileName).arg(AppName);
	QSettings set(fileName, QSettings::IniFormat);

	set.beginGroup("AppConfig");
	App::LeftPanelWidth = set.value("LeftPanelWidth").toInt();
	App::RightPanelWidth = set.value("RightPanelWidth").toInt();
	App::PlatForm = set.value("PlatForm").toString();
    App::SendFileName = set.value("SendFileName").toString();
	App::DeviceFileName = set.value("DeviceFileName").toString();
	set.endGroup();

	set.beginGroup("TcpServerConfig");
	App::HexSendTcpServer = set.value("HexSendTcpServer").toBool();
	App::HexReceiveTcpServer = set.value("HexReceiveTcpServer").toBool();
	App::TcpListenPort = set.value("TcpListenPort").toInt();
	set.endGroup();

    set.beginGroup("CarControlConfig");
    App::ManualControl_Mode = set.value("ManualControl_Mode").toBool();
    App::Car_Mode = set.value("Car_Mode").toInt();
    set.endGroup();
}

void App::WriteConfig()
{
    QString fileName = QString("%1/%2/%3_Config.ini").arg(AppPath).arg(FileName).arg(AppName);
	QSettings set(fileName, QSettings::IniFormat);

	set.beginGroup("AppConfig");
	set.setValue("LeftPanelWidth", App::LeftPanelWidth);
	set.setValue("RightPanelWidth", App::RightPanelWidth);
	set.setValue("PlatForm", App::PlatForm);
	set.setValue("SendFileName", App::SendFileName);
	set.setValue("DeviceFileName", App::DeviceFileName);
	set.endGroup();

	set.beginGroup("TcpServerConfig");
	set.setValue("HexSendTcpServer", App::HexSendTcpServer);
	set.setValue("HexReceiveTcpServer", App::HexReceiveTcpServer);
	set.setValue("TcpListenPort", App::TcpListenPort);
	set.endGroup();

    set.beginGroup("CarControlConfig");
    set.setValue("ManualControl_Mode", App::ManualControl_Mode);
    set.setValue("Car_Mode", App::Car_Mode);
    set.endGroup();

}

void App::NewConfig()
{
#if (QT_VERSION <= QT_VERSION_CHECK(5,0,0))
#endif
	WriteConfig();
}

bool App::CheckConfig()
{
    QString fileName = QString("%1/%2/%3_Config.ini").arg(AppPath).arg(FileName).arg(AppName);

	//如果配置文件大小为0,则以初始值继续运行,并生成配置文件
	QFile file(fileName);

	if (file.size() == 0) {
		NewConfig();
		return false;
	}

	//如果配置文件不完整,则以初始值继续运行,并生成配置文件
	if (file.open(QFile::ReadOnly)) {
		bool ok = true;

		while (!file.atEnd()) {
			QString line = file.readLine();
			line = line.replace("\r", "");
			line = line.replace("\n", "");
			QStringList list = line.split("=");

			if (list.count() == 2) {
				if (list.at(1) == "") {
					ok = false;
					break;
				}
			}
		}

		if (!ok) {
			NewConfig();
			return false;
		}
	} else {
		NewConfig();
		return false;
	}

	return true;
}

void App::WriteError(QString str)
{
    QString fileName = QString("%1/%2/%3_Error_%4.txt").arg(AppPath).arg(FileName).arg(AppName).arg(QDATE);
	QFile file(fileName);
	file.open(QIODevice::WriteOnly | QIODevice::Append | QFile::Text);
	QTextStream stream(&file);
	stream << DATETIME << "  " << str << "\n";
}

void App::NewDir(QString dirName)
{
	//如果路径中包含斜杠字符则说明是绝对路径
	//linux系统路径字符带有 /  windows系统 路径字符带有 :/
	if (!dirName.startsWith("/") && !dirName.contains(":/")) {
        dirName = QString("%1/%2/%3").arg(AppPath).arg(FileName).arg(dirName);
	}

	QDir dir(dirName);

	if (!dir.exists()) {
		dir.mkpath(dirName);
	}
}
