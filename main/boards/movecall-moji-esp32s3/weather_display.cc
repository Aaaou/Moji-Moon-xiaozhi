/*
 * @Date: 2025-01-27
 * @Description: 天气显示模块实现
 */
#include "weather_display.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <wifi_station.h>
#include <cstring>
#include <string>

static const char* TAG = "WeatherDisplay";

namespace WeatherDisplay {

static std::string api_key = "";
static WeatherInfo current_weather = {{0}, {0}, {0}, false};
static char weather_string[128] = "获取中...";
static bool is_initialized = false;
static esp_http_client_handle_t http_client = nullptr;

// HTTP响应回调函数
esp_err_t weather_http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP请求错误");
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP请求完成");
            break;
        default:
            break;
    }
    return ESP_OK;
}

void Init() {
    ESP_LOGI(TAG, "开始初始化天气显示模块");
    if (is_initialized) {
        ESP_LOGI(TAG, "天气显示模块已经初始化过");
        return;
    }
    
    // 配置HTTP客户端
    esp_http_client_config_t config = {};
    config.url = "http://api.seniverse.com/v3/weather/now.json";  // 使用 HTTP
    config.event_handler = weather_http_event_handler;
    config.timeout_ms = 10000;
    config.is_async = false;
    config.disable_auto_redirect = false;
    
    ESP_LOGI(TAG, "正在初始化天气HTTP客户端...");
    http_client = esp_http_client_init(&config);
    if (http_client == nullptr) {
        ESP_LOGE(TAG, "天气HTTP客户端初始化失败");
        return;
    }
    
    is_initialized = true;
    ESP_LOGI(TAG, "天气显示模块初始化完成");
}

void SetApiKey(const std::string& key) {
    api_key = key;
    ESP_LOGI(TAG, "天气API密钥已设置");
}

