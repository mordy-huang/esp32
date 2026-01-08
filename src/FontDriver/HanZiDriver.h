#ifndef HANZI_DRIVER_H
#define HANZI_DRIVER_H

#include <Arduino.h>
#include "UserFont.h" // 确保这里包含了上面的 MyFontDef 定义

typedef void (*DrawPixelCallback)(int16_t x, int16_t y, uint16_t color);

class HanziDriver
{
private:
  DrawPixelCallback _drawPixel;
  const MyFontDef *currentFont; // 👈 核心：保存当前使用的字体指针

  // 内部函数：绘制单个字符 (改为动态宽高)
  void drawSingleChar(int16_t x, int16_t y, const unsigned char *bitmap, uint16_t color)
  {
    if (bitmap == NULL || currentFont == NULL)
      return;

    int byteIdx = 0;
    // ⚠️ 改动1：使用 currentFont->w 计算
    int bytesPerRow = (currentFont->w + 7) / 8;

    // ⚠️ 改动2：使用 currentFont->h 循环
    for (int j = 0; j < currentFont->h; j++)
    {
      for (int k = 0; k < bytesPerRow; k++)
      {
        uint8_t byteVal = pgm_read_byte(&bitmap[byteIdx++]);
        for (int bit = 0; bit < 8; bit++)
        {
          if (byteVal & (0x80 >> bit))
          {
            int pixelX = x + (k * 8) + bit;
            int pixelY = y + j;
            // ⚠️ 改动3：边界检查用 currentFont->w
            if (pixelX < x + currentFont->w)
            {
              if (color == EPD_GRAY)
              { // 假设 4 是 EPD_GRAY
                // 检查坐标之和的奇偶性，形成网格纹理
                if ((pixelX + pixelY) % 2 == 0)
                {
                  _drawPixel(pixelX, pixelY, EPD_BLACK); // 画黑点
                }
                else
                {
                  _drawPixel(pixelX, pixelY, EPD_WHITE); // 画白点
                }
              }
              else if (color == EPD_ORANGE)
              { // 5 代表橙色
                // 🟠 橙色混色算法：红黄交替
                if ((pixelX + pixelY) % 2 == 0)
                {
                  _drawPixel(pixelX, pixelY, EPD_RED); // 偶数点画红
                }
                else
                {
                  _drawPixel(pixelX, pixelY, EPD_YELLOW); // 奇数点画黄
                }
              }
              else if (color == EPD_PICK)
              { // 假设 6 代表粉色
                // 🌸 粉色混色算法：红白交替
                if ((pixelX + pixelY) % 2 == 0)
                {
                  _drawPixel(pixelX, pixelY, EPD_RED);
                }
                else
                {
                  _drawPixel(pixelX, pixelY, EPD_WHITE);
                }
              }
              else if (color == EPD_DARK_YELLOW)
              { // 7 代表暗黄
                // 🔘 灰色混色算法：黑黄交替
                if ((pixelX + pixelY) % 2 == 0)
                {
                  _drawPixel(pixelX, pixelY, EPD_BLACK);
                }
                else
                {
                  _drawPixel(pixelX, pixelY, EPD_YELLOW);
                }
              }
              else if (color == EPD_DARK_RED)
              { // 假设 8 代表暗红
                // 🌸 粉色混色算法：红黑交替
                if ((pixelX + pixelY) % 2 == 0)
                {
                  _drawPixel(pixelX, pixelY, EPD_RED);
                }
                else
                {
                  _drawPixel(pixelX, pixelY, EPD_BLACK);
                }
              }
              else
              {
                // 正常颜色
                _drawPixel(pixelX, pixelY, color);
              }
            }
          }
        }
      }
    }
  }

public:
  HanziDriver(DrawPixelCallback callback)
  {
    _drawPixel = callback;
    currentFont = NULL; // 初始化为空，强制用户 setFont
  }

  // ✅ 新增：设置字体
  void setFont(const MyFontDef &font)
  {
    currentFont = &font;
  }

  void drawText(int16_t x, int16_t y, String str, uint16_t color)
  {
    if (currentFont == NULL)
      return; // 防止未设置字体就调用

    int cursorX = x;
    int i = 0;
    int len = str.length();

    // 从结构体里取出当前字体的参数，方便后面写代码
    int fontW = currentFont->w;

    while (i < len)
    {
      uint16_t unicode = 0;
      uint8_t c = str[i];
      int byteLen = 0;

      // --- UTF-8 解码 ---
      if (c < 0x80)
      {
        unicode = c;
        byteLen = 1;
      }
      else if ((c & 0xE0) == 0xC0)
      {
        unicode = ((c & 0x1F) << 6) | (str[i + 1] & 0x3F);
        byteLen = 2;
      }
      else if ((c & 0xF0) == 0xE0)
      {
        unicode = ((c & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
        byteLen = 3;
      }
      else
      {
        i++;
        continue;
      }

      // ⚠️ 改动4：调用函数指针 currentFont->getBitmap
      const unsigned char *bitmap = currentFont->getBitmap(unicode);

      if (bitmap != NULL)
      {
        drawSingleChar(cursorX, y, bitmap, color);
      }

      // --- 间距逻辑 (改用 struct 里的值) ---
      int advanceWidth = 0;
      int gap = 0;

      bool isCurrentAscii = (unicode < 128);
      if (isCurrentAscii)
      {
        advanceWidth = fontW / 2; // 数字算半宽
      }
      else
      {
        advanceWidth = fontW; // 中文算全宽
      }

      // Peek Next 逻辑
      bool isNextAscii = false;
      if (i + byteLen < len)
      {
        uint8_t nextC = str[i + byteLen];
        isNextAscii = (nextC < 0x80);
      }
      else
      {
        isNextAscii = isCurrentAscii;
      }

      // ⚠️ 改动5：间距使用 currentFont->gap_xxx
      if (isCurrentAscii && isNextAscii)
      {
        gap = currentFont->gap_ascii;
      }
      else if (!isCurrentAscii && !isNextAscii)
      {
        gap = currentFont->gap_hanzi;
      }
      else if (isCurrentAscii && !isNextAscii)
      {
        // 数字 -> 中文 (2月)
        gap = currentFont->gap_ascii_hanzi;
      }
      else
      {
        // 中文 -> 数字 (年2)
        gap = currentFont->gap_hanzi_ascii;
      }

      cursorX += (advanceWidth + gap);
      i += byteLen;
    }
  }
};

#endif