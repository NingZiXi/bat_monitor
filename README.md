![alt text](image.jpg)
<h1 align="center">🔋 ESP32 Battery Monitor</h1>

<p align="center">
bat_monitor是一个专为ESP32设计的电池电量监测组件<br/>
支持实时电压监测和充电状态检测<br/>
提供低电量报警和充满电通知功能
</p>

<p align="center">
<a href="./README_EN.md">English</a>
· 简体中文
· <a href="https://github.com/NingZiXi/bat_monitor/releases">更新日志</a>
· <a href="https://github.com/NingZiXi/bat_monitor/issues">反馈问题</a>
</p>

<p align="center">
  <a href="LICENSE">
    <img alt="License" src="https://img.shields.io/badge/License-MIT-blue.svg" />
  </a>
  <a href="https://www.espressif.com/">
    <img alt="ESP32" src="https://img.shields.io/badge/ESP32-ESP32S3-77216F?logo=espressif" />
  </a>
  <a href="https://docs.espressif.com/projects/esp-idf/">
    <img alt="ESP-IDF" src="https://img.shields.io/badge/ESP--IDF-v5.3+-orange.svg" />
  </a>
  <a href="https://www.espressif.com/">
    <img alt="Platform" src="https://img.shields.io/badge/Platform-ESP32-green.svg" />
  </a>
  <a href="">
    <img alt="Version" src="https://img.shields.io/badge/Version-v1.0.0-brightgreen.svg" />
  </a>
  <a href="https://github.com/NingZiXi/bat_monitor/stargazers">
    <img alt="GitHub Stars" src="https://img.shields.io/github/stars/NingZiXi/bat_monitor.svg?style=social&label=Stars" />
  </a>
</p>

---
## 🚀 简介
**bat_monitor**组件是一个专为ESP32系列芯片设计的电池电量⚡监测解决方案，它能够实时监测电池电压、检测充电状态（支持GPIO检测和电压变化检测两种方式），并在电池低电量、充满电🔋等状态变化时触发相应事件通知。组件采用ADC采样和电压分压原理，支持自定义🔧电压阈值和报告间隔，可轻松集成到ESP-IDF项目中。
## 🛠️ 快速开始

### 1. 克隆项目

要将组件添加到项目中请在IDF终端执行下方命令:

```bash
idf.py add-dependency "ningzixi/bat_monitor^1.0.0"
```

或者直接克隆本仓库到项目`components`目录下:

```bash
git clone https://github.com/NingZiXi/bat_monitor
```
### 2. 使用示例
``` c
#include "bat_monitor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "BatteryApp"

// 电池事件回调函数
static void battery_event_handler(bat_monitor_event_t event, float voltage, void *user_data) {
    switch (event) {
        case BAT_EVENT_VOLTAGE_REPORT:
            ESP_LOGI(TAG, "电池电压: %.2fV", voltage);
            break;
        case BAT_EVENT_FULL:
            ESP_LOGI(TAG, "电池已充满 (%.2fV)", voltage);
            break;
        case BAT_EVENT_LOW:
            ESP_LOGI(TAG, "电池电量低 (%.2fV)", voltage);
            break;
        case BAT_EVENT_CHARGING_BEGIN:
            ESP_LOGI(TAG, "开始充电");
            break;
        case BAT_EVENT_CHARGING_STOP:
            ESP_LOGI(TAG, "停止充电");
            break;
    }
}

void app_main() {
    // 配置电池监测参数
    bat_monitor_config_t config = {
        .adc_ch = ADC_CHANNEL_5,     // ADC通道5
        .charge_io = GPIO_NUM_40,    // 充电检测IO引脚，如不使用配置为-1
        .v_div_ratio = 3.0f,         // 电压分压比
        .v_min = 6.3f,               // 电池亏点电压6.3V
        .v_max = 7.7f,               // 电池满电电压7.7V
        .low_thresh = 10.0f,         // 低电量阈值10%
        .report_ms = 1000            // 1秒报告间隔
    };

    // 创建电池监测实例
    bat_monitor_handle_t handle = bat_monitor_create(&config);
    if (!handle) {
        ESP_LOGE(TAG, "电池监测初始化失败");
        return;
    }

    // 设置事件回调
    bat_monitor_set_event_cb(handle, battery_event_handler, NULL);

    ESP_LOGI(TAG, "电池监测已启动");

    // 主循环
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10秒延迟
    }

    // 销毁实例
    bat_monitor_destroy(handle);
}
```
## 🤝 贡献
本项目采用 MIT 许可证，详情请参阅 [LICENSE](LICENSE) 文件。

<p align="center">
感谢您使用 ESP32 Battery Monitor！🔋<br/>
如果觉得项目对您有帮助，请给个 ⭐ Star 支持一下！
</p>

