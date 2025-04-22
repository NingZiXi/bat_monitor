#include <string.h>
#include "bat_monitor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "bat_monitor"

// 充电检测电压变化阈值
#define CHARGE_DETECT_DELTA 100

// ADC校准方案
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
    #define ADC_CALI_SCHEME_SUPPORTED ADC_CALI_SCHEME_VER_LINE_FITTING
    #define ADC_CALI_CREATE_SCHEME(config, handle) adc_cali_create_scheme_line_fitting(config, handle)
    #define ADC_CALI_DELETE_SCHEME(handle) adc_cali_delete_scheme_line_fitting(handle)
    #define ADC_CALI_CONFIG_TYPE adc_cali_line_fitting_config_t
#elif CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C5
    #define ADC_CALI_SCHEME_SUPPORTED ADC_CALI_SCHEME_VER_CURVE_FITTING
    #define ADC_CALI_CREATE_SCHEME(config, handle) adc_cali_create_scheme_curve_fitting(config, handle)
    #define ADC_CALI_DELETE_SCHEME(handle) adc_cali_delete_scheme_curve_fitting(handle)
    #define ADC_CALI_CONFIG_TYPE adc_cali_curve_fitting_config_t
#else
    #error "Unsupported target"
#endif

typedef struct {
    bat_monitor_config_t config;
    bat_monitor_event_cb_t event_cb;
    void *user_data;
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t adc_cali_handle;
    TaskHandle_t monitor_task;
    bool running;
} bat_monitor_t;

static void IRAM_ATTR charge_detect_isr(void *arg) {
    bat_monitor_t* handle = (bat_monitor_t*)arg;
    bool charge_state = gpio_get_level(handle->config.charge_io);
    
    if (charge_state) {
        handle->event_cb(BAT_EVENT_CHARGING_BEGIN, 0, handle->user_data);
    } else {
        handle->event_cb(BAT_EVENT_CHARGING_STOP, 0, handle->user_data);
    }
}

static void monitor_task(void *arg) {
    bat_monitor_t* handle = (bat_monitor_t*)arg;
    float last_voltage = 0.0f;
    bool charging_state = false;
    
    while (handle->running) {
        float voltage = 0.0f;
        // 获取ADC电压并计算电池电压
        int raw = 0;
        adc_oneshot_read(handle->adc_handle, handle->config.adc_ch, &raw);
        
        int mv = 0;
        adc_cali_raw_to_voltage(handle->adc_cali_handle, raw, &mv);
        
        voltage = (float)mv / 1000.0f * handle->config.v_div_ratio;
        
        // 如果没有配置充电检测IO，则通过电压跳变检测充电状态
        if (handle->config.charge_io == GPIO_NUM_NC) {
            // 检测电压上升速率，判断是否开始充电
            if (voltage > last_voltage + CHARGE_DETECT_DELTA) {
                if (!charging_state) {
                    charging_state = true;
                    if (handle->event_cb) {
                        handle->event_cb(BAT_EVENT_CHARGING_BEGIN, voltage, handle->user_data);
                    }
                }
            } 
            // 检测电压稳定或下降，判断是否停止充电
            else if (voltage <= last_voltage && charging_state) {
                charging_state = false;
                if (handle->event_cb) {
                    handle->event_cb(BAT_EVENT_CHARGING_STOP, voltage, handle->user_data);
                }
            }
        }
        
        // 计算电量百分比
        float percentage = ((voltage - handle->config.v_min) / 
                          (handle->config.v_max - handle->config.v_min)) * 100.0f;
        percentage = percentage < 0 ? 0 : (percentage > 100 ? 100 : percentage);
        
        // 触发电压报告事件
        if (handle->event_cb) {
            handle->event_cb(BAT_EVENT_VOLTAGE_REPORT, voltage, handle->user_data);
            
            // 检查低电量事件
            if (percentage <= handle->config.low_thresh) {
                handle->event_cb(BAT_EVENT_LOW, voltage, handle->user_data);
            }
            
            // 检查充满事件
            if (voltage >= handle->config.v_max) {
                handle->event_cb(BAT_EVENT_FULL, voltage, handle->user_data);
            }
        }
        
        last_voltage = voltage;
        vTaskDelay(pdMS_TO_TICKS(handle->config.report_ms));
    }
    
    vTaskDelete(NULL);
}

bat_monitor_handle_t bat_monitor_create(const bat_monitor_config_t *config) {
    bat_monitor_t *monitor = calloc(1, sizeof(bat_monitor_t));
    if (!monitor) return NULL;
    
    memcpy(&monitor->config, config, sizeof(bat_monitor_config_t));
    monitor->running = true;
    
    // 初始化ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &monitor->adc_handle));
    
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(monitor->adc_handle, config->adc_ch, &chan_cfg));
    
    // 初始化ADC校准
    adc_cali_handle_t cali_handle = NULL;
    
    ADC_CALI_CONFIG_TYPE cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    
    esp_err_t ret = ADC_CALI_CREATE_SCHEME(&cali_config, &cali_handle);
    if (ret == ESP_OK) {
        monitor->adc_cali_handle = cali_handle;
        ESP_LOGI(TAG, "ADC calibration created successfully");
    } else {
        ESP_LOGE(TAG, "Failed to create ADC calibration: %s", esp_err_to_name(ret));
    }
    
    //初始化GPIO中断
    if (config->charge_io != GPIO_NUM_NC) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << config->charge_io),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_ANYEDGE
        };
        gpio_config(&io_conf);
        gpio_install_isr_service(0);
        gpio_isr_handler_add(config->charge_io, charge_detect_isr, monitor);
    }
    
    // 创建监控任务
    xTaskCreate(monitor_task, "bat_monitor", 2048, monitor, 5, &monitor->monitor_task);
    
    return (bat_monitor_handle_t)monitor;
}
void bat_monitor_set_event_cb(bat_monitor_handle_t handle, 
                            bat_monitor_event_cb_t event_cb, 
                            void *user_data) {
    if (!handle) return;
    
    bat_monitor_t *monitor = (bat_monitor_t *)handle;
    monitor->event_cb = event_cb;
    monitor->user_data = user_data;
}

void bat_monitor_destroy(bat_monitor_handle_t handle) {
    if (!handle) return;
    
    bat_monitor_t *monitor = (bat_monitor_t *)handle;
    monitor->running = false;
    
    // 等待任务结束
    vTaskDelay(10);
    
    // 释放资源
    gpio_isr_handler_remove(monitor->config.charge_io);
    
    // 删除ADC校准句柄
    if (monitor->adc_cali_handle) {
        ADC_CALI_DELETE_SCHEME(monitor->adc_cali_handle);
    }
    
    // 删除ADC单次模式句柄
    adc_oneshot_del_unit(monitor->adc_handle);
    
    free(monitor);
}