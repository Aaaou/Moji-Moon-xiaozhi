/*
 * @Date: 2025-01-27
 * @Description: 天气显示模块
 */
#pragma once

#include <string>
#include "city_data.h"

namespace WeatherDisplay {
    struct WeatherInfo {
        char temperature[32];  // 温度
        char text[64];        // 天气描述
        char city[64];        // 城市名称
        bool is_valid;        // 数据是否有效
    };

    // 初始化天气显示模块
    void Init();
    
    // 设置心知天气API密钥
    void SetApiKey(const std::string& key);
    
    // 根据地址获取天气信息
    WeatherInfo GetWeatherByAddress(const std::string& address);
    
    // 获取天气显示字符串
    const char* GetWeatherString();
    
    // 更新天气信息
    void UpdateWeather(const std::string& address);
}
