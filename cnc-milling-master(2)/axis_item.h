#ifndef AXIS_ITEM_H
#define AXIS_ITEM_H

#include <qgraphicsitem.h>
#include <qgraphicsscene.h>
#include <qpoint.h>

struct Range
{
    //设置宽高
    double width;
    double height;
};

class AxisItem : public QGraphicsItem
{
  public:
    AxisItem(Range range);
    ~AxisItem();
    [[nodiscard]] QRectF boundingRect() const override;
    //重写这个函数的目的是为了提供图形项的准确边界矩形，以便于图形场景进行正确的绘制和布局
    //根据轴的范围和其他属性来计算图形项的边界矩形，确保它包含了整个轴的绘制区域

    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    void setRange(Range range);

  private:
    Range _range;
};

#endif
