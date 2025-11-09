#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "key_map.h"
#include <windows.h>

#include <QtConcurrent>
#include <QSet>
#include <QCursor>
#include <QInputDialog>
#include <QMessageBox>
#include <thread>

MainWindow *mainWindow = nullptr;

// 按键记录
QString recordStr = "";
QMutex recordStrMutex;

// 全局钩子句柄
static HHOOK g_keyboardHook = nullptr;

// 低级键盘钩子过程函数
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;

        // 检查按键按下事件（WM_KEYDOWN 或 WM_SYSKEYDOWN）
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // 检查是否为 F7 或 F8 键
            if (kbStruct->vkCode == VK_F7) {
                //qDebug() << "F7 press";

                // 使用Qt的信号机制确保在主线程中执行
                QMetaObject::invokeMethod(mainWindow, [=]() {
                    mainWindow->startRecordOrStop();
                }, Qt::QueuedConnection);
            }else if(kbStruct->vkCode == VK_F8){
                //qDebug() << "F8 press";

                // 使用Qt的信号机制确保在主线程中执行
                QMetaObject::invokeMethod(mainWindow, [=]() {
                    mainWindow->startPlayOrStop();
                }, Qt::QueuedConnection);
            }
        }
    }

    // 将事件传递给下一个钩子或系统
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mainWindow = this;

    // 获取软件本地数据目录
    appDataDir = QDir::homePath() + "/AppData/Local/KeyRecorderData/";
    QDir dir = QDir(QDir::homePath() + "/AppData/Local/");
    // 使用 QDir 创建不存在的目录
    if (!dir.exists("KeyRecorderData")) {
        if (!dir.mkdir("KeyRecorderData")) {
            QMessageBox::critical(this, "错误","存放录制文件的文件夹创建失败");
            //return;
        }
    }

    // 文件夹创建失败
    if(!dir.exists("KeyMappingToolData")){
        appDataDir = "";
    }

    // 扫描录制文件
    scanRecordFiles();

    // 安装低级键盘钩子
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
    if (!g_keyboardHook) {
        QMessageBox::critical(this,"错误", "安装键盘钩子失败！");
        return;
    }

    // 注册原始输入设备
    if (!registerRawInput()) {
        QMessageBox::critical(this, "错误", "注册原始输入设备失败!");
    } else {
        qDebug() << "注册原始输入设备成功!";
    }
}

MainWindow::~MainWindow()
{
    delete ui;

    // 卸载钩子
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }

    setIsRecording(false);
    setIsPlaying(false);

    // 恢复所有按键
    releaseAllKeys();
}

// 键盘按键是否处于被按下的状态
bool MainWindow::isKeyPressed(int keyScanCode){
    // 特殊处理左右修饰键
    switch(keyScanCode) {
    case 0x2A: // 左Shift扫描码
        return (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
    case 0x36: // 右Shift扫描码
        return (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
    case 0x1D: // 左Ctrl扫描码
        return (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0;
    case 0xE01D: // 右Ctrl扫描码（扩展扫描码）
        return (GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0;
    case 0x38: // 左Alt扫描码
        return (GetAsyncKeyState(VK_LMENU) & 0x8000) != 0;
    case 0xE038: // 右Alt扫描码（扩展扫描码）
        return (GetAsyncKeyState(VK_RMENU) & 0x8000) != 0;
    default:
        // 硬件扫描码转换成虚拟键码, 再获取状态
        // 其他按键使用虚拟键码查询
        int virtualKey = MapVirtualKey(keyScanCode, MAPVK_VSC_TO_VK);
        return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    }
}

bool MainWindow::isMouseButtonPressed(int mouseButtonVK){
    return (GetAsyncKeyState(mouseButtonVK) & 0x8000) != 0;
}

void MainWindow::on_pushButton_clicked()
{
    startRecordOrStop();
}

void MainWindow::handleAndRecordKey(bool keyPressed, QString keyName, QSet<QString> *pressedKeySet, QString *recordStr, qint64 actionTime){
    if(keyPressed){
        // 按键按下
        if(!pressedKeySet->contains(keyName)){
            pressedKeySet->insert(keyName);

            QMutexLocker locker(&recordStrMutex);
            recordStr->append(QString::number(actionTime) + " " + keyName + ":press\n");

            //qDebug() << keyName << ":press;";
        }
    }else{
        // 按键松开
        if(pressedKeySet->contains(keyName)){
            pressedKeySet->remove(keyName);

            QMutexLocker locker(&recordStrMutex);
            recordStr->append(QString::number(actionTime) + " " + keyName + ":release\n");

            //qDebug() << keyName << ":release;";
        }
    }
}

bool MainWindow::getIsRecording(){
    //QMutexLocker locker(&isRecordingMutex);
    //return isRecording;

    return isRecording.load(std::memory_order_acquire);
}
void MainWindow::setIsRecording(bool val){
    //QMutexLocker locker(&isRecordingMutex);
    //isRecording = val;

    isRecording.store(val, std::memory_order_release);
}
bool MainWindow::getIsPlaying(){
    // QMutexLocker locker(&isPlayingMutex);
    // return isPlaying;

    return isPlaying.load(std::memory_order_acquire);
}
void MainWindow::setIsPlaying(bool val){
    // QMutexLocker locker(&isPlayingMutex);
    // isPlaying = val;

    isPlaying.store(val, std::memory_order_release);
}


void MainWindow::on_pushButton_2_clicked()
{
    startPlayOrStop();
}

bool MainWindow::saveRecorrdToFile(QString fileName, QString data){
    // 创建一个 QFile 对象，并打开文件进行写入
    QFile file(appDataDir + fileName + ".record");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);  // 创建一个文本流对象
        out << data;             // 写入文本

        // 关闭文件
        file.close();

        return true;
    } else {
        return false;
    }
}

void MainWindow::simulateKeyPress(short scanCode, bool isKeyRelease){
    // 模拟键盘操作
    if(scanCode > 0){
        INPUT input = {0};

        short tmpDwFlags;

        // 设置为使用硬件扫描码, 并为某些功能按键添加扩展码
        if(scanCode >= 0xC5 && scanCode <= 0xDF ){
            tmpDwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY;
        }else{
            tmpDwFlags = KEYEVENTF_SCANCODE;
        }

        // 模拟按下键
        input.type = INPUT_KEYBOARD;
        input.ki.dwFlags = tmpDwFlags;
        // 设置扫描码
        input.ki.wScan = scanCode;

        // 模拟释放键
        if(isKeyRelease){
            input.ki.dwFlags = tmpDwFlags | KEYEVENTF_KEYUP;
        }

        SendInput(1, &input, sizeof(INPUT));
    }
}

// 模拟鼠标相对移动
void MainWindow::simulateMouseRelativeMove(int dx, int dy){
    // 构造鼠标事件
    INPUT input = {0};
    input.type = INPUT_MOUSE;

    input.mi.dwFlags = MOUSEEVENTF_MOVE;  // 相对移动
    input.mi.dx = dx;
    input.mi.dy = dy;

    // 发送鼠标事件
    SendInput(1, &input, sizeof(INPUT));
}

// 模拟鼠标绝对移动
void MainWindow::simulateMouseAbsolutelyMove(int x, int y){
    // 构造鼠标事件
    INPUT input = {0};
    input.type = INPUT_MOUSE;

    // 获取屏幕分辨率
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 转换为绝对坐标（0-65535）
    int absoluteX = (x * 65535) / (screenWidth - 1);
    int absoluteY = (y * 65535) / (screenHeight - 1);

    // 使用绝对移动
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    input.mi.dx = absoluteX;
    input.mi.dy = absoluteY;

    // 发送鼠标事件
    SendInput(1, &input, sizeof(INPUT));
}

void MainWindow::simulateMouseAction(QString btnName, bool isKeyRelease){
    // 构造鼠标事件
    INPUT input = {0};
    input.type = INPUT_MOUSE;

    // {"mouseLeft", 0x01},// 左键
    // {"mouseRight", 0x02},// 右键
    // {"mouseMiddle", 0x04},// 中键（滚轮按下）
    // {"mouseSide1", 0x05}, // 侧键1（后退）
    // {"mouseSide2", 0x06}, // 侧键2（前进）

    // 鼠标左键点击
    if(btnName == "mouseLeft"){
        input.mi.dwFlags = !isKeyRelease ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    }
    // 鼠标右键点击
    else if(btnName == "mouseRight"){
        input.mi.dwFlags = !isKeyRelease ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    }
    // 鼠标中键
    else if(btnName == "mouseMiddle"){
        input.mi.dwFlags = !isKeyRelease ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    }
    // 鼠标侧键（前进/后退）
    else if(btnName == "mouseSide1" || btnName == "mouseSide2") {
        // 判断是按下还是释放
        if (!isKeyRelease) {
            input.mi.dwFlags = MOUSEEVENTF_XDOWN;  // 侧键按下

            // 指定具体的侧键
            if (btnName == "mouseSide2") {
                input.mi.mouseData = XBUTTON2;  // 前进按钮 (0x0002)
            } else if (btnName == "mouseSide1") {
                input.mi.mouseData = XBUTTON1;  // 后退按钮 (0x0001)
            }
        } else {
            input.mi.dwFlags = MOUSEEVENTF_XUP;  // 侧键释放

            // 指定具体的侧键
            if (btnName == "mouseSide2") {
                input.mi.mouseData = XBUTTON2;
            } else if (btnName == "mouseSide1") {
                input.mi.mouseData = XBUTTON1;
            }
        }
    }
    else{
        // 其它无效操作不模拟
        return;
    }


    // 发送鼠标事件
    SendInput(1, &input, sizeof(INPUT));
}


void MainWindow::scanRecordFiles(){
    ui->comboBox->clear();

    QDir directory(appDataDir);

    // 设置过滤器，查找所有.record文件
    QStringList filters;
    filters << "*.record";

    // 获取目录下所有符合过滤条件的文件
    // QDir::Files 只列出文件，QDir::NoDotAndDotDot 不列出"."和".."
    QFileInfoList fileList = directory.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

    // 遍历并处理找到的文件
    foreach (const QFileInfo &fileInfo, fileList) {
        ui->comboBox->addItem(fileInfo.fileName());
    }
}


void MainWindow::on_pushButton_3_clicked()
{
    std::string dirPath = QDir::toNativeSeparators(appDataDir).toStdString();
    std::string command = "explorer \"" + dirPath + "\"";  // 使用双引号包裹路径，处理空格

    // 将窗口最小化
    showMinimized();

    // 使用 QProcess 执行命令, 打开资源管理器
    QProcess::startDetached(command.data());
}


void MainWindow::startRecordOrStop(){

    // 正在播放, 不能开始录制
    if(getIsPlaying()){
        return;
    }

    // 正在录制, 停止
    if(getIsRecording()){
        setIsRecording(false);

        ui->label_2->setText("已结束录制");
        ui->label_2->setStyleSheet("QLabel{color:rgb(240, 106, 74);}");
        ui->pushButton->setText("开始录制(F7)");
        ui->pushButton->setStyleSheet("QPushButton{background-color:rgb(57, 187, 244);}");

        // 恢复播放按钮
        ui->pushButton_2->setDisabled(false);
        ui->pushButton_2->setStyleSheet("QPushButton{background-color:rgb(6, 200, 99);}");
    }else{
        setIsRecording(true);

        ui->label_2->setText("正在录制");
        ui->label_2->setStyleSheet("QLabel{color:rgb(6, 200, 99);}");
        ui->pushButton->setText("结束录制(F7)");
        ui->pushButton->setStyleSheet("QPushButton{background-color:rgb(240, 106, 74);}");

        // 录制时, 不允许点击播放按钮
        ui->pushButton_2->setDisabled(true);
        ui->pushButton_2->setStyleSheet("QPushButton{background-color:rgb(220, 220, 220);}");

        QtConcurrent::run([=](){

            // 设置系统定时器精度为1ms
            timeBeginPeriod(1);

            recordStrMutex.lock();
            // 重置录制的信息
            recordStr = "";
            recordStrMutex.unlock();

            // 当前按下的按键集合
            QSet<QString> pressedKeySet;

            // 初始鼠标位置
            POINT cursorPosInitial;
            if (GetCursorPos(&cursorPosInitial)) {
                recordStrMutex.lock();

                // 记录初始的鼠标位置
                recordStr.append(INITIAL_POS).append(":").append(QString::number(cursorPosInitial.x)).append(",").append(QString::number(cursorPosInitial.y)).append("\n");

                recordStrMutex.unlock();
            }

            // 记录开始录制的时间
            //startRecordTimeMs = QDateTime::currentMSecsSinceEpoch();

            // 开始计时
            timer.start();

            while(getIsRecording()){
                //qint64 actionTime = QDateTime::currentMSecsSinceEpoch() - startRecordTimeMs;
                // 计时器当前纳秒
                qint64 actionTime = timer.nsecsElapsed();

                // 获取鼠标按键状态
                for (auto item = MOUSE_VK_MAP.begin(); item != MOUSE_VK_MAP.end(); ++item){
                    bool keyPressed = isMouseButtonPressed(item.value());
                    handleAndRecordKey(keyPressed, item.key(), &pressedKeySet, &recordStr, actionTime);
                }

                // 获取键盘按键状态
                for (auto item = VSC_MAP.begin(); item != VSC_MAP.end(); ++item){
                    // 跳过热键
                    if(item.key() == "F7" || item.key() == "F8"){
                        continue;
                    }

                    bool keyPressed = isKeyPressed(item.value());
                    handleAndRecordKey(keyPressed, item.key(), &pressedKeySet, &recordStr, actionTime);
                }


                QThread::msleep(m_recordInterval);
            }

            // 恢复系统计时器精度
            timeEndPeriod(1);

            QMetaObject::invokeMethod(mainWindow, [=]() {
                // 创建一个输入对话框
                bool ok;
                QString inputText = QInputDialog::getText(mainWindow, "保存录制", "请输入保存的名称:", QLineEdit::Normal, "", &ok);

                // 点击了OK
                if (ok) {
                    if(inputText.isEmpty()){
                        inputText = "录制_" + QTime::currentTime().toString("yyyyMMdd_HHmmss");
                    }

                    if(saveRecorrdToFile(inputText, recordStr)){
                        QMessageBox::information(mainWindow, "提醒", "录制保存成功!");
                    }else{
                        QMessageBox::information(mainWindow, "错误", "录制保存失败, 创建文件时失败!");
                    }

                    // 重新扫描一次文件
                    scanRecordFiles();
                }
            }, Qt::QueuedConnection);
        });
    }
}


void MainWindow::startPlayOrStop(){

    // 正在录制, 不能开始播放
    if(getIsRecording()){
        return;
    }


    if(getIsPlaying()){
        setIsPlaying(false);

        // 恢复所有按键
        releaseAllKeys();

        ui->label_2->setText("已结束播放");
        ui->label_2->setStyleSheet("QLabel{color:rgb(240, 106, 74);}");
        ui->pushButton_2->setText("播放(F8)");
        ui->pushButton_2->setStyleSheet("QPushButton{background-color:rgb(6, 200, 99);}");

        // 恢复录制按钮
        ui->pushButton->setDisabled(false);
        ui->pushButton->setStyleSheet("QPushButton{background-color:rgb(57, 187, 244);}");
        // 恢复下拉框选择录制文件
        ui->comboBox->setDisabled(false);
        ui->comboBox->setStyleSheet("QComboBox{background-color:rgba(251, 251, 251, 1); padding-left:12px;}");
    }else{
        if(ui->comboBox->currentText().isEmpty()){
            QMessageBox::warning(this, "警告", "未选择录制文件, 无法播放!");
            return;
        }

        setIsPlaying(true);

        ui->label_2->setText("正在播放");
        ui->label_2->setStyleSheet("QLabel{color:rgb(6, 200, 99);}");
        ui->pushButton_2->setText("结束播放(F8)");
        ui->pushButton_2->setStyleSheet("QPushButton{background-color:rgb(240, 106, 74);}");

        // 播放时, 不允许点击录制按钮
        ui->pushButton->setDisabled(true);
        ui->pushButton->setStyleSheet("QPushButton{background-color:rgb(220, 220, 220);}");
        // 不允许选择录制文件
        ui->comboBox->setDisabled(true);
        ui->comboBox->setStyleSheet("QComboBox{background-color:rgb(220, 220, 220);  padding-left:12px;}");

        // 播放
        QtConcurrent::run([=](){
            // 校准鼠标移动的缩放因子
            //calibratePlayback();

            // 设置鼠标速度1:1
            set1To1MouseMovement();

            // 设置系统定时器精度为1ms
            timeBeginPeriod(1);

            // 选择的录制文件
            QString filePath = appDataDir + "/" + ui->comboBox->currentText();
            // 打开选择的录制文件
            QFile file(filePath);
            // 尝试以只读和文本模式打开文件
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMetaObject::invokeMethod(mainWindow, [=]{
                    QMessageBox::critical(mainWindow, "错误", "无法打开文件:" + filePath);
                    mainWindow->startPlayOrStop();
                    mainWindow->scanRecordFiles();
                });
                return;
            }

            QTextStream in(&file);
            QString line;


            // 当前是否是第一行
            bool isFirstLine = true;

            // 鼠标初始位置
            int firstX = 0, firstY = 0;

            // 操作列表
            QList<ActionInfo> actionList;

            // 读取录制文件到内存
            while(getIsPlaying() && !in.atEnd()){
                // 读取一行
                line = in.readLine();

                // 读取第一行, 获取初始鼠标位置
                if(isFirstLine){
                    // 第一行信息错误
                    if(!line.contains(INITIAL_POS) || line.split(":").size() != 2){
                        QMetaObject::invokeMethod(mainWindow, [=]{
                            QMessageBox::critical(mainWindow, "错误", "录制文件的首行信息格式错误!");
                        });
                        return;
                    }else{
                        auto itemSplit = line.split(":");
                        QString key = itemSplit[0];
                        QString pos = itemSplit[1];

                        auto posList = pos.split(",");
                        firstX = posList[0].toInt();
                        firstY = posList[1].toInt();
                    }

                    isFirstLine = false;

                    // 第一行读取完毕, 不进行后续操作
                    continue;
                }

                // 数据格式示例: "10 mouseMove:0,0"  示例2: "18 A:press"
                // 取出开头的时间
                auto lineSplit = line.split(" ");
                qint64 actionTime = lineSplit[0].toLongLong();

                // 按键操作的信息
                auto item = lineSplit[1];

                if(item.isEmpty()){
                    continue;
                }

                auto itemSplit = item.split(":");
                if(itemSplit.size() < 2){
                    continue;
                }

                // 操作的按键名称
                QString key = itemSplit[0];
                // 其它信息
                // 如果key是键盘按键或鼠标按键, action则记录按键按下或者松开, 格式: 键盘按键 key:action 示例 "F:release", 鼠标按键 key:action 示例 "mouseLeft:press"
                // 如果key是鼠标移动, action则记录移动的量, key:action 示例 "mouseMove:dx,dy"
                QString action = itemSplit[1];

                // 鼠标移动
                if(key == "mouseMove"){
                    auto moveDis = action.split(",");
                    int dx = moveDis[0].toInt();
                    int dy = moveDis[1].toInt();

                    //mouseActionList.append(ActionInfo{actionTime, key, 0, dx, dy, false});
                    actionList.append(ActionInfo{actionTime, key, 0, dx, dy, false});
                }
                // 鼠标按键
                else if(key.contains("mouse")){
                    //mouseActionList.append(ActionInfo{actionTime, key, 0, 0, 0, action == "release" ? true : false});
                    actionList.append(ActionInfo{actionTime, key, 0, 0, 0, action == "release" ? true : false});
                }else{
                    if(!VSC_MAP.contains(key)){
                        continue;
                    }

                    // 键盘按键
                    //keyboardActionList.append(ActionInfo{actionTime, key, VSC_MAP[key], 0, 0, action == "release" ? true : false});
                    actionList.append(ActionInfo{actionTime, key, VSC_MAP[key], 0, 0, action == "release" ? true : false});
                }

            }

            if(actionList.isEmpty()){
                return;
            }

            // 循环播放
            while(getIsPlaying()){
                // 移动鼠标到初始位置
                moveMouseToPos(firstX, firstY);

                // 高精度计时器开始计时
                QElapsedTimer runTimer;
                runTimer.start();

                for(auto actionInfo : actionList){
                    if(!getIsPlaying()){
                        break;
                    }

                    // 还没到操作时间
                    if(actionInfo.actionTime > runTimer.nsecsElapsed()){
                        // 忙等待, 等待操作时间到
                        while(true){
                            if(actionInfo.actionTime <= runTimer.nsecsElapsed()){
                                break;
                            }

                            QThread::msleep(1);
                        }
                    }

                    // 鼠标移动
                    if(actionInfo.actionName == "mouseMove"){
                        // 模拟鼠标移动
                        simulateMouseRelativeMove(actionInfo.dx, actionInfo.dy);
                    }else if(actionInfo.actionName.contains("mouse")){
                        // 模拟鼠标按键
                        simulateMouseAction(actionInfo.actionName, actionInfo.isRelease);
                    }else{
                        // 模拟键盘按键
                        simulateKeyPress(actionInfo.keyboardScanCode, actionInfo.isRelease);
                    }
                }

                // 等待一下再进入下一轮循环
                QThread::msleep(500);
            }

            // 恢复鼠标速度
            restoreMouseSettings();

            // 在线程结束前恢复默认精度
            timeEndPeriod(1);
        });
    }
}


void MainWindow::releaseAllKeys(){
    // 还在按下的键盘按键
    QSet<int> keyboardPressSet;
    // 还在按下的鼠标按键
    QSet<QString> mouseBtnPressSet;

    // 获取鼠标按键状态
    for (auto item = MOUSE_VK_MAP.begin(); item != MOUSE_VK_MAP.end(); ++item){
        bool keyPressed = isMouseButtonPressed(item.value());
        if(keyPressed){
            mouseBtnPressSet.insert(item.key());
        }
    }

    // 获取键盘按键状态
    for (auto item = VSC_MAP.begin(); item != VSC_MAP.end(); ++item){
        bool keyPressed = isKeyPressed(item.value());
        if(keyPressed){
            keyboardPressSet.insert(item.value());
        }
    }

    // 松开按键
    for(auto it : keyboardPressSet){
        simulateKeyPress(it, true);
    }
    for(auto it : mouseBtnPressSet){
        simulateMouseAction(it, true);
    }
}

bool MainWindow::registerRawInput()
{
    // 配置原始输入设备：鼠标
    RAWINPUTDEVICE rid[1];

    rid[0].usUsagePage = 0x01;  // 通用桌面控制
    rid[0].usUsage = 0x02;      // 鼠标设备
    rid[0].dwFlags = RIDEV_INPUTSINK; // 即使程序不处于活动状态也接收输入
    rid[0].hwndTarget = (HWND)this->winId(); // 接收消息的窗口句柄

    // 注册设备
    if (!RegisterRawInputDevices(rid, 1, sizeof(rid[0]))) {
        qWarning() << "RegisterRawInputDevices failed:" << GetLastError();
        return false;
    }

    return true;
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result){
    Q_UNUSED(eventType)

    MSG *msg = static_cast<MSG*>(message);

    if (msg->message == WM_INPUT) {
        // 处理原始输入消息
        HRAWINPUT rawInput = (HRAWINPUT)msg->lParam;
        processRawInput(rawInput);

        // 表明消息已处理
        if (result) {
            *result = 0;
        }
        return true;
    }

    // 对于其他消息，调用基类实现
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::processRawInput(HRAWINPUT rawInput)
{
    UINT dwSize = 0;

    // 首先获取所需缓冲区大小
    if (GetRawInputData(rawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER)) == -1) {
        return;
    }

    QVector<BYTE> buffer(dwSize);
    RAWINPUT *raw = (RAWINPUT*)buffer.data();

    // 获取原始数据
    if (GetRawInputData(rawInput, RID_INPUT, buffer.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
        qWarning() << "GetRawInputData returned wrong size";
        return;
    }

    // 检查是否为鼠标输入
    if (raw->header.dwType == RIM_TYPEMOUSE) {
        const RAWMOUSE& mouse = raw->data.mouse;

        int deltaX = 0, deltaY = 0;

        // 处理相对移动
        if (mouse.usFlags == MOUSE_MOVE_RELATIVE) {
            deltaX = mouse.lLastX;
            deltaY = mouse.lLastY;

            //正在录制, 记录
            if(getIsRecording() && (deltaX != 0 || deltaY != 0)){
                recordStrMutex.lock();
                recordStr.append(QString::number(timer.nsecsElapsed())).append(" ").append("mouseMove:")
                    .append(QString::number(deltaX)).append(",").append(QString::number(deltaY)).append("\n");
                recordStrMutex.unlock();
            }

            //qDebug() << "deltaX,deltaY : " << deltaX << "," << deltaY;
        }
    }
}


void  MainWindow::calibratePlayback(){
    qDebug() << "开始校准鼠标缩放因子...";

    // 测试移动：记录一个已知距离
    int testDistance = 300;

    // 记录开始位置
    POINT startPos;
    GetCursorPos(&startPos);

    // 使用当前因子模拟移动
    simulateMouseRelativeMove(testDistance, 0);
    Sleep(100); // 等待移动完成

    // 测量实际移动距离
    POINT endPos;
    GetCursorPos(&endPos);
    int actualMove = endPos.x - startPos.x;

    // 计算新的校准因子
    if (actualMove > 0) {
        m_calibrationFactor = 1.0f * testDistance / actualMove;
        m_calibrationFactor = qBound(0.01f, m_calibrationFactor, 1.0f); // 限制范围
    }

    qDebug() << "校准结果: 理论" << testDistance << "实际" << actualMove
             << "新因子:" << m_calibrationFactor;
}



void MainWindow::on_pushButton_4_clicked()
{
    scanRecordFiles();
}


void MainWindow::moveMouseToPos(int targetX, int targetY){
    // 将当前鼠标 线性移动到 targetX,targetY
    while(getIsPlaying()){
        int dx = 0, dy = 0;
        // 获取鼠标的屏幕坐标（物理位置）
        POINT cursorPos;
        if (GetCursorPos(&cursorPos)) {
            if(targetX > cursorPos.x){
                dx = (cursorPos.x + 10) > targetX ? targetX : (cursorPos.x + 10);
            }else{
                dx = (cursorPos.x - 10) < targetX ? targetX : (cursorPos.x - 10);
            }

            if(targetY > cursorPos.y){
                dy = (cursorPos.y + 10) > targetY ? targetY : (cursorPos.y + 10);
            }else{
                dy = (cursorPos.y - 10) < targetY ? targetY : (cursorPos.y - 10);
            }

            //qDebug() << "targetX,targetY: " << targetX << "," << targetY << "; dx,dy: " << dx << "," << dy;

            // 模拟鼠标移动
            simulateMouseAbsolutelyMove(dx, dy);

            // 已到达目标位置
            if(dx == targetX && dy == targetY){
                break;
            }
        }

        QThread::msleep(15);
    }
}
