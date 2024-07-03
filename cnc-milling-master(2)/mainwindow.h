#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QTimer>
#include <string>
#include <vector>

#include <qlabel.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE
//引入别的类
class QGraphicsScene;
class AxisItem;
class MillingItem;
class StatusTable;
class MainWindow : public QMainWindow
{
    Q_OBJECT
    enum
    {
        DEFAULT_PT_LEN = 1000,
    };

  public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

  protected:
    void resizeEvent(QResizeEvent* event) override;

    void showEvent(QShowEvent* event) override;

    void parse_nc();//解析nc程序
    void timer_Tick();
    void update_pos(int x, int y);

  private:
    Ui::MainWindow* ui;
    QGraphicsScene* _scene;
    AxisItem* _axis;
    QLabel* _label_pos;
    MillingItem* _milling_item;
    QTimer* _timer;
    StatusTable* _status_table;

  private:
    int nQuadrant = 0;  // 画圆象限
    int first = 1;  // 画圆参数 根据圆的起点判断起点的象限
    //用float型变量就搭配stof()，用double就用stod()，作用都是将string型变量转为小数变量
    float fXbeginArray[DEFAULT_PT_LEN] {};
    float fYbeginArray[DEFAULT_PT_LEN] {};
    float fXendArray[DEFAULT_PT_LEN] {};
    float fYendArray[DEFAULT_PT_LEN] {};
    float fFArray[DEFAULT_PT_LEN] {};

    float fIArray[DEFAULT_PT_LEN] {};
    float fJArray[DEFAULT_PT_LEN] {};
    double dRArray[DEFAULT_PT_LEN] {};

    int nDirArray[DEFAULT_PT_LEN] {};
    int nSArray[DEFAULT_PT_LEN] {};
    int nTArray[DEFAULT_PT_LEN] {};
    int nCool1Array[DEFAULT_PT_LEN] {};
    int nCool2Array[DEFAULT_PT_LEN] {};
    int nStatusArray[DEFAULT_PT_LEN] {};

    int nLineNum = 0;  // NC程序第nLineNum行
    int finish = 0;  // NC程序识别完成置1
    int x0 {}, y0 {}, x1 {}, y1 {};  // 画图参数 (x0,y0)--(x1,y1)

    int G90[DEFAULT_PT_LEN] {};  // 默认绝对值编程
    int G54[DEFAULT_PT_LEN] {};  // 默认机床坐标系
    std::vector<std::string> _cmd_cache;
};
#endif  // MAINWINDOW_H
