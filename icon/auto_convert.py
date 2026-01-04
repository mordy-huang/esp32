import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from PIL import Image, ImageTk
import math

# --- 核心逻辑配置 ---
# 墨水屏 4 色定义 (RGB)
PALETTE = {
    "Black":  (0, 0, 0),
    "White":  (255, 255, 255),
    "Yellow": (255, 240, 0),
    "Red":    (255, 0, 0)
}

# 对应生成的 C 数组数值
COLOR_MAP_VALUES = {
    "Black":  0x03,
    "White":  0x00, # ✅ 实体白色保持 0x00
    "Yellow": 0x01,
    "Red":    0x02
}
# 透明色数值
TRANSPARENT_VAL = 0xFF 


class ImageConverterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("PNG 转 C 数组 (透明色分离版)")
        self.root.geometry("900x700")
        
        self.source_image = None
        self.processed_image = None

        self.setup_ui()

    def setup_ui(self):
        # 左侧控制面板
        control_frame = tk.Frame(self.root, width=250, bg="#f0f0f0", padx=10, pady=10)
        control_frame.pack(side=tk.LEFT, fill=tk.Y)

        # 1. 选择文件
        tk.Label(control_frame, text="1. 图片源", bg="#f0f0f0", font=("Arial", 10, "bold")).pack(anchor="w", pady=(0,5))
        tk.Button(control_frame, text="打开图片 (PNG/JPG)", command=self.load_image, height=2).pack(fill=tk.X)

        tk.Frame(control_frame, height=1, bg="#ccc").pack(fill=tk.X, pady=15)

        # 2. 缩放设置
        tk.Label(control_frame, text="2. 调整尺寸 (px)", bg="#f0f0f0", font=("Arial", 10, "bold")).pack(anchor="w", pady=(0,5))
        
        frame_w = tk.Frame(control_frame, bg="#f0f0f0")
        frame_w.pack(fill=tk.X)
        tk.Label(frame_w, text="宽:", bg="#f0f0f0").pack(side=tk.LEFT)
        self.entry_w = tk.Entry(frame_w, width=8)
        self.entry_w.pack(side=tk.RIGHT)
        
        frame_h = tk.Frame(control_frame, bg="#f0f0f0")
        frame_h.pack(fill=tk.X, pady=5)
        tk.Label(frame_h, text="高:", bg="#f0f0f0").pack(side=tk.LEFT)
        self.entry_h = tk.Entry(frame_h, width=8)
        self.entry_h.pack(side=tk.RIGHT)

        self.var_aspect = tk.BooleanVar(value=True)
        tk.Checkbutton(control_frame, text="锁定长宽比", variable=self.var_aspect, bg="#f0f0f0").pack(anchor="w")

        tk.Button(control_frame, text="应用缩放", command=self.apply_resize).pack(fill=tk.X, pady=5)

        tk.Frame(control_frame, height=1, bg="#ccc").pack(fill=tk.X, pady=15)

        # 3. 转换与生成
        tk.Label(control_frame, text="3. 生成代码", bg="#f0f0f0", font=("Arial", 10, "bold")).pack(anchor="w", pady=(0,5))
        tk.Button(control_frame, text="转换为 C 数组", command=self.convert_to_c, bg="#4CAF50", fg="white", height=2).pack(fill=tk.X)

        # 右侧预览与代码区
        right_frame = tk.Frame(self.root, padx=10, pady=10)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        self.preview_label = tk.Label(right_frame, text="暂无图片", bg="#e0e0e0")
        self.preview_label.pack(fill=tk.BOTH, expand=True, pady=(0, 10))

        tk.Label(right_frame, text="生成的 C 代码:").pack(anchor="w")
        self.text_output = tk.Text(right_frame, height=15, font=("Consolas", 9))
        self.text_output.pack(fill=tk.X)
        
        tk.Button(right_frame, text="复制到剪贴板", command=self.copy_to_clipboard).pack(side=tk.RIGHT, pady=5)

    def load_image(self):
        file_path = filedialog.askopenfilename(filetypes=[("Image files", "*.png;*.jpg;*.jpeg;*.bmp")])
        if not file_path:
            return
        
        try:
            raw_img = Image.open(file_path)
            
            # ✅ 关键修改：强制转为 RGBA 模式，保留 Alpha 通道，不进行背景填充
            self.source_image = raw_img.convert("RGBA")

            self.entry_w.delete(0, tk.END)
            self.entry_w.insert(0, str(self.source_image.width))
            self.entry_h.delete(0, tk.END)
            self.entry_h.insert(0, str(self.source_image.height))
            
            self.processed_image = self.source_image.copy()
            self.update_preview()
            self.text_output.delete(1.0, tk.END) 
        except Exception as e:
            messagebox.showerror("错误", f"无法打开图片: {e}")

    def apply_resize(self):
        if not self.source_image:
            return
            
        try:
            target_w = int(self.entry_w.get())
            target_h = int(self.entry_h.get())
            
            if self.var_aspect.get():
                original_w, original_h = self.source_image.size
                ratio = min(target_w / original_w, target_h / original_h)
                new_w = int(original_w * ratio)
                new_h = int(original_h * ratio)
                self.entry_w.delete(0, tk.END)
                self.entry_w.insert(0, str(new_w))
                self.entry_h.delete(0, tk.END)
                self.entry_h.insert(0, str(new_h))
            else:
                new_w, new_h = target_w, target_h

            self.processed_image = self.source_image.resize((new_w, new_h), Image.Resampling.LANCZOS)
            self.update_preview()
            
        except ValueError:
            messagebox.showwarning("提示", "请输入有效的宽度和高度数值")

    def update_preview(self):
        if not self.processed_image:
            return
            
        # 预览时创建一个白底背景，方便用户查看内容
        display_img = Image.new("RGBA", self.processed_image.size, (200, 200, 200, 255))
        display_img.paste(self.processed_image, (0,0), self.processed_image)
        
        display_img.thumbnail((400, 300)) 
        tk_img = ImageTk.PhotoImage(display_img)
        self.preview_label.config(image=tk_img, text="")
        self.preview_label.image = tk_img

    def get_closest_color(self, r, g, b):
        min_dist = float('inf')
        chosen_color_name = "White"
        
        for name, rgb in PALETTE.items():
            dist = math.sqrt((r - rgb[0])**2 + (g - rgb[1])**2 + (b - rgb[2])**2)
            if dist < min_dist:
                min_dist = dist
                chosen_color_name = name
        
        return COLOR_MAP_VALUES[chosen_color_name]

    def convert_to_c(self):
        if not self.processed_image:
            messagebox.showwarning("提示", "请先加载图片")
            return

        w, h = self.processed_image.size
        # 获取像素数据 (RGBA)
        pixels = self.processed_image.load()
        
        c_array_content = []
        c_array_content.append(f"// Generated by Python Tool")
        c_array_content.append(f"// Width: {w}, Height: {h}")
        c_array_content.append(f"// 0x00=White, 0x01=Yellow, 0x02=Red, 0x03=Black, 0xFF=Transparent")
        c_array_content.append(f"const int IMAGE_WIDTH = {w};")
        c_array_content.append(f"const int IMAGE_HEIGHT = {h};")
        c_array_content.append(f"const unsigned char image_data[{w*h}] = {{")
        
        line_buffer = []
        
        for y in range(h):
            for x in range(w):
                # 获取 RGBA
                pixel = pixels[x, y]
                
                # 判断是否有 Alpha 通道
                if len(pixel) == 4:
                    r, g, b, a = pixel
                else:
                    r, g, b = pixel
                    a = 255
                
                # ✅ 核心逻辑：判断透明度
                # 如果 Alpha < 128，认为是透明 -> 0xFF
                if a < 128:
                    val = TRANSPARENT_VAL
                else:
                    # 如果不透明，寻找最近的颜色 -> White(0x00) / Red / Yellow / Black
                    val = self.get_closest_color(r, g, b)
                
                line_buffer.append(f"0x{val:02X}")
                
                if len(line_buffer) >= 16:
                    c_array_content.append("  " + ", ".join(line_buffer) + ",")
                    line_buffer = []
        
        if line_buffer:
             c_array_content.append("  " + ", ".join(line_buffer))

        c_array_content.append("};")
        
        final_text = "\n".join(c_array_content)
        
        self.text_output.delete(1.0, tk.END)
        self.text_output.insert(tk.END, final_text)

    def copy_to_clipboard(self):
        self.root.clipboard_clear()
        self.root.clipboard_append(self.text_output.get(1.0, tk.END))
        messagebox.showinfo("成功", "代码已复制到剪贴板！")

if __name__ == "__main__":
    root = tk.Tk()
    app = ImageConverterApp(root)
    root.mainloop()