# plot_handler.py

import matplotlib.pyplot as plt
import seaborn as sns # type: ignore
import pandas as pd
import os

def set_plot_style(font_list: list):
    """
    Sets the global style for matplotlib plots.
    """
    sns.set_theme(style="whitegrid")
    plt.rcParams['figure.figsize'] = (12, 7)
    plt.rcParams['font.sans-serif'] = font_list
    plt.rcParams['axes.unicode_minus'] = False 

def plot_yearly_chart(df: pd.DataFrame, year: int, output_file: str):
    """
    Generates and saves a bar chart of total monthly expenses for a given year.
    """
    print("Generating yearly expense chart...")
    plt.figure()
    ax = sns.barplot(x='month', y='total_expenses', data=df, palette="viridis", hue='month', dodge=False, legend=False)
    
    for p in ax.patches:
        ax.annotate(f'{p.get_height():.2f}', 
                    (p.get_x() + p.get_width() / 2., p.get_height()), 
                    ha='center', va='center', 
                    xytext=(0, 9), 
                    textcoords='offset points')

    plt.title(f'{year}年度月度消费总额', fontsize=16)
    plt.xlabel('月份', fontsize=12)
    plt.ylabel('总消费金额', fontsize=12)
    plt.xticks(ticks=range(len(df['month'])), labels=[f"{m}月" for m in df['month']])
    
    plt.tight_layout()
    plt.savefig(output_file)
    plt.close() # 关闭图形，释放内存
    print(f"Chart successfully saved to '{output_file}'")

def plot_monthly_chart(df: pd.DataFrame, month: str, output_file: str):
    """
    Generates and saves a bar chart of expenses by category for a given month.
    """
    print("Generating monthly expense chart...")
    plt.figure()
    ax = sns.barplot(x='total_expenses', y='parent_category', data=df, palette="plasma", hue='parent_category', dodge=False, legend=False)
    
    for p in ax.patches:
        width = p.get_width()
        ax.annotate(f'{width:.2f}', 
                    (width, p.get_y() + p.get_height() / 2.),
                    ha='left', va='center', 
                    xytext=(5, 0), 
                    textcoords='offset points')

    plt.title(f'{month[:4]}年{month[4:]}月 各大类消费分布', fontsize=16)
    plt.xlabel('总消费金额', fontsize=12)
    plt.ylabel('消费类别', fontsize=12)
    
    plt.tight_layout()
    plt.savefig(output_file)
    plt.close() # 关闭图形，释放内存
    print(f"Chart successfully saved to '{output_file}'")

# [重构] 绘制月度详细消费图表的函数
def plot_monthly_details_by_category(df: pd.DataFrame, month: str, output_prefix: str):
    """
    为指定月份的每个消费大类生成独立的消费明细图表。
    """
    print("正在按消费大类生成月度详细消费图表...")
    
    # 获取所有唯一的父类别
    parent_categories = df['parent_category'].unique()

    for category in parent_categories:
        # 筛选出当前类别的数据
        category_df = df[df['parent_category'] == category].copy()

        # 为了更好的可读性，创建一个组合标签，并对长描述进行截断
        category_df['label'] = category_df['sub_category'] + ' - ' + category_df['description'].str.slice(0, 20)
        
        # 动态调整图表高度，防止标签重叠
        num_items = len(category_df)
        fig_height = max(7, num_items * 0.4)
        
        # 创建新的图形实例
        plt.figure(figsize=(12, fig_height))

        ax = sns.barplot(x='amount', y='label', data=category_df, palette="rocket", hue='label', dodge=False, legend=False)

        for p in ax.patches:
            width = p.get_width()
            ax.annotate(f'{width:.2f}', 
                        (width, p.get_y() + p.get_height() / 2.),
                        ha='left', va='center', 
                        xytext=(5, 0), 
                        textcoords='offset points')

        plt.title(f'{month[:4]}年{month[4:]}月 - {category} - 消费明细', fontsize=16)
        plt.xlabel('消费金额', fontsize=12)
        plt.ylabel('具体项目', fontsize=12)

        # 构建每个图表的独立文件名
        output_file = f"{output_prefix}_{category}.png"

        plt.tight_layout()
        plt.savefig(output_file)
        plt.close() # 每次画完一个图就关闭，防止占用过多内存
        print(f"  -> 图表已成功保存至: '{output_file}'")