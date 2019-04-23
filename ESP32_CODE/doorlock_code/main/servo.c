// Created by Ameer Ali
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"


#define SERVO_MIN_PULSEWIDTH 780 //Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 2190 //Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 120 //Maximum angle in degree upto which servo can rotate
//has 120 degree rotation, 60 in each direction

//Interrupt for MCPWM for possible integration later maybe we use smephores to pass between task when we want
//this running
///**
// * @brief Register MCPWM interrupt handler, the handler is an ISR.
// *        the handler will be attached to the same CPU core that this function is running on.
// *
// * @param mcpwm_num set MCPWM unit(0-1)
// * @param fn interrupt handler function.
// * @param arg user-supplied argument passed to the handler function.
// * @param intr_alloc_flags flags used to allocate the interrupt. One or multiple (ORred)
// *        ESP_INTR_FLAG_* values. see esp_intr_alloc.h for more info.
// * @param arg parameter for handler function
// * @param handle pointer to return handle. If non-NULL, a handle for the interrupt will
// *        be returned here.
// *
// * @return
// *     - ESP_OK Success
// *     - ESP_ERR_INVALID_ARG Function pointer error.
// */
//esp_err_t mcpwm_isr_register(mcpwm_unit_t mcpwm_num, void (*fn)(void *), void *arg, int intr_alloc_flags, intr_handle_t *handle);

void mcpwm_gpio_initialize()
{
    printf("initializing mcpwm servo control gpio......\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, 18);    //Set GPIO 18 as PWM0A, to which servo is connected
}

/**
 * @brief Use this function to calcute pulse width for per degree rotation
 *
 * @param  degree_of_rotation the angle in degree to which servo has to rotate
 *
 * @return
 *     - calculated pulse width
 */
uint32_t servo_per_degree_init(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (SERVO_MIN_PULSEWIDTH + (((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * (degree_of_rotation)) / (SERVO_MAX_DEGREE)));
    return cal_pulsewidth;
}

//Use this function whenever you want the servo back in the "locked" position
void lockup ()
{
	uint32_t locked = 0;
	uint32_t angle;

	printf("Angle of rotation: %d\n", locked);
	angle = servo_per_degree_init(locked);

	printf("pulse width: %dus\n", angle);
	mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
	vTaskDelay(10);    //Here for safety
}
//Use this function whenever you wan the servo in the "unlocked" position
void unlocked ()
{
	uint32_t unlock = SERVO_MAX_DEGREE;
    uint32_t angle;

    printf("Angle of rotation: %d\n", unlock);
    angle = servo_per_degree_init(unlock);

    printf("pulse width: %dus\n", angle);
	mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
    vTaskDelay(10);    //Here for safety

}
/**
 * @brief Configure MCPWM module
 */
void mcpwm_servo_control_init(void )
{
    //uint32_t angle, count;
    //1. mcpwm gpio initialization
    mcpwm_gpio_initialize();

    //2. initial mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm......\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;    //frequency = 50Hz, i.e. for our servo motor time period should be 20ms
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
}
