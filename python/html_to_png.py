import os
import time
import sys

# 需要安装 playwright:
# pip install playwright
# playwright install chromium

def html_to_png(html_path, output_path, width=800, height=480):
    """
    将 HTML 文件转换为 PNG 图片
    :param html_path: HTML 文件路径
    :param output_path: 输出图片路径
    :param width: 视口宽度
    :param height: 视口高度
    """
    try:
        from playwright.sync_api import sync_playwright
    except ImportError:
        print("请先安装 playwright: pip install playwright && playwright install chromium")
        return

    abs_html_path = os.path.abspath(html_path)
    if not os.path.exists(abs_html_path):
        print(f"错误: 找不到文件 {abs_html_path}")
        return

    print(f"正在转换: {abs_html_path} -> {output_path}")

    with sync_playwright() as p:
        # 启动浏览器
        browser = p.chromium.launch()
        # 创建新页面，设置视口大小
        page = browser.new_page(viewport={'width': width, 'height': height})
        
        # 加载本地 HTML 文件
        page.goto(f"file:///{abs_html_path}")
        
        # 等待页面加载完成 (如果有动画或字体加载，可以适当增加等待时间)
        time.sleep(1) 
        
        # 截图
        page.screenshot(path=output_path)
        
        browser.close()
        print(f"成功保存图片: {output_path}")

if __name__ == "__main__":
    # 默认路径配置
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    html_file = os.path.join(project_root, "python", "calendarUI.html")
    output_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "calendar_ui.png")
    
    html_to_png(html_file, output_file)
