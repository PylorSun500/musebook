#include "MainWindow.h"

#include "NotebookView.h"

#include <QAction>
#include <QKeySequence>
#include <QScrollBar>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), m_view(new NotebookView(this)) {
    setWindowTitle(tr("Musebook"));
    resize(1100, 780);

    setCentralWidget(m_view);
    createCellActions();

    // 菜单栏与文档标题属于第二批后续任务。这些操作先以快捷键形式存在，
    // 菜单建立时直接引用同一批 QAction，不必再写一遍。
    statusBar();

    connect(m_view, &NotebookView::currentIndexChanged, this, &MainWindow::updateActionStates);
    connect(m_view, &NotebookView::cellCountChanged, this, &MainWindow::updateActionStates);
    updateActionStates();
}

QAction* MainWindow::addCellAction(CellType type, const QString& text,
                                   const QKeySequence& shortcut) {
    auto* action = new QAction(text, this);
    action->setShortcut(shortcut);
    connect(action, &QAction::triggered, this,
            [this, type] { m_view->insertCellAfterCurrent(type); });
    addAction(action);
    return action;
}

void MainWindow::createCellActions() {
    // 三种类型各有一个新建操作。Cmd+1/2/3 与类型顺序对应，
    // 比记住「新建单元格再改类型」两步更直接。
    addCellAction(CellType::Text, tr("新建文本单元格"), QKeySequence(tr("Ctrl+1")));
    addCellAction(CellType::Score, tr("新建乐谱单元格"), QKeySequence(tr("Ctrl+2")));
    addCellAction(CellType::Media, tr("新建媒体单元格"), QKeySequence(tr("Ctrl+3")));

    m_deleteAction = new QAction(tr("删除当前单元格"), this);
    // Backspace 而非 Delete：macOS 的主键盘上没有独立的 Delete 键。
    m_deleteAction->setShortcuts({QKeySequence(tr("Ctrl+Backspace")), QKeySequence::Delete});
    connect(m_deleteAction, &QAction::triggered, m_view, &NotebookView::removeCurrentCell);
    addAction(m_deleteAction);

    m_moveUpAction = new QAction(tr("上移当前单元格"), this);
    m_moveUpAction->setShortcut(QKeySequence(tr("Ctrl+Shift+Up")));
    connect(m_moveUpAction, &QAction::triggered, m_view, &NotebookView::moveCurrentUp);
    addAction(m_moveUpAction);

    m_moveDownAction = new QAction(tr("下移当前单元格"), this);
    m_moveDownAction->setShortcut(QKeySequence(tr("Ctrl+Shift+Down")));
    connect(m_moveDownAction, &QAction::triggered, m_view, &NotebookView::moveCurrentDown);
    addAction(m_moveDownAction);
}

void MainWindow::updateActionStates() {
    const int index = m_view->currentIndex();
    const int count = m_view->cellCount();

    m_deleteAction->setEnabled(index >= 0);
    m_moveUpAction->setEnabled(index > 0);
    m_moveDownAction->setEnabled(index >= 0 && index < count - 1);
}

void MainWindow::populatePlaceholders(int count) {
    for (int i = 0; i < count; ++i) {
        m_view->appendCell(static_cast<CellType>(i % 3));
    }
    // 填充完停在开头，而不是停在最后一个新建的单元格上。
    m_view->setCurrentIndex(count > 0 ? 0 : -1);
    m_view->verticalScrollBar()->setValue(0);
}
