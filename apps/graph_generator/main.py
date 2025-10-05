# main.py

# --- [核心修改] ---
# 移除 sys.path 的修改，因为 Python 会自动将 main.py 所在的目录加入搜索路径
import sys
# import os
# sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))

# 导入语句保持不变，因为 chart_generator 包与 main.py 在同一目录下
from chart_generator.config import handler as config_handler
from chart_generator.data import database as database_handler
from chart_generator.visualization import plot as plot_handler
from chart_generator.cli import handler as cli_handler

def main():
    """
    Main function to run the chart generation program.
    """
    try:
        # 1. 加载并验证配置
        app_config = config_handler.load_and_validate_config()

        # 2. 解析命令行参数
        args = cli_handler.parse_arguments(app_config.db_path)

        # 3. 动态决定输出文件名或前缀
        output_target = args.out
        if output_target is None:
            if args.type == 'year':
                output_target = f"{args.period}_yearly_chart.png"
            elif args.type == 'month':
                output_target = f"{args.period}_monthly_chart.png"
            elif args.type == 'month-details':
                output_target = f"{args.period}_details"
        
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
                    plot_handler.plot_yearly_chart(df, year_val, output_target)
                elif df is not None:
                    print(f"警告: 未找到 {year_val} 年的任何消费数据。")

            except ValueError:
                print(f"错误: 'period' 参数的年份格式无效。请输入一个有效的年份，例如 2024。", file=sys.stderr)
        
        elif args.type == 'month':
            if len(args.period) == 6 and args.period.isdigit():
                df = database_handler.fetch_monthly_data(args.db, args.period)

                if df is not None and not df.empty:
                    plot_handler.plot_monthly_chart(df, args.period, output_target)
                elif df is not None:
                    print(f"警告: 未找到 {args.period} 月的任何消费数据。")
            else:
                print("错误: 'period' 参数的月份格式无效。请使用 YYYYMM 格式 (例如: 202407)。", file=sys.stderr)
        
        elif args.type == 'month-details':
            if len(args.period) == 6 and args.period.isdigit():
                df = database_handler.fetch_monthly_details_data(args.db, args.period)

                if df is not None and not df.empty:
                    plot_handler.plot_monthly_details_by_category(df, args.period, output_target)
                elif df is not None:
                    print(f"警告: 未找到 {args.period} 月的任何消费明细数据。")
            else:
                print("错误: 'period' 参数的月份格式无效。请使用 YYYYMM 格式 (例如: 202407)。", file=sys.stderr)

    except ValueError as e:
        print(e, file=sys.stderr)
    except Exception as e:
        print(f"程序发生意外错误: {e}", file=sys.stderr)

if __name__ == '__main__':
    main()