#pragma once

#include "CellType.h"

#include <QMainWindow>

class NotebookView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    NotebookView* notebookView() const { return m_view; }

    /// 填充占位单元格。阶段 1 第一批用它制造滚动测试所需的规模。
    void populatePlaceholders(int count);

private:
    /// 单元格操作集中为 QAction，使快捷键与随后要加的菜单项共用同一个入口。
    void createCellActions();
    /// 按当前是否有单元格、是否已到首尾，开关相应的操作。
    void updateActionStates();
    QAction* addCellAction(CellType type, const QString& text, const QKeySequence& shortcut);

    NotebookView* m_view;
    QAction* m_deleteAction = nullptr;
    QAction* m_moveUpAction = nullptr;
    QAction* m_moveDownAction = nullptr;
};
