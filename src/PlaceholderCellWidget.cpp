#include "PlaceholderCellWidget.h"

#include <QHBoxLayout>
#include <QLabel>

namespace {

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

} // namespace

PlaceholderCellWidget::PlaceholderCellWidget(CellType type, int index, bool wrapping,
                                             QWidget* parent)
    : QFrame(parent), m_type(type), m_label(new QLabel(this)) {
    setFrameShape(QFrame::StyledPanel);

    const bool wrapThis = wrapping && type == CellType::Text;
    if (wrapThis) {
        // 高度交给内容决定，布局必须为每个单元格做 heightForWidth 计算。
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        m_label->setWordWrap(true);
        m_label->setText(sampleParagraph(index));
    } else {
        setFixedHeight(preferredHeight(type));
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        m_label->setText(QStringLiteral("%1  ·  %2").arg(index).arg(typeName(type)));
    }
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->addWidget(m_label);
}
