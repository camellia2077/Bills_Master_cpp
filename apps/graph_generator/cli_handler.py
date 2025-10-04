# cli_handler.py

import argparse

def parse_arguments(default_db_path: str):
    """
    Configures and parses command-line arguments.
    
    Args:
        default_db_path (str): The default database path from the config.
        
    Returns:
        argparse.Namespace: An object containing the parsed arguments.
    """
    parser = argparse.ArgumentParser(
        description="从账单数据库生成消费图表。",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog="""
范例:
  1. 查询 2025 年的年度数据:
     python main.py year 2025

  2. 查询 2025 年 9 月的数据并指定输出文件名:
     python main.py month 202509 --out 2025_september.png
"""
    )
    parser.add_argument(
        'type', 
        choices=['year', 'month'], 
        help="需要生成的图表类型。"
    )
    parser.add_argument(
        'period', 
        type=str,
        help="图表的具体时间周期。\n"
             "对于 'year' 类型, 请提供年份 (例如: 2024)。\n"
             "对于 'month' 类型, 请提供 YYYYMM 格式的月份 (例如: 202407)。"
    )
    parser.add_argument(
        '--db', 
        type=str, 
        default=default_db_path,
        help=f"SQLite 数据库文件的路径 (默认: {default_db_path})。"
    )
    parser.add_argument(
        '--out',
        type=str,
        default=None,
        help="输出图表的图片文件名 (默认: 自动生成, 例如 2025_yearly_chart.png)。"
    )
    
    return parser.parse_args()