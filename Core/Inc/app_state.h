/* app_state.h */
#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include "app_context.h"

typedef enum {
    APP_STATE_INTRO,
	APP_STATE_MENU,
    APP_STATE_LASER_EXPERIMENT,
    APP_STATE_COUNT
} app_state_t;

typedef struct {
    app_state_t current_state;
    app_state_t previous_state;
    void (*state_handlers[APP_STATE_COUNT])(void* context);
    app_context_t app_context;
} app_state_manager_t;


void blink_led(GPIO_TypeDef* gpio_port, uint16_t gpio_pin, uint32_t delay);
void app_state_init(app_state_manager_t* manager);
void app_state_transition(app_state_manager_t* manager, app_state_t new_state);
void app_update(app_state_manager_t* manager);

void handle_intro_state(void* context);
void handle_menu_state(void* context);
void handle_experiment_state(void* context);

#endif /* APP_STATE_H */
