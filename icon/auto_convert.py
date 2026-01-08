import math
from PIL import Image, ImageDraw

# ================= 配置区域 =================

# 【关键修复】
# 内部绘图画布必须足够大，才能容纳半径45的太阳和其他特效
# 设置为 120，让绘图元素居中且不被裁剪
DRAW_SIZE = 160 

# 【最终输出】
# 脚本会自动将 120px 的高清图缩放到这里定义的尺寸
OUTPUT_SIZE = 60 

OUTPUT_FILENAME = "WeatherIcons.h"

# 16进制字符串
HEX_WHITE  = "0x00"
HEX_YELLOW = "0x01"
HEX_RED    = "0x02"
HEX_BLACK  = "0x03"

# 颜色定义 (R, G, B)
COLOR_BLACK      = (0, 0, 0)
COLOR_WHITE      = (255, 255, 255)
COLOR_RED        = (230, 57, 70)
COLOR_YELLOW     = (255, 215, 0)
COLOR_LIGHT_GRAY = (224, 224, 224) 
COLOR_GRAY       = (176, 176, 176) 
COLOR_DARK_GRAY  = (112, 112, 112) 

# 定义标准调色板
PALETTE_COLORS = [
    COLOR_BLACK, COLOR_WHITE, COLOR_RED, COLOR_YELLOW, 
    COLOR_LIGHT_GRAY, COLOR_GRAY, COLOR_DARK_GRAY
]

# ================= 绘图辅助函数 =================

def create_canvas():
    return Image.new("RGBA", (DRAW_SIZE, DRAW_SIZE), (255, 255, 255, 0))

def rotate_point(x, y, cx, cy, angle_deg):
    rad = math.radians(angle_deg)
    cos_a = math.cos(rad)
    sin_a = math.sin(rad)
    nx = cx + (x - cx) * cos_a - (y - cy) * sin_a
    ny = cy + (x - cx) * sin_a + (y - cy) * cos_a
    return nx, ny

def draw_circle(draw, cx, cy, r, color, outline=None):
    draw.ellipse([(cx-r, cy-r), (cx+r, cy+r)], fill=color, outline=outline)

def draw_line(draw, x1, y1, x2, y2, color, width=3):
    draw.line([(x1, y1), (x2, y2)], fill=color, width=width)

def draw_cloud(draw, cx, cy, color, scale=1.0):
    # 稍微调整云朵坐标以适应大画布
    offsets = [(-40, 20, 20), (-25, -15, 25), (15, -25, 30), (45, 0, 25), (30, 20, 20)]
    for ox, oy, r in offsets:
        draw_circle(draw, cx + ox*scale, cy + oy*scale, r*scale, color)
    draw.rectangle([cx - 40*scale, cy, cx + 45*scale, cy + 20*scale], fill=color)

def draw_sun(draw, cx, cy, r):
    draw_circle(draw, cx, cy, r, COLOR_YELLOW)
    for i in range(10):
        angle = i * 36
        # 调整光芒长度比例
        x1, y1 = rotate_point(cx, cy - (r+8), cx, cy, angle)
        x2, y2 = rotate_point(cx, cy - (r+22), cx, cy, angle)
        draw_line(draw, x1, y1, x2, y2, COLOR_YELLOW, width=5)

def draw_moon(draw, cx, cy, r):
    draw_circle(draw, cx, cy, r, COLOR_YELLOW)
    MASK_COLOR = (254, 254, 254) 
    # 调整月亮遮罩位置，使其更像新月
    draw_circle(draw, cx + r*0.4, cy, r*0.85, MASK_COLOR) 
    draw_star(draw, cx+35, cy-35, 5, COLOR_YELLOW)
    draw_star(draw, cx-35, cy-15, 4, COLOR_YELLOW)

def draw_star(draw, cx, cy, r, color):
    draw_circle(draw, cx, cy, r, color)

# 【同步更新】根据 HTML 文件重写精致雪花逻辑
def draw_snowflake_branch(draw, cx, cy, size, angle, color):
    # 主干
    ex, ey = rotate_point(cx, cy - size, cx, cy, angle)
    draw_line(draw, cx, cy, ex, ey, color, width=3)
    
    # 对应 HTML 中的 positions = [0.4, 0.65, 0.85]
    positions = [0.4, 0.65, 0.85]
    
    for pos in positions:
        # 计算分叉点
        bx, by = rotate_point(cx, cy - size * pos, cx, cy, angle)
        
        # 计算分叉长度 (仿照 HTML: subLen = len * 0.35 * (1 - pos * 0.6))
        sub_len = size * 0.45 * (1 - pos * 0.5) # 稍微加粗一点点适应墨水屏
        
        # 绘制左右分叉
        # 左分叉
        lx, ly = rotate_point(bx - sub_len, by - sub_len * 0.8, bx, by, angle)
        draw_line(draw, bx, by, lx, ly, color, width=3)
        
        # 右分叉
        rx, ry = rotate_point(bx + sub_len, by - sub_len * 0.8, bx, by, angle)
        draw_line(draw, bx, by, rx, ry, color, width=3)

