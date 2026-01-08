#include "CalendarClient.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <config.h>

CalendarClient::CalendarClient(String url)
{
  _apiUrl = url;
}

CalendarCell *CalendarClient::getGridData()
{
  return _cells;
}

DailyInfo CalendarClient::getDailyInfo()
{
  return _info;
}

WeatherInfo CalendarClient::getWeatherInfo()
{
  return _weather;
}
LocationInfo CalendarClient::getLocationInfo()
{
  return _loc_info;
}
bool CalendarClient::update(int year, int month, int day)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Error: WiFi not connected");
    return false;
  }
  String ip = WiFi.localIP().toString();
  HTTPClient http;
  String url = _apiUrl + "?year=" + String(year) + "&month=" + String(month) + "&day=" + String(day) + "&city=" + CITY_CODE;
  Serial.println("url: "+url);

  http.setTimeout(20000);
  http.begin(url);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    http.end();

    // 调试：打印数据长度，确保没超内存
    // Serial.print("Payload size: ");
    // Serial.println(payload.length());

    // 分配 64KB ~ 96KB 内存 (根据你的数据量调整)
    DynamicJsonDocument doc(65536);

    DeserializationError error = deserializeJson(doc, payload);

    if (!error)
    {
      // ============================================
      // 1. 解析 Grid (日历网格)
      // ============================================
      JsonArray calArr = doc["calendar"];
      for (int i = 0; i < 42; i++)
      {
        _cells[i].solarDay = calArr[i]["solarDay"];
        _cells[i].lunarText = calArr[i]["lunarText"].as<String>();
        _cells[i].status = calArr[i]["status"];
        _cells[i].isCurrentMonth = calArr[i]["isCurrentMonth"];
        _cells[i].isToday = calArr[i]["isToday"];
        // 基础日期
        JsonObject infoObj = calArr[i]["lunar_info"];

        _info.dateString = infoObj["dateString"].as<String>();
        _info.weekDay = infoObj["weekDay"].as<String>();
        _info.xingZuo = infoObj["xingZuo"].as<String>();

        // 农历
        _info.lunarString = infoObj["lunarString"].as<String>();
        _info.lunarYear = infoObj["lunarYear"].as<String>();
        _info.lunarMonth = infoObj["lunarMonth"].as<String>();
        _info.lunarDay = infoObj["lunarDay"].as<String>();

        // 生肖
        _info.shengXiao = infoObj["shengXiao"].as<String>();
        _info.shengXiaoFull = infoObj["shengXiaoFull"].as<String>();

        // 干支
        _info.ganZhi = infoObj["ganZhi"].as<String>();
        _info.ganZhiYear = infoObj["ganZhiYear"].as<String>();
        _info.ganZhiMonth = infoObj["ganZhiMonth"].as<String>();
        _info.ganZhiDay = infoObj["ganZhiDay"].as<String>();
        // 干支生肖拼写
        _info.ganZhiShengXiaoFull = infoObj["ganZhiShengXiaoFull"].as<String>();

        // 宜忌
        _info.yi = infoObj["yi"].as<String>();
        _info.ji = infoObj["ji"].as<String>();

        // 文化
        _info.wuHou = infoObj["wuHou"].as<String>();
        _info.jieQi = infoObj["jieQi"].as<String>();
        _info.shuJiu = infoObj["shuJiu"].as<String>();
        _info.fu = infoObj["fu"].as<String>();

        // 风水
        _info.naYin = infoObj["naYin"].as<String>();
        _info.taiShen = infoObj["taiShen"].as<String>();
        _info.chong = infoObj["chong"].as<String>();
        _info.sha = infoObj["sha"].as<String>();
        _info.pengZu = infoObj["pengZu"].as<String>();
        _info.xiShen = infoObj["xiShen"].as<String>();
        _info.caiShen = infoObj["caiShen"].as<String>();
        _info.fuShen = infoObj["fuShen"].as<String>();
        // ⚠️ 注意：既然你服务器代码里去掉了 detail_data/lunar_info
        // 这里就不要再解析它了，否则会报错或数据为空
        // _cells[i].lunarInfo = ... (已移除)
      }

      // ============================================
      // 2. 解析 Info (单日详情) - 手动赋值
      // ============================================
      JsonObject infoObj = doc["info"];

      // 基础日期
      _info.dateString = infoObj["dateString"].as<String>();
      _info.weekDay = infoObj["weekDay"].as<String>();
      _info.xingZuo = infoObj["xingZuo"].as<String>();

      // 农历
      _info.lunarString = infoObj["lunarString"].as<String>();
      _info.lunarYear = infoObj["lunarYear"].as<String>();
      _info.lunarMonth = infoObj["lunarMonth"].as<String>();
      _info.lunarDay = infoObj["lunarDay"].as<String>();

      // 生肖
      _info.shengXiao = infoObj["shengXiao"].as<String>();
      _info.shengXiaoFull = infoObj["shengXiaoFull"].as<String>();

      // 干支
      _info.ganZhi = infoObj["ganZhi"].as<String>();
      _info.ganZhiYear = infoObj["ganZhiYear"].as<String>();
      _info.ganZhiMonth = infoObj["ganZhiMonth"].as<String>();
      _info.ganZhiDay = infoObj["ganZhiDay"].as<String>();

      // 宜忌
      _info.yi = infoObj["yi"].as<String>();
      _info.ji = infoObj["ji"].as<String>();

      // 文化
      _info.wuHou = infoObj["wuHou"].as<String>();
      _info.jieQi = infoObj["jieQi"].as<String>();
      _info.shuJiu = infoObj["shuJiu"].as<String>();
      _info.fu = infoObj["fu"].as<String>();

      // 风水
      _info.naYin = infoObj["naYin"].as<String>();
      _info.taiShen = infoObj["taiShen"].as<String>();
      _info.chong = infoObj["chong"].as<String>();
      _info.sha = infoObj["sha"].as<String>();
      _info.pengZu = infoObj["pengZu"].as<String>();
      _info.xiShen = infoObj["xiShen"].as<String>();
      _info.caiShen = infoObj["caiShen"].as<String>();
      _info.fuShen = infoObj["fuShen"].as<String>();

      // ============================================
      // 3. 解析 Weather (天气) - 手动赋值
      // ============================================
      JsonObject weatherObj = doc["weather"];
      _weather.city = weatherObj["city"].as<String>();
      _weather.currentTemp = weatherObj["current_temp"].as<String>();
      _weather.currentWeather = weatherObj["current_weather"].as<String>();
      _weather.province = weatherObj["province"].as<String>();


      JsonObject locationrObj = doc["loc_info"];
      _loc_info.adcode = weatherObj["adcode"].as<String>();
      _loc_info.city = weatherObj["city"].as<String>();
      _loc_info.province = weatherObj["province"].as<String>();
      _loc_info.rectangle = weatherObj["rectangle"].as<String>();

      JsonArray forecastArr = weatherObj["forecast_list"];
      _weather.forecastCount = 0;

      // 防止数组越界，最多读4个
      int count = forecastArr.size();
      if (count > 4)
        count = 4;

      for (int i = 0; i < count; i++)
      {
        JsonObject f = forecastArr[i];

        _weather.forecasts[i].date = f["date"].as<String>();
        _weather.forecasts[i].week = f["week"].as<String>();
        _weather.forecasts[i].dayWeather = f["dayweather"].as<String>();
        _weather.forecasts[i].nightWeather = f["nightweather"].as<String>();
        _weather.forecasts[i].dayTemp = f["daytemp"].as<String>();
        _weather.forecasts[i].nightTemp = f["nighttemp"].as<String>();
        _weather.forecasts[i].dayWind = f["daywind"].as<String>();
        _weather.forecasts[i].nightWind = f["nightwind"].as<String>();
        _weather.forecasts[i].dayPower = f["daypower"].as<String>();
        _weather.forecasts[i].nightPower = f["nightpower"].as<String>();

        _weather.forecastCount++;
      }

      Serial.println("✅ 数据更新成功");
      return true;
    }
    else
    {
      Serial.print("❌ JSON 解析失败: ");
      Serial.println(error.c_str());
      return false;
    }
  }
  else
  {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}