WeatherInfo GetWeatherByAddress(const std::string& address) {
    WeatherInfo weather = {{0}, {0}, {0}, false};
    strncpy(weather.text, "获取中...", sizeof(weather.text) - 1);
    weather.text[sizeof(weather.text) - 1] = '\0';
    
    if (!is_initialized || !http_client) {
        ESP_LOGE(TAG, "天气服务未初始化");
        strncpy(weather.text, "服务未初始化", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    if (api_key.empty()) {
        ESP_LOGE(TAG, "天气API密钥未设置");
        strncpy(weather.text, "API密钥未设置", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    // 检查WiFi连接状态
    if (!WifiStation::GetInstance().IsConnected()) {
        ESP_LOGW(TAG, "WiFi未连接，无法获取天气");
        strncpy(weather.text, "WiFi未连接", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    // 从地址中提取城市名
    std::string city = "广东/深圳/南山";  // 默认格式
    
    // 解析地址格式 "广东省 深圳市 南山区"
    size_t province_pos = address.find("省");
    size_t city_pos = address.find("市");
    size_t district_pos = address.find("区");
    
    if (province_pos != std::string::npos && 
        city_pos != std::string::npos && 
        district_pos != std::string::npos) {
        
        std::string province = address.substr(0, province_pos);  // 广东
        
        // 找到省和市之间的城市名
        size_t city_start = address.find(" ", province_pos) + 1;
        std::string city_name = address.substr(city_start, city_pos - city_start);  // 深圳
        
        // 找到区名
        size_t district_start = address.find(" ", city_pos) + 1;
        std::string district_name = address.substr(district_start, district_pos - district_start);  // 南山
        
        // 构建符合 city_data.h 格式的城市名: "广东/深圳/南山"
        city = province + "/" + city_name + "/" + district_name;
        ESP_LOGI(TAG, "构建城市名: %s", city.c_str());
    }
    
    if (city.empty()) {
        ESP_LOGW(TAG, "无法从地址 '%s' 中提取城市名", address.c_str());
        strncpy(weather.text, "地址解析失败", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    // 查找城市经纬度
    double lat, lon;
    if (!FindCityByName(city.c_str(), lat, lon)) {
        ESP_LOGW(TAG, "未找到城市 '%s' 的经纬度", city.c_str());
        strncpy(weather.text, "未找到城市", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    // 构建请求URL
    char url[512];
    snprintf(url, sizeof(url), 
             "http://api.seniverse.com/v3/weather/now.json?key=%s&location=%.4f:%.4f&language=zh-Hans&unit=c",
             api_key.c_str(), lat, lon);
    
    // 准备接收数据的缓冲区
    char* buffer = (char*)malloc(2048);
    if (!buffer) {
        ESP_LOGE(TAG, "内存分配失败");
        strncpy(weather.text, "内存不足", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    // 发送请求
    esp_http_client_set_url(http_client, url);
    esp_http_client_set_method(http_client, HTTP_METHOD_GET);
    
    ESP_LOGI(TAG, "发送天气请求到: %s", url);
    
    esp_err_t err = esp_http_client_open(http_client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "天气HTTP连接失败: %s", esp_err_to_name(err));
        free(buffer);
        strncpy(weather.text, "连接失败", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    esp_http_client_fetch_headers(http_client);
    int status_code = esp_http_client_get_status_code(http_client);
    ESP_LOGI(TAG, "天气HTTP状态码: %d", status_code);
    
    if (status_code != 200) {
        ESP_LOGE(TAG, "天气HTTP状态码错误: %d", status_code);
        esp_http_client_close(http_client);
        free(buffer);
        strncpy(weather.text, "服务器错误", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    // 读取响应数据
    int total_read = 0;
    int read_len = 0;
    while (total_read < 2047) {
        read_len = esp_http_client_read(http_client, buffer + total_read, 2047 - total_read);
        if (read_len <= 0) break;
        total_read += read_len;
    }
    buffer[total_read] = '\0';
    
    esp_http_client_close(http_client);
    
    // 解析JSON响应
    cJSON *json = cJSON_Parse(buffer);
    if (!json) {
        ESP_LOGE(TAG, "天气JSON解析失败");
        free(buffer);
        strncpy(weather.text, "数据解析失败", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    cJSON *results = cJSON_GetObjectItem(json, "results");
    if (!results || !cJSON_IsArray(results) || cJSON_GetArraySize(results) == 0) {
        ESP_LOGE(TAG, "天气响应格式错误");
        cJSON_Delete(json);
        free(buffer);
        strncpy(weather.text, "响应格式错误", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    cJSON *result = cJSON_GetArrayItem(results, 0);
    if (!result) {
        ESP_LOGE(TAG, "天气结果为空");
        cJSON_Delete(json);
        free(buffer);
        strncpy(weather.text, "结果为空", sizeof(weather.text) - 1);
        weather.text[sizeof(weather.text) - 1] = '\0';
        return weather;
    }
    
    // 获取位置信息
    cJSON *location = cJSON_GetObjectItem(result, "location");
    if (location) {
        cJSON *name = cJSON_GetObjectItem(location, "name");
        if (name && name->valuestring) {
            strncpy(weather.city, name->valuestring, sizeof(weather.city) - 1);
            weather.city[sizeof(weather.city) - 1] = '\0';
        }
    }
    
    // 获取天气信息
    cJSON *now = cJSON_GetObjectItem(result, "now");
    if (now) {
        cJSON *temperature = cJSON_GetObjectItem(now, "temperature");
        cJSON *text = cJSON_GetObjectItem(now, "text");
        
        if (temperature && temperature->valuestring) {
            snprintf(weather.temperature, sizeof(weather.temperature), "%s°C", temperature->valuestring);
        }
        
        if (text && text->valuestring) {
            strncpy(weather.text, text->valuestring, sizeof(weather.text) - 1);
            weather.text[sizeof(weather.text) - 1] = '\0';
        }
        
        weather.is_valid = true;
    }
    
    cJSON_Delete(json);
    free(buffer);
    
    current_weather = weather;
        ESP_LOGI(TAG, "获取到天气: %s %s %s",
             weather.city, weather.temperature, weather.text);
    
    return weather;
}

void UpdateWeather(const std::string& address) {
    GetWeatherByAddress(address);
}

const char* GetWeatherString() {
    if (!current_weather.is_valid) {
        return current_weather.text;
    }
    snprintf(weather_string, sizeof(weather_string), "%s %s", 
             current_weather.temperature, current_weather.text);
    return weather_string;
}

} // namespace WeatherDisplay
