## v0.2.7.0 - 2025-10-02
根据使用后产生的问题,优化程序逻辑
### v0.2.7.1
1. reprocessing的validator现在验证备注行，正则从REMARK:修改为remark
2. 从plugins文件夹中读取格式化输出 DLL
3. 测试程序输出的文件夹命名修改为py_output

### v0.2.7.2
1. 命令行实现逻辑移动到单独的文件夹
2. 优化测试py脚本
### v0.2.7.3
1. 创建json配置检验模块

### v0.2.7.5
1. command_handler模块修改为命令模式 (Command Pattern)

### v0.2.7.6
1. 优化app_controller模块程序结构

### v0.2.7.7 
1. 命令行新增--full-workflow
2. query文件夹重命名为reports
3. Bills_Master重命名为bills_master并且放入apps文件夹
4. log_generator移动到apps文件夹


## v0.2.8.0 - 2025-10-06
1. 使用+代表收入，-代表支出
2. 对金额自动求和,不加符号默认为正数
3. json新增total_expense、total_income 和 balance字段
4. 数据库插入total_expense、total_income 和 balance
5. reports新增total_expense、total_income

## v0.2.8.1 - 2025-10-07
1. 对txt的格式进行基本验证，再将 txt 转换为 json，再从 json 中对内容进行检验
2. 将检验错误输出从 std::cerr 改为 std::cout
