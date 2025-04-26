![alt text](image.jpg)
<h1 align="center">🔋 ESP32 Battery Monitor</h1>

<p align="center">
bat_monitor is a battery monitoring component designed for ESP32<br/>
Supports real-time voltage monitoring and charging status detection<br/>
Provides low battery alerts and full charge notifications
</p>

<p align="center">
English
· <a href="./README.md">简体中文</a>
· <a href="https://github.com/NingZiXi/bat_monitor/releases">Changelog</a>
· <a href="https://github.com/NingZiXi/bat_monitor/issues">Report Issues</a>
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
## 🚀 Introduction
**bat_monitor** is a battery monitoring solution designed for ESP32 series chips. It can monitor battery voltage in real-time, detect charging status (supports both GPIO detection and voltage change detection), and trigger corresponding event notifications when battery status changes (low battery, fully charged 🔋, etc.). The component uses ADC sampling and voltage divider principle, supports customizable voltage thresholds and reporting intervals, and can be easily integrated into ESP-IDF projects.

## 🛠️ Quick Start

### 1. Clone the Project

To add this component to your project, execute the following command in the IDF terminal:

```bash
idf.py add-dependency "ningzixi/bat_monitor^1.0.0"
```

Or clone the repository directly to your project's `components` directory:

```bash
git clone https://github.com/NingZiXi/bat_monitor
```

### 2. Usage Example
```c
#include "bat_monitor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "BatteryApp"

// Battery event callback function
static void battery_event_handler(bat_monitor_event_t event, float voltage, void *user_data) {
    switch (event) {
        case BAT_EVENT_VOLTAGE_REPORT:
            ESP_LOGI(TAG, "Battery voltage: %.2fV", voltage);
            break;
        case BAT_EVENT_FULL:
            ESP_LOGI(TAG, "Battery fully charged (%.2fV)", voltage);
            break;
        case BAT_EVENT_LOW:
            ESP_LOGI(TAG, "Low battery (%.2fV)", voltage);
            break;
        case BAT_EVENT_CHARGING_BEGIN:
            ESP_LOGI(TAG, "Charging started");
            break;
        case BAT_EVENT_CHARGING_STOP:
            ESP_LOGI(TAG, "Charging stopped");
            break;
    }
}

void app_main() {
    // Configure battery monitoring parameters
    bat_monitor_config_t config = {
        .adc_ch = ADC_CHANNEL_5,     // ADC channel 5
        .charge_io = GPIO_NUM_40,    // Charging detection IO pin, set to -1 if not used
        .v_div_ratio = 3.0f,         // Voltage divider ratio
        .v_min = 6.3f,               // Minimum battery voltage 6.3V
        .v_max = 7.7f,               // Maximum battery voltage 7.7V
        .low_thresh = 10.0f,         // Low battery threshold 10%
        .report_ms = 1000            // 1 second reporting interval
    };

    // Create battery monitor instance
    bat_monitor_handle_t handle = bat_monitor_create(&config);
    if (!handle) {
        ESP_LOGE(TAG, "Failed to initialize battery monitor");
        return;
    }

    // Set event callback
    bat_monitor_set_event_cb(handle, battery_event_handler, NULL);

    ESP_LOGI(TAG, "Battery monitor started");

    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10 second delay
    }

    // Destroy instance
    bat_monitor_destroy(handle);
}
```

## 🤝 Contribution
This project is licensed under MIT License. See [LICENSE](LICENSE) file for details.

<p align="center">
Thank you for using ESP32 Battery Monitor! 🔋<br/>
If you find this project helpful, please give it a ⭐ Star!
</p>
