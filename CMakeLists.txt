cmake_minimum_required(VERSION 3.20)
project(APPSTSGenerate VERSION 1.0.0 LANGUAGES C)

include(GNUInstallDirs)

option(APPSTS_STATS "Collect generation statistics" ON)
option(APPSTS_STATIC_LEX "Precompute triple lookup tables" ON)
option(APPSTS_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

add_executable(antipasch_psts
    antipasch_psts.c backtrack.c canonicity.c labeled_count.c lexicographic.c matrix.c mitres.c stats.c)
set_target_properties(antipasch_psts PROPERTIES
    C_STANDARD 17 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
target_compile_definitions(antipasch_psts PRIVATE
    $<$<BOOL:${APPSTS_STATS}>:STATS>
    $<$<BOOL:${APPSTS_STATIC_LEX}>:STATIC_LEX>
    $<$<CONFIG:Debug>:DEBUG;ASSERTIONS>)
if(MSVC)
    target_compile_options(antipasch_psts PRIVATE /W4)
else()
    target_compile_options(antipasch_psts PRIVATE -Wall -Wextra -Wpedantic -Wstrict-prototypes)
endif()
if(APPSTS_SANITIZERS)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(antipasch_psts PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(antipasch_psts PRIVATE -fsanitize=address,undefined)
    else()
        message(FATAL_ERROR "APPSTS_SANITIZERS requires Clang or GCC")
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_FOUND)
        add_test(NAME generator_regression
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_generator.py"
                    "$<TARGET_FILE:antipasch_psts>")
    else()
        message(STATUS "Python3 unavailable: CLI regression test disabled")
    endif()
    add_executable(test_helpers tests/test_helpers.c labeled_count.c lexicographic.c matrix.c mitres.c)
    target_include_directories(test_helpers PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(test_helpers PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
    target_compile_definitions(test_helpers PRIVATE $<$<BOOL:${APPSTS_STATIC_LEX}>:STATIC_LEX>)
    if(APPSTS_SANITIZERS)
        target_compile_options(test_helpers PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(test_helpers PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME helpers COMMAND test_helpers)
endif()
install(TARGETS antipasch_psts RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME}")
