#include "PlaceholderCellWidget.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPalette>

namespace {

/// 单元格内部边距。高度计算与实际渲染必须用同一组数值，
/// 否则虚拟化分配的槽位会与 widget 的真实高度错开。
constexpr int horizontalPadding = 10;
constexpr int verticalPadding = 8;

/// 当前单元格的左侧标记宽度。画在内边距里，不占文字宽度，因此不影响高度计算。
constexpr int currentMarkerWidth = 3;

/// 标签的对齐与换行标志。测量与渲染用同一组标志，理由同上。
constexpr Qt::Alignment labelAlignment = Qt::AlignLeft | Qt::AlignTop;

/// StyledPanel 的边框宽度由样式决定，不是常量，而布局确实为它留了空间。
/// 用一次性的原型问出来，比写死一个数字更稳——差一像素，每个单元格都会错开。
int frameThickness() {
    static const int thickness = [] {
        const PlaceholderCellWidget probe;
        return probe.frameWidth();
    }();
    return thickness;
}

QString typeName(CellType type) {
    switch (type) {
    case CellType::Text:
        return QStringLiteral("Text");
    case CellType::Score:
        return QStringLiteral("Score");
    case CellType::Media:
        return QStringLiteral("Media");
    }
    return QStringLiteral("Unknown");
}

/// 三种类型给不同高度，让滚动测试面对不均匀的行高，
/// 这比等高单元格更接近真实文档。
int preferredHeight(CellType type) {
    switch (type) {
    case CellType::Text:
        return 72;
    case CellType::Score:
        return 120;
    case CellType::Media:
        return 160;
    }
    return 72;
}

/// 长度不等的段落，让换行计算面对不同的文本量。
QString sampleParagraph(int index) {
    static const QString base =
        QStringLiteral("大调音阶的全音与半音排列是三四级之间与七八级之间为半音，其余为全音。"
                       "这一结构决定了调式的色彩，也是理解调性和声的起点。");
    const int repeat = 1 + (index % 4);
    QString text = QStringLiteral("%1  ·  Text\n").arg(index);
    for (int i = 0; i < repeat; ++i) {
        text += base;
    }
    return text;
}

/// 换行高度由内容决定，固定高度不由内容决定。两处都要判断，因此集中在这里。
bool wrapsContent(CellType type, bool wrapping) {
    return wrapping && type == CellType::Text;
}

} // namespace

QString placeholderCellText(CellType type, int serial, bool wrapping) {
    if (wrapsContent(type, wrapping)) {
        return sampleParagraph(serial);
    }
    return QStringLiteral("%1  ·  %2").arg(serial).arg(typeName(type));
}

int placeholderCellHeight(const QFontMetrics& metrics, CellType type, const QString& text,
                          int cellWidth, bool wrapping) {
    if (!wrapsContent(type, wrapping)) {
        return preferredHeight(type);
    }

    const int available = cellWidth - 2 * (horizontalPadding + frameThickness());
    if (available <= 0) {
        return preferredHeight(type);
    }

    const QRect bounds =
        metrics.boundingRect(QRect(0, 0, available, 0), Qt::TextWordWrap | labelAlignment, text);
    return bounds.height() + 2 * (verticalPadding + frameThickness());
}

PlaceholderCellWidget::PlaceholderCellWidget(QWidget* parent)
    : QFrame(parent), m_label(new QLabel(this)) {
    setFrameShape(QFrame::StyledPanel);

    // 占位单元格没有任何自己的交互，鼠标事件一律上传到 viewport，
    // 由视图根据落点决定当前单元格。widget 被复用，它不知道自己是第几个，
    // 让视图统一处理比给每个 widget 维护位置更简单。
    // 阶段 2 的单元格具备编辑能力后，这里需要改为各自处理。
    setAttribute(Qt::WA_TransparentForMouseEvents);
    m_label->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_label->setAlignment(labelAlignment);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(horizontalPadding, verticalPadding, horizontalPadding,
                               verticalPadding);
    layout->addWidget(m_label);
}

void PlaceholderCellWidget::setCell(CellType type, const QString& text, bool wrapping) {
    m_type = type;
    m_label->setWordWrap(wrapsContent(type, wrapping));
    m_label->setText(text);
}

QString PlaceholderCellWidget::text() const {
    return m_label->text();
}

void PlaceholderCellWidget::setCurrent(bool current) {
    if (m_current == current) {
        return;
    }
    m_current = current;
    update();
}

void PlaceholderCellWidget::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);
    if (!m_current) {
        return;
    }
    // 当前单元格画一条左侧色条。取自调色板而不是写死颜色，
    // 这样深色与浅色外观下都成立。
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, currentMarkerWidth, height()),
                     palette().color(QPalette::Highlight));
}
