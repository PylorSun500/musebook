#include "ScrollBenchmark.h"

#include "NotebookView.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QScrollBar>

#include <algorithm>
#include <vector>

#if defined(Q_OS_MACOS)
#include <mach/mach.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
// psapi.h 必须在 windows.h 之后
#include <psapi.h>
#endif

namespace {

double percentile(const std::vector<double>& sorted, double fraction) {
    if (sorted.empty()) {
        return 0.0;
    }
    const auto maxIndex = static_cast<double>(sorted.size() - 1);
    const auto index = static_cast<std::size_t>(fraction * maxIndex);
    return sorted[index];
}

} // namespace

QString ScrollBenchmarkResult::toText() const {
    return QStringLiteral("单元格 %1 个\n"
                          "构造 %2 ms，布局 %3 ms\n"
                          "滚动帧耗时（%4 帧）：中位 %5 ms，p95 %6 ms，最大 %7 ms\n"
                          "改变宽度耗时：中位 %8 ms，最大 %9 ms\n"
                          "常驻内存增量 %10 MB")
        .arg(cellCount)
        .arg(buildMs, 0, 'f', 1)
        .arg(layoutMs, 0, 'f', 1)
        .arg(frameCount)
        .arg(medianFrameMs, 0, 'f', 2)
        .arg(p95FrameMs, 0, 'f', 2)
        .arg(maxFrameMs, 0, 'f', 2)
        .arg(medianResizeMs, 0, 'f', 1)
        .arg(maxResizeMs, 0, 'f', 1)
        .arg(static_cast<double>(rssBytes) / (1024.0 * 1024.0), 0, 'f', 1);
}

qint64 currentRssBytes() {
#if defined(Q_OS_MACOS)
    mach_task_basic_info info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info),
                  &count) == KERN_SUCCESS) {
        return static_cast<qint64>(info.resident_size);
    }
    return 0;
#elif defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<qint64>(pmc.WorkingSetSize);
    }
    return 0;
#else
    return 0;
#endif
}

ScrollBenchmarkResult runScrollBenchmark(NotebookView* view, int count, int frames) {
    ScrollBenchmarkResult result;
    result.cellCount = count;

    const qint64 rssBefore = currentRssBytes();

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < count; ++i) {
        // 三种类型轮转，使行高不均匀，更接近真实文档。
        const auto type = static_cast<CellType>(i % 3);
        view->appendCell(type);
    }
    result.buildMs = static_cast<double>(timer.nsecsElapsed()) / 1e6;

    // 让 layout 完成一次完整计算，这部分开销与构造分开记录。
    timer.restart();
    view->widget()->adjustSize();
    QCoreApplication::processEvents();
    view->widget()->ensurePolished();
    result.layoutMs = static_cast<double>(timer.nsecsElapsed()) / 1e6;

    QScrollBar* bar = view->verticalScrollBar();
    const int range = bar->maximum() - bar->minimum();

    std::vector<double> frameTimes;
    frameTimes.reserve(static_cast<std::size_t>(frames));

    for (int f = 0; f < frames; ++f) {
        const int pos =
            bar->minimum() + static_cast<int>(static_cast<double>(range) * f / (frames - 1));
        bar->setValue(pos);

        timer.restart();
        // repaint 而非 update：强制同步绘制，否则测到的是排队时间而不是绘制时间。
        view->viewport()->repaint();
        frameTimes.push_back(static_cast<double>(timer.nsecsElapsed()) / 1e6);
    }

    // 改变宽度会让每个单元格重新计算几何，这是没有视图回收的真实代价。
    // 滚动只重绘可见区域，宽度变化却触及全部 widget。
    std::vector<double> resizeTimes;
    QWidget* window = view->window();
    const QSize original = window->size();
    for (int i = 0; i < 10; ++i) {
        const int delta = (i % 2 == 0) ? -120 : 120;
        timer.restart();
        window->resize(original.width() + delta, original.height());
        view->widget()->ensurePolished();
        view->viewport()->repaint();
        resizeTimes.push_back(static_cast<double>(timer.nsecsElapsed()) / 1e6);
    }
    window->resize(original);

    std::sort(resizeTimes.begin(), resizeTimes.end());
    result.medianResizeMs = percentile(resizeTimes, 0.50);
    result.maxResizeMs = resizeTimes.empty() ? 0.0 : resizeTimes.back();

    result.rssBytes = currentRssBytes() - rssBefore;
    result.frameCount = static_cast<int>(frameTimes.size());

    std::sort(frameTimes.begin(), frameTimes.end());
    result.medianFrameMs = percentile(frameTimes, 0.50);
    result.p95FrameMs = percentile(frameTimes, 0.95);
    result.maxFrameMs = frameTimes.empty() ? 0.0 : frameTimes.back();

    return result;
}
