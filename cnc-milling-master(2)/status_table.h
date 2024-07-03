#ifndef STATUS_TABLE_H
#define STATUS_TABLE_H

#include <qabstractitemmodel.h>

struct Status
{
    int knife_count {};//刀具
    int direction {};//旋转方向
    int speed {};//主轴速度
    int lubricant_one {};//冷却液
    int lubricant_two {};
    QString supply_speed {"0"};
};

enum Cols
{
    KEY,
    VALUE,
    COL_COUNT
};

enum Rows
{
    KNIFE_COUNT,
    DIRECTION,
    SPEED,
    Lubricant_ONE,
    Lubricant_TWO,
    SUPPLY_SPEED,

    ROW_COUNT,
};

class StatusTable : public QAbstractTableModel
{
    //继承 QAbstractTableModel 后，至少要实现三个纯虚函数才能进行实例化
  public:
    //将 QAbstractTableModel 中的构造函数引入到 StatusTable 类中,就不用自己定义了
    using QAbstractTableModel::QAbstractTableModel;

    [[nodiscard]] int rowCount(
        const QModelIndex& parent = QModelIndex()) const override;

    [[nodiscard]] int columnCount(
        const QModelIndex& parent = QModelIndex()) const override;

    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role = Qt::DisplayRole) const override;

    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;

    void update_status(const Status& status)
    {
        beginResetModel();
        _status = status;
        endResetModel();
    }

  private:
    Status _status;
};

#endif
