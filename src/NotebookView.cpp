#include "NotebookView.h"

#include "PlaceholderCellWidget.h"

#include <QEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
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
    explicit CellCanvas(QWidget* parent = nullptr) : QWidget(parent) {
        // 容器自身也不处理鼠标，使按下事件直接落到 viewport，
        // 而不是依赖「子 widget 忽略事件后上传给父级」这条链路。
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

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
    // 方向键要落到本类，而不是被滚动区域当作滚动命令。
    setFocusPolicy(Qt::StrongFocus);
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
    // 追加不改变当前单元格，也不滚动。它是批量填充用的原语，
    // 「新建后选中并滚过去」是用户操作的行为，属于 insertCell。
    insertCellAt(cellCount(), type);
    updateOffsetsFrom(cellCount() - 1);
    updateVisibleCells();
    Q_EMIT cellCountChanged(cellCount());
}

void NotebookView::insertCellAt(int index, CellType type) {
    Cell cell;
    cell.type = type;
    cell.text = placeholderCellText(type, m_nextSerial, m_wrapping);
    ++m_nextSerial;
    // 只测量新单元格。已有单元格的高度不受插入影响，重算它们是纯浪费——
    // 逐个追加单元格时，那个浪费会累积成 O(n²) 次文字测量。
    cell.height = measureCell(cell);
    m_cells.insert(index, std::move(cell));
}

void NotebookView::insertCell(int index, CellType type) {
    const int at = std::clamp(index, 0, cellCount());
    insertCellAt(at, type);
    m_currentIndex = at;
    rebuildAfterStructureChange(at);
    scrollToCurrent();
}

void NotebookView::insertCellAfterCurrent(CellType type) {
    insertCell(m_currentIndex < 0 ? cellCount() : m_currentIndex + 1, type);
}

void NotebookView::removeCell(int index) {
    if (index < 0 || index >= cellCount()) {
        return;
    }
    m_cells.removeAt(index);
    // 当前位置保持不动，使连续删除能一直删下去；删掉末尾时退到新的末尾。
    m_currentIndex = std::min(index, cellCount() - 1);
    rebuildAfterStructureChange(index);
    scrollToCurrent();
}

void NotebookView::removeCurrentCell() {
    removeCell(m_currentIndex);
}

void NotebookView::moveCurrentUp() {
    if (m_currentIndex <= 0) {
        return;
    }
    m_cells.swapItemsAt(m_currentIndex, m_currentIndex - 1);
    --m_currentIndex;
    rebuildAfterStructureChange(m_currentIndex);
    scrollToCurrent();
}

void NotebookView::moveCurrentDown() {
    if (m_currentIndex < 0 || m_currentIndex >= cellCount() - 1) {
        return;
    }
    m_cells.swapItemsAt(m_currentIndex, m_currentIndex + 1);
    ++m_currentIndex;
    rebuildAfterStructureChange(m_currentIndex - 1);
    scrollToCurrent();
}

void NotebookView::clearCells() {
    for (PlaceholderCellWidget* widget : std::as_const(m_live)) {
        releaseWidget(widget);
    }
    m_live.clear();
    m_cells.clear();
    m_currentIndex = -1;
    updateCanvasSize();
    Q_EMIT currentIndexChanged(m_currentIndex);
    Q_EMIT cellCountChanged(0);
}

void NotebookView::rebuildAfterStructureChange(int fromIndex) {
    // m_live 以位置为键。fromIndex 之后的键与单元格的对应关系整体错位，必须交回池中；
    // 它之前的键仍然指向同一个单元格，几何和内容都还对，留着即可。
    // 在末尾追加时这里一个都不释放，因此批量追加不会反复重建可见区间的 widget。
    for (auto it = m_live.begin(); it != m_live.end();) {
        if (it.key() >= fromIndex) {
            releaseWidget(it.value());
            it = m_live.erase(it);
        } else {
            ++it;
        }
    }

    // 只有偏移需要更新，高度不受结构变化影响：单元格的宽度没变，文字也没变。
    // 若在这里调用 relayout()，逐个追加单元格就会退化成 O(n²) 次文字测量。
    updateOffsetsFrom(fromIndex);
    updateVisibleCells();

    Q_EMIT currentIndexChanged(m_currentIndex);
    Q_EMIT cellCountChanged(cellCount());
}

void NotebookView::setCurrentIndex(int index) {
    const int next = (index < 0 || index >= cellCount()) ? -1 : index;
    if (next == m_currentIndex) {
        return;
    }
    m_currentIndex = next;
    // 选中态只是外观变化，不影响几何，因此不需要重排。
    for (auto it = m_live.cbegin(); it != m_live.cend(); ++it) {
        it.value()->setCurrent(it.key() == m_currentIndex);
    }
    Q_EMIT currentIndexChanged(m_currentIndex);
}

