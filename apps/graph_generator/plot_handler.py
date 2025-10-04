# plot_handler.py

import matplotlib.pyplot as plt
import seaborn as sns # type: ignore
import pandas as pd

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
    print(f"Chart successfully saved to '{output_file}'")