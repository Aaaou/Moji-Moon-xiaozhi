/*
 * @Date: 2025-01-27
 * @Description: 地址显示模块，用于替换农历显示功能
 */
#pragma once

#include <string>
#include <ctime>

namespace LocationDisplay {
    // 初始化地址显示模块
    void Init();
    
    // 获取当前地址信息
    std::string GetCurrentLocation();
    
    // 更新地址信息
    void UpdateLocation();
    
    // 设置腾讯地图API密钥
    void SetApiKey(const std::string& key);
    
    // 获取地址字符串（用于显示）
    std::string GetLocationString();
}
