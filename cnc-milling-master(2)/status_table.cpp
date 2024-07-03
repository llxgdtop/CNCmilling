#include "status_table.h"

#include <qglobal.h>
#include <qnamespace.h>

int StatusTable::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return ROW_COUNT;
}

int StatusTable::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return COL_COUNT;
}

QVariant StatusTable::data(const QModelIndex& index, int role) const
{
    switch (role) {
        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case KEY:
                    return Qt::AlignLeft;//将“刀具”“冷却液”等等这几项居左对齐
                case VALUE:
                    return Qt::AlignCenter;//将它们对应的值居中对齐
            }
        case Qt::DisplayRole:
            switch (index.column()) {
                case KEY:
                    switch (Rows(index.row())) {
                        case Rows::KNIFE_COUNT:
                            return "刀具";
                        case DIRECTION:
                            return "主轴旋转方向";
                        case SPEED:
                            return "主轴速度";
                        case Lubricant_ONE:
                            return "切削液1";
                        case Lubricant_TWO:
                            return "切削液2";
                        case SUPPLY_SPEED:
                            return "进给速度";
                        case ROW_COUNT:
                            break;
                    }
                case VALUE:
                    switch (Rows(index.row())) {
                        case Rows::KNIFE_COUNT:
                            return _status.knife_count ? _status.knife_count: 1;
                        case DIRECTION:
                            return _status.direction;
                        case SPEED:
                            return _status.speed;
                        case Lubricant_ONE:
                            return _status.lubricant_one;
                        case Lubricant_TWO:
                            return _status.lubricant_two;
                        case SUPPLY_SPEED:
                            return _status.supply_speed;
                        case ROW_COUNT:
                            break;
                    }
            }
    }
    return {};
}

Qt::ItemFlags StatusTable::flags(const QModelIndex& index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;//设置状态栏为不可选中、不可编辑的状态
}
