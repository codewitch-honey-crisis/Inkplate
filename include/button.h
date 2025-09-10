#ifndef BUTTON_H
#define BUTTON_H
#include <stdbool.h>
#include <stddef.h>
typedef void(*button_on_press_changed_callback_t)(bool pressed, void* state);
#ifdef __cplusplus
extern "C" {
#endif
bool button_init(void);
bool button_update(void);
bool button_pressed(void);
void button_on_press_changed_callback(button_on_press_changed_callback_t callback, void* state);
#ifdef __cplusplus
}
#endif
#endif // BUTTON_H