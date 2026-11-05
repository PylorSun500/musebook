#include "MainWindow.h"

#include <QLabel>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("Musebook"));
    resize(1100, 780);

    // 阶段 0 只验证构建与启动链路。
    // 菜单栏、滚动区域和单元格属于阶段 1。
    auto* placeholder = new QLabel(tr("Musebook"), this);
    placeholder->setAlignment(Qt::AlignCenter);
    setCentralWidget(placeholder);

    statusBar();
}
