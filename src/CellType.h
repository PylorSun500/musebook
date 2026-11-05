#pragma once

/// 整个应用只有这三种单元格类型。
/// 导入的乐谱扫描件或乐谱图片属于 Media，而不是 Score。
enum class CellType {
    Text,
    Score,
    Media,
};
