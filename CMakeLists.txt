# CMakeのバージョンを設定
cmake_minimum_required(VERSION 3.5)
# プロジェクト名と使用する言語を設定
project(test_cmake CXX)

# 外部ライブラリの検索
find_package(OpenCV REQUIRED)
# ArenaSDKのインストールディレクトリ用の変数
set(ArenaSDK_ROOTDIR /home/tenten/ArenaSDK_v0.1.26_Linux_x64/ArenaSDK_Linux_x64)

# オプション用の変数
file(GLOB ArenaSDK_LIBRALY ${ArenaSDK_ROOTDIR}/lib64/*.so)
file(GLOB GENICAM_LIBRALY ${ArenaSDK_ROOTDIR}/GenICam/library/lib/Linux64_x64/*.so)
file(GLOB FFMPEG_LIBRALY ${ArenaSDK_ROOTDIR}/ffmpeg/*.so)

set(GENICAMLIBS -lGCBase_gcc421_v3_0
                -lGenApi_gcc421_v3_0
                -lLog_gcc421_v3_0
                -llog4cpp_gcc421_v3_0
                -lMathParser_gcc421_v3_0
                -lNodeMapData_gcc421_v3_0
                -lXmlParser_gcc421_v3_0)

set(FFMPEGLIBS -lavcodec
               -lavdevice
               -lavfilter
               -lavformat
               -lavutil
               -lswresample
               -lswscale
               )

# tool_kitという実行ファイルをmain.cppとtest.cppから作成
add_executable(tool_kit tool_kit.cpp)
# コンパイルオプションの指定
target_compile_options(tool_kit PUBLIC -Wall -g -Wno-unknown-pragmas -O2 -std=c++11 )

# c++14を指定(debianのcmakeだとcxx_std_14がtarget_compile_features()で使えない)
set(CMake_CXX_STANDARD 11)
# includeディレクトリの指定
target_include_directories(tool_kit PUBLIC
                ${OpenCV_INCLUDE_DIRS}
                ${ArenaSDK_ROOTDIR}/include/Arena
                ${ArenaSDK_ROOTDIR}/include/Save
                ${ArenaSDK_ROOTDIR}/include/GenTL
                ${ArenaSDK_ROOTDIR}/GenICam/library/CPP/include
                )
# ライブラリディレクトリの指定
target_link_libraries(tool_kit
                ${OpenCV_LIBS}
                ${ArenaSDK_LIBRALY}
                ${GENICAM_LIBRALY}
                ${FFMPEG_LIBRALY}
                )
