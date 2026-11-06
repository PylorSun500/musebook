#include "NotebookView.h"

#include "PlaceholderCellWidget.h"

#include <QEvent>
#include <QFontMetrics>
#include <QScrollBar>
#include <QWidget>

#include <algorithm>
#include <utility>

namespace {

/// 与第一批的 QVBoxLayout 取值一致，使新旧两种实现的性能数据可以直接对比。
constexpr int contentMargin = 16;
constexpr int cellSpacing = 6;

/// 可见区域之外多物化几个单元格，使快速滚动的边缘不出现空白。
constexpr int overscanCells = 2;

/// 容器宽度变化会让滚动条出现或消失，从而再次改变 viewport 宽度。
/// 允许几次迭代收敛，上限避免在阈值附近无限来回。
constexpr int maxSyncPasses = 3;

} // namespace

/// 承载单元格的容器。不使用 QLayout，因此 sizeHint 必须自己给出——
/// 否则 adjustSize() 会按子 widget 的边界把它缩小，而子 widget 只有可见那几个。
class CellCanvas : public QWidget {
public:
    explicit CellCanvas(QWidget* parent = nullptr) : QWidget(parent) {}

    QSize sizeHint() const override { return m_documentSize; }

    void setDocumentSize(QSize size) {
        if (size == m_documentSize) {
            return;
        }
        m_documentSize = size;
        updateGeometry();
        resize(size);
    }

private:
    QSize m_documentSize{0, 0};
};

NotebookView::NotebookView(QWidget* parent) : QScrollArea(parent), m_canvas(new CellCanvas) {
    setWidget(m_canvas);
    // 容器尺寸由本类直接计算，不能交给 QScrollArea 按 sizeHint 去调整。
    setWidgetResizable(false);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    updateCanvasSize();
}

int NotebookView::cellWidth() const {
    return std::max(1, viewport()->width() - 2 * contentMargin);
}

int NotebookView::documentHeight() const {
    if (m_cells.isEmpty()) {
        return 2 * contentMargin;
    }
    return m_cells.constLast().bottom() + contentMargin;
}

void NotebookView::updateCanvasSize() {
    m_canvas->setDocumentSize(QSize(viewport()->width(), documentHeight()));
}

void NotebookView::appendCell(CellType type) {
    const int index = cellCount();

    Cell cell;
    cell.type = type;
    cell.text = placeholderCellText(type, index, m_wrapping);
    cell.top = m_cells.isEmpty() ? contentMargin : m_cells.constLast().bottom() + cellSpacing;
    cell.height = placeholderCellHeight(QFontMetrics(m_canvas->font()), cell.type, cell.text,
                                        cellWidth(), m_wrapping);
    m_cells.append(std::move(cell));

    // 追加不影响已有单元格的偏移，因此不需要重排整个文档。
    updateCanvasSize();
    updateVisibleCells();
}

void NotebookView::clearCells() {
    for (PlaceholderCellWidget* widget : std::as_const(m_live)) {
        releaseWidget(widget);
    }
    m_live.clear();
    m_cells.clear();
    updateCanvasSize();
}

void NotebookView::relayout() {
    const QFontMetrics metrics(m_canvas->font());
    const int width = cellWidth();

    int top = contentMargin;
    for (Cell& cell : m_cells) {
        cell.top = top;
        cell.height = placeholderCellHeight(metrics, cell.type, cell.text, width, m_wrapping);
        top = cell.bottom() + cellSpacing;
    }
    updateCanvasSize();
}

void NotebookView::updateVisibleCells() {
    const int top = verticalScrollBar()->value();
    const int bottom = top + viewport()->height();

    // 二分查找定位可见区间，使这一步的代价与单元格总数无关。
    const auto endsAbove = [](const Cell& cell, int y) { return cell.bottom() <= y; };
    const auto startsBefore = [](const Cell& cell, int y) { return cell.top < y; };
    const auto firstIt = std::lower_bound(m_cells.cbegin(), m_cells.cend(), top, endsAbove);
    const auto endIt = std::lower_bound(firstIt, m_cells.cend(), bottom, startsBefore);

    const int first = std::max(0, static_cast<int>(firstIt - m_cells.cbegin()) - overscanCells);
    const int last =
        std::min(cellCount() - 1, static_cast<int>(endIt - m_cells.cbegin()) - 1 + overscanCells);

    for (auto it = m_live.begin(); it != m_live.end();) {
        if (it.key() < first || it.key() > last) {
            releaseWidget(it.value());
            it = m_live.erase(it);
        } else {
            ++it;
        }
    }

    const int width = cellWidth();
    for (int index = first; index <= last; ++index) {
        const Cell& cell = m_cells.at(index);
        PlaceholderCellWidget*& widget = m_live[index];
        if (widget == nullptr) {
            widget = acquireWidget();
            widget->setCell(cell.type, cell.text, m_wrapping);
        }
        const QRect slot(contentMargin, cell.top, width, cell.height);
        if (widget->geometry() != slot) {
            widget->setGeometry(slot);
        }
        widget->show();
    }
}

void NotebookView::syncLayoutWidth() {
    if (m_syncing) {
        return;
    }
    m_syncing = true;
    for (int pass = 0; pass < maxSyncPasses; ++pass) {
        if (viewport()->width() == m_layoutWidth) {
            break;
        }
        m_layoutWidth = viewport()->width();
        relayout();
    }
    updateVisibleCells();
    m_syncing = false;
}

bool NotebookView::viewportEvent(QEvent* event) {
    const bool handled = QScrollArea::viewportEvent(event);
    if (event->type() == QEvent::Resize) {
        // 宽度变化才需要重算高度；只改变高度时单元格的换行不变。
        syncLayoutWidth();
    }
    return handled;
}

void NotebookView::scrollContentsBy(int dx, int dy) {
    QScrollArea::scrollContentsBy(dx, dy);
    updateVisibleCells();
}

PlaceholderCellWidget* NotebookView::acquireWidget() {
    if (!m_pool.isEmpty()) {
        return m_pool.takeLast();
    }
    return new PlaceholderCellWidget(m_canvas);
}

void NotebookView::releaseWidget(PlaceholderCellWidget* widget) {
    widget->hide();
    m_pool.append(widget);
}
