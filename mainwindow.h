#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>
#include <hidusage.h>

#include <QMainWindow>
#include <QMutex>

#define INITIAL_POS "initialPos"

struct ActionInfo
{
    qint64 actionTime; // 操作时间
    QString actionName;// 操作的按键名称
    int keyboardScanCode;// 键盘按键的扫描码
    int dx;// 鼠标移动的x轴量
    int dy;// y轴量
    bool isRelease;// 是否为松开按键
};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void startRecordOrStop();
    void startPlayOrStop();

protected:
    // 重写nativeEvent以处理Windows原生消息
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;


private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

private:
    Ui::MainWindow *ui;

    // 纳秒级高精度计时器
    QElapsedTimer timer;

    std::atomic<bool> isRecording{false}; // 是否正在录制
    //qint64 startRecordTimeMs = 0; // 开始录制的时间, 毫秒
    //QMutex isRecordingMutex;// 互斥量


    std::atomic<bool> isPlaying{false};// 是否正在播放
    //qint64 startPlayTimeMs = 0;// 开始播放的时间, 毫秒
    //QMutex isPlayingMutex;// 互斥量

    // 记录的时间间隔 us
    const qint64 m_recordInterval = 10;

    // 保存的录制文件所在文件夹
    QString appDataDir;

    // 添加时间控制成员变量
    //qint64 m_lastRecordTime = 0;

    // 临时存储最新的鼠标数据
    int m_latestDeltaX = 0;
    int m_latestDeltaY = 0;
    // 鼠标原始数据累积量
    int m_accumulatedDeltaX = 0;
    int m_accumulatedDeltaY = 0;

    // 鼠标缩放校准因子
    float m_calibrationFactor = 1.0f;

    // 原始鼠标速度
    int m_originalSpeed;

    // 校准函数：找到正确的缩放因子
    void calibratePlayback();

    bool getIsRecording();
    void setIsRecording(bool val);
    bool getIsPlaying();
    void setIsPlaying(bool val);

    bool isKeyPressed(int keyScanCode);
    bool isMouseButtonPressed(int mouseButton);
    void handleAndRecordKey(bool keyPressed, QString keyName, QSet<QString> *pressedKeySet, QString *recordStr, qint64 actionTime);
    bool saveRecorrdToFile(QString fileName, QString data);

    void simulateKeyPress(short scanCode, bool isKeyRelease);
    void simulateMouseAction(QString btnName, bool isKeyRelease);
    // 模拟鼠标相对移动
    void simulateMouseRelativeMove(int dx, int dy);
    // 模拟鼠标绝对移动
    void simulateMouseAbsolutelyMove(int x, int y);


    void scanRecordFiles();

    // 释放所有按键
    void releaseAllKeys();

    // 注册原始输入设备
    bool registerRawInput();

    // 处理原始输入数据
    void processRawInput(HRAWINPUT rawInput);

    // 鼠标设置
    void set1To1MouseMovement() {
        // 保存原始设置
        SystemParametersInfo(SPI_GETMOUSESPEED, 0, &m_originalSpeed, 0);

        // 设置鼠标速度为1:1（值为10，范围1-20）
        SystemParametersInfo(SPI_SETMOUSESPEED, 0, (void*)10, SPIF_UPDATEINIFILE);

        // 禁用鼠标加速度
        int mouseParams[3] = {0, 0, 0}; // 禁用加速度
        SystemParametersInfo(SPI_SETMOUSE, 0, mouseParams, SPIF_UPDATEINIFILE);
    }

    // 恢复鼠标设置
    void restoreMouseSettings() {
        SystemParametersInfo(SPI_SETMOUSESPEED, 0, (void*)m_originalSpeed, SPIF_UPDATEINIFILE);
    }

    // 线性移动鼠标到指定位置
    void moveMouseToPos(int targetX, int targetY);

};
#endif // MAINWINDOW_H
