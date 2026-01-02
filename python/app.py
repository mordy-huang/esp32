# -*- coding: utf-8 -*-
from flask import Flask, jsonify, request
from datetime import datetime, timedelta, date
from lunar_python import Lunar, Solar
import chinesecalendar   # 必须安装: pip install chinesecalendar

app = Flask(__name__)

# 配置：为了 ESP32 省流量，Grid 数据使用缩写 Key
USE_SHORT_KEYS = False

def get_lunar_text(lunar_obj, solar_obj):
    """
    优先级：农历节日 > 公历节日 > 节气 > 农历日子
    """
    festivals = lunar_obj.getFestivals()
    if festivals: return festivals[0]
    solar_festivals = solar_obj.getFestivals()
    if solar_festivals: return solar_festivals[0]
    jie_qi = lunar_obj.getJieQi()
    if jie_qi: return jie_qi
    return lunar_obj.getDayInChinese()

@app.route('/api/unified', methods=['GET'])
def get_unified_data():
    try:
        now = datetime.now()
        # 获取参数
        year = int(request.args.get('year', now.year))
        month = int(request.args.get('month', now.month))
        target_day_num = int(request.args.get('day', now.day))

        # ==========================================
        # Part 1: 生成 42 个格子的日历数据 (cal)
        # ==========================================
        first_day = datetime(year, month, 1)
        # 寻找日历视图的起始日期 (周一为起始)
        weekday_of_first = first_day.weekday()
        start_date = first_day - timedelta(days=weekday_of_first)
        
        calendar_cells = []
        for i in range(42):
            curr_dt = start_date + timedelta(days=i)
            curr_date = curr_dt.date() # 转为 date 类型给 chinesecalendar 用
            
            # 转为 lunar 对象
            s_obj = Solar.fromYmd(curr_dt.year, curr_dt.month, curr_dt.day)
            l_obj = s_obj.getLunar()
            
            # --- 核心：判断 休/班 逻辑 ---
            # on_holiday: 是否放假 / hol_name: 节日名
            on_holiday, hol_name = chinesecalendar.get_holiday_detail(curr_date)
            
            # s 状态码定义: 0=班(普通), 1=休(周末), 2=休(法定), 3=班(调休)
            status_code = 0 
            
            if on_holiday:
                if hol_name is not None:
                    status_code = 2 # 法定节假日 -> 画红圈 "休"
                else:
                    status_code = 1 # 普通周末 -> 画红字
            else:
                # 如果不放假，但今天是周六日，说明是补班
                if curr_dt.weekday() >= 5:
                    status_code = 3 # 调休补班 -> 画灰圈 "班"
                else:
                    status_code = 0 # 普通工作日 -> 画黑字

            # --- 组装 Grid 数据 ---
            l_str = get_lunar_text(l_obj, s_obj)
            is_cm = (curr_dt.month == month)
            is_t = (curr_date == now.date())

            if USE_SHORT_KEYS:
                calendar_cells.append({
                    "d": curr_dt.day,       # 公历日
                    "l": l_str,             # 农历/节日显示文本
                    "cm": 1 if is_cm else 0,# 是否本月
                    "t": 1 if is_t else 0,  # 是否今天
                    "s": status_code        # 【重要】休班状态码
                })
            else:
                calendar_cells.append({
                    "day": curr_dt.day, "lunar": l_str, "status": status_code
                })

        # ==========================================
        # Part 2: 生成选中日期的全量黄历详情 (info)
        # ==========================================
        try:
            target_solar = Solar.fromYmd(year, month, target_day_num)
        except:
            # 防止日期越界 (如2月30日)，默认回退到1号
            target_solar = Solar.fromYmd(year, month, 1)
            
        lunar = target_solar.getLunar()
        
        detail_data = {
            # --- 基础显示 ---
            "date_str": target_solar.toYmd(),
            "week": "星期" + target_solar.getWeekInChinese(),
            "lunar_str": f"{lunar.getMonthInChinese()}月{lunar.getDayInChinese()}",
            
            # --- 干支与生肖 ---
            "gz_year": lunar.getYearInGanZhi(),  # 乙巳
            "gz_month": lunar.getMonthInGanZhi(),# 戊子
            "gz_day": lunar.getDayInGanZhi(),    # 乙丑
            "sx": lunar.getYearShengXiao(),      # 蛇
            "nayin": lunar.getDayNaYin(),        # 海中金
            
            # --- 宜忌 (截取前8个，防爆屏) ---
            "yi": " ".join(lunar.getDayYi()[:8]), 
            "ji": " ".join(lunar.getDayJi()[:8]),
            
            # --- 核心神煞 ---
            "chong": f"{lunar.getDayChongDesc()}", # 冲羊
            "sha": f"{lunar.getDaySha()}",         # 煞东
            "pos_cai": lunar.getDayPositionCaiDesc(), # 财神方位
            "pos_xi": lunar.getDayPositionXiDesc(),   # 喜神方位
            "pos_fu": lunar.getDayPositionFuDesc(),   # 福神方位
            
            # --- 进阶信息 (可选显示) ---
            "taishen": lunar.getDayTaiShen(), # 胎神
            "xiu": f"{lunar.getXiu()}宿",     # 星宿
            "zhixing": lunar.getZhiXing(),    # 建除 (建、除、满...)
            "wuhou": lunar.getWuHou()         # 物候
        }

        # ==========================================
        # 返回最终 JSON
        # ==========================================
        return jsonify({
            "year": year,
            "month": month,
            "cal": calendar_cells,
            "info": detail_data
        })

    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    # 允许局域网访问
    app.run(host='0.0.0.0', port=5000, debug=True)