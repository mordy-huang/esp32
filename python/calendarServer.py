# -*- coding: utf-8 -*-
from flask import Flask, jsonify, request
from datetime import datetime, timedelta, date
from lunar_python import Lunar, Solar
from chinese_calendar import is_holiday, is_workday, get_holiday_detail
import requests 

app = Flask(__name__)

# === 配置区域 ===
# 【重要】请在这里替换为你申请的高德地图 Web服务 Key
AMAP_KEY = "c441e39f88ac93698c4e758e59a3dcba" 

# 默认城市编码 (例如: 110000 是北京，440300 是深圳)
DEFAULT_ADCODE = "110000" 

def get_lunar_text(lunar_obj, solar_obj):
    festivals = lunar_obj.getFestivals()
    if festivals: return festivals[0]
    solar_festivals = solar_obj.getFestivals()
    if solar_festivals: return solar_festivals[0]
    jie_qi = lunar_obj.getJieQi()
    if jie_qi: return jie_qi
    return lunar_obj.getDayInChinese()

def get_weather_data(adcode):
    """
    获取天气数据：包括实时温度、天气现象、以及当天的最高/最低温
    """
    # 如果没填Key或者Key是默认提示文案，直接返回None
    if not AMAP_KEY or "你的高德" in AMAP_KEY:
        return None 

    weather_info = {
        "temp": "N/A",      # 当前温度
        "weather": "N/A",   # 天气现象
        "low": "N/A",       # 最低温
        "high": "N/A",      # 最高温
        "city": "未知"
    }
    
    try:
        # 1. 获取实况天气 (base)
        url_base = f"https://restapi.amap.com/v3/weather/weatherInfo?city={adcode}&key={AMAP_KEY}&extensions=base"
        res_base = requests.get(url_base, timeout=2).json()
        
        if res_base['status'] == '1' and res_base['lives']:
            live = res_base['lives'][0]
            weather_info['temp'] = live['temperature']
            weather_info['weather'] = live['weather']
            weather_info['city'] = live['city']

        # 2. 获取预报天气 (all) - 用于显示高低温
        url_all = f"https://restapi.amap.com/v3/weather/weatherInfo?city={adcode}&key={AMAP_KEY}&extensions=all"
        res_all = requests.get(url_all, timeout=2).json()
        
        if res_all['status'] == '1' and res_all['forecasts']:
            today_forecast = res_all['forecasts'][0]['casts'][0]
            weather_info['low'] = today_forecast['nighttemp']
            weather_info['high'] = today_forecast['daytemp']
            
            # 补全天气现象
            if weather_info['weather'] == "N/A":
                weather_info['weather'] = today_forecast['dayweather']

    except Exception as e:
        print(f"Weather API Error: {e}")
        return None
    
    return weather_info

@app.route('/api/getCalendar', methods=['GET'])
def get_unified_data():
    try:
        now = datetime.now()
        year = int(request.args.get('year', now.year))
        month = int(request.args.get('month', now.month))
        target_day_num = int(request.args.get('day', now.day))
        
        # 获取城市参数
        adcode = request.args.get('city', DEFAULT_ADCODE)

        # --- 1. 生成网格数据 ---
        first_day = datetime(year, month, 1)
        weekday_of_first = first_day.weekday()
        start_date = first_day - timedelta(days=weekday_of_first)
        
        calendar_cells = []
        for i in range(42):
            curr_dt = start_date + timedelta(days=i)
            curr_date = curr_dt.date()
            
            s_obj = Solar.fromYmd(curr_dt.year, curr_dt.month, curr_dt.day)
            l_obj = s_obj.getLunar()
            
            # 休班逻辑
            on_holiday, hol_name = get_holiday_detail(curr_date)
            status_code = 0 
            if on_holiday:
                status_code = 2 if hol_name else 1 
            else:
                status_code = 3 if curr_dt.weekday() >= 5 else 0 

            l_str = get_lunar_text(l_obj, s_obj)
            is_cm = (curr_dt.month == month)
            is_t = (curr_date == now.date())

            calendar_cells.append({
                "solarDay": curr_dt.day,          
                "lunarText": l_str,               
                "isCurrentMonth": is_cm,          
                "isToday": is_t,                  
                "status": status_code             
            })

        # --- 2. 生成详情数据 ---
        try:
            target_solar = Solar.fromYmd(year, month, target_day_num)
        except:
            target_solar = Solar.fromYmd(year, month, 1)
        lunar = target_solar.getLunar()
        
        detail_data = {
            "dateString": target_solar.toYmd(),
            "weekDay": "星期" + target_solar.getWeekInChinese(),
            "lunarString": f"{lunar.getMonthInChinese()}月{lunar.getDayInChinese()}",
            "ganZhi": f"{lunar.getYearInGanZhi()} {lunar.getMonthInGanZhi()} {lunar.getDayInGanZhi()}",
            "yi": " ".join(lunar.getDayYi()[:6]), # 截取前6个避免太长
            "ji": " ".join(lunar.getDayJi()[:6]),
            "wuHou": lunar.getWuHou()
        }

        # --- 3. 获取天气 ---
        weather_data = get_weather_data(adcode)

        return jsonify({
            "year": year,
            "month": month,
            "calendar": calendar_cells,
            "info": detail_data,
            "weather": weather_data
        })

    except Exception as e:
        print(e)
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)