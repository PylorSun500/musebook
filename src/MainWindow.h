#pragma once

#include "CellType.h"

#include <QList>
#include <QMainWindow>

class NotebookView;
class QMenu;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    NotebookView* notebookView() const { return m_view; }

    /// 填充占位单元格。阶段 1 第一批用它制造滚动测试所需的规模。
    void populatePlaceholders(int count);

private:
    /// 单元格操作集中为 QAction，使快捷键与菜单项共用同一个入口。
    void createCellActions();
    /// 按当前是否有单元格、是否已到首尾，开关相应的操作。
    void updateActionStates();
    QAction* addCellAction(CellType type, const QString& text, const QKeySequence& shortcut);

    /// 建立菜单栏。菜单项全部引用已有的 QAction，不重复定义任何操作。
    void createMenus();
    void createFileMenu();
    void createHelpMenu();
    /// 清空笔记并留下一个空白文本单元格。阶段 2 接入文档模型后，
    /// 这里要先经过未保存提示。
    void newNotebook();
    void showAbout();

    NotebookView* m_view;
    QAction* m_deleteAction = nullptr;
    QAction* m_moveUpAction = nullptr;
    QAction* m_moveDownAction = nullptr;
    /// 三个新建单元格操作，按 Text / Score / Media 顺序。菜单构建时需要它们。
    QList<QAction*> m_newCellActions;
};
