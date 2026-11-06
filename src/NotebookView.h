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

    /// 追加到末尾。不改变当前单元格、不滚动，适合批量填充。
    void appendCell(CellType type);
    void clearCells();
    int cellCount() const { return static_cast<int>(m_cells.size()); }

    /// 在 index 处插入一个单元格，index 等于 cellCount() 时追加到末尾。
    /// 插入的单元格成为当前单元格。
    void insertCell(int index, CellType type);
    /// 在当前单元格之后插入。没有当前单元格时追加到末尾。
    void insertCellAfterCurrent(CellType type);
    /// 删除 index 处的单元格。删除后当前单元格落在原位置，
    /// 若删掉的是最后一个则落在新的末尾。
    void removeCell(int index);
    void removeCurrentCell();

    /// 当前单元格前移或后移一格。到头时什么也不做。
    void moveCurrentUp();
    void moveCurrentDown();

    /// 当前单元格的位置。没有单元格时为 -1。
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);
    /// 把当前单元格完整滚入可见区域。
    void scrollToCurrent();
    /// 当前单元格上移或下移一个位置，并滚动到它。
    void stepCurrent(int delta);

    /// 让 Text 单元格使用自动换行与内容决定的高度。需在添加单元格之前设置。
    void setWrappingCells(bool wrapping) { m_wrapping = wrapping; }

    /// 当前已物化的 widget 数量。虚拟化是否生效看这个数字，而不是单元格总数。
    int liveWidgetCount() const { return static_cast<int>(m_live.size()); }

    /// index 处单元格在文档坐标系中的顶边。越界时返回 -1。
    int cellTop(int index) const;
    /// index 处单元格的文本。越界时返回空字符串。
    QString cellText(int index) const;
    /// index 处单元格在当前宽度下重新测量出的高度。越界时返回 -1。
    int expectedCellHeight(int index) const;

Q_SIGNALS:
    void currentIndexChanged(int index);
    void cellCountChanged(int count);

protected:
    bool viewportEvent(QEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    /// viewport 坐标系中的一次鼠标按下。把落点所在的单元格设为当前单元格。
    void handleViewportPress(const QPoint& viewportPos);
    /// 单元格的全部状态。没有 widget 时它依然完整存在，这是虚拟化的前提。
    struct Cell {
        CellType type = CellType::Text;
        QString text;
        int top = 0;
        int height = 0;

        int bottom() const { return top + height; }
    };

    /// 结构变化（插入、删除、移动）后重建视图。
    ///
    /// m_live 以位置为键，而结构变化会让位置与单元格的对应关系整体错位，
    /// 因此必须先把全部 widget 交回池中，不能只更新受影响的那几个。
    /// fromIndex 是第一个偏移可能变化的位置，它之前的单元格保持原状。
    void rebuildAfterStructureChange(int fromIndex);
    /// 文档坐标系中的 y 落在哪个单元格上。不在任何单元格上时返回 -1。
    int cellIndexAt(int documentY) const;

    int cellWidth() const;
    int documentHeight() const;
    /// 重算全部单元格的高度与偏移。只在宽度变化时调用——这是唯一会让已有高度失效的事件。
    void relayout();
    /// 从 index 起重排偏移，不重算高度。结构变化走这条路，代价与受影响的单元格数成正比。
    void updateOffsetsFrom(int index);
    /// 单个单元格在当前宽度下的高度。
    int measureCell(const Cell& cell) const;
    /// 构造一个单元格并插入到 index。只改动 m_cells，不碰 widget、偏移与当前位置。
    void insertCellAt(int index, CellType type);
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
    int m_currentIndex = -1;
    /// 下一个要分配的创建序号。它只增不减，因此标签上的数字在插删后保持稳定，
    /// 肉眼就能看出某个单元格有没有真的移动过。
    int m_nextSerial = 0;
    bool m_syncing = false;
    bool m_wrapping = false;
};
