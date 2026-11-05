#pragma once

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
    NotebookView* m_view;
};
