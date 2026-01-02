#ifndef CALENDAR_CLIENT_H
#define CALENDAR_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>

// 1. 日历格子结构体 (使用全名)
struct CalendarCell {
  int solarDay;           // 公历日期
  String lunarText;       // 农历/节日
  int status;             // 0=班, 1=休, 2=法定休, 3=补班
  bool isCurrentMonth;    // 是否本月
  bool isToday;           // 是否今天
};

// 2. 详细黄历结构体 (使用全名)
struct DailyInfo {
  String dateString;   
  String weekDay;      
  String lunarString;  
  String shengXiao;    // 生肖
  String ganZhi;       // 干支
  String yi;        
  String ji;        
  String chong;     
  String sha;       
  String caiShen;
  // ... 其他你想要显示的字段可以在这里加
};

class CalendarClient {
  private:
    String _apiUrl; 
    CalendarCell _cells[42];
    DailyInfo _info;

  public:
    CalendarClient(String url);
    bool update(int year, int month, int day);
    CalendarCell* getGridData(); 
    DailyInfo getDailyInfo();
};

#endif