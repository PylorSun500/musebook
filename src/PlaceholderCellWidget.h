#pragma once

#include "CellType.h"

#include <QFrame>
#include <QString>

class QFontMetrics;
class QLabel;

/// 占位单元格的文本内容。
///
/// 内容由视图生成并保存，不由 widget 持有：不可见的单元格没有 widget，
/// 但它的高度仍然必须算得出来。这是虚拟化成立的前提。
///
/// serial 是创建时分配的序号，不是单元格在文档中的位置。
/// 因此插入、删除和移动都不会改变已有单元格的文本，
/// 标签上的数字跟着单元格走，用肉眼就能看出排序操作是否生效。
QString placeholderCellText(CellType type, int serial, bool wrapping);

/// 不创建 widget 就能算出的单元格高度。
///
/// 结果必须与 PlaceholderCellWidget 的实际渲染一致，因此两者共用本文件里的边距常量。
int placeholderCellHeight(const QFontMetrics& metrics, CellType type, const QString& text,
                          int cellWidth, bool wrapping);

/// 阶段 1 的占位单元格，只负责占据版面并显示类型。
///
/// 它可以复用：滚出可见区域的 widget 回到视图的池中，再用 setCell 装上另一个单元格的内容。
/// 几何尺寸由视图直接 setGeometry 给定，widget 不参与任何父级布局计算。
class PlaceholderCellWidget : public QFrame {
    Q_OBJECT

public:
    explicit PlaceholderCellWidget(QWidget* parent = nullptr);

    /// wrapping 为真时，Text 单元格自动换行且高度由内容决定。
    /// 固定高度的占位单元格会低估布局开销，所以测量必须覆盖换行这一路。
    void setCell(CellType type, const QString& text, bool wrapping);

    /// 当前单元格的视觉状态。这里只做到「能看出选中的是哪一个」，
    /// 三种状态的最终样式属于第二批中专门的视觉状态任务。
    void setCurrent(bool current);

    CellType cellType() const { return m_type; }
    /// 当前显示的文本。复用 widget 时它必须与所装单元格一致。
    QString text() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    CellType m_type = CellType::Text;
    bool m_current = false;
    QLabel* m_label;
};
