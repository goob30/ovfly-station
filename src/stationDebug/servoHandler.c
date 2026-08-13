// This handler is not to be run with the Station! This is only for debugging while the Onboard
// is unavailable.
#include "servoHandler.h"
#include <serialHandler.h>
#include <dataHandler.h>


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

// servo id (refer to comment on servoHandler) and angle (0 to 180)
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

float mapRange(uint16_t value, uint16_t inMin, uint16_t inMax, float outMin, float outMax) {
    return outMin + (float)(value - inMin) * (outMax - outMin) / (float)(inMax - inMin);
}

int uint16ToDegrees(uint16_t in) {
    int out = mapRange(in, 0, 65535, 0, 180);
    return out;
}

void servoTask(void *pvParameters) {
    (void)pvParameters;
    while (1) {
        driveServo(0, uint16ToDegrees(lastDFrame.ail)); // drive servo 0 which acts as an aileron motor
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}