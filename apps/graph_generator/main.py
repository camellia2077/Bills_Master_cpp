# main.py

import sys
import config_handler
import database_handler
import plot_handler
import cli_handler

def main():
    """
    Main function to run the chart generation program.
    """
    try:
        # 1. 加载并验证配置
        app_config = config_handler.load_and_validate_config()

        # 2. 解析命令行参数
        args = cli_handler.parse_arguments(app_config.db_path)

        # 3. 动态决定输出文件名
        output_filename = args.out
        if output_filename is None:
            if args.type == 'year':
                output_filename = f"{args.period}_yearly_chart.png"
            elif args.type == 'month':
                output_filename = f"{args.period}_monthly_chart.png"
        
        # 4. 设置图表样式
        plot_handler.set_plot_style(app_config.font_list)
        
        # 5. 根据参数获取数据并绘图
        if args.type == 'year':
            try:
                year_val = int(args.period)
                if not (1900 <= year_val <= 2100):
                    raise ValueError("年份超出合理范围。")
                
                df = database_handler.fetch_yearly_data(args.db, year_val)
                
                if df is not None and not df.empty:
                    plot_handler.plot_yearly_chart(df, year_val, output_filename)
                elif df is not None:
                    print(f"警告: 未找到 {year_val} 年的任何消费数据。")

            except ValueError:
                print(f"错误: 'period' 参数的年份格式无效。请输入一个有效的年份，例如 2024。", file=sys.stderr)
        
        elif args.type == 'month':
            if len(args.period) == 6 and args.period.isdigit():
                df = database_handler.fetch_monthly_data(args.db, args.period)

                if df is not None and not df.empty:
                    plot_handler.plot_monthly_chart(df, args.period, output_filename)
                elif df is not None:
                    print(f"警告: 未找到 {args.period} 月的任何消费数据。")
            else:
                print("错误: 'period' 参数的月份格式无效。请使用 YYYYMM 格式 (例如: 202407)。", file=sys.stderr)

    except ValueError as e:
        print(e, file=sys.stderr)
    except Exception as e:
        print(f"程序发生意外错误: {e}", file=sys.stderr)

if __name__ == '__main__':
    main()