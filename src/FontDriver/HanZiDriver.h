#ifndef HANZI_DRIVER_H
#define HANZI_DRIVER_H

#include <Arduino.h>
// 引入网页工具生成的字库文件 (文件名必须一致)
#include "UserFont.h"

// 定义画点函数的回调类型
typedef void (*DrawPixelCallback)(int16_t x, int16_t y,uint16_t color);

class HanziDriver {
  private:
    DrawPixelCallback _drawPixel; // 保存用户的画点函数

    // 内部函数：绘制单个字符
    void drawSingleChar(int16_t x, int16_t y, const unsigned char* bitmap,uint16_t color) {
      if (bitmap == NULL) return; // 空指针保护

      int byteIdx = 0;
      // 计算每行占用的字节数 (例如宽32 -> 4字节)
      int bytesPerRow = (FONT_W + 7) / 8;

      for (int j = 0; j < FONT_H; j++) {
        for (int k = 0; k < bytesPerRow; k++) {
          // 从 Flash 读取一个字节
          uint8_t byteVal = pgm_read_byte(&bitmap[byteIdx++]);
          
          for (int bit = 0; bit < 8; bit++) {
            // 检查每一位是否为 1
            if (byteVal & (0x80 >> bit)) {
              // 计算绝对坐标并调用画点
              int pixelX = x + (k * 8) + bit;
              int pixelY = y + j;
              
              // 防止越界绘制 (根据需要可加)
              if (pixelX < x + FONT_W) { 
                 _drawPixel(pixelX, pixelY,color);
              }
            }
          }
        }
      }
    }

  public:
    // 构造函数：初始化时传入画点函数
    HanziDriver(DrawPixelCallback callback) {
      _drawPixel = callback;
    }

    // 核心函数：显示字符串
    void drawText(int16_t x, int16_t y, String str,uint16_t color) {
      int cursorX = x;
      int i = 0;
      int len = str.length();

      while (i < len) {
        uint16_t unicode = 0;
        uint8_t c = str[i];

        // --- UTF-8 解码逻辑 ---
        if (c < 0x80) { // ASCII (英文/数字)
          unicode = c;
          i += 1;
        } else if ((c & 0xE0) == 0xC0) { // 2字节字符
          unicode = ((c & 0x1F) << 6) | (str[i + 1] & 0x3F);
          i += 2;
        } else if ((c & 0xF0) == 0xE0) { // 3字节字符 (常用汉字)
          unicode = ((c & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
          i += 3;
        } else {
          i++; continue; // 跳过异常字节
        }

        // --- 查表 & 绘制 ---
        // getFontBitmap 函数来自 UserFont.h
        const unsigned char* bitmap = getFontBitmap(unicode);

        if (bitmap != NULL) {
          drawSingleChar(cursorX, y, bitmap,color);
          // 判断是否为数字字符 ('0' 到 '9')
          if (unicode >= '0' && unicode <= '9') {
            cursorX += (FONT_W / 2); // 数字：间隔减半
          } else {
            cursorX += (FONT_W-4);       // 其他字符：全宽
          }        } else {
          // 如果字库里没这个字，留半个身位的空格
          cursorX += (FONT_W / 2); 
        }
      }
    }
};

#endif