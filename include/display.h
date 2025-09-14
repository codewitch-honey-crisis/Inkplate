#ifndef DISPLAY_H
#define DISPLAY_H
/* Adapted from the following by honey the codewitch
*
 *              https://github.com/e-radionicacom/Inkplate-Arduino-library
 *              For support, please reach over forums: forum.e-radionica.com/en
 *              For more info about the product, please check: www.inkplate.io
 *
 *              This code is released under the GNU Lesser General Public
 *License v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html Please review the
 *LICENSE file included with this example. If you have any questions about
 *licensing, please contact techsupport@e-radionica.com Distributed as-is; no
 *warranty is given.
 *
 * @authors     @ Soldered                 

*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef void(*display_on_wash_complete_callback_t)(void*state);
#ifdef __cplusplus
extern "C" {
#endif
/// @brief Initializes the display
/// @return True if successful, otherwise false
bool display_init(void);
/// @brief Sleeps the display
/// @return True if successful, otherwise false
bool display_sleep(void);
/// @brief Sends the current frame buffer to the display
/// @return True if successful, otherwise false
bool display_update_8bit(void);
/// @brief Gets the size of the 8bit grayscale buffer
/// @return The size in bytes
size_t display_buffer_8bit_size(void);
/// @brief Gets the display buffer for the 8bit grayscale
/// @return A pointer to the buffer
uint8_t* display_buffer_8bit(void);
/// @brief Indicates whether the display has been washed for 8 bit opps
/// @return True if already washed, otherwise false
bool display_washed_8bit(void);
/// @brief Sets the optional callback that will be invoked on wash completion
/// @param callback The callback to invoke. May be made from another thread/task
/// @param state A user defined state to pass with the callback
void display_on_washed_complete_callback(display_on_wash_complete_callback_t callback, void* state);
/// @brief Begins a wash of the screen for 8 bit opps
/// @return True if successful, otherwise false
bool display_wash_8bit_async(void);
/// @brief Waits for any pending 8 bit wash to complete
void display_wash_8bit_wait(void);
/// @brief Sends the current frame buffer to the display
/// @return True if successful, otherwise false
bool display_update_1bit(void);
/// @brief Sends the changed data to the display
/// @return True if successful, otherwise false
bool display_partial_update_1bit(void);
/// @brief Gets the size of the 1bit mono buffer
/// @return The size in bytes
size_t display_buffer_1bit_size(void);
/// @brief Gets the display buffer for the 1bit mono
/// @return A pointer to the buffer
uint8_t* display_buffer_1bit(void);
#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H