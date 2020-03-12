# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.0)

# 项目名称
project(MyWebServer)
    #设置编译器（可选的有gcc，g++）
SET(CMAKE_CXX_COMPILER "g++")     

# 设置用debug还是release模式。debug允许断点，而release更快
set( CMAKE_BUILD_TYPE Debug )
#set( CMAKE_BUILD_TYPE Release )

#设置支持gdb调试
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")


# 设置编译选项
# 允许c++11标准、O3优化、多线程。match选项可避免一些cpu上的问题
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3 -pthread" )

#设定可执行二进制文件的目录
SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) 

#设定存放编译出来的库文件的目录
SET( LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib) 

#并且把该目录设为连接目录
LINK_DIRECTORIES( ${PROJECT_SOURCE_DIR}/lib)
#搜索并加载外部工程 REAQUIRED表示该外部工程是必须的 如果没有找到将会停止构建
find_package (Threads REQUIRED)


#设定头文件目录
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/include)

# 设置头代码文件的路径
FILE(GLOB INCLUDE_FILE "./include/*.cpp" "./include/*.h")

#设置源文件代码路径
FILE(GLOB SC_FILES "./src/*.cpp" "./src/*.h")

# 指定生成目标
add_executable(${PROJECT_NAME} ${SC_FILES} ${INCLUDE_FILE})