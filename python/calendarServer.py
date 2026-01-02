# -*- coding: utf-8 -*-
from flask import Flask, jsonify, request
from datetime import datetime, timedelta, date
from lunar_python import Lunar, Solar
from chinese_calendar import is_holiday, is_workday,get_holiday_detail


app = Flask(__name__)

# === 配置：使用全名 ===
USE_SHORT_KEYS = False 

def get_lunar_text(lunar_obj, solar_obj):
    festivals = lunar_obj.getFestivals()
    if festivals: return festivals[0]
    solar_festivals = solar_obj.getFestivals()
    if solar_festivals: return solar_festivals[0]
    jie_qi = lunar_obj.getJieQi()
    if jie_qi: return jie_qi
    return lunar_obj.getDayInChinese()

@app.route('/api/getCalendar', methods=['GET'])
def get_unified_data():
    try:
        now = datetime.now()
        year = int(request.args.get('year', now.year))
        month = int(request.args.get('month', now.month))
        target_day_num = int(request.args.get('day', now.day))

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
                status_code = 2 if hol_name else 1 # 2=法定休, 1=周末
            else:
                status_code = 3 if curr_dt.weekday() >= 5 else 0 # 3=补班, 0=工作日

            l_str = get_lunar_text(l_obj, s_obj)
            is_cm = (curr_dt.month == month)
            is_t = (curr_date == now.date())

            # === 这里改为全名 Key ===
            calendar_cells.append({
                "solarDay": curr_dt.day,          # 公历日期
                "lunarText": l_str,               # 农历文本
                "isCurrentMonth": is_cm,          # 是否本月
                "isToday": is_t,                  # 是否今天
                "status": status_code             # 休班状态
            })

        # --- 2. 生成详情数据 ---
        try:
            target_solar = Solar.fromYmd(year, month, target_day_num)
        except:
            target_solar = Solar.fromYmd(year, month, 1)
        lunar = target_solar.getLunar()
        
        # === 详情也使用全名 ===
        detail_data = {
            "dateString": target_solar.toYmd(),
            "weekDay": "星期" + target_solar.getWeekInChinese(),
            "lunarString": f"{lunar.getMonthInChinese()}月{lunar.getDayInChinese()}",
            "ganZhi": f"{lunar.getYearInGanZhi()} {lunar.getMonthInGanZhi()} {lunar.getDayInGanZhi()}",
            "shengXiao": lunar.getYearShengXiao(),
            "naYin": lunar.getDayNaYin(),
            "yi": " ".join(lunar.getDayYi()[:8]),
            "ji": " ".join(lunar.getDayJi()[:8]),
            "chong": f"{lunar.getDayChongDesc()}",
            "sha": f"{lunar.getDaySha()}",
            "caiShen": lunar.getDayPositionCaiDesc(),
            "xiShen": lunar.getDayPositionXiDesc(),
            "fuShen": lunar.getDayPositionFuDesc(),
            "tianShen": lunar.getDayTianShen(),
            "xingXiu": f"{lunar.getXiu()}宿",
            "zhiXing": lunar.getZhiXing(),
            "wuHou": lunar.getWuHou()
        }

        return jsonify({
            "year": year,
            "month": month,
            "calendar": calendar_cells, # 这里的key我也改全名了 cal -> calendar
            "info": detail_data
        })

    except Exception as e:
        print(e)
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)