#pragma once

#include "CellType.h"

#include <QHash>
#include <QList>
#include <QScrollArea>
#include <QString>

class CellCanvas;
class PlaceholderCellWidget;

/// 单元格的垂直滚动容器。
///
/// 这里刻意不使用 QLayout。阶段 1 的测量结论是：宽度变化时 QVBoxLayout 会向每个
/// 单元格索取一次 heightForWidth，代价约 0.28 ms 每单元格，约 55 个单元格就超出
/// 一帧的预算。要避免的正是这个计算，而 QLayout 必然触发它。
///
/// 因此几何由本类自己管理：高度在没有 widget 的情况下直接用 QFontMetrics 算出，
/// 真实 widget 只为可见区域创建，滚出视野后回到池中复用。
class NotebookView : public QScrollArea {
    Q_OBJECT

public:
    explicit NotebookView(QWidget* parent = nullptr);

    void appendCell(CellType type);
    void clearCells();
    int cellCount() const { return static_cast<int>(m_cells.size()); }

    /// 让 Text 单元格使用自动换行与内容决定的高度。需在添加单元格之前设置。
    void setWrappingCells(bool wrapping) { m_wrapping = wrapping; }

    /// 当前已物化的 widget 数量。虚拟化是否生效看这个数字，而不是单元格总数。
    int liveWidgetCount() const { return static_cast<int>(m_live.size()); }

protected:
    bool viewportEvent(QEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    /// 单元格的全部状态。没有 widget 时它依然完整存在，这是虚拟化的前提。
    struct Cell {
        CellType type = CellType::Text;
        QString text;
        int top = 0;
        int height = 0;

        int bottom() const { return top + height; }
    };

    int cellWidth() const;
    int documentHeight() const;
    /// 重算全部单元格的高度与偏移。宽度变化时调用，代价只有一次文字测量。
    void relayout();
    /// 让可见区间内的单元格各有一个 widget，区间外的交回池中。
    void updateVisibleCells();
    /// 把布局宽度对齐到当前 viewport 宽度，必要时重排。
    void syncLayoutWidth();
    void updateCanvasSize();
    PlaceholderCellWidget* acquireWidget();
    void releaseWidget(PlaceholderCellWidget* widget);

    CellCanvas* m_canvas;
    QList<Cell> m_cells;
    /// 单元格序号 -> 已物化的 widget。只覆盖可见区间及少量预取。
    ///
    /// 键是序号而不是单元格标识，因此在中间插入或删除单元格会让它整体错位。
    /// 实现增删与排序时必须先把全部 widget 交回池中，再重新物化。
    QHash<int, PlaceholderCellWidget*> m_live;
    /// 滚出视野的 widget。复用它们，使快速滚动不必反复构造 widget。
    QList<PlaceholderCellWidget*> m_pool;
    /// 当前高度是按哪个宽度算出来的。-1 表示还没算过。
    int m_layoutWidth = -1;
    bool m_syncing = false;
    bool m_wrapping = false;
};
