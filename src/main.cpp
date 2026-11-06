#include "MainWindow.h"
#include "NotebookView.h"
#include "ScrollBenchmark.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QTranslator>

#include <cstdio>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("Musebook"));
    QApplication::setApplicationVersion(QStringLiteral(MUSEBOOK_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Musebook"));

    // macOS 的程序菜单里有几项（关于、服务、隐藏、退出）由 Qt 自己生成，文字取自
    // Qt 的翻译文件而不是本项目的代码。不装载它，那几项就会在一片中文菜单里显示英文。
    // 界面语言固定为简体中文，因此这里不跟随系统区域设置。必须在建立菜单栏之前装载。
    auto* qtTranslator = new QTranslator(&app);
    if (qtTranslator->load(QStringLiteral("qtbase_zh_CN"),
                           QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(qtTranslator);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Musebook"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption cellsOption(QStringLiteral("cells"),
                                   QStringLiteral("启动时填充的占位单元格数量。"),
                                   QStringLiteral("n"), QStringLiteral("0"));
    QCommandLineOption benchmarkOption(
        QStringLiteral("benchmark"),
        QStringLiteral("运行滚动性能测量并把结果打印到标准输出，然后退出。"));
    QCommandLineOption wrapOption(
        QStringLiteral("wrap"),
        QStringLiteral("Text 单元格使用自动换行与内容决定的高度，更接近真实文本单元格。"));
    parser.addOption(cellsOption);
    parser.addOption(benchmarkOption);
    parser.addOption(wrapOption);
    parser.process(app);

    const int cells = parser.value(cellsOption).toInt();

    MainWindow window;
    window.notebookView()->setWrappingCells(parser.isSet(wrapOption));
    window.show();

    if (parser.isSet(benchmarkOption)) {
        const int benchCells = cells > 0 ? cells : 500;
        // 窗口必须先完成一次显示，否则 viewport 尺寸为零，重绘测不出东西。
        QApplication::processEvents();

        const ScrollBenchmarkResult result =
            runScrollBenchmark(window.notebookView(), benchCells, 60);
        std::fputs(qPrintable(result.toText()), stdout);
        std::fputc('\n', stdout);
        return 0;
    }

    window.populatePlaceholders(cells);

    return QApplication::exec();
}
