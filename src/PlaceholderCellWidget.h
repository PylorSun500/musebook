#pragma once

#include "CellType.h"

#include <QFrame>

class QLabel;

/// 阶段 1 第一批的占位单元格，只负责占据版面并显示类型。
/// 它存在的目的是回答滚动性能问题，不承载任何内容编辑能力。
class PlaceholderCellWidget : public QFrame {
    Q_OBJECT

public:
    /// wrapping 为真时，Text 单元格使用自动换行且高度由内容决定，
    /// 从而触发 heightForWidth 计算。这是真实文本单元格的行为，
    /// 固定高度的占位单元格会低估布局开销。
    explicit PlaceholderCellWidget(CellType type, int index, bool wrapping = false,
                                   QWidget* parent = nullptr);

    CellType cellType() const { return m_type; }

private:
    CellType m_type;
    QLabel* m_label;
};
