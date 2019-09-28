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

# polarized_def.cppとstreaming_def.cppをコンパイルしてライブラリを作成
add_library(tool_kit polarized_def.cpp streaming_def.cpp)

# コンパイルオプションの指定
target_compile_options(tool_kit PUBLIC -Wall -g -Wno-unknown-pragmas -O2 -std=c++11 )

# c++11を指定(debianのcmakeだとcxx_std_14がtarget_compile_features()で使えない)
set(CMake_CXX_STANDARD 11)

# includeディレクトリの指定
target_include_directories(tool_kit PUBLIC
                ${OpenCV_INCLUDE_DIRS}
                ${ArenaSDK_ROOTDIR}/include/Arena
                ${ArenaSDK_ROOTDIR}/include/Save
                ${ArenaSDK_ROOTDIR}/include/GenTL
                ${ArenaSDK_ROOTDIR}/GenICam/library/CPP/include
                ${PROJECT_SOURCE_DIR}/include
                )

# ライブラリディレクトリの指定
target_link_libraries(tool_kit
                ${OpenCV_LIBS}
                ${ArenaSDK_LIBRALY}
                ${GENICAM_LIBRALY}
                ${FFMPEG_LIBRALY}
                )
