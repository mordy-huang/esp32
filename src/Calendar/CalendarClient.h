#ifndef CALENDAR_CLIENT_H
#define CALENDAR_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>

// 1. 详细黄历结构体 (放在最前面，因为被后面引用)
struct DailyInfo {
  // --- 基础日期 ---
  String dateString;   
  String weekDay;      
  String xingZuo;      

  // --- 农历基础 ---
  String lunarString;  
  String lunarYear;    
  String lunarMonth;   
  String lunarDay;     

  // --- 生肖 ---
  String shengXiao;    
  String shengXiaoFull;

  // --- 干支 ---
  String ganZhi;       
  String ganZhiYear;   
  String ganZhiMonth;  
  String ganZhiDay;    
  String ganZhiShengXiaoFull;
  // --- 宜忌 ---
  String yi;        
  String ji;        

  // --- 传统文化 ---
  String wuHou;        
  String jieQi;        
  String shuJiu;       
  String fu;           

  // --- 迷信/风水 ---
  String naYin;        
  String taiShen;      
  String chong;        
  String sha;          
  String pengZu;       
  String xiShen;       
  String caiShen;      
  String fuShen;       
};

// 2. 日历格子结构体
struct CalendarCell {
  int solarDay;           
  String lunarText;       
  int status;             
  bool isCurrentMonth;    
  bool isToday;           
  DailyInfo lunarInfo;    // 每个格子都包含一份详细数据
};

// 3. 单天天气预报结构体
struct ForecastItem {
  String date;
  String week;
  String dayWeather;
  String nightWeather;
  String dayTemp;
  String nightTemp;
  String dayWind;
  String nightWind;
  String dayPower;
  String nightPower;
};

// 4. 总天气信息结构体
struct WeatherInfo {
  String city;
  String currentTemp;     
  String currentWeather;  
  String province;  

  ForecastItem forecasts[4]; 
  int forecastCount;         
};
struct LocationInfo  {
  String adcode;
  String city;
  String province;
  String rectangle;
};
class CalendarClient {
  private:
    String _apiUrl; 
    CalendarCell _cells[42];
    DailyInfo _info;
    WeatherInfo _weather;
    LocationInfo  _loc_info;
  public:
    CalendarClient(String url);
    bool update(int year, int month, int day);
    
    CalendarCell* getGridData(); 
    DailyInfo getDailyInfo();
    WeatherInfo getWeatherInfo();
    LocationInfo  getLocationInfo();

};

#endif