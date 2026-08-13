#pragma once
#include <driver/ledc.h>

#include <stdint.h>

#define SERVO_0DEG_PWM_US 500
#define SERVO_180DEG_PWM_US 2500
#define SERVO_NEUTRAL_PWM_US 1500
#define SERVO_PWM_HZ 50
#define SERVO_PWM_PERIOD_US 20000

#define LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RESOLUTION LEDC_TIMER_14_BIT
#define LEDC_TIMER_NUM LEDC_TIMER_0
#define LEDC_DUTY_MAX 16383

#define SERVO_MAX_COUNT 1

typedef struct {
    gpio_num_t gpio;
    ledc_channel_t channel;
    int minAngle;
    int maxAngle;
    uint32_t minPulseUs;
    uint32_t maxPulseUs;
} servo_t;

static servo_t servos[1] = {
    {
        .gpio = GPIO_NUM_18,
        .channel = LEDC_CHANNEL_0,
        .minAngle = 0,
        .maxAngle = 180,
        .minPulseUs = 500,
        .maxPulseUs = 2500
    }
};
void initServos(void);
void servoTask(void *pvParameters);