def draw_snowflake(draw, cx, cy, size):
    for i in range(6):
        draw_snowflake_branch(draw, cx, cy, size, i * 60, COLOR_BLACK)

def draw_rain_drop(draw, cx, cy, color):
    # 稍微加大雨滴尺寸
    draw_line(draw, cx - 4, cy - 10, cx + 4, cy + 10, color, width=5)

def draw_bolt(draw, cx, cy):
    # 调整闪电坐标
    points = [(cx+5, cy-20), (cx-10, cy), (cx+2, cy), (cx-8, cy+25), (cx+12, cy+5), (cx, cy+5), (cx+10, cy-20)]
    draw.polygon(points, fill=COLOR_RED)
    draw.line(points + [points[0]], fill=COLOR_BLACK, width=2)

def draw_wind_lines(draw, cx, cy):
    draw_line(draw, cx-50, cy-15, cx+30, cy-15, COLOR_GRAY, width=6)
    draw_line(draw, cx-60, cy+15, cx+40, cy+15, COLOR_GRAY, width=6)
    draw.ellipse([(cx+30, cy-8), (cx+50, cy+8)], fill=COLOR_RED)

# ================= 图标渲染逻辑 =================

def render_icon(name):
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = DRAW_SIZE // 2, DRAW_SIZE // 2

    # 根据 DRAW_SIZE=120 调整了部分相对位置
    if name == "image_qing":      
        draw_sun(draw, cx, cy, 50) # 放大一点
    elif name == "image_qing_n":  
        draw_moon(draw, cx, cy, 45)
    elif name == "image_duoyun":  
        draw_sun(draw, cx - 25, cy - 25, 40)
        draw_cloud(draw, cx + 10, cy + 20, COLOR_LIGHT_GRAY, scale=1.0)
    elif name == "image_duoyun_n": 
        draw_moon(draw, cx - 20, cy - 25, 40)
        draw_cloud(draw, cx + 15, cy + 25, COLOR_GRAY, scale=1.0) 
    elif name == "image_yin":     
        draw_cloud(draw, cx - 25, cy - 20, COLOR_DARK_GRAY, scale=1.0)
        draw_cloud(draw, cx + 15, cy + 15, COLOR_GRAY, scale=1.0)
    elif name == "image_yu":      
        draw_rain_drop(draw, cx - 20, cy + 45, COLOR_BLACK)
        draw_rain_drop(draw, cx, cy + 55, COLOR_BLACK)
        draw_rain_drop(draw, cx + 20, cy + 45, COLOR_BLACK)
        draw_cloud(draw, cx, cy - 5, COLOR_GRAY)
    elif name == "image_xue":     
        # 对应 HTML: 左下大雪花(24)，右下小雪花(18)
        # 考虑到 120px 画布，尺寸稍微按比例调整
        draw_snowflake(draw, cx - 28, cy + 30, 26) 
        draw_snowflake(draw, cx + 32, cy + 25, 20)
        draw_cloud(draw, cx, cy - 20, COLOR_LIGHT_GRAY)
    elif name == "image_sleet":
        draw_rain_drop(draw, cx - 25, cy + 50, COLOR_BLACK) 
        draw_snowflake(draw, cx + 25, cy + 50, 22)          
        draw_cloud(draw, cx, cy - 10, COLOR_GRAY)
    elif name == "image_lei":     
        draw_bolt(draw, cx - 25, cy + 35)
        draw_bolt(draw, cx + 15, cy + 45)
        draw_cloud(draw, cx, cy - 10, COLOR_DARK_GRAY)
    elif name == "image_wu":      
        draw_line(draw, cx - 50, cy + 10, cx + 20, cy + 10, COLOR_GRAY, 8)
        draw_line(draw, cx - 20, cy + 30, cx + 50, cy + 30, COLOR_GRAY, 8)
        draw_cloud(draw, cx, cy - 25, COLOR_LIGHT_GRAY, scale=0.9)
    elif name == "image_wind":
        draw_wind_lines(draw, cx, cy)
        draw_cloud(draw, cx - 25, cy - 25, COLOR_WHITE, scale=0.9)
    elif name == "image_sand":
        draw_line(draw, cx - 50, cy + 20, cx + 50, cy + 20, COLOR_YELLOW, 6)
        draw_cloud(draw, cx, cy - 10, COLOR_YELLOW) 
        for i in range(5):
            draw_circle(draw, cx - 35 + i*18, cy + 35 + (i%2)*5, 3, COLOR_RED)
    elif name == "image_unknown": 
        draw_cloud(draw, cx, cy, COLOR_LIGHT_GRAY)
        
    return img

