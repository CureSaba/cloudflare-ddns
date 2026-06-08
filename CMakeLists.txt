cmake_minimum_required(VERSION 4.2)
project(ddns)

set(CMAKE_CXX_STANDARD 26)

# --- cpr ライブラリの自動ダウンロードと組み込み ---
include(FetchContent)
FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG        1.14.2 # 安定したバージョンを指定
)
# HTTPSを有効化し、不要なテストのビルドを無効化
set(OPENSSL_USE_STATIC_LIBS ON CACHE INTERNAL "")
set(CPR_ENABLE_SSL ON CACHE INTERNAL "")
set(OPENSSL_USE_STATIC_LIBS ON CACHE INTERNAL "")
set(CPR_CURL_USE_LIBPSL OFF CACHE INTERNAL "")
set(BUILD_TESTING OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(cpr)
# ---------------------------------------------

add_executable(ddns main.cpp)
target_link_libraries(ddns PRIVATE cpr::cpr)
