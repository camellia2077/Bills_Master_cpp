# 调用在根 CMakeLists.txt 中定义的函数来创建插件
# 参数1: 插件的目标名称
# 参数2: 插件自己的源文件
create_formatter_plugin("typ_year_formatter" "${CMAKE_CURRENT_SOURCE_DIR}/YearTypFormat.cpp")