#include "servoHandler.h"
#include <serialHandler.h>

static const ledc_timer_config_t ledcTimer = {
    .speed_mode      = LEDC_SPEED_MODE,
    .duty_resolution = LEDC_DUTY_RESOLUTION,
    .timer_num       = LEDC_TIMER_NUM,
    .freq_hz         = SERVO_PWM_HZ,
    .clk_cfg         = LEDC_AUTO_CLK
};

void initServos(void) {
    ledc_timer_config((ledc_timer_config_t *)&ledcTimer);

    for (int i = 0; i < SERVO_MAX_COUNT; i++) {
        ledc_channel_config_t chan_conf = {
            .gpio_num   = servos[i].gpio,
            .speed_mode = LEDC_SPEED_MODE,
            .channel    = servos[i].channel,
            .timer_sel  = LEDC_TIMER_NUM,
            .duty       = 0,
            .hpoint     = 0
        };
        ledc_channel_config(&chan_conf);
    }
}

void driveServo(int id, int angle) {
    servo_t *s = &servos[id];

    if (angle < s->minAngle) angle = s->minAngle;
    if (angle > s->maxAngle) angle = s->maxAngle;

    uint32_t pulse_us = s->minPulseUs +
        ((uint32_t)(s->maxPulseUs - s->minPulseUs) * (angle - s->minAngle)) /
        (s->maxAngle - s->minAngle);

    uint32_t duty = (pulse_us * LEDC_DUTY_MAX) / SERVO_PWM_PERIOD_US;

    ledc_set_duty(LEDC_SPEED_MODE, s->channel, duty);
    ledc_update_duty(LEDC_SPEED_MODE, s->channel);
}

// todo: add method to convert DFrame uint to value between 0 and 180 or whatever is needed for the servo
void servoTask(void *pvParameters) {
    (void)pvParameters;
    while (1) {
        driveServo(0, 90);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}