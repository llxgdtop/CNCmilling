#include <QGraphicsScene>
#include <QMessageBox>
#include <QScrollBar>
#include <QTimer>
#include <cmath>
#include <string>

#include "mainwindow.h"

#include <qboxlayout.h>
#include <qlabel.h>
#include <qmainwindow.h>
#include <qnamespace.h>
#include <qtimer.h>

#include "./ui_mainwindow.h"
#include "axis_item.h"
#include "milling_item.h"
#include "status_table.h"

enum
{
    WIDTH = 300,
    HEIGHT = WIDTH
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _scene(new QGraphicsScene(this))
    , _axis(new AxisItem({WIDTH, HEIGHT}))
    , _label_pos(new QLabel)
    , _milling_item(new MillingItem(_axis))
    , _timer(new QTimer(this))
    , _status_table(new StatusTable)//构造函数
{
    ui->setupUi(this);
    ui->table_status->setModel(_status_table);
    ui->table_status->verticalHeader()->setVisible(false);//隐藏表格行号与列名
    ui->table_status->horizontalHeader()->setVisible(false);
    ui->view_milling->setScene(_scene);
    _scene->addItem(_axis);//加坐标轴
    //关掉路径显示那部分视图控件的左右和上下滚动条
    ui->view_milling->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->view_milling->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    const auto vlay = new QVBoxLayout(ui->view_milling);
    vlay->addWidget(_label_pos);
    vlay->setAlignment(Qt::AlignRight | Qt::AlignTop);//xy位置显示

    //开始按钮
    connect(ui->btn_start,
            &QPushButton::clicked,
            this,
            [=]
            {
                this->parse_nc();
                _timer->start(10);
            });
    //重置按钮
    connect(ui->btn_reset,
            &QPushButton::clicked,
            [=]
            {
                _timer->stop();
                _cmd_cache.clear();//释放内存
                _milling_item->MillingItem::reset();
                finish = 0;
                fXbeginArray[0] = 0;
                fYbeginArray[0] = 0;
                x0 = (int)fXbeginArray[0];
                y0 = (int)fYbeginArray[0];
                nStatusArray[0] = 0;
                fIArray[0] = 0;
                fJArray[0] = 0;
                dRArray[0] = 0;
                nDirArray[0] = 0;
                nSArray[0] = 0;
                nTArray[0] = 1;
                nCool1Array[0] = 0;
                nCool2Array[0] = 0;
                G90[0] = 1;
                G54[0] = 0;
                nLineNum = 0;
                _status_table->update_status({});
            });
    connect(_timer, &QTimer::timeout, this, &MainWindow::timer_Tick);
    update_pos(0, 0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    _axis->setRange({double(ui->view_milling->rect().width()),
                     double(ui->view_milling->rect().height())});
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    _axis->setRange({double(ui->view_milling->rect().width()),
                     double(ui->view_milling->rect().height())});
}

void MainWindow::parse_nc()
{
    // 获取NC程序总行数
    _cmd_cache.clear();
    //获取文本编辑器的每一行，遍历的语法结构为for(auto xx:容器名)
    //split() 函数可以将一个字符串，拆分成一个装有多个子串的QStringList
    for (const auto& line : ui->edit_cmd->toPlainText().split("\n")) {
        if (!line.contains("//") && !line.isEmpty())
            _cmd_cache.push_back(line.toStdString());
    }
    const auto lineCount = _cmd_cache.size();

    // 读取第nLineNum行
    while (nLineNum <= lineCount - 1) {
        // 以下参数代表对应字母是否存在于这一行
        int nX_exist = 0, nY_exist = 0;
        int nM03_exist = 0, nM04_exist = 0, nM07_exist = 0, nM08_exist = 0,nM09_exist = 0;
        int nT_exist = 0, nF_exist = 0, nS_exist = 0;
        int nG00_exist = 0, nG01_exist = 0, nG02_exist = 0, nG03_exist = 0,nG90_exist = 0;
        int nCharNum = 0;  // 一行字符串第nCharNum个字符
        int nCodeNum = 0;  // 该行第nCodeNum个指令
        std::string str_code = "";  // 第nCodeNum个指令内容
        std::string codeArray[100];//指令数组

        // 获取第nLineNum行字符串
        auto lineContent = _cmd_cache[nLineNum];

        // 去除空格，将空格移到容器末尾，之后erase
        lineContent.erase(
            std::remove(lineContent.begin(), lineContent.end(), ' '),
            lineContent.end());

        // 读每一位字符
        while (nCharNum <= lineContent.length() - 1) {
            // 这一行字符串中第nCharNum个字符
            std::string str = lineContent.substr(nCharNum, 1);//复制子字符串，要求从指定位置开始，并有指定长度1
            if (std::isdigit(str[0]) || str == "." || str == "-")// 该字符为数字或小数追加在该指令后面
            {
                str_code = str_code + str;
                codeArray[nCodeNum] = str_code;
            } else {  // 该字符为字母
                nCodeNum += 1;
                str_code = "";
                str_code = str_code + str;
                codeArray[nCodeNum] = str_code;
            }
            nCharNum++;
        }
        nCodeNum = 1;  // 一行中第nCharNum个指令从1开始(codeArray[0]为"")

        // 识别此行NC程序
        while (codeArray[nCodeNum] != ";" && !codeArray[nCodeNum].empty())//即没有指令
        {
            // 提取此行第nCodeNum个指令指令第1位字符
            std::string firstChar = codeArray[nCodeNum].substr(0, 1);//复制子字符串，要求从指定位置开始，并有指定长度1

            // 提取此行第nCodeNum个指令内容
            //substr()中只有一个参数的话，会返回从那个位置之后的字符串，即返回字母后的数字
            std::string txt = codeArray[nCodeNum].substr(1);

            // G代码
            if (firstChar == "G") {
                //stoi()： string型变量转换为int型变量，下面的stof是转为float
                switch (std::stoi(txt)) {
                    case 0:
                        nG00_exist = 1;
                        nStatusArray[nLineNum] = 4;
                        break;
                    case 1:
                        nG01_exist = 1;
                        nStatusArray[nLineNum] = 1;
                        break;
                    case 2:
                        nG02_exist = 1;
                        nStatusArray[nLineNum] = 2;
                        break;
                    case 3:
                        nG03_exist = 1;
                        nStatusArray[nLineNum] = 3;
                        break;
                    case 90:
                        nG90_exist = 1;
                        G90[nLineNum] = 1;
                        break;
                    case 91:
                        nG90_exist = 1;
                        G90[nLineNum] = 0;
                        break;
                    default:
                        break;
                }
            }

            // G90继承,默认nG90exist也是0
            if (nG90_exist == 0 && (nLineNum != 0))
                G90[nLineNum] = G90[nLineNum - 1];

            // X
            if (firstChar == "X") {
                nX_exist = 1;
                switch (G90[nLineNum]) {
                    case 1:
                        // 绝对坐标模式下，直接将读取的值赋给终点X坐标
                        fXendArray[nLineNum] = std::stof(txt);
                        break;
                    case 0:
                        // 相对坐标模式下，将读取的值与起点X坐标相加
                        fXendArray[nLineNum] = std::stof(txt) + fXbeginArray[nLineNum];
                        break;
                    default:
                        break;
                }
            }

            // Y
            if (firstChar == "Y") {
                nY_exist = 1;
                switch (G90[nLineNum]) {
                    case 1:
                        // 绝对坐标模式下，直接将读取的值赋给终点Y坐标
                        fYendArray[nLineNum] = std::stof(txt);
                        break;
                    case 0:
                        // 相对坐标模式下，将读取的值与起点Y坐标相加
                        fYendArray[nLineNum] = std::stof(txt) + fYbeginArray[nLineNum];
                        break;
                    default:
                        break;
                }
            }

            // F代码
            if (firstChar == "F") {
                nF_exist = 1;
                fFArray[nLineNum] = std::stof(txt);
            }
            // R
            if (firstChar == "R") {
                float rr = std::stof(codeArray[nCodeNum].substr(1));
                double b, c1, c2, A, B, C;
                if (fXbeginArray[nLineNum] == fXendArray[nLineNum]) {//即圆弧是竖直方向的
                    double d;
                    if (((nStatusArray[nLineNum] == 2) && (rr > 0))
                        || ((nStatusArray[nLineNum] == 3) && (rr < 0)))
                    {
                        if (fYbeginArray[nLineNum] > fYendArray[nLineNum]) {
                            b = (fYbeginArray[nLineNum] + fYendArray[nLineNum]) / 2; // 圆心的 Y 轴坐标
                            d = fXbeginArray[nLineNum] - sqrt(rr * rr - pow(fYbeginArray[nLineNum] - fYendArray[nLineNum], 2) / 4); // 圆心的 X 坐标
                        } else {
                            b = (fYbeginArray[nLineNum] + fYendArray[nLineNum]) / 2; // 圆心的 Y 轴坐标
                            d = fXbeginArray[nLineNum] + sqrt(rr * rr - pow(fYbeginArray[nLineNum] - fYendArray[nLineNum], 2) / 4); // 圆心的 X 坐标
                        }

                        fIArray[nLineNum] = static_cast<float>(d - fXbeginArray[nLineNum]); // 转换为 float 类型
                        fJArray[nLineNum] = static_cast<float>(b - fYbeginArray[nLineNum]); // 转换为 float 类型
                        dRArray[nLineNum] = sqrt(rr * rr);

                    }
                }
                if (fXbeginArray[nLineNum] != fXendArray[nLineNum]) {
                    c1 = (fYendArray[nLineNum] * fYendArray[nLineNum]
                          - fYbeginArray[nLineNum] * fYbeginArray[nLineNum]
                          + fXendArray[nLineNum] * fXendArray[nLineNum]
                          - fXbeginArray[nLineNum] * fXbeginArray[nLineNum])
                        / 2 / (fXendArray[nLineNum] - fXbeginArray[nLineNum]);
                    c2 = (fYendArray[nLineNum] - fYbeginArray[nLineNum])
                        / (fXendArray[nLineNum] - fXbeginArray[nLineNum]);
                    A = c2 * c2 + 1;
                    B = 2 * fXbeginArray[nLineNum] * c2 - 2 * c1 * c2
                        - 2 * fYbeginArray[nLineNum];
                    C = fXbeginArray[nLineNum] * fXbeginArray[nLineNum]
                        - 2 * fXbeginArray[nLineNum] * c1 + c1 * c1
                        + fYbeginArray[nLineNum] * fYbeginArray[nLineNum]
                        - rr * rr;//求圆心过程，见说明书

                    double d;  // X

                    if (((nStatusArray[nLineNum] == 2) && (rr > 0))
                        || ((nStatusArray[nLineNum] == 3) && (rr < 0)))
                    {
                        float discriminant = B * B - 4 * A * C;
                        float squareRootPart = sqrt(discriminant);
                        b = (-B + (fXbeginArray[nLineNum] < fXendArray[nLineNum] ? -1 : 1) * squareRootPart) / (2 * A);  // 绝对Y
                        //使用三目运算符根据条件确定平方根部分符号
                        d = c1 - c2 * b;  // 绝对X
                        fIArray[nLineNum] = static_cast<float>(d - fXbeginArray[nLineNum]);
                        fJArray[nLineNum] = static_cast<float>(b - fYbeginArray[nLineNum]);
                        dRArray[nLineNum] = sqrt(rr * rr);

                    } else {
                        float squareRootPart = sqrt(B * B - 4 * A * C);
                        float denominator = 2 * A;
                        int sign = (fXbeginArray[nLineNum] < fXendArray[nLineNum]) ? 1 : -1;
                        b = (-B + sign * squareRootPart) / denominator;  // 绝对Y
                        d = c1 - c2 * b;  // 绝对X
                        fIArray[nLineNum] = static_cast<float>(d - fXbeginArray[nLineNum]);
                        fJArray[nLineNum] = static_cast<float>(b - fYbeginArray[nLineNum]);
                        dRArray[nLineNum] = sqrt(rr * rr);
                    }
                }
            }
            //I,J表示的是圆弧的圆弧圆心相对起点的增量值
            // I代码
            if (firstChar == "I") {
                fIArray[nLineNum] = std::stof(txt);
            }
            // J代码
            if (firstChar == "J") {
                fJArray[nLineNum] = std::stof(txt);

                // 通过fIArray, fJArray计算dRArray
                dRArray[nLineNum] =
                    sqrt(fIArray[nLineNum] * fIArray[nLineNum]
                         + fJArray[nLineNum] * fJArray[nLineNum]);
            }
            // M代码
            if (firstChar == "M") {
                // M03
                if (txt == "03") {
                    nM03_exist = 1;
                    nDirArray[nLineNum] = 1;
                }
                // M04
                if (txt == "04") {
                    nM04_exist = 1;
                    nDirArray[nLineNum] = -1;
                }
                // M05
                if (txt == "05") {
                    nDirArray[nLineNum] = 2;
                }
                // M07
                if (txt == "07") {  // nCool2Array 开
                    nM07_exist = 1;
                    nCool2Array[nLineNum] = 1;
                }
                // M08
                if (txt == "08") {  // nCool1Array 开
                    nM08_exist = 1;
                    nCool1Array[nLineNum] = 1;
                }
                // M09
                if (txt == "09") {
                    nM09_exist = 1;
                    nCool1Array[nLineNum] = 0;
                    nCool2Array[nLineNum] = 0;
                }
            }
            // S代码
            if (firstChar == "S") {
                nS_exist = 1;
                nSArray[nLineNum] = std::stoi(txt);
            }
            // nTArray
            if (firstChar == "T") {
                nT_exist = 1;
                nTArray[nLineNum] = std::stoi(txt);
            }

            nCodeNum += 1;  // 该行下一个指令
        }  // 第nLineNum行识别完成

        // 继承
        if (nX_exist == 0) {
            fXendArray[nLineNum] = fXbeginArray[nLineNum];
        }
        if (nY_exist == 0) {
            fYendArray[nLineNum] = fYbeginArray[nLineNum];
        }

        if ((nM03_exist == 0 && nM04_exist == 0) && (nLineNum != 0)) {
            nDirArray[nLineNum] = nDirArray[nLineNum - 1];
        }
        if (nS_exist == 0 && (nLineNum != 0)) {
            nSArray[nLineNum] = nSArray[nLineNum - 1];
        }
        if (nM07_exist == 0 && nM09_exist == 0 && (nLineNum != 0)) {
            nCool2Array[nLineNum] = nCool2Array[nLineNum - 1];
        }
        if (nM08_exist == 0 && nM09_exist == 0 && (nLineNum != 0)) {
            nCool1Array[nLineNum] = nCool1Array[nLineNum - 1];
        }
        if (nT_exist == 0 && (nLineNum != 0)) {
            nTArray[nLineNum] = nTArray[nLineNum - 1];
        }
        if (nF_exist == 0 && (nLineNum != 0)) {
            fFArray[nLineNum] = fFArray[nLineNum - 1];
        }
        // 如果一行坐标前未出现插补方式，则默认为上一行插补方式
        if ((nX_exist != 0 || nY_exist != 0)
            && (nG00_exist == 0 && nG01_exist == 0 && nG02_exist == 0
                && nG03_exist == 0))
        {
            nStatusArray[nLineNum] = nStatusArray[nLineNum - 1];
        }
        fXbeginArray[nLineNum + 1] = fXendArray[nLineNum];
        fYbeginArray[nLineNum + 1] = fYendArray[nLineNum];
        nLineNum = nLineNum + 1;  // 下一行
    }

    // 所有行识别完成
    nLineNum = 0;
    // 完成标志置1
    finish = 1;
}

void MainWindow::timer_Tick()
{
    if (finish == 1 && nLineNum > _cmd_cache.size()) {
        finish = 0;
        nLineNum = 0;
        _timer->stop();
        QMessageBox::information(this, "提示", "仿真完成");
    }

    int cmd_count = _cmd_cache.size();
    if (finish == 1 && nLineNum <= cmd_count) {
        Status status;
        status.knife_count = nTArray[nLineNum];
        status.direction = nDirArray[nLineNum];
        status.lubricant_one = nCool1Array[nLineNum];
        status.lubricant_two = nCool2Array[nLineNum];
        status.speed = nSArray[nLineNum];
        status.supply_speed = QString::number(fFArray[nLineNum]);//因为G00是快速定位，故转成qstring类型
        //QString::number是将数数字（整数、浮点数、有符号、无符号等）转换为QString类型，常用于UI数据显示
        // 显示进给速度
        if (nStatusArray[nLineNum] == 1 || nStatusArray[nLineNum] == 2
            || nStatusArray[nLineNum] == 3)
            status.supply_speed = QString::number(fFArray[nLineNum]);
        if (nStatusArray[nLineNum] == 4)
            status.supply_speed = "快速定位";
        if (nStatusArray[nLineNum] == 0) {
            status.supply_speed = "0";
            nLineNum = nLineNum + 1;
        }
        _status_table->update_status(status);

        // 直线插补两点重合跳过
        if ((fXbeginArray[nLineNum] == fXendArray[nLineNum])
            && (fYbeginArray[nLineNum] == fYendArray[nLineNum])
            && (nStatusArray[nLineNum] == 0 || nStatusArray[nLineNum] == 1))
        {
            nLineNum = nLineNum + 1;
        }

        // 逐点比较法
        //G01 G00
        // 第1象限
        if ((nStatusArray[nLineNum] == 4 || nStatusArray[nLineNum] == 1)
            && fXbeginArray[nLineNum] < fXendArray[nLineNum]
            && fYbeginArray[nLineNum] <= fYendArray[nLineNum])
        {
            // 根据偏差方程进行判断
            if (((fXendArray[nLineNum] - fXbeginArray[nLineNum]) * (y0 - fYbeginArray[nLineNum])
                 - (fYendArray[nLineNum] - fYbeginArray[nLineNum]) * (x0 - fXbeginArray[nLineNum]))
                >= 0)
            {
                x1 = x0 + 1;
                y1 = y0;
            }
            else
            {
                y1 = y0 + 1;
                x1 = x0;
            }

            // 实时坐标显示
            update_pos(x1, y1);

            // 此段插补的终点作为下一小段插补的起点
            x0 = x1;
            y0 = y1;

            // 终点判别
            if (x1 == fXendArray[nLineNum] && y1 == fYendArray[nLineNum])
            {
                nLineNum ++;
            }
        }


        // 第2象限
        if ((nStatusArray[nLineNum] == 4 || nStatusArray[nLineNum] == 1)
            && fXbeginArray[nLineNum] >= fXendArray[nLineNum]
            && fYbeginArray[nLineNum] < fYendArray[nLineNum])
        {
            if (((fXendArray[nLineNum] - fXbeginArray[nLineNum]) * (y0 - fYbeginArray[nLineNum])
                 - (fYendArray[nLineNum] - fYbeginArray[nLineNum]) * (x0 - fXbeginArray[nLineNum]))
                >= 0)
            {
                x1 = x0;
                y1 = y0 + 1;
            }
            else
            {
                y1 = y0;
                x1 = x0 - 1;
            }

            update_pos(x1, y1);

            x0 = x1;
            y0 = y1;

            if (x1 == fXendArray[nLineNum] && y1 == fYendArray[nLineNum])
            {
                nLineNum++;
            }
        }


        // 第3象限
        if ((nStatusArray[nLineNum] == 4 || nStatusArray[nLineNum] == 1)
            && fXbeginArray[nLineNum] > fXendArray[nLineNum]
            && fYbeginArray[nLineNum] >= fYendArray[nLineNum])
        {
            if (((fXendArray[nLineNum] - fXbeginArray[nLineNum]) * (y0 - fYbeginArray[nLineNum])
                 - (fYendArray[nLineNum] - fYbeginArray[nLineNum]) * (x0 - fXbeginArray[nLineNum]))
                >= 0)
            {
                x1 = x0 - 1;
                y1 = y0;
            }
            else
            {
                y1 = y0 - 1;
                x1 = x0;
            }

            update_pos(x1, y1);

            x0 = x1;
            y0 = y1;

            if (x1 == fXendArray[nLineNum] && y1 == fYendArray[nLineNum])
            {
                nLineNum++;
            }
        }


        // 第4象限
        if ((nStatusArray[nLineNum] == 4 || nStatusArray[nLineNum] == 1)
            && fXbeginArray[nLineNum] <= fXendArray[nLineNum]
            && fYbeginArray[nLineNum] > fYendArray[nLineNum])
        {
            if (((fXendArray[nLineNum] - fXbeginArray[nLineNum]) * (y0 - fYbeginArray[nLineNum])
                 - (fYendArray[nLineNum] - fYbeginArray[nLineNum]) * (x0 - fXbeginArray[nLineNum]))
                <= 0)
            {
                x1 = x0 + 1;
                y1 = y0;
            }
            else
            {
                y1 = y0 - 1;
                x1 = x0;
            }

            update_pos(x1, y1);

            x0 = x1;
            y0 = y1;

            if (x1 == fXendArray[nLineNum] && y1 == fYendArray[nLineNum])
            {
                nLineNum++;
            }
        }

        // G02
        if (nStatusArray[nLineNum] == 2)  // G02
        {
            if (first == 1) {
                if (fIArray[nLineNum] <= 0 && fJArray[nLineNum] < 0)  // 第1象限
                {
                    nQuadrant = 1;
                }
                if (fIArray[nLineNum] > 0 && fJArray[nLineNum] <= 0)  // 第2象限
                {
                    nQuadrant = 2;
                }
                if (fIArray[nLineNum] >= 0 && fJArray[nLineNum] > 0)  // 第3象限
                {
                    nQuadrant = 3;
                }
                if (fIArray[nLineNum] < 0 && fJArray[nLineNum] >= 0)  // 第4象限
                {
                    nQuadrant = 4;
                }
                first = 0;
            }

            if (nQuadrant == 1)  // 点位于圆的第1象限
            {
                if (((x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                         * (x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                     + (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                         * (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                     - dRArray[nLineNum] * dRArray[nLineNum])
                    > 0)
                {
                    y1 = y0 - 1;
                    x1 = x0;
                } else {
                    x1 = x0 + 1;
                    y1 = y0;
                }
                // 实时坐标显示
                update_pos(x1, y1);

                // 此段插补的终点作为下一小段插补的起点
                y0 = y1;
                x0 = x1;
                // 终点判别
                if ((x1 == fXendArray[nLineNum])
                    && (y1 == fYendArray[nLineNum]))
                {
                    nLineNum ++;
                    first = 1;
                }
                // 判断是否跨越象限
                if (x1
                        == (fXbeginArray[nLineNum] + fIArray[nLineNum]+ dRArray[nLineNum])
                    && y1
                        == (fYbeginArray[nLineNum]+ fJArray[nLineNum]))
                {
                    // 第一象限顺时针跨越进入第四象限
                    nQuadrant = 4;
                }
            }

            else if (nQuadrant == 2)  // 画第2象限
            {
                if (((x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                         * (x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                     + (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                         * (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                     - dRArray[nLineNum] * dRArray[nLineNum])
                    > 0)
                {
                    y1 = y0;
                    x1 = x0 + 1;
                } else {
                    x1 = x0;
                    y1 = y0 + 1;
                }
                update_pos(x1, y1);
                y0 = y1;
                x0 = x1;
                if ((x1 == fXendArray[nLineNum])
                    && (y1 == fYendArray[nLineNum]))
                {
                    nLineNum = nLineNum + 1;
                    first = 1;
                }
                if (x1 == (fXbeginArray[nLineNum] + fIArray[nLineNum])
                    && y1
                        == (fYbeginArray[nLineNum] + fJArray[nLineNum]
                            + dRArray[nLineNum]))
                {
                    nQuadrant = 1;
                }

            }

            else if (nQuadrant == 3)  // 画第3象限
            {
                if (((x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                         * (x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                     + (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                         * (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                     - dRArray[nLineNum] * dRArray[nLineNum])
                    > 0)
                {
                    y1 = y0 + 1;
                    x1 = x0;
                } else {
                    x1 = x0 - 1;
                    y1 = y0;
                }
                update_pos(x1, y1);
                y0 = y1;
                x0 = x1;
                if ((x1 == fXendArray[nLineNum])
                    && (y1 == fYendArray[nLineNum]))
                {
                    nLineNum = nLineNum + 1;
                    first = 1;
                }
                if (x1
                        == (fXbeginArray[nLineNum] + fIArray[nLineNum]
                            - dRArray[nLineNum])
                    && y1
                        == (fYbeginArray[nLineNum]
                            + fJArray[nLineNum]))
                {
                    nQuadrant = 2;
                }
            }

            else if (nQuadrant == 4)  // 画第4象限
            {
                if (((x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                         * (x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                     + (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                         * (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                     - dRArray[nLineNum] * dRArray[nLineNum])
                    > 0)
                {
                    y1 = y0;
                    x1 = x0 - 1;
                } else {
                    x1 = x0;
                    y1 = y0 - 1;
                }
                update_pos(x1, y1);
                y0 = y1;
                x0 = x1;
                if ((x1 == fXendArray[nLineNum])
                    && (y1 == fYendArray[nLineNum]))
                {
                    nLineNum = nLineNum + 1;
                    first = 1;
                }
                if (x1 == (fXbeginArray[nLineNum] + fIArray[nLineNum])
                    && y1
                        == (fYbeginArray[nLineNum] + fJArray[nLineNum]
                            - dRArray[nLineNum]))
                {
                    nQuadrant = 3;
                }
            }
        }

        // G03
        if (nStatusArray[nLineNum] == 3)
        {
            if (first == 1) {
                if (fIArray[nLineNum] < 0 && fJArray[nLineNum] <= 0)  // 第1象限
                {
                    nQuadrant = 1;
                }
                if (fIArray[nLineNum] >= 0 && fJArray[nLineNum] < 0)  // 第2象限
                {
                    nQuadrant = 2;
                }
                if (fIArray[nLineNum] > 0 && fJArray[nLineNum] >= 0)  // 第3象限
                {
                    nQuadrant = 3;
                }
                if (fIArray[nLineNum] <= 0 && fJArray[nLineNum] > 0)  // 第4象限
                {
                    nQuadrant = 4;
                }
                first = 0;
            }
            if (nQuadrant == 1)  // 画第1象限
            {
                if (((x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                         * (x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                     + (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                         * (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                     - dRArray[nLineNum] * dRArray[nLineNum])
                    > 0)
                {
                    y1 = y0;
                    x1 = x0 - 1;
                } else {
                    x1 = x0;
                    y1 = y0 + 1;
                }

                update_pos(x1, y1);
                y0 = y1;
                x0 = x1;
                if ((x1 == fXendArray[nLineNum])
                    && (y1 == fYendArray[nLineNum]))
                {
                    nLineNum = nLineNum + 1;
                    first = 1;
                }
                if (x1 == (fXbeginArray[nLineNum] + fIArray[nLineNum])
                    && y1
                        == (fYbeginArray[nLineNum] + fJArray[nLineNum]
                            + dRArray[nLineNum]))
                {
                    nQuadrant = 2;
                }
            }

            else if (nQuadrant == 2)
            {
                if (((x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                         * (x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                     + (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                         * (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                     - dRArray[nLineNum] * dRArray[nLineNum])
                    > 0)
                {
                    y1 = y0 - 1;
                    x1 = x0;
                } else {
                    x1 = x0 - 1;
                    y1 = y0;
                }
                update_pos(x1, y1);

                y0 = y1;
                x0 = x1;
                if ((x1 == fXendArray[nLineNum])
                    && (y1 == fYendArray[nLineNum]))
                {
                    nLineNum = nLineNum + 1;
                    first = 1;
                }
                if (x1
                        == (fXbeginArray[nLineNum] + fIArray[nLineNum]
                            - dRArray[nLineNum])
                    && y1
                        == (fYbeginArray[nLineNum]
                            + fJArray[nLineNum]))
                {
                    nQuadrant = 3;
                }

            }

            else if (nQuadrant == 3)  // 画第3象限
            {
                if (((x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                         * (x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                     + (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                         * (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                     - dRArray[nLineNum] * dRArray[nLineNum])
                    > 0)
                {
                    y1 = y0;
                    x1 = x0 + 1;
                } else {
                    x1 = x0;
                    y1 = y0 - 1;
                }
                update_pos(x1, y1);
                y0 = y1;
                x0 = x1;
                if ((x1 == fXendArray[nLineNum])
                    && (y1 == fYendArray[nLineNum]))
                {
                    nLineNum = nLineNum + 1;
                    first = 1;
                }
                if (x1 == (fXbeginArray[nLineNum] + fIArray[nLineNum])
                    && y1
                        == (fYbeginArray[nLineNum] + fJArray[nLineNum]
                            - dRArray[nLineNum]))
                {
                    nQuadrant = 4;
                }

            }

            else if (nQuadrant == 4)  // 画第4象限
            {
                if (((x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                         * (x1 - fXbeginArray[nLineNum] - fIArray[nLineNum])
                     + (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                         * (y1 - fYbeginArray[nLineNum] - fJArray[nLineNum])
                     - dRArray[nLineNum] * dRArray[nLineNum])
                    > 0)
                {
                    y1 = y0 + 1;
                    x1 = x0;
                } else {
                    x1 = x0 + 1;
                    y1 = y0;
                }
                update_pos(x1, y1);

                y0 = y1;
                x0 = x1;
                if ((x1 == fXendArray[nLineNum])
                    && (y1 == fYendArray[nLineNum]))
                {
                    nLineNum = nLineNum + 1;
                    first = 1;
                }
                if (x1
                        == (fXbeginArray[nLineNum] + fIArray[nLineNum]
                            + dRArray[nLineNum])
                    && y1
                        == (fYbeginArray[nLineNum]
                            + fJArray[nLineNum]))
                {
                    nQuadrant = 1;
                }
            }
        }
    }
}

void MainWindow::update_pos(int x, int y)
{
    _label_pos->setText("( " + QString::number(x) + "," + QString::number(y)
                        + " )");
    _milling_item->lineTo(x, y);
}
