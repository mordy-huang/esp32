import tkinter as tk
from tkinter import filedialog
from PIL import Image, ImageOps, ImageEnhance
import os
import sys

# ================= 用户配置区域 =================

OUTPUT_HEADER = "python/Ap_29demo.h"  # 输出路径
TARGET_W = 800
TARGET_H = 480

# 清晰度模式
# "TEXT"  = 文字/Logo (边缘锐利)
# "PHOTO" = 照片 (有颗粒感)
CONVERT_MODE = "TEXT" 

# ===============================================

def create_4color_palette():
    p_img = Image.new('P', (1, 1))
    palette = [
        0, 0, 0,       # 0: Black
        255, 255, 255, # 1: White
        255, 255, 0,   # 2: Yellow
        255, 0, 0,     # 3: Red
    ]
    palette.extend([0, 0, 0] * 252)
    p_img.putpalette(palette)
    return p_img

def map_index_to_code(idx):
    # SPD1656 硬件映射
    mapping = {
        0: 0x03, # Black
        1: 0x00, # White
        2: 0x01, # Yellow
        3: 0x02  # Red
    }
    return mapping.get(idx, 0x03)

def select_file():
    # 优先检查命令行参数
    if len(sys.argv) > 1:
        arg_path = sys.argv[1]
        if os.path.exists(arg_path):
            return arg_path
        else:
            print(f"[WARN] Argument path does not exist: {arg_path}")

    root = tk.Tk()
    root.withdraw()
    
    print("[INFO] Waiting for file selection...")
    file_path = filedialog.askopenfilename(
        title="Select Image (800x480)",
        filetypes=[("Images", "*.jpg *.jpeg *.png *.bmp"), ("All Files", "*.*")]
    )
    return file_path

def process_image():
    # 1. 选择文件
    input_path = select_file()
    
    if not input_path:
        print("[WARN] No file selected. Exiting.")
        return

    print(f"[INFO] Selected: {input_path}")
    print(f"[INFO] Mode: {CONVERT_MODE}")

    try:
        im = Image.open(input_path).convert("RGB")
    except Exception as e:
        print(f"[ERROR] Cannot open image: {e}")
        return
    
    # 2. 预处理
    enhancer = ImageEnhance.Contrast(im)
    im = enhancer.enhance(1.3)
    
    # 3. 缩放
    print("[INFO] Resizing...")
    im = ImageOps.fit(im, (TARGET_W, TARGET_H), method=Image.Resampling.LANCZOS)
    
    # 4. 颜色转换
    palette_img = create_4color_palette()
    
    if CONVERT_MODE == "TEXT":
        print("[INFO] Dither: OFF (Text Mode)")
        im_final = im.quantize(palette=palette_img, dither=Image.NONE)
    else:
        print("[INFO] Dither: ON (Photo Mode)")
        im_final = im.quantize(palette=palette_img, dither=Image.FLOYDSTEINBERG)
    
    # 5. 生成预览
    preview_filename = "python/preview_output.png"
    im_final.convert("RGB").save(preview_filename)
    print(f"[INFO] Preview saved: {preview_filename}")

    # 6. 生成代码
    print("[INFO] Generating C code...")
    pixels = im_final.load()
    hex_data = []
    
    for y in range(TARGET_H):
        for x in range(0, TARGET_W, 4):
            byte_val = 0
            for k in range(4):
                if x + k < TARGET_W:
                    idx = pixels[x + k, y]
                    code = map_index_to_code(idx)
                    byte_val |= (code << (6 - 2*k))
            hex_data.append(f"0X{byte_val:02X}")

    # 7. 写入文件 (强制使用 utf-8 编码防止报错)
    # 如果 OUTPUT_HEADER 路径不存在，则回退到当前目录
    final_output = OUTPUT_HEADER
    output_dir = os.path.dirname(OUTPUT_HEADER)
    if output_dir and not os.path.exists(output_dir):
        # 尝试查找正确的 src 目录
        possible_src = os.path.join(os.getcwd(), "src", "A32-GDEM075F52", "A32-GDEM075F52", "A32-GDEM075F52-F-20250321", "GDEM075F52_Arduino", "Ap_29demo.h")
        if os.path.exists(os.path.dirname(possible_src)):
             final_output = possible_src
        else:
             final_output = "python/Ap_29demo.h"

    c_code = f"// Source: {os.path.basename(input_path)} | Mode: {CONVERT_MODE}\n"
    c_code += f"const unsigned char gImage_1[{len(hex_data)}] = {{\n"
    for i in range(0, len(hex_data), 16):
        c_code += ",".join(hex_data[i:i+16]) + ",\n"
    c_code += "};\n"
    
    with open(final_output, "w", encoding='utf-8') as f:
        f.write(c_code)
        
    print("-" * 30)
    print(f"[SUCCESS] Code saved to: {final_output}")
    print("Please upload to ESP32 now.")
    print("-" * 30)

if __name__ == "__main__":
    process_image()