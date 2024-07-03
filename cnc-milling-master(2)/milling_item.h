#ifndef MILLING_ITEM_H
#define MILLING_ITEM_H

#include <qgraphicsitem.h>
class MillingItem : public QGraphicsPathItem
{
  public:
    MillingItem(QGraphicsItem* parent = nullptr);
    ~MillingItem();

    void reset();//重置

    void lineTo(int x, int y);

  protected:
    //绘制铣削路径
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

  private:
    QPainterPath _path;
    //QPainterPath 用来描述和管理绘图路径的形状，QPainter用来实际的绘制操作，包括绘制 QPainterPath 描述的路径以及其他图形、文本等。
};

#endif
