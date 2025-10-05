# database_handler.py

import sqlite3
import pandas as pd
import sys

def fetch_yearly_data(db_path: str, year: int):
    """
    Fetches and aggregates expense data for a specific year.
    
    Returns:
        pd.DataFrame: A DataFrame with monthly expenses, or None if an error occurs.
    """
    print(f"Fetching data for the year {year}...")
    try:
        conn = sqlite3.connect(db_path)
        query = """
        SELECT
            b.month,
            SUM(t.amount) AS total_expenses
        FROM
            transactions t
        JOIN
            bills b ON t.bill_id = b.id
        WHERE
            b.year = ? AND t.transaction_type = 'Expense'
        GROUP BY
            b.month
        ORDER BY
            b.month;
        """
        df = pd.read_sql_query(query, conn, params=(year,))
        conn.close()
        return df
    except sqlite3.Error as e:
        print(f"Database error: {e}", file=sys.stderr)
        return None

def fetch_monthly_data(db_path: str, month: str):
    """
    Fetches and aggregates expense data for a specific month.

    Returns:
        pd.DataFrame: A DataFrame with expenses by category, or None if an error occurs.
    """
    print(f"Fetching data for the month {month}...")
    try:
        conn = sqlite3.connect(db_path)
        query = """
        SELECT
            t.parent_category,
            SUM(t.amount) AS total_expenses
        FROM
            transactions t
        JOIN
            bills b ON t.bill_id = b.id
        WHERE
            b.bill_date = ? AND t.transaction_type = 'Expense'
        GROUP BY
            t.parent_category
        ORDER BY
            total_expenses DESC;
        """
        df = pd.read_sql_query(query, conn, params=(month,))
        conn.close()
        return df
    except sqlite3.Error as e:
        print(f"Database error: {e}", file=sys.stderr)
        return None

# [修改] 获取月度详细消费记录的函数
def fetch_monthly_details_data(db_path: str, month: str):
    """
    获取指定月份的所有单笔消费记录。

    Returns:
        pd.DataFrame: 包含每笔消费详情的 DataFrame，或在出错时返回 None。
    """
    print(f"正在查询 {month} 月的详细消费数据...")
    try:
        conn = sqlite3.connect(db_path)
        # [修改] 在查询中加入 parent_category
        query = """
        SELECT
            parent_category,
            description,
            sub_category,
            amount
        FROM
            transactions t
        JOIN
            bills b ON t.bill_id = b.id
        WHERE
            b.bill_date = ? AND t.transaction_type = 'Expense'
        ORDER BY
            parent_category, amount DESC;
        """
        df = pd.read_sql_query(query, conn, params=(month,))
        conn.close()
        return df
    except sqlite3.Error as e:
        print(f"数据库错误: {e}", file=sys.stderr)
        return None