/**
 * @file battery_monitor.h
 * @author 宁子希 (1589326497@qq.com)
 * @brief       电池监测模块
 * @version 1.0.0
 * @date 2024-11-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/adc.h"

typedef enum {
    BAT_EVENT_VOLTAGE_REPORT,    // 电压报告事件
    BAT_EVENT_FULL,             // 电池充满事件
    BAT_EVENT_LOW,              // 电池低电量事件
    BAT_EVENT_CHARGING_BEGIN,   // 开始充电事件
    BAT_EVENT_CHARGING_STOP     // 停止充电事件
} bat_monitor_event_t;

typedef struct {
    adc_channel_t adc_ch;    // ADC通道
    gpio_num_t  charge_io;   // GPIO引脚
    float v_div_ratio;       // 分压比 (R1+R2)/R2
    float v_min;             // 最低电压
    float v_max;             // 最高电压
    float low_thresh;        // 低电量阈值(%)
    uint32_t report_ms;      // 报告间隔(ms)
} bat_monitor_config_t;

typedef void *bat_monitor_handle_t;

// 事件回调函数类型定义
typedef void (*bat_monitor_event_cb_t)(bat_monitor_event_t event, float voltage, void *user_data);

#ifdef __cplusplus
extern "C" {
#endif

// 创建电池监测实例
bat_monitor_handle_t bat_monitor_create(const bat_monitor_config_t *config);

// 设置事件回调
void bat_monitor_set_event_cb(bat_monitor_handle_t handle, 
                            bat_monitor_event_cb_t event_cb, 
                            void *user_data);

// 销毁电池监测实例
void bat_monitor_destroy(bat_monitor_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // BATTERY_MONITOR_H

