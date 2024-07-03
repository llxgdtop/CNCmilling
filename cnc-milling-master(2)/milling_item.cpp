#include <QGraphicsScene>
#include <QPainter>

#include "milling_item.h"

#include <qpen.h>
MillingItem::MillingItem(QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
{
    setPen(QPen(Qt::red, 1));
}

MillingItem::~MillingItem() {}

void MillingItem::reset()
{
    _path.clear();//路径清空
    setPath(_path);
    scene()->update();//更新自绘数据如图表，实时数据显示(x,y值)等
}

void MillingItem::paint(QPainter* painter,
                        const QStyleOptionGraphicsItem* option,
                        QWidget* widget)
{
    //因为qt的坐标轴与正常理解坐标轴正方向不一样，故调用缩放语句，x缩放因子为1（不变），y倒过来
    painter->scale(1, -1);
    QGraphicsPathItem::paint(painter, option, widget);
}

void MillingItem::lineTo(int x, int y)//画直线
{
    _path.lineTo(x, y);
    setPath(_path);
    scene()->update();
}
