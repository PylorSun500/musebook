#include "MainWindow.h"

#include "NotebookView.h"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), m_view(new NotebookView(this)) {
    setWindowTitle(tr("Musebook"));
    resize(1100, 780);

    setCentralWidget(m_view);
    // 先建操作再建菜单：菜单项直接引用这些 QAction，不重复定义任何操作，
    // 因此快捷键、启用状态在菜单和键盘两条路径上天然一致。
    createCellActions();
    createMenus();

    // 文档标题属于第二批后续任务。
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
    m_newCellActions = {
        addCellAction(CellType::Text, tr("新建文本单元格"), QKeySequence(tr("Ctrl+1"))),
        addCellAction(CellType::Score, tr("新建乐谱单元格"), QKeySequence(tr("Ctrl+2"))),
        addCellAction(CellType::Media, tr("新建媒体单元格"), QKeySequence(tr("Ctrl+3"))),
    };

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

void MainWindow::createMenus() {
    // 菜单在两个平台上各走自己的原生路线，这里不做任何干预。
    // macOS：Qt 的 Cocoa 插件实现了 QPlatformMenuBar，菜单变成真正的 NSMenu，
    //        出现在屏幕顶部的系统菜单栏里，窗口内不占高度。
    // Windows：Qt 不走 Win32 的 HMENU，而是把 QMenuBar 作为普通 widget 画在标题栏之下。
    // 两边看起来不一样，但各自都是该系统上的常规样子，这正是想要的结果。
    createFileMenu();

    QMenu* cellMenu = menuBar()->addMenu(tr("单元格"));
    cellMenu->addActions(m_newCellActions);
    cellMenu->addSeparator();
    cellMenu->addAction(m_deleteAction);
    cellMenu->addSeparator();
    cellMenu->addAction(m_moveUpAction);
    cellMenu->addAction(m_moveDownAction);

    createHelpMenu();
}

void MainWindow::createFileMenu() {
    QMenu* fileMenu = menuBar()->addMenu(tr("文件"));

    // 打开与保存需要文档模型和序列化，属于阶段 2。这里先按惯例位置把菜单项建出来并置灰，
    // 使菜单结构在阶段 1 就能完整审阅；阶段 2 接上实现时去掉 setEnabled(false) 即可。
    const auto addPendingAction = [this, fileMenu](const QString& text,
                                                   QKeySequence::StandardKey key) {
        auto* action = new QAction(text, this);
        action->setShortcut(key);
        action->setEnabled(false);
        fileMenu->addAction(action);
    };

    auto* newAction = new QAction(tr("新建"), this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newNotebook);
    fileMenu->addAction(newAction);
    addPendingAction(tr("打开…"), QKeySequence::Open);

    fileMenu->addSeparator();

    addPendingAction(tr("保存"), QKeySequence::Save);
    addPendingAction(tr("另存为…"), QKeySequence::SaveAs);

#ifndef Q_OS_MACOS
    // macOS 的程序菜单已经自带「退出 Musebook」，无须再给一份——给了也会被
    // QuitRole 并进那一项，只在文件菜单末尾留下一条孤立的分隔线。
    // 其余平台的退出项按惯例放在文件菜单末尾。
    fileMenu->addSeparator();
    auto* quitAction = new QAction(tr("退出 Musebook"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    quitAction->setMenuRole(QAction::QuitRole);
    connect(quitAction, &QAction::triggered, this, [] { QApplication::closeAllWindows(); });
    fileMenu->addAction(quitAction);
#endif
}

void MainWindow::createHelpMenu() {
    QMenu* helpMenu = menuBar()->addMenu(tr("帮助"));

    // 「关于」按 Windows 的惯例放在帮助菜单里，AboutRole 让 macOS 下的 Qt 把它
    // 搬进程序标题菜单——那才是 macOS 的惯例位置。一处声明，两个平台各自正确。
    //
    // menuRole 必须显式写出。它的默认值 TextHeuristicRole 是靠匹配英文菜单文本
    // 来判断该搬哪些项的，对中文菜单一律不生效。
    //
    // 两项都会被搬走，于是 macOS 下帮助菜单变空而不显示，菜单栏只剩文件和单元格。
    // 这是对的：macOS 应用在没有帮助内容时本就不出现帮助菜单。
    auto* aboutAction = new QAction(tr("关于 Musebook"), this);
    aboutAction->setMenuRole(QAction::AboutRole);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAction);

    auto* aboutQtAction = new QAction(tr("关于 Qt"), this);
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    connect(aboutQtAction, &QAction::triggered, this, [] { QApplication::aboutQt(); });
    helpMenu->addAction(aboutQtAction);
}

void MainWindow::newNotebook() {
    m_view->clearCells();
    // 留一个空白单元格，而不是一个空笔记：没有单元格时界面上无处落手。
    m_view->insertCell(0, CellType::Text);
}

void MainWindow::showAbout() {
    QMessageBox::about(
        this, tr("关于 Musebook"),
        tr("<h3>Musebook %1</h3>"
           "<p>面向音乐理论学习者的桌面笔记应用。"
           "在同一份笔记中编排文本、原创乐谱和多媒体材料。</p>"
           "<p>基于 Qt %2 构建。</p>")
            .arg(QApplication::applicationVersion(), QStringLiteral(QT_VERSION_STR)));
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
