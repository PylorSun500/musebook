# Musebook

面向音乐理论学习者的跨平台桌面笔记应用。在同一份笔记中编排文本、原创乐谱和多媒体材料。

目标平台：macOS、Windows。开发计划见 [INSTRUCTIONS.md](INSTRUCTIONS.md)。

当前进度：阶段 0，工程骨架。

## 环境要求

- Qt 6.12.0（Widgets）
- CMake 3.28+
- Ninja
- 支持 C++20 的编译器（macOS: Apple clang；Windows: MSVC 2022）

## 在 macOS 上构建

Qt 安装器自带的 CMake 和 Ninja 不在 PATH 上，需要先加进去：

```sh
export PATH="$HOME/Qt/Tools/CMake/CMake.app/Contents/bin:$HOME/Qt/Tools/Ninja:$PATH"
```

然后：

```sh
cmake --preset macos-debug
cmake --build --preset macos-debug
open build/macos-debug/src/musebook.app
```

Release 构建把 `macos-debug` 换成 `macos-release`（配置为 RelWithDebInfo）。

预设中的 Qt 路径写死为 `$HOME/Qt/6.12.0/macos`。升级 Qt 后改 `CMakePresets.json` 里的这一行。

## 在 Windows 上构建

在 x64 Native Tools Command Prompt for VS 2022 中，先让 `QT_ROOT_DIR` 指向 Qt 安装目录：

```cmd
set QT_ROOT_DIR=C:\Qt\6.12.0\msvc2022_64
cmake --preset windows-debug
cmake --build --preset windows-debug
```

## IDE

构建配置以 CMake 为准，不依赖特定 IDE。Qt Creator 和 CLion 都能直接识别 `CMakePresets.json`。

在 Qt Creator 中打开本目录下的 `CMakeLists.txt` 即可。

## 代码格式

格式规范在 `.clang-format`。macOS 上的 clang-format 来自 Xcode 工具链，不在 PATH 上：

```sh
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang-format -i src/*.cpp src/*.h
```

Qt Creator 会自动读取 `.clang-format`。

## 已知待办

- **CI 上 Windows 侧的 Qt 版本低于本地。** 本地与 CI 的 macOS 都用 Qt 6.12.0，但 CI 的 Windows job 只能用 6.10.3。Qt 从 6.11.0 起改变了 Windows 仓库的目录布局，`Updates.xml` 从 `qt6_6120/qt6_6120/` 移到了 `qt6_6120/qt6_6120_msvc2022_64/`，而 `aqtinstall` 3.3.0（当前最新版）仍按旧布局查找，安装会失败。6.10.3 是旧布局下的最后一个版本。等 aqtinstall 支持新布局后应改回与本地一致。

  实际影响：Windows 侧的编译检查基于 Qt 6.10 的 API，用到 6.11 或 6.12 新增接口时 CI 不会报错，但 Windows 本地构建会。阶段 7 做真正的 Windows 发布时需要在本机用 6.12 验证。

  Windows job 已验证通过（Qt 6.10.3 + MSVC 2022 + Ninja）。

- **CI 不是 macOS 的验收手段。** macOS runner 排队时间过长。macOS 侧以本机构建为准，CI 的 macOS job 只覆盖一件本地做不到的事：从干净 checkout 构建，能发现漏提交的源文件。它失败时才需要关注。

## 许可证

[MIT](LICENSE)。

依赖的第三方组件：Qt 6（LGPLv3 / GPLv2）。阶段 1.5 会在 Lomse（MIT）与 Verovio（LGPLv3）之间选定乐谱库。
