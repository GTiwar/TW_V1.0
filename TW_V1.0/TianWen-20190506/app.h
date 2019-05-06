#ifndef APP_H
#define APP_H

class QString;

class App
{
public:
    //全局配置参数
    static int LeftPanelWidth;          //左面板宽度
    static int RightPanelWidth;         //右面板宽度

    static QString FileName;            //文件名
    static QString ImgFileName;         //图片缓存文件名
    static QString PlatForm;            //平台
    static QString SendFileName;        //发送配置文件名
    static QString DeviceFileName;      //模拟设备数据文件名
    static QString TcpIPAddress;        //IP地址
    //TCP服务器配置参数
    static bool HexSendTcpServer;       //16进制发送
    static bool HexReceiveTcpServer;    //16进制接收
    static int TcpListenPort;           //监听端口

    //运动控制按钮
//    static bool Start_Mode;       //启0停1 状态
    static bool ManualControl_Mode;       //自动0/手动1 控制
//    static bool Target_Mode;       //车体0/机械臂1控制
    static int Car_Mode;       //车体控制模式
    enum CarType
    {
        Car_Stop                = 0,
        Car_Go                  = 1,
        Car_Back                = 2,
        Car_Left                = 3,
        Car_Right               = 4,
        Car_Ahead               = 5,
        Car_Behind              = 6,
        Car_Up                  = 7,
        Car_Down                = 8,
        Car_clockwise           = 9,
        Car_counterclockwise    = 10,
        Car_angledetect         = 11
    };

    //读写配置参数及其他操作
    static void FileConfig();           //工作文件夹配置
    static void ReadConfig();           //读取配置参数
    static void WriteConfig();          //写入配置参数
    static void NewConfig();            //以初始值新建配置文件
    static bool CheckConfig();          //校验配置文件
    static void WriteError(QString str);//写入错误信息
    static void NewDir(QString dirName);//新建目录
};

#endif // APP_H
