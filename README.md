# 🌍 小智 Moji 地址天气显示功能

<div align="center">

![ESP32](https://img.shields.io/badge/ESP32-S3-red?style=for-the-badge&logo=espressif)
![LVGL](https://img.shields.io/badge/LVGL-9.2.2-blue?style=for-the-badge)
![C++](https://img.shields.io/badge/C++-17-green?style=for-the-badge&logo=c%2B%2B)
![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)

**为小智 Moji ESP32-S3 设备添加实时地址和天气显示功能，提供 Moon 同款表盘样式**

[📖 功能特性](#功能特性) • [⚙️ 快速配置](#快速配置) • [🔧 安装指南](#安装指南) • [📊 API 信息](#api-服务信息) • [❓ 故障排除](#故障排除)

</div>

---

## 🚀 功能概述

本项目将原有的农历显示功能升级为智能地址和天气显示系统，为小智 Moji ESP32-S3 设备提供 Moon 同款表盘样式：

- 📍 **实时地址定位** - 基于腾讯地图 IP 定位 API
- 🌤️ **实时天气信息** - 基于心知天气 API  
- ⏰ **智能显示切换** - 15秒地址/20秒天气交替显示
- 🔄 **自动更新机制** - 开机更新一次，后续每2小时自动更新
- 🗺️ **内置城市数据库** - 包含3218个城市的精确坐标
- 🎨 **Moon 表盘样式** - 采用与 Moon 设备相同的界面设计和显示风格

## ✨ 功能特性

### 📱 显示逻辑
| 功能 | 显示时长 | 更新频率 | 数据源 |
|------|----------|----------|--------|
| 📍 地址显示 | 15秒 | 开机+2小时 | 腾讯地图 API |
| 🌤️ 天气显示 | 20秒 | 开机+2小时 | 心知天气 API |
| 🔄 循环周期 | 35秒 | 自动切换 | - |

### 📊 数据格式示例
```
📍 地址: 广东省 深圳市 南山区
🌤️ 天气: 32°C 晴
🗺️ 坐标: 22.5360°N, 113.9256°E
```

### 🔧 技术特性
- ✅ **HTTP 协议** - 避免 SSL 证书问题
- ✅ **错误处理** - 完整的网络异常处理
- ✅ **内存优化** - 城市数据库占用仅 369KB
- ✅ **WiFi 检测** - 仅在网络连接时更新
- ✅ **日志记录** - 详细的调试信息输出
- ✅ **Moon 风格** - 采用与 Moon 设备一致的 UI 设计语言

## ⚙️ 快速配置

### 1️⃣ 获取 API Key

#### 📍 腾讯位置服务 API
1. 访问 [腾讯位置服务控制台](https://lbs.qq.com/)
2. 注册并登录账号
3. 创建应用，选择"WebServiceAPI"
4. 复制生成的 Key

#### 🌤️ 心知天气 API  
1. 访问 [心知天气官网](https://www.seniverse.com/)
2. 注册并登录账号
3. 创建应用，选择"天气数据"
4. 复制生成的 Key

### 2️⃣ 配置 API Key

#### 腾讯位置服务 API Key
需要在两个位置配置：

**位置1：** `main/boards/movecall-moji-esp32s3/location_display.cc` (第15行)
```cpp
static std::string api_key = "你的腾讯API密钥";
```

**位置2：** `main/boards/movecall-moji-esp32s3/movecall_moji_esp32s3.cc` (第142行)
```cpp
LocationDisplay::SetApiKey("你的腾讯API密钥");
```

#### 心知天气 API Key
**文件位置：** `main/boards/movecall-moji-esp32s3/movecall_moji_esp32s3.cc` (第147行)

```cpp
WeatherDisplay::SetApiKey("你的心知天气API密钥");
```

### 3️⃣ ESP-IDF 配置

#### 启用 TabView 支持
1. 运行 `idf.py menuconfig`或者使用vscode idf插件进入sdk配置（推荐）
2. 导航到 `Component config` → `LVGL configuration` → `Widgets`
3. 启用 `TABVIEW`
4. 保存配置

#### 选择设备型号
1. 在 `Board Options` 中选择 `moji小智衍生版`
2. 保存配置

## 📁 项目结构

```
main/boards/movecall-moji-esp32s3/
├── 📄 movecall_moji_esp32s3.cc    # 主界面文件（修改，Moon 风格）
├── 📄 location_display.h          # 地址显示模块头文件（新增）
├── 📄 location_display.cc         # 地址显示模块实现（新增）
├── 📄 weather_display.h           # 天气显示模块头文件（新增）
├── 📄 weather_display.cc          # 天气显示模块实现（新增）
├── 📄 city_data.h                 # 城市坐标数据（新增）
├── 📄 city_data_impl.cc           # 城市坐标查找实现（新增）
├── 🐍 csv_to_cpp.py              # CSV转C++代码生成脚本（新增）
└── 🧪 test_location_api.py       # API测试脚本（新增）
```

## 📊 API 服务信息

### 📍 腾讯地图 IP 定位 API
| 项目 | 详情 |
|------|------|
| **服务地址** | `http://apis.map.qq.com/ws/location/v1/ip` |
| **请求方式** | GET |
| **参数** | `key` (API密钥) |
| **返回格式** | JSON |
|              |                                            |

### 🌤️ 心知天气 API
| 项目 | 详情 |
|------|------|
| **服务地址** | `http://api.seniverse.com/v3/weather/now.json` |
| **请求方式** | GET |
| **参数** | `key`, `location`, `language`, `unit` |
| **返回格式** | JSON |
|  |  |

## ❓ 故障排除

### 🔍 常见问题

#### 1. SSL 错误 `mbedtls_ssl_fetch_input error=29312`
**说明：** 这是 ESP-IDF 内部 SSL 库问题，不影响功能
**解决方案：** 可以忽略，代码已使用 HTTP 协议避免 SSL 问题

### 🔧 调试信息

项目包含详细的日志输出，可通过串口监视器查看：

```bash
# 地址获取过程
I (5679) LocationDisplay: 获取到地址: 广东省 深圳市 南山区

# 天气请求状态  
I (5809) WeatherDisplay: 天气HTTP状态码: 200
I (5809) WeatherDisplay: 获取到天气: 深圳 32°C 晴

# 城市坐标查找结果
I (5689) WeatherDisplay: 构建城市名: 广东/深圳/南山
```

## ⚠️ 注意事项

| 项目 | 说明 |
|------|------|
| 🔢 **API 限制** | 注意各 API 的免费使用额度 |
| 🌐 **网络依赖** | 功能需要稳定的网络连接 |
| 💾 **内存使用** | 城市坐标数据库占用约 369KB Flash 空间 |
| ⏱️ **更新频率** | 建议不要过于频繁地更新数据以避免超出 API 限制 |
| 🎨 **界面风格** | 采用 Moon 设备的表盘设计风格，保持视觉一致性 |

## 🤝 贡献指南

> **该版本不会花时间维护更新，有精力的开发者可以：**
> - 🔄 Fork 此仓库自行修改上传
> - 📝 申请 Pull Request 到此仓库
> - 🐛 提交 Issue 报告问题
> - 💡 提出功能建议

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

---

<div align="center">

**🌟 如果这个项目对你有帮助，请给它一个 Star！**

Made with ❤️ for xiaozhi • Moon 风格表盘

</div>

   

