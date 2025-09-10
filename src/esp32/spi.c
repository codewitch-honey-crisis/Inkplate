#ifdef ESP_PLATFORM
#include "spi.h"
#include <memory.h>
#include "hardware.h"
#include "driver/spi_master.h"
static bool spi_initialized = false;
bool spi_init(void) {
    if(spi_initialized) {
        return true;
    }
#ifdef SPI_PORT
    spi_bus_config_t buscfg;
    memset(&buscfg, 0, sizeof(buscfg));
    buscfg.sclk_io_num = SPI_CLK;
    buscfg.mosi_io_num = SPI_MOSI;
    buscfg.miso_io_num = SPI_MISO;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    
    buscfg.max_transfer_sz = 32*1024+8;
    if(ESP_OK!=spi_bus_initialize(SPI_PORT, &buscfg, SPI_DMA_CH_AUTO)) {
        return false;
    }
#endif
    spi_initialized=true;
    return true;
}
#endif