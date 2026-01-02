#include "CalendarClient.h"
#include <HTTPClient.h>
#include <WiFi.h>

CalendarClient::CalendarClient(String url) {
  _apiUrl = url;
}

CalendarCell* CalendarClient::getGridData() {
  return _cells;
}

DailyInfo CalendarClient::getDailyInfo() {
  return _info;
}

bool CalendarClient::update(int year, int month, int day) {
  if (WiFi.status() != WL_CONNECTED){  Serial.print("WL_CONNECTED Error: ") ;return false;}

  HTTPClient http;
  String url = _apiUrl + "?year=" + String(year) + "&month=" + String(month) + "&day=" + String(day);
  
  // 增加超时设置，防止网络不好时卡死
  http.setTimeout(5000);
  http.begin(url);
  
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    http.end(); 

    // 因为 key 变长了，JSON 体积会变大，稍微给大点内存
    DynamicJsonDocument doc(24576); // 24KB
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // 1. 解析 Grid (注意 key 变成了 "calendar")
      JsonArray calArr = doc["calendar"];
      for (int i = 0; i < 42; i++) {
        _cells[i].solarDay       = calArr[i]["solarDay"];
        _cells[i].lunarText      = calArr[i]["lunarText"].as<String>();
        _cells[i].status         = calArr[i]["status"];
        _cells[i].isCurrentMonth = calArr[i]["isCurrentMonth"];
        _cells[i].isToday        = calArr[i]["isToday"];
      }

      // 2. 解析 Info
      JsonObject infoObj = doc["info"];
      _info.dateString  = infoObj["dateString"].as<String>();
      _info.weekDay     = infoObj["weekDay"].as<String>();
      _info.lunarString = infoObj["lunarString"].as<String>();
      _info.shengXiao   = infoObj["shengXiao"].as<String>();
      _info.ganZhi      = infoObj["ganZhi"].as<String>();
      _info.yi          = infoObj["yi"].as<String>();
      _info.ji          = infoObj["ji"].as<String>();
      _info.chong       = infoObj["chong"].as<String>();
      _info.sha         = infoObj["sha"].as<String>();
      _info.caiShen     = infoObj["caiShen"].as<String>();

      return true;
    } else {
      Serial.print("JSON Error: "); Serial.println(error.c_str());
      return false;
    }
  } else {
    http.end();
    return false;
  }
}