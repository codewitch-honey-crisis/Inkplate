#ifndef BUTTON_H
#define BUTTON_H
#include <stdbool.h>
#include <stddef.h>
typedef void(*button_on_press_changed_callback_t)(bool pressed, void* state);
#ifdef __cplusplus
extern "C" {
#endif
/// @brief Initializes the button
/// @return True if successful, otherwise false
bool button_init(void);
/// @brief Polls the button for updates. Should be called in the main loop
/// @return True if successful, otherwise false
bool button_update(void);
/// @brief Indicates whether or not the button is pressed 
/// @return True if pressed, false if not, or on error
bool button_pressed(void);
/// @brief Sets the callback for when the button is pressed or released
/// @param callback The callback
/// @param state Any user defined state to pass
void button_on_press_changed_callback(button_on_press_changed_callback_t callback, void* state);

#ifdef __cplusplus
}
#endif
#endif // BUTTON_H