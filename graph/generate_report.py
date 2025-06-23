import sqlite3
import sys
import os
import json
import matplotlib.pyplot as plt

# --- 实用工具函数 ---

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
            if "font_sizes" not in config:
                print("警告: 'generate_report.json' 中缺少 'font_sizes' 配置。将使用默认字体大小。")
                return default_config
            return config
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"警告: 'generate_report.json' 未找到或格式无效 ({e})。将使用默认字体大小。")
        return default_config

# --- 逻辑类 ---

class DatabaseManager:
    """
    一个专门用于处理数据库交互的类。
    """
    _SQL_QUERY = """
        SELECT p.title, SUM(i.amount) as total
        FROM Item AS i
        JOIN Child AS c ON i.child_id = c.id
        JOIN Parent AS p ON c.parent_id = p.id
        JOIN YearMonth AS ym ON p.year_month_id = ym.id
        WHERE ym.value LIKE ?
        GROUP BY p.title
        ORDER BY total DESC;
    """

    def __init__(self, db_path: str):
        if not os.path.exists(db_path):
            raise FileNotFoundError(f"错误: 数据库文件 '{db_path}' 不存在。")
        self.db_path = db_path
        self.conn = None

    def __enter__(self):
        """上下文管理器进入方法，建立数据库连接。"""
        try:
            self.conn = sqlite3.connect(self.db_path)
            return self
        except sqlite3.Error as e:
            print(f"数据库连接失败: {e}")
            raise

    def __exit__(self, exc_type, exc_val, exc_tb):
        """上下文管理器退出方法，关闭数据库连接。"""
        if self.conn:
            self.conn.close()

    def get_parent_expenses(self, date_filter: str):
        """
        根据日期筛选查询父项目的总支出。
        :param date_filter: 日期筛选字符串 (YYYY 或 YYYYMM)
        :return: 查询结果的列表，每个元素是一个元组 (父分类, 总金额)。
        """
        if not self.conn:
            print("错误: 数据库未连接。")
            return []
        
        query_pattern = date_filter
        if len(date_filter) == 4:
            query_pattern += '%' # 年度查询
        
        try:
            cursor = self.conn.cursor()
            cursor.execute(self._SQL_QUERY, (query_pattern,))
            return cursor.fetchall()
        except sqlite3.Error as e:
            print(f"数据库查询时发生错误: {e}")
            return []

class ReportGenerator:
    """
    一个专门用于生成图表报告的类。
    """
    def __init__(self, config: dict):
        self.font_sizes = config.get("font_sizes")
        self._setup_matplotlib_for_chinese()

    def _setup_matplotlib_for_chinese(self):
        """配置 Matplotlib 以支持中文显示。"""
        try:
            plt.rcParams['font.sans-serif'] = ['SimHei']
            plt.rcParams['axes.unicode_minus'] = False
        except Exception:
            try:
                plt.rcParams['font.sans-serif'] = ['Microsoft YaHei']
                plt.rcParams['axes.unicode_minus'] = False
            except Exception:
                print("警告: 未找到指定的中文字体 'SimHei' 或 'Microsoft YaHei'。图表中的中文可能无法正确显示。")

    def _generate_chart_title(self, date_filter: str, total_amount: float):
        """根据日期和总金额生成图表主标题。"""
        if len(date_filter) == 4:
            title = f"{date_filter}年 各大类支出汇总"
        else:
            year, month = date_filter[:4], date_filter[4:]
            title = f"{year}年{month}月 各大类支出汇总"
        return f"{title} (总支出: {total_amount:,.2f})"
        
    def _get_bar_colors(self, amounts: list):
        """根据金额计算条形图的颜色。"""
        github_colors = ["#ebedf0", "#9be9a8", "#40c463", "#30a14e", "#216e39"]
        if not amounts:
            return []
            
        min_amount, max_amount = min(amounts), max(amounts)
        if max_amount == min_amount:
            return [github_colors[-1]] * len(amounts)

        bar_colors = []
        for amount in amounts:
            normalized_value = (amount - min_amount) / (max_amount - min_amount)
            color_index = int(round(normalized_value * (len(github_colors) - 1)))
            bar_colors.append(github_colors[color_index])
        return bar_colors

    def create_bar_chart(self, data: list, date_filter: str, output_filename: str, precision: int = 2):
        """
        根据数据生成并保存柱状图。
        """
        if not data:
            print("没有找到符合条件的数据，无法生成图表。")
            return

        parent_categories = [row[0] for row in data]
        amounts = [row[1] for row in data]
        total_amount = sum(amounts)

        fig, ax = plt.subplots(figsize=(12, 8))
        
        # 准备颜色和标签
        bar_colors = self._get_bar_colors(amounts)
        bar_labels = [f"{amount:,.2f}\n({(amount / total_amount) * 100:.{precision}f}%)" for amount in amounts]
        
        # 绘制图表
        bars = ax.bar(parent_categories, amounts, color=bar_colors)

        # 设置字体和标签
        ax.bar_label(bars, labels=bar_labels, padding=3, fontsize=self.font_sizes['bar_label'])
        ax.set_ylabel('总金额', fontsize=self.font_sizes['axis_label'])
        ax.set_title(self._generate_chart_title(date_filter, total_amount), fontsize=self.font_sizes['title'], pad=20)
        plt.xticks(ticks=range(len(parent_categories)), labels=parent_categories, rotation=0, fontsize=self.font_sizes['tick_label'])
        
        # 美化图表
        ax.spines['top'].set_visible(False)
        ax.spines['right'].set_visible(False)
        plt.tight_layout()

        # 保存图表
        try:
            plt.savefig(output_filename)
            print(f"图表已成功保存为: {os.path.abspath(output_filename)}")
        except Exception as e:
            print(f"保存图表时出错: {e}")
        finally:
            plt.close(fig) # 释放内存


def main():
    """
    主函数，处理命令行输入并协调程序流程。
    """
    if len(sys.argv) != 2:
        print("用法: python generate_report_refactored.py <YYYY|YYYYMM>")
        return

    date_filter = sys.argv[1]
    if not date_filter.isdigit() or len(date_filter) not in [4, 6]:
        print(f"错误: 无效的日期格式 '{date_filter}'。")
        return

    DB_FILE = "bills.db"
    output_filename = f"barchart_{date_filter}.png"
    config = load_config()

    try:
        # 使用 with 语句自动管理数据库连接
        with DatabaseManager(DB_FILE) as db:
            data = db.get_parent_expenses(date_filter)
        
        if data:
            reporter = ReportGenerator(config)
            reporter.create_bar_chart(data, date_filter, output_filename)

    except FileNotFoundError as e:
        print(e)
    except Exception as e:
        print(f"处理过程中发生未知错误: {e}")


if __name__ == '__main__':
    main()
