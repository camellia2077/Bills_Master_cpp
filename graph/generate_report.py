import sqlite3
import sys
import os
import json
import matplotlib.pyplot as plt

def load_config():
    """
    加载 JSON 配置文件。如果文件不存在或无效，则返回默认配置。
    """
    default_config = {
        "font_sizes": {
            "title": 16,
            "axis_label": 12,
            "tick_label": 10,
            "bar_label": 9
        }
    }
    try:
        with open("generate_report.json", 'r', encoding='utf-8') as f:
            config = json.load(f)
            # 简单验证，确保关键的键存在
            if "font_sizes" not in config:
                print("警告: 'generate_report.json' 中缺少 'font_sizes' 配置。将使用默认字体大小。")
                return default_config
            return config
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"警告: 'generate_report.json' 未找到或格式无效 ({e})。将使用默认字体大小。")
        return default_config

def setup_matplotlib_for_chinese():
    """
    配置 Matplotlib 以正确显示中文字符。
    """
    try:
        plt.rcParams['font.sans-serif'] = ['SimHei']
        plt.rcParams['axes.unicode_minus'] = False
    except Exception:
        try:
            plt.rcParams['font.sans-serif'] = ['Microsoft YaHei']
            plt.rcParams['axes.unicode_minus'] = False
        except Exception:
            print("警告: 未找到指定的中文字体 'SimHei' 或 'Microsoft YaHei'。图表中的中文可能无法正确显示。")

def query_parent_expenses(date_filter: str):
    """
    连接到数据库并根据日期筛选查询父项目的总支出。
    """
    DB_FILE = "bills.db"
    if not os.path.exists(DB_FILE):
        print(f"错误: 数据库文件 '{DB_FILE}' 不存在。")
        return None

    if len(date_filter) == 4:
        query_pattern = date_filter + '%'
        chart_title = f"{date_filter}年 各大类支出汇总"
    elif len(date_filter) == 6:
        query_pattern = date_filter
        year, month = date_filter[:4], date_filter[4:]
        chart_title = f"{year}年{month}月 各大类支出汇总"
    else:
        print(f"错误: 无效的日期格式 '{date_filter}'。")
        return []

    conn = None
    try:
        conn = sqlite3.connect(DB_FILE)
        cursor = conn.cursor()
        sql_query = """
            SELECT p.title, SUM(i.amount) as total
            FROM Item AS i
            JOIN Child AS c ON i.child_id = c.id
            JOIN Parent AS p ON c.parent_id = p.id
            JOIN YearMonth AS ym ON p.year_month_id = ym.id
            WHERE ym.value LIKE ?
            GROUP BY p.title
            ORDER BY total DESC;
        """
        cursor.execute(sql_query, (query_pattern,))
        return cursor.fetchall(), chart_title
    except sqlite3.Error as e:
        print(f"数据库查询时发生错误: {e}")
        return [], ""
    finally:
        if conn:
            conn.close()

def create_bar_chart(data, title, output_filename, precision=1, font_sizes=None):
    """
    根据查询到的数据生成并保存柱状图。
    """
    if not data:
        print("没有找到符合条件的数据，无法生成图表。")
        return

    parent_categories = [row[0] for row in data]
    amounts = [row[1] for row in data]
    total_amount = sum(amounts)

    github_colors = ["#ebedf0", "#9be9a8", "#40c463", "#30a14e", "#216e39"]
    bar_colors = []
    min_amount = amounts[-1]
    max_amount = amounts[0]

    if max_amount == min_amount:
        bar_colors = [github_colors[-1]] * len(amounts)
    else:
        for amount in amounts:
            normalized_value = (amount - min_amount) / (max_amount - min_amount)
            color_index = int(round(normalized_value * (len(github_colors) - 1)))
            bar_colors.append(github_colors[color_index])

    setup_matplotlib_for_chinese()

    fig, ax = plt.subplots(figsize=(12, 8))
    bars = ax.bar(parent_categories, amounts, color=bar_colors)

    bar_labels = []
    for amount in amounts:
        percentage = (amount / total_amount) * 100
        label = f"{amount:,.2f}\n({percentage:.{precision}f}%)"
        bar_labels.append(label)

    # --- 修改：使用从配置中读取的字体大小 ---
    ax.bar_label(bars, labels=bar_labels, padding=3, fontsize=font_sizes['bar_label'])
    ax.set_ylabel('总金额', fontsize=font_sizes['axis_label'])
    chart_title_with_total = f"{title} (总支出: {total_amount:,.2f})"
    ax.set_title(chart_title_with_total, fontsize=font_sizes['title'], pad=20)
    plt.xticks(ticks=range(len(parent_categories)), labels=parent_categories, rotation=0, fontsize=font_sizes['tick_label'])
    
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    plt.tight_layout()

    try:
        plt.savefig(output_filename)
        print(f"图表已成功保存为: {os.path.abspath(output_filename)}")
    except Exception as e:
        print(f"保存图表时出错: {e}")

def main():
    """
    主函数，处理命令行输入并协调程序流程。
    """
    if len(sys.argv) != 2:
        print("用法: python generate_report.py <YYYY|YYYYMM>")
        return

    date_filter = sys.argv[1]
    if not date_filter.isdigit() or len(date_filter) not in [4, 6]:
        print(f"错误: 无效的日期格式 '{date_filter}'。")
        return

    # --- 修改：加载配置 ---
    config = load_config()
    font_sizes = config.get("font_sizes")

    result = query_parent_expenses(date_filter)
    if result is None:
        return
    
    data, chart_title = result

    percentage_precision = 2
    output_filename = f"barchart_{date_filter}.png"
    
    # --- 修改：将字体配置传递给绘图函数 ---
    create_bar_chart(data, chart_title, output_filename, percentage_precision, font_sizes)

if __name__ == '__main__':
    main()