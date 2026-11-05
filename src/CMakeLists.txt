qt_add_executable(musebook
    main.cpp
    CellType.h
    MainWindow.h
    MainWindow.cpp
    NotebookView.h
    NotebookView.cpp
    PlaceholderCellWidget.h
    PlaceholderCellWidget.cpp
    ScrollBenchmark.h
    ScrollBenchmark.cpp
)

target_compile_definitions(musebook
    PRIVATE
        MUSEBOOK_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(musebook
    PRIVATE
        Qt6::Widgets
        musebook_warnings
)

set_target_properties(musebook PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Musebook"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.musebook.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)
