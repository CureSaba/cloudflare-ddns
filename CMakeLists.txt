cmake_minimum_required(VERSION 4.2)
project(ddns)

set(CMAKE_CXX_STANDARD 26)
set(OPENSSL_USE_STATIC_LIBS OFF)

# --- cpr ライブラリの自動ダウンロードと組み込み ---
include(FetchContent)
FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG        1.14.2
)
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.12.0
)

if(MSVC)
    # Windows (MSVC): サイズ優先最適化(/O1), 一意の関数呼び出しの折りたたみ(/OPT:REF, /OPT:ICF)
    set(CMAKE_CXX_FLAGS_RELEASE "/O1 /GL /DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/EXE /LTCG /OPT:REF /OPT:ICF")
else()
    # Linux / macOS (GCC, Clang): サイズ最優先最適化(-Os), デッドコード削除, 未使用シンボル削除
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -ffunction-sections -fdata-sections -DNDEBUG")
    if(APPLE)
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-dead_strip")
    else()
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -s") #「-s」で不要なデバッグシンボルを完全にストリップ
    endif()
endif()

# HTTPSを有効化し、不要なテストのビルドを無効化
set(CPR_ENABLE_CURL_HTTP_ONLY ON CACHE INTERNAL "")
set(CPR_ENABLE_SSL ON CACHE INTERNAL "")
set(CPR_BUILD_TESTS OFF CACHE INTERNAL "")
set(CPR_CURL_USE_LIBPSL OFF CACHE INTERNAL "")
set(CPR_GENERATE_COVERAGE OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(cpr)
FetchContent_MakeAvailable(json)
# ---------------------------------------------

add_executable(ddns main.cpp)
target_link_libraries(ddns PRIVATE cpr::cpr)
target_link_libraries(ddns PRIVATE nlohmann_json::nlohmann_json)
