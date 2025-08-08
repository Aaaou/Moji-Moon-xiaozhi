#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "led/single_led.h"
#include "lunar_calendar.h"

#include <wifi_station.h>
#include <esp_log.h>
#include <esp_efuse_table.h>
#include <driver/i2c_master.h>
#include <font_awesome_symbols.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_gc9a01.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#define TAG "MovecallMojiESP32S3"

LV_FONT_DECLARE(font_puhui_20_4);
LV_FONT_DECLARE(font_awesome_20_4);
LV_FONT_DECLARE(time40);
LV_FONT_DECLARE(time70);
LV_FONT_DECLARE(lunar);
#include <ctime>
#include <string>


class CustomLcdDisplay : public SpiLcdDisplay {
public:
    lv_obj_t* tabview_ = nullptr;
    lv_obj_t* tab1_ = nullptr;
    lv_obj_t* tab2_ = nullptr;
    lv_obj_t* container_ = nullptr;
    lv_obj_t* container_toggle_btn_ = nullptr;
    lv_obj_t* status_bar_ = nullptr;
    lv_obj_t* content_ = nullptr;
    bool container_visible_ = true;

    CustomLcdDisplay(esp_lcd_panel_io_handle_t io_handle, 
                    esp_lcd_panel_handle_t panel_handle,
                    int width,
                    int height,
                    int offset_x,
                    int offset_y,
                    bool mirror_x,
                    bool mirror_y,
                    bool swap_xy)
        : SpiLcdDisplay(io_handle, panel_handle, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy,
            {
                .text_font = &font_puhui_20_4,
                .icon_font = &font_awesome_20_4,
                .emoji_font = font_emoji_64_init(),
            }) {
        DisplayLockGuard lock(this);
        tabview_ = lv_tabview_create(lv_scr_act());
        lv_obj_set_size(tabview_, lv_pct(100), lv_pct(100));
        lv_tabview_set_tab_bar_position(tabview_, LV_DIR_TOP);
        lv_tabview_set_tab_bar_size(tabview_, 0);
        lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tabview_);
        lv_obj_add_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);
        // 设置 tabview 内容区为黑色
        lv_obj_t* tabview_content = lv_tabview_get_content(tabview_);
        lv_obj_set_style_bg_color(tabview_content, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(tabview_content, LV_OPA_COVER, LV_PART_MAIN);
        tab1_ = lv_tabview_add_tab(tabview_, "Chat");
        lv_obj_clear_flag(tab1_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(tab1_, LV_SCROLLBAR_MODE_OFF);
        // SetupTab1();
        // 新增 tab2_ 并显示时钟
        tab2_ = lv_tabview_add_tab(tabview_, "Clock");
        lv_obj_clear_flag(tab2_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(tab2_, LV_SCROLLBAR_MODE_OFF);
        SetupTab2();
        // 强制切换到 tab2_（时钟界面）
        lv_tabview_set_act(tabview_, 1, LV_ANIM_OFF);
    }

    void SetupTab1() {
        LcdDisplay::SetupUI();
    }

    void SetupTab2() {
        lv_obj_set_style_bg_color(tab2_, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(tab2_, LV_OPA_COVER, LV_PART_MAIN);
        // 秒钟标签
        lv_obj_t* second_label = lv_label_create(tab2_);
        lv_obj_set_style_text_font(second_label, &time40, 0);
        lv_obj_set_style_text_color(second_label, lv_color_white(), 0);
        lv_obj_align(second_label, LV_ALIGN_TOP_MID, 0, 10);
        lv_label_set_text(second_label, "00");

        // 日期标签
        lv_obj_t* date_label = lv_label_create(tab2_);
        lv_obj_set_style_text_font(date_label, fonts_.text_font, 0);
        lv_obj_set_style_text_color(date_label, lv_color_white(), 0);
        lv_label_set_text(date_label, "01-01");
        lv_obj_align(date_label, LV_ALIGN_TOP_MID, -60, 35);

        // 星期标签
        lv_obj_t* weekday_label = lv_label_create(tab2_);
        lv_obj_set_style_text_font(weekday_label, fonts_.text_font, 0);
        lv_obj_set_style_text_color(weekday_label, lv_color_white(), 0);
        lv_label_set_text(weekday_label, "星期一");
        lv_obj_align(weekday_label, LV_ALIGN_TOP_MID, 60, 35);

        // 时间容器
        lv_obj_t* time_container = lv_obj_create(tab2_);
        lv_obj_remove_style_all(time_container);
        lv_obj_set_size(time_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(time_container, 0, 0);
        lv_obj_set_style_bg_opa(time_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(time_container, 0, 0);
        lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_align(time_container, LV_ALIGN_CENTER, 0, 0);

        // 小时标签
        lv_obj_t* hour_label = lv_label_create(time_container);
        lv_obj_set_style_text_font(hour_label, &time70, 0);
        lv_obj_set_style_text_color(hour_label, lv_color_white(), 0);
        lv_label_set_text(hour_label, "00 :");

        // 分钟标签（橙色）
        lv_obj_t* minute_label = lv_label_create(time_container);
        lv_obj_set_style_text_font(minute_label, &time70, 0);
        lv_obj_set_style_text_color(minute_label, lv_color_hex(0xFFA500), 0);
        lv_label_set_text(minute_label, " 00");

        // 农历标签
        lv_obj_t* lunar_label = lv_label_create(tab2_);
        lv_obj_set_style_text_font(lunar_label, &lunar, 0);
        lv_obj_set_style_text_color(lunar_label, lv_color_white(), 0);
        lv_obj_set_width(lunar_label, LV_HOR_RES * 0.8);
        lv_label_set_long_mode(lunar_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(lunar_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(lunar_label, "农历癸卯年正月初一");
        lv_obj_align(lunar_label, LV_ALIGN_BOTTOM_MID, 0, -36);

        // 定时器更新时间
        static lv_obj_t* hour_lbl = hour_label;
        static lv_obj_t* minute_lbl = minute_label;
        static lv_obj_t* second_lbl = second_label;
        static lv_obj_t* date_lbl = date_label;
        static lv_obj_t* weekday_lbl = weekday_label;
        static lv_obj_t* lunar_lbl = lunar_label;

        lv_timer_create([](lv_timer_t* t) {
            if (!hour_lbl || !minute_lbl || !second_lbl || !date_lbl || !weekday_lbl || !lunar_lbl) return;
            // 获取当前时间
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);
            // 格式化时分秒
            char hour_str[6];
            char minute_str[3];
            char second_str[3];
            sprintf(hour_str, "%02d : ", timeinfo.tm_hour);
            sprintf(minute_str, "%02d", timeinfo.tm_min);
            sprintf(second_str, "%02d", timeinfo.tm_sec);
            lv_label_set_text(hour_lbl, hour_str);
            lv_label_set_text(minute_lbl, minute_str);
            lv_label_set_text(second_lbl, second_str);
            // 格式化日期
            char date_str[25];
            snprintf(date_str, sizeof(date_str), "%d/%d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
            lv_label_set_text(date_lbl, date_str);
            // 星期
            const char* weekdays[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
            if (timeinfo.tm_wday >= 0 && timeinfo.tm_wday < 7) {
                lv_label_set_text(weekday_lbl, weekdays[timeinfo.tm_wday]);
            }
            // 直接调用农历算法
            std::string lunar_date = LunarCalendar::GetLunarDate(
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday
            );
            lv_label_set_text(lunar_lbl, lunar_date.c_str());
        }, 1000, NULL);
    }
};

class MovecallMojiESP32S3 : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button boot_button_;
    Display* display_;

    void InitializeCodecI2c() {
        // Initialize I2C peripheral
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));
    }

    // SPI初始化
    void InitializeSpi() {
        ESP_LOGI(TAG, "Initialize SPI bus");
        spi_bus_config_t buscfg = GC9A01_PANEL_BUS_SPI_CONFIG(DISPLAY_SPI_SCLK_PIN, DISPLAY_SPI_MOSI_PIN, 
                                    DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    // GC9A01初始化
    void InitializeGc9a01Display() {
        ESP_LOGI(TAG, "Init GC9A01 display");

        ESP_LOGI(TAG, "Install panel IO");
        esp_lcd_panel_io_handle_t io_handle = NULL;
        esp_lcd_panel_io_spi_config_t io_config = GC9A01_PANEL_IO_SPI_CONFIG(DISPLAY_SPI_CS_PIN, DISPLAY_SPI_DC_PIN, NULL, NULL);
        io_config.pclk_hz = DISPLAY_SPI_SCLK_HZ;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &io_handle));
    
        ESP_LOGI(TAG, "Install GC9A01 panel driver");
        esp_lcd_panel_handle_t panel_handle = NULL;
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_SPI_RESET_PIN;    // Set to -1 if not use
        panel_config.rgb_endian = LCD_RGB_ENDIAN_BGR;           //LCD_RGB_ENDIAN_RGB;
        panel_config.bits_per_pixel = 16;                       // Implemented by LCD command `3Ah` (16/18)

        ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true)); 

        display_ = new CustomLcdDisplay(io_handle, panel_handle,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
    }

public:
    MovecallMojiESP32S3() : boot_button_(BOOT_BUTTON_GPIO) {  
        InitializeCodecI2c();
        InitializeSpi();
        InitializeGc9a01Display();
        InitializeButtons();
        GetBacklight()->RestoreBrightness();
    }

    virtual Led* GetLed() override {
        static SingleLed led_strip(BUILTIN_LED_GPIO);
        return &led_strip;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }
    
    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(codec_i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }
};

DECLARE_BOARD(MovecallMojiESP32S3);