# ================= 颜色处理与灰度抖动 =================

def get_closest_palette_color(rgb):
    min_dist = float('inf')
    closest = COLOR_WHITE
    r, g, b = rgb
    for color in PALETTE_COLORS:
        cr, cg, cb = color
        dist = (r - cr)**2 + (g - cg)**2 + (b - cb)**2
        if dist < min_dist:
            min_dist = dist
            closest = color
    return closest

def get_pixel_hex(r, g, b, a, x, y):
    if a < 100 or (r,g,b) == (254, 254, 254):
        return HEX_WHITE
    
    current_color = get_closest_palette_color((r, g, b))

    if current_color == COLOR_BLACK:  return HEX_BLACK
    if current_color == COLOR_WHITE:  return HEX_WHITE
    if current_color == COLOR_RED:    return HEX_RED
    if current_color == COLOR_YELLOW: return HEX_YELLOW
    
    is_even_pixel = ((x + y) % 2 == 0)

    if current_color == COLOR_LIGHT_GRAY:
        return HEX_WHITE 

    if current_color == COLOR_GRAY:
        return HEX_BLACK if is_even_pixel else HEX_WHITE

    if current_color == COLOR_DARK_GRAY:
        return HEX_BLACK

    return HEX_WHITE

# ================= 生成头文件 =================

def generate_header():
    icons_map = {
        "image_qing":     "image_qing",
        "image_qing_n":   "image_qing_n",
        "image_duoyun":   "image_duoyun",
        "image_duoyun_n": "image_duoyun_n",
        "image_yin":      "image_yin",
        "image_yu":       "image_yu",
        "image_xue":      "image_xue",
        "image_sleet":    "image_sleet",
        "image_lei":      "image_lei",
        "image_wu":       "image_wu",
        "image_wind":     "image_wind",
        "image_sand":     "image_sand",
        "image_unknown":  "image_unknown"
    }
    
    print(f"Config: DRAW_SIZE={DRAW_SIZE}, OUTPUT_SIZE={OUTPUT_SIZE}")
    
    with open(OUTPUT_FILENAME, "w", encoding="utf-8") as f:
        f.write("#ifndef WEATHER_ICONS_H\n")
        f.write("#define WEATHER_ICONS_H\n\n")
        f.write("// Auto-generated by Python script\n")
        f.write("// Color Map: 0x00=White, 0x01=Yellow, 0x02=Red, 0x03=Black\n\n")

        f.write(f"const int IMAGE_WIDTH = {OUTPUT_SIZE};\n")
        f.write(f"const int IMAGE_HEIGHT = {OUTPUT_SIZE};\n\n")
        
        for py_name, cpp_name in icons_map.items():
            print(f"Generating {cpp_name}...")
            
            # 1. 在大画布(120px)上高清绘制
            img_original = render_icon(py_name)
            
            # 2. 高质量缩放到目标尺寸(60px)
            if DRAW_SIZE != OUTPUT_SIZE:
                img_final = img_original.resize((OUTPUT_SIZE, OUTPUT_SIZE), Image.Resampling.LANCZOS)
            else:
                img_final = img_original

            pixels = img_final.load()
            
            f.write(f"const uint8_t {cpp_name}[] PROGMEM = {{\n")
            
            for y in range(OUTPUT_SIZE):
                line_data = []
                for x in range(OUTPUT_SIZE):
                    r, g, b, a = pixels[x, y]
                    hex_val = get_pixel_hex(r, g, b, a, x, y)
                    line_data.append(hex_val)
                f.write("  " + ", ".join(line_data) + ",\n")
            
            f.write("};\n\n")
        
        f.write("#endif\n")
    print(f"Done! Saved to {OUTPUT_FILENAME}")

if __name__ == "__main__":
    generate_header()