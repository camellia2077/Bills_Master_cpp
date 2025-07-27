import os
import subprocess
import sys

# ===================================================================
# 全局配置
# ===================================================================

# 用于 Pandoc 编译的字体选项
# 您可以更改此变量来切换不同的中文字体
PANDOC_FONT_OPTION = "Noto Serif SC"

# 需要编译的 Markdown 内容
MARKDOWN_CONTENT = """
# DATE:202208
# TOTAL:CNY2284.60
# REMARK:

# PET
总计:CNY999.27
占比:43.74%

## pet_equipments
小计:CNY768.10(占比:76.87%(
- CNY258.00 老鱼匠加热器
- CNY106.90 ubv灯
- CNY89.00 探头温度计
- CNY54.00 加热垫usb
- CNY46.00 青萍温度计
- CNY16.00 森森探头温度计
- CNY11.80 爬将军浮台中号
- CNY11.30 浮台小号强力吸盘

## pet_fish
小计:CNY9.50(占比:0.95%(
- CNY9.50 小区门口买的玛丽鱼

## pet_fodder
小计:CNY52.00(占比:5.20%(
- CNY34.00 德彩龟粮250ml
- CNY18.00 冻干虾一升

## pet_medicines
小计:CNY51.90(占比:5.19%(
- CNY43.00 妈咪爱
- CNY8.90 维生素葡萄糖粉两包

## pet_tortoise
小计:CNY117.77(占比:11.79%(
- CNY35.00 小青
- CNY29.97 草龟
- CNY6.80 巴西
"""
# ===================================================================

class MarkdownToPDFConverter:
    """
    一个用于将Markdown字符串通过Pandoc编译成PDF的类。
    """
    def __init__(self, font_name):
        """
        构造函数。
        :param font_name: 用于Pandoc编译的字体名称。
        """
        self.font_name = font_name
        self._check_pandoc_installed()

    def _check_pandoc_installed(self):
        """
        检查系统中是否安装了 pandoc。如果未安装，则打印错误并退出。
        """
        if subprocess.run(["pandoc", "--version"], capture_output=True).returncode != 0:
            print("错误: 系统中未找到 'pandoc' 命令。", file=sys.stderr)
            print("请先安装 Pandoc 和 LaTeX 引擎 (如 MiKTeX, MacTeX, or TeX Live)。", file=sys.stderr)
            sys.exit(1)

    def convert(self, md_content, output_filename="md_report.pdf"):
        """
        执行转换过程。
        :param md_content: 包含Markdown内容的字符串。
        :param output_filename: 输出的PDF文件名。
        """
        temp_md_filename = "temp_report.md"
        
        # 1. 将Markdown内容写入临时文件
        try:
            with open(temp_md_filename, "w", encoding="utf-8") as f:
                f.write(md_content)
            print(f"成功创建临时Markdown文件: {temp_md_filename}")
        except IOError as e:
            print(f"错误: 无法写入临时文件 {temp_md_filename}: {e}", file=sys.stderr)
            return

        # 2. 构建并执行Pandoc命令
        command = [
            "pandoc",
            temp_md_filename,
            "--pdf-engine=xelatex",
            "-V",
            f"mainfont={self.font_name}",
            "-o",
            output_filename
        ]
        
        print(f"正在执行命令: {' '.join(command)}")
        
        try:
            result = subprocess.run(
                command, 
                check=True, 
                capture_output=True, 
                text=True,
                encoding="utf-8"
            )
            print(f"\n成功! PDF文件已生成: {output_filename}")
            if result.stdout:
                print("\nPandoc 输出:")
                print(result.stdout)

        except subprocess.CalledProcessError as e:
            print("\n错误: Pandoc 编译失败。", file=sys.stderr)
            print(f"返回码: {e.returncode}", file=sys.stderr)
            print("\n--- Pandoc 错误信息 ---", file=sys.stderr)
            print(e.stderr, file=sys.stderr)
            print("------------------------", file=sys.stderr)

        finally:
            # 3. 清理临时文件
            if os.path.exists(temp_md_filename):
                os.remove(temp_md_filename)
                print(f"已清理临时文件: {temp_md_filename}")

def main():
    """
    脚本主入口函数。
    """
    print("--- 开始将Markdown编译为PDF ---")
    converter = MarkdownToPDFConverter(font_name=PANDOC_FONT_OPTION)
    converter.convert(md_content=MARKDOWN_CONTENT, output_filename="report.pdf")
    print("\n--- 任务结束 ---")

if __name__ == "__main__":
    main()
