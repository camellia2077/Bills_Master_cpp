# 图表配置 (Graph Configuration)

`graph` 脚本通过读取 `generate_report.json` 文件来进行配置。如果此文件不存在或无效，脚本将使用默认设置。

### `generate_report.json` 示例

```json
{
  "font_sizes": {
    "title": 20,
    "axis_label": 14,
    "tick_label": 12,
    "bar_label": 10
  }
}
````

### 配置项说明

* `font_sizes`: 一个包含图表不同部分字体大小设置的对象。
* `title`: 主图表标题的字体大小。
* `axis_label`: X 轴和 Y 轴标签的字体大小。
* `tick_label`: 坐标轴刻度标签的字体大小。
* `bar_label`: 显示在每个条形图上的标签（金额和百分比）的字体大小。
