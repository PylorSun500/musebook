#include "MainWindow.h"

#include "NotebookView.h"

#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), m_view(new NotebookView(this)) {
    setWindowTitle(tr("Musebook"));
    resize(1100, 780);

    // 菜单栏、文档标题和单元格操作属于阶段 1 第二批，
    // 它们的实现方式取决于滚动性能测试的结论。
    setCentralWidget(m_view);

    statusBar();
}

void MainWindow::populatePlaceholders(int count) {
    for (int i = 0; i < count; ++i) {
        m_view->appendCell(static_cast<CellType>(i % 3));
    }
}
