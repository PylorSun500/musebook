#pragma once

#include "CellType.h"

#include <QScrollArea>

class QVBoxLayout;
class QWidget;

/// 单元格的垂直滚动容器。
///
/// 阶段 1 第一批刻意采用最直接的实现：QScrollArea + QVBoxLayout，
/// 每个单元格一个真实 widget，没有视图回收。这正是需要被测量的架构，
/// 测量结果决定后续是否必须引入虚拟化。
class NotebookView : public QScrollArea {
    Q_OBJECT

public:
    explicit NotebookView(QWidget* parent = nullptr);

    void appendCell(CellType type);
    void clearCells();

    int cellCount() const { return m_cellCount; }

    /// 让 Text 单元格使用自动换行与内容决定的高度。
    /// 仅影响此后新建的单元格。
    void setWrappingCells(bool wrapping) { m_wrapping = wrapping; }

private:
    QWidget* m_container;
    QVBoxLayout* m_layout;
    int m_cellCount = 0;
    bool m_wrapping = false;
};
