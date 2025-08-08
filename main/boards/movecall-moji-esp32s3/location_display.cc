#include "location_display.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <wifi_station.h>
#include <cstring>
#include <string>

static const char* TAG = "LocationDisplay";

namespace LocationDisplay {

static std::string api_key = "你的key";
static std::string current_location = "正在获取地址...";
static bool is_initialized = false;
static esp_http_client_handle_t http_client = nullptr;

// HTTP响应回调函数
esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                // 这里可以处理响应数据，但在这个实现中我们使用同步方式
            }
            break;
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
        ESP_LOGI(TAG, "开始初始化地址显示模块");
        if (is_initialized) {
            ESP_LOGI(TAG, "地址显示模块已经初始化过");
            return;
        }
        
        // 配置HTTP客户端（使用HTTP协议）
        esp_http_client_config_t config = {};
        config.url = "http://apis.map.qq.com/ws/location/v1/ip";
        config.event_handler = http_event_handler;
        config.timeout_ms = 10000; // 10秒超时
        config.is_async = false; // 使用同步模式
        config.disable_auto_redirect = false;
        
        ESP_LOGI(TAG, "正在初始化HTTP客户端...");
        http_client = esp_http_client_init(&config);
        if (http_client == nullptr) {
            ESP_LOGE(TAG, "HTTP客户端初始化失败");
            return;
        }
        
        is_initialized = true;
        ESP_LOGI(TAG, "地址显示模块初始化完成");
    }

void SetApiKey(const std::string& key) {
    api_key = key;
    ESP_LOGI(TAG, "API密钥已设置: %s", key.c_str());
}

std::string GetCurrentLocation() {
    ESP_LOGI(TAG, "开始获取地址信息");
    if (!is_initialized || !http_client) {
        ESP_LOGE(TAG, "地址服务未初始化 - is_initialized: %s, http_client: %s", 
                 is_initialized ? "true" : "false", 
                 http_client ? "valid" : "null");
        return "地址服务未初始化";
    }
    
    // 检查WiFi连接状态
    if (!WifiStation::GetInstance().IsConnected()) {
        ESP_LOGW(TAG, "WiFi未连接，无法获取地址");
        return "WiFi未连接";
    }
    
            // 构建请求URL（使用HTTP避免SSL问题）
        std::string url = "http://apis.map.qq.com/ws/location/v1/ip?key=" + api_key;
    
            esp_http_client_set_url(http_client, url.c_str());
        esp_http_client_set_method(http_client, HTTP_METHOD_GET);
        
        ESP_LOGI(TAG, "发送HTTP请求到: %s", url.c_str());
        
        // 准备接收数据的缓冲区
        char* buffer = (char*)malloc(4096); // 分配固定大小的缓冲区
        if (!buffer) {
            ESP_LOGE(TAG, "内存分配失败");
            return "内存分配失败";
        }
        
        // 发送请求并直接读取响应
        esp_http_client_set_method(http_client, HTTP_METHOD_GET);
        esp_err_t err = esp_http_client_open(http_client, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP连接失败: %s", esp_err_to_name(err));
            free(buffer);
            return "连接失败";
        }
        
        esp_http_client_fetch_headers(http_client);
        int status_code = esp_http_client_get_status_code(http_client);
        ESP_LOGI(TAG, "HTTP状态码: %d", status_code);
        
        if (status_code != 200) {
            ESP_LOGE(TAG, "HTTP状态码错误: %d", status_code);
            esp_http_client_close(http_client);
            free(buffer);
            return "服务器响应错误";
        }
        
        // 读取响应数据
        int total_read = 0;
        int read_len = 0;
        while (total_read < 4095) {
            read_len = esp_http_client_read(http_client, buffer + total_read, 4095 - total_read);
            if (read_len <= 0) break;
            total_read += read_len;
        }
        buffer[total_read] = '\0';
        
        esp_http_client_close(http_client);
        

        
        // 检查是否读取到有效数据
        if (total_read == 0) {
            ESP_LOGE(TAG, "没有读取到响应数据");
            free(buffer);
            return "没有响应数据";
        }
    
        // 解析JSON响应
        cJSON *json = cJSON_Parse(buffer);
        if (!json) {
            ESP_LOGE(TAG, "JSON解析失败，原始数据: %s", buffer);
            free(buffer);
            return "数据解析失败";
        }
    
    // 检查状态码
    cJSON *status = cJSON_GetObjectItem(json, "status");
    if (!status || status->valueint != 0) {
        ESP_LOGE(TAG, "API返回错误状态: %d", status ? status->valueint : -1);
        cJSON_Delete(json);
        free(buffer);
        return "API返回错误";
    }
    
    // 获取地址信息
    cJSON *result = cJSON_GetObjectItem(json, "result");
    if (!result) {
        ESP_LOGE(TAG, "响应中缺少result字段");
        cJSON_Delete(json);
        free(buffer);
        return "响应格式错误";
    }
    
    cJSON *ad_info = cJSON_GetObjectItem(result, "ad_info");
    if (!ad_info) {
        ESP_LOGE(TAG, "响应中缺少ad_info字段");
        cJSON_Delete(json);
        free(buffer);
        return "地址信息缺失";
    }
    
    // 构建地址字符串
    std::string location_str = "";
    
    cJSON *province = cJSON_GetObjectItem(ad_info, "province");
    cJSON *city = cJSON_GetObjectItem(ad_info, "city");
    cJSON *district = cJSON_GetObjectItem(ad_info, "district");
    
    if (province && province->valuestring) {
        location_str += province->valuestring;
    }
    
    if (city && city->valuestring) {
        if (!location_str.empty()) {
            location_str += " ";
        }
        location_str += city->valuestring;
    }
    
    if (district && district->valuestring) {
        if (!location_str.empty()) {
            location_str += " ";
        }
        location_str += district->valuestring;
    }
    
    if (location_str.empty()) {
        location_str = "未知地址";
    }
    
    cJSON_Delete(json);
    free(buffer);
    
    current_location = location_str;
    ESP_LOGI(TAG, "获取到地址: %s", location_str.c_str());
    
    return location_str;
}

void UpdateLocation() {
    GetCurrentLocation();
}

std::string GetLocationString() {
    return current_location;
}

} // namespace LocationDisplay
