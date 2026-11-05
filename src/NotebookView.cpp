#include "NotebookView.h"

#include "PlaceholderCellWidget.h"

#include <QVBoxLayout>
#include <QWidget>

NotebookView::NotebookView(QWidget* parent)
    : QScrollArea(parent), m_container(new QWidget), m_layout(new QVBoxLayout(m_container)) {
    m_layout->setContentsMargins(16, 16, 16, 16);
    m_layout->setSpacing(6);
    m_layout->addStretch(1);

    setWidget(m_container);
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void NotebookView::appendCell(CellType type) {
    auto* cell = new PlaceholderCellWidget(type, m_cellCount, m_wrapping, m_container);
    // 插在末尾的 stretch 之前，让单元格顶部对齐而不是被拉伸填满。
    m_layout->insertWidget(m_layout->count() - 1, cell);
    ++m_cellCount;
}

void NotebookView::clearCells() {
    while (m_layout->count() > 1) {
        QLayoutItem* item = m_layout->takeAt(0);
        delete item->widget();
        delete item;
    }
    m_cellCount = 0;
}
