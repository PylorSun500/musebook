#pragma once

#include <QString>

class NotebookView;

/// 阶段 1 的滚动性能测量。
///
/// 流畅度无法靠主观感受判断，因此直接测量决定它的量：单次操作的同步耗时。
/// 超过 16.7 毫秒就无法维持 60 帧。
struct ScrollBenchmarkResult {
    int cellCount = 0;
    double buildMs = 0.0;
    double layoutMs = 0.0;
    int frameCount = 0;
    double medianFrameMs = 0.0;
    double p95FrameMs = 0.0;
    double maxFrameMs = 0.0;
    double medianResizeMs = 0.0;
    double maxResizeMs = 0.0;
    qint64 rssBytes = 0;

    QString toText() const;
};

/// 填充 count 个占位单元格，然后分别测量两件事：
/// 滚动时的逐帧重绘耗时，以及改变窗口宽度时的重新布局耗时。
/// 后者才是没有视图回收的真实代价所在。
ScrollBenchmarkResult runScrollBenchmark(NotebookView* view, int count, int frames);

/// 当前进程的常驻内存。取不到时返回 0。
qint64 currentRssBytes();
