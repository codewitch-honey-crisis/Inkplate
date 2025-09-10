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
bool display_update_3bit(void);
/// @brief Gets the size of the 3bit grayscale buffer
/// @return The size in bytes
size_t display_buffer_3bit_size(void);
/// @brief Gets the display buffer for the 3bit grayscale
/// @return A pointer to the buffer
uint8_t* display_buffer_3bit(void);
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