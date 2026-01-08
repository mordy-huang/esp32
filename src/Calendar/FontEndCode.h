
// ==========================================
// 🌐 网页前端代码 (包含图片处理逻辑)
// ==========================================
// 在 CalendarApp.cpp 顶部或 setupWebServer 函数外部定义
// R"rawliteral(...)rawliteral" 是为了防止 HTML 里恰好有 )" 导致截断，加个自定义标记更安全
const char *html_upload_page = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 画框 Pro Max +</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; background: #f0f2f5; margin: 0; padding: 20px; color: #333; }
    .container { background: white; max-width: 900px; margin: 0 auto; padding: 25px; border-radius: 12px; box-shadow: 0 4px 20px rgba(0,0,0,0.08); }
    h2 { margin-top: 0; color: #1a1a1a; display: flex; align-items: center; gap: 10px; }
    .badge { background: #e6f7ff; color: #1890ff; font-size: 12px; padding: 4px 8px; border-radius: 4px; border: 1px solid #91d5ff; }

    /* 控制区 */
    .controls { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 15px; background: #fafafa; padding: 15px; border-radius: 8px; margin: 20px 0; border: 1px solid #eee; }
    .control-group { display: flex; flex-direction: column; gap: 5px; }
    .control-group label { font-size: 13px; font-weight: bold; color: #666; display: flex; justify-content: space-between; }
    input[type=range] { width: 100%; cursor: pointer; }

    .btn-group { display: flex; gap: 10px; margin-top: 5px; }
    .btn-tool { flex: 1; padding: 8px; background: #fff; border: 1px solid #ddd; border-radius: 6px; cursor: pointer; font-size: 14px; transition: 0.2s; }
    .btn-tool:hover { background: #f0f0f0; border-color: #ccc; }
    .btn-tool:active { background: #e6e6e6; }

    /* 预览区 */
    .preview-area { display: flex; flex-wrap: wrap; gap: 20px; margin-top: 20px; }
    .card { flex: 1; min-width: 300px; background: #fff; border: 1px solid #eee; border-radius: 8px; overflow: hidden; }
    .card-header { background: #f9f9f9; padding: 10px 15px; font-size: 14px; font-weight: bold; border-bottom: 1px solid #eee; color: #555; display: flex; justify-content: space-between; align-items: center; }
    .card-body { padding: 10px; display: flex; justify-content: center; background: #e0e0e0; min-height: 200px; align-items: center; position: relative; }
    
    /* 🔴 关键样式：鼠标手势 */
    #canvasProcess { cursor: grab; touch-action: none; } 
    #canvasProcess:active { cursor: grabbing; }

    canvas { max-width: 100%; height: auto; image-rendering: pixelated; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    
    .btn-main { background: #1890ff; color: white; width: 100%; padding: 15px; border: none; border-radius: 8px; font-size: 16px; font-weight: bold; cursor: pointer; transition: 0.2s; margin-top: 20px; }
    .btn-main:hover { background: #40a9ff; }
    .btn-main:disabled { background: #ccc; cursor: not-allowed; }
    #status { text-align: center; margin-top: 15px; font-size: 14px; color: #666; }
    .tip { font-size: 12px; color: #1890ff; font-weight: normal; margin-left: auto; }
  </style>
</head>
<body>
  <div class="container">
    <h2>🖼️ 画框上传 <span class="badge">Pro Max +</span></h2>
    
    <input type="file" id="fileInput" accept="image/*" style="margin-bottom: 10px; width: 100%; padding: 10px; border: 2px dashed #ddd; border-radius: 8px;">

    <div class="controls">
      <div class="control-group">
        <label>旋转与位置</label>
        <div class="btn-group">
          <button class="btn-tool" onclick="rotate(-90)">↺ 左转</button>
          <button class="btn-tool" onclick="resetPos()">🎯 复位</button>
          <button class="btn-tool" onclick="rotate(90)">↻ 右转</button>
        </div>
      </div>
      <div class="control-group">
        <label>对比度 <span id="val-contrast">1.6</span></label>
        <input type="range" id="contrast" min="0.5" max="2.5" step="0.1" value="1.6" oninput="updatePreview()">
      </div>
      <div class="control-group">
        <label>亮度 <span id="val-brightness">5</span></label>
        <input type="range" id="brightness" min="-50" max="50" step="5" value="5" oninput="updatePreview()">
      </div>
      <div class="control-group">
        <label>锐化 <span id="val-sharpen">0.9</span></label>
        <input type="range" id="sharpen" min="0" max="2.5" step="0.1" value="0.9" oninput="updatePreview()">
      </div>
    </div>

    <div class="preview-area">
      <div class="card">
        <div class="card-header">
          <span>1. 拖拽调整构图</span>
          <span class="tip">👆 按住画布拖动</span>
        </div>
        <div class="card-body">
          <canvas id="canvasProcess"></canvas>
        </div>
      </div>
      <div class="card">
        <div class="card-header">
          <span>2. 最终效果</span>
          <span id="rot-badge" class="badge">0°</span>
        </div>
        <div class="card-body">
          <canvas id="canvasFinal"></canvas>
        </div>
      </div>
    </div>

    <button id="uploadBtn" onclick="upload()" class="btn-main" disabled>🚀 发送到屏幕</button>
    <p id="status">等待选择图片...</p>
  </div>

  <script>
    const fileInput = document.getElementById('fileInput');
    const canvasProcess = document.getElementById('canvasProcess');
    const ctxProcess = canvasProcess.getContext('2d', { willReadFrequently: true });
    const canvasFinal = document.getElementById('canvasFinal');
    const ctxFinal = canvasFinal.getContext('2d', { willReadFrequently: true });
    
    const WIDTH = 800;
    const HEIGHT = 480;
    
    // 状态变量
    let currentRotation = 0;
    let offsetX = 0; // 手动偏移 X
    let offsetY = 0; // 手动偏移 Y
    let originalImage = null;

    // 交互相关变量
    let isDragging = false;
    let lastX = 0;
    let lastY = 0;

    const PALETTE = [
      {r:0,   g:0,   b:0,   val:3}, 
      {r:255, g:255, b:255, val:0}, 
      {r:220, g:40,  b:40,  val:2}, 
      {r:240, g:230, b:50,  val:1}  
    ];

    [canvasProcess, canvasFinal].forEach(c => { c.width = WIDTH; c.height = HEIGHT; });

    // === 事件监听：鼠标/触摸拖拽 ===
    function handleStart(x, y) {
      isDragging = true;
      lastX = x;
      lastY = y;
    }
    
    function handleMove(x, y) {
      if (!isDragging) return;
      // 计算画布显示的缩放比例 (CSS像素 vs 实际像素)
      const scale = WIDTH / canvasProcess.clientWidth;
      
      const dx = (x - lastX) * scale;
      const dy = (y - lastY) * scale;
      
      offsetX += dx;
      offsetY += dy;
      
      lastX = x;
      lastY = y;
      
      // 这里的优化：拖动时可以只画图不抖动(提升性能)，或者全画
      // 现代手机很快，直接全画即可
      updatePreview();
    }

    function handleEnd() { isDragging = false; }

    // 鼠标事件
    canvasProcess.addEventListener('mousedown', e => handleStart(e.clientX, e.clientY));
    window.addEventListener('mousemove', e => handleMove(e.clientX, e.clientY));
    window.addEventListener('mouseup', handleEnd);

    // 触摸事件 (手机端)
    canvasProcess.addEventListener('touchstart', e => {
      e.preventDefault(); // 防止滚动页面
      handleStart(e.touches[0].clientX, e.touches[0].clientY);
    }, {passive: false});
    window.addEventListener('touchmove', e => {
      if(isDragging) e.preventDefault();
      handleMove(e.touches[0].clientX, e.touches[0].clientY);
    }, {passive: false});
    window.addEventListener('touchend', handleEnd);


    // === 图片加载 ===
    fileInput.addEventListener('change', (e) => {
      if(e.target.files.length === 0) return;
      const reader = new FileReader();
      reader.onload = (event) => {
        const img = new Image();
        img.onload = () => {
          originalImage = img;
          resetPos(); // 加载新图时复位
          document.getElementById('uploadBtn').disabled = false;
        };
        img.src = event.target.result;
      };
      reader.readAsDataURL(e.target.files[0]);
    });

    // === 控制逻辑 ===
    function rotate(deg) {
      if(!originalImage) return;
      currentRotation = (currentRotation + deg) % 360;
      if (currentRotation < 0) currentRotation += 360;
      updatePreview();
    }

    function resetPos() {
      offsetX = 0;
      offsetY = 0;
      updatePreview();
    }

    // === 核心渲染管线 ===
    function updatePreview() {
      if(!originalImage) return;

      const contrast = parseFloat(document.getElementById('contrast').value);
      const brightness = parseInt(document.getElementById('brightness').value);
      const sharpenAmt = parseFloat(document.getElementById('sharpen').value);

      document.getElementById('val-contrast').innerText = contrast;
      document.getElementById('val-brightness').innerText = brightness;
      document.getElementById('val-sharpen').innerText = sharpenAmt;
      document.getElementById('rot-badge').innerText = currentRotation + "°";

      // 1. 基础绘制 (应用 旋转 + 自动填充 + 手动偏移)
      ctxProcess.save();
      ctxProcess.fillStyle = "white";
      ctxProcess.fillRect(0, 0, WIDTH, HEIGHT);

      // A. 移动坐标原点到画布中心 + 手动偏移量
      // 💡 关键点：偏移量加在这里，意味着你是移动"窗口"下的图片
      ctxProcess.translate(WIDTH/2 + offsetX, HEIGHT/2 + offsetY);
      
      // B. 旋转
      ctxProcess.rotate(currentRotation * Math.PI / 180);

      // C. 计算缩放 (Cover模式)
      let isVertical = (currentRotation % 180 !== 0);
      let targetW = isVertical ? HEIGHT : WIDTH;
      let targetH = isVertical ? WIDTH : HEIGHT;
      
      let ratio = Math.max(targetW / originalImage.width, targetH / originalImage.height);
      let drawW = originalImage.width * ratio;
      let drawH = originalImage.height * ratio;

      // D. 绘制 (因为已经Translate到中心了，所以xy是负的一半)
      ctxProcess.imageSmoothingEnabled = true;
      ctxProcess.imageSmoothingQuality = "high";
      ctxProcess.drawImage(originalImage, -drawW/2, -drawH/2, drawW, drawH);
      
      ctxProcess.restore();

      // 2. 图像处理 (对比度/锐化)
      let imgData = ctxProcess.getImageData(0, 0, WIDTH, HEIGHT);
      imgData = applyImageEnhancement(imgData, contrast, brightness, sharpenAmt);
      ctxProcess.putImageData(imgData, 0, 0);

      // 3. 抖动 (生成预览)
      let ditherData = new ImageData(new Uint8ClampedArray(imgData.data), WIDTH, HEIGHT);
      ditherData = applyFloydSteinberg(ditherData);
      ctxFinal.putImageData(ditherData, 0, 0);
    }

    // ... (图像算法函数保持不变) ...
    function applyImageEnhancement(imgData, contrast, brightness, sharpenAmt) {
      const data = imgData.data; const w = imgData.width; const h = imgData.height;
      const intercept = 128 * (1 - contrast);
      const lut = new Uint8Array(256);
      for (let i = 0; i < 256; i++) {
        let val = i * contrast + intercept + brightness;
        lut[i] = Math.min(255, Math.max(0, val));
      }
      for (let i = 0; i < data.length; i += 4) {
        data[i] = lut[data[i]]; data[i+1] = lut[data[i+1]]; data[i+2] = lut[data[i+2]];
      }
      if (sharpenAmt > 0) {
        const output = new Uint8ClampedArray(data); const k = sharpenAmt;
        for (let y = 1; y < h - 1; y++) {
          for (let x = 1; x < w - 1; x++) {
            const idx = (y * w + x) * 4;
            for (let c = 0; c < 3; c++) {
              const val = 5 * data[idx + c]
                -1 * data[((y-1)*w + x) * 4 + c] -1 * data[((y+1)*w + x) * 4 + c]
                -1 * data[(y*w + (x-1)) * 4 + c] -1 * data[(y*w + (x+1)) * 4 + c];
              output[idx + c] = Math.min(255, Math.max(0, val * k + data[idx+c] * (1-k)));
            }
            output[idx+3] = 255;
          }
        }
        return new ImageData(output, w, h);
      }
      return imgData;
    }

    function getNearestColor(r, g, b) {
      let minDist = Infinity; let nearest = PALETTE[1];
      for (let p of PALETTE) {
        let rmean = (r + p.r) / 2;
        let weightR = 2 + rmean/256; let weightG = 4.0; let weightB = 2 + (255-rmean)/256;
        let dist = weightR*(r-p.r)**2 + weightG*(g-p.g)**2 + weightB*(b-p.b)**2;
        if (dist < minDist) { minDist = dist; nearest = p; }
      }
      return nearest;
    }

    function applyFloydSteinberg(imgData) {
      const w = imgData.width; const h = imgData.height; const d = imgData.data;
      for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
          const i = (y * w + x) * 4;
          const oldR = d[i]; const oldG = d[i+1]; const oldB = d[i+2];
          const newCol = getNearestColor(oldR, oldG, oldB);
          d[i] = newCol.r; d[i+1] = newCol.g; d[i+2] = newCol.b;
          const errR = oldR - newCol.r; const errG = oldG - newCol.g; const errB = oldB - newCol.b;
          const distribute = (dx, dy, factor) => {
             const nx = x + dx; const ny = y + dy;
             if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
               const ni = (ny * w + nx) * 4;
               const k = factor / 16;
               d[ni]   = Math.min(255, Math.max(0, d[ni]   + errR * k));
               d[ni+1] = Math.min(255, Math.max(0, d[ni+1] + errG * k));
               d[ni+2] = Math.min(255, Math.max(0, d[ni+2] + errB * k));
             }
          };
          distribute(1, 0, 7); distribute(-1, 1, 3); distribute(0, 1, 5); distribute(1, 1, 1);
        }
      }
      return imgData;
    }

    function upload() {
      const btn = document.getElementById('uploadBtn');
      const status = document.getElementById('status');
      btn.disabled = true;
      status.innerText = "正在打包数据...";

      setTimeout(() => {
        try {
          const imgData = ctxFinal.getImageData(0, 0, WIDTH, HEIGHT);
          const data = imgData.data;
          const byteArray = new Uint8Array(WIDTH * HEIGHT);
          for (let i = 0; i < WIDTH * HEIGHT; i++) {
            byteArray[i] = getNearestColor(data[i*4], data[i*4+1], data[i*4+2]).val;
          }
          const blob = new Blob([byteArray], { type: "application/octet-stream" });
          const formData = new FormData();
          formData.append("file", blob, "gallery.bin");

          const xhr = new XMLHttpRequest();
          xhr.open("POST", "/upload", true);
          xhr.upload.onprogress = (e) => {
             if(e.lengthComputable) status.innerText = `上传中: ${Math.round(e.loaded/e.total*100)}%`;
          };
          xhr.onload = () => {
             btn.disabled = false;
             if(xhr.status === 200) status.innerText = "✅ 成功! 屏幕即将刷新";
             else status.innerText = "❌ 失败: " + xhr.statusText;
          };
          xhr.onerror = () => { btn.disabled = false; status.innerText = "⚠️ 网络错误"; };
          xhr.send(formData);
        } catch(e) {
          btn.disabled = false;
          status.innerText = "Err: " + e.message;
        }
      }, 50);
    }
  </script>
</body>
</html>
)rawliteral";