void NotebookView::scrollToCurrent() {
    if (m_currentIndex < 0) {
        return;
    }
    const Cell& cell = m_cells.at(m_currentIndex);
    QScrollBar* bar = verticalScrollBar();
    const int top = bar->value();
    const int bottom = top + viewport()->height();

    if (cell.top < top) {
        bar->setValue(std::max(0, cell.top - contentMargin));
    } else if (cell.bottom() > bottom) {
        // 单元格比 viewport 还高时，对齐顶部而不是底部，否则会看不到它的开头。
        const int target = cell.height > viewport()->height()
                               ? cell.top - contentMargin
                               : cell.bottom() - viewport()->height() + contentMargin;
        bar->setValue(std::clamp(target, 0, bar->maximum()));
    }
}

void NotebookView::stepCurrent(int delta) {
    if (m_cells.isEmpty()) {
        return;
    }
    // 没有当前单元格时，向下进入第一个，向上进入最后一个。
    const int from = m_currentIndex >= 0 ? m_currentIndex : (delta > 0 ? -1 : cellCount());
    setCurrentIndex(std::clamp(from + delta, 0, cellCount() - 1));
    scrollToCurrent();
}

int NotebookView::cellTop(int index) const {
    if (index < 0 || index >= cellCount()) {
        return -1;
    }
    return m_cells.at(index).top;
}

QString NotebookView::cellText(int index) const {
    if (index < 0 || index >= cellCount()) {
        return QString();
    }
    return m_cells.at(index).text;
}

int NotebookView::expectedCellHeight(int index) const {
    if (index < 0 || index >= cellCount()) {
        return -1;
    }
    return measureCell(m_cells.at(index));
}

int NotebookView::cellIndexAt(int documentY) const {
    const auto endsAbove = [](const Cell& cell, int y) { return cell.bottom() <= y; };
    const auto it = std::lower_bound(m_cells.cbegin(), m_cells.cend(), documentY, endsAbove);
    if (it == m_cells.cend() || documentY < it->top) {
        return -1;
    }
    return static_cast<int>(it - m_cells.cbegin());
}

void NotebookView::keyPressEvent(QKeyEvent* event) {
    // 这里只管在单元格之间移动当前位置。增删和排序由窗口上的 QAction 承担，
    // 因为它们还要出现在菜单里。
    switch (event->key()) {
    case Qt::Key_Up:
        stepCurrent(-1);
        return;
    case Qt::Key_Down:
        stepCurrent(1);
        return;
    case Qt::Key_Home:
        setCurrentIndex(0);
        scrollToCurrent();
        return;
    case Qt::Key_End:
        setCurrentIndex(cellCount() - 1);
        scrollToCurrent();
        return;
    default:
        break;
    }
    QScrollArea::keyPressEvent(event);
}

void NotebookView::handleViewportPress(const QPoint& viewportPos) {
    // 单元格之间的间隙也会落到这里，点空白处不改变当前单元格。
    const int index = cellIndexAt(viewportPos.y() + verticalScrollBar()->value());
    if (index >= 0) {
        setCurrentIndex(index);
    }
}

void NotebookView::relayout() {
    const QFontMetrics metrics(m_canvas->font());
    const int width = cellWidth();
    for (Cell& cell : m_cells) {
        cell.height = placeholderCellHeight(metrics, cell.type, cell.text, width, m_wrapping);
    }
    updateOffsetsFrom(0);
}

void NotebookView::updateOffsetsFrom(int index) {
    const int from = std::max(0, index);
    int top = from > 0 ? m_cells.at(from - 1).bottom() + cellSpacing : contentMargin;
    for (int i = from; i < cellCount(); ++i) {
        Cell& cell = m_cells[i];
        cell.top = top;
        top = cell.bottom() + cellSpacing;
    }
    updateCanvasSize();
}

int NotebookView::measureCell(const Cell& cell) const {
    const QFontMetrics metrics(m_canvas->font());
    return placeholderCellHeight(metrics, cell.type, cell.text, cellWidth(), m_wrapping);
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
        // 复用来的 widget 可能带着上一个单元格的选中态，每次都要显式设定。
        widget->setCurrent(index == m_currentIndex);
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
    switch (event->type()) {
    case QEvent::Resize:
        // 宽度变化才需要重算高度；只改变高度时单元格的换行不变。
        syncLayoutWidth();
        break;
    case QEvent::MouseButtonPress:
        // 占位单元格与其中的标签都不接受鼠标按下，事件会一路上传到 viewport。
        // 因此点击在这里统一处理，不必给每个单元格装事件过滤器。
        handleViewportPress(static_cast<QMouseEvent*>(event)->position().toPoint());
        break;
    default:
        break;
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
