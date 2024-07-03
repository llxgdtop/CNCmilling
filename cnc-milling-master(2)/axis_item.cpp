#include <QPainter>
#include <QWidget>

#include "axis_item.h"
AxisItem::AxisItem(Range range)
    : _range(std::move(range))
{
}

AxisItem::~AxisItem() {}

QRectF AxisItem::boundingRect() const
{
    return {-_range.width / 2, -_range.height / 2, _range.width, _range.height};
}

void AxisItem::paint(QPainter* painter,
                     const QStyleOptionGraphicsItem* option,
                     QWidget* widget)
{
    Q_UNUSED(option);//用于避免编译器警告
    painter->setPen(QPen(Qt::black, 1));//轴的颜色，像素
    const auto start_x = -_range.width /2;
    const auto end_x = _range.width / 2;
    const auto start_y = -_range.height / 2;
    const auto end_y = _range.height / 2;
    painter->drawLine(start_x, 0, end_x, 0);
    painter->drawLine(0, start_y, 0, end_y);

    // 绘制x轴刻度
    const auto tick_length = 10;
    const auto step = 50.0;//刻度的长度和间距
    for (int i = start_x / step; i < end_x / step; i++) {
        const auto x = i * step;
        painter->drawLine(x, -tick_length, x, tick_length);
        const auto text_rect =
            widget->fontMetrics().boundingRect(QString::number(x));//将数字x转换为字符串，并传递给boundingRect()来计算这个字符串的边界矩形的宽高
        // 轴上数字
        painter->drawText(
            x - text_rect.width() / 2, -tick_length - 10, QString::number(x));
            //第一个参数确定数字的水平位置，x为刻度线的位置，text_rect.width() / 2 是文本宽度的一半，这样就可以将文本的中心对准刻度线
            //第三个参数将x转成string类型
    }

    //y轴
    for (int i = start_y / step; i < end_y / step; i++) {
        const auto y = i * step;
        painter->drawLine(-tick_length, y, tick_length, y);
        const auto text_rect =
            widget->fontMetrics().boundingRect(QString::number(y));
        painter->drawText(
            -tick_length - text_rect.width(),y - text_rect.height() / 2,
                          QString::number(-y));
    }
}

void AxisItem::setRange(Range range)
{
    //std::move() 将对象的状态或者所有权从一个对象转移到另一个对象，只是转移，没有内存的搬迁或者内存拷贝，所以可以提高效率，改善性能
    _range = std::move(range);
    update();//触发重新绘制图形项,在轴范围发生变化时
}
