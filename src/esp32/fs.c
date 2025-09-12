#ifdef ESP_PLATFORM
#include "fs.h"
#include "esp_random.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include <memory.h>
#include <stdio.h>
#include "hardware.h"
#include "spi.h"
static sdmmc_card_t* fs_card = NULL;
static bool internal_init = false;
static const char mount_point[] = "/sdcard";
bool fs_external_init(void) {
    if(fs_card!=NULL) {
        return true;
    }
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config;
    memset(&mount_config, 0, sizeof(mount_config));
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 0;
#ifdef SD_CS
    if(!spi_init()) {
        return false;
    }
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_PORT;
    sdspi_device_config_t slot_config;
    memset(&slot_config, 0, sizeof(slot_config));
    slot_config.host_id = (spi_host_device_t)SD_PORT;
    slot_config.gpio_cs = (gpio_num_t)SD_CS;
    slot_config.gpio_cd = SDSPI_SLOT_NO_CD;
    slot_config.gpio_wp = SDSPI_SLOT_NO_WP;
    slot_config.gpio_int = GPIO_NUM_NC;
    if (ESP_OK != esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config,
                                          &mount_config, &fs_card)) {
        return false;
    }
    return true;
    
#elif defined(SDMMC_CLK)
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz =20*1000;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = (gpio_num_t)SDMMC_CLK;
    slot_config.cmd = (gpio_num_t)SDMMC_CMD;
    slot_config.d0 = (gpio_num_t)SDMMC_D0;
    slot_config.width = 1;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &fs_card);
    if(ret!=ESP_OK) {
        return false;
    }
    return true;
#else
    return false;
#endif
}
void fs_external_end(void) {
    if(fs_card==NULL) {
        return;
    }
#if SD_CS
    esp_vfs_fat_sdcard_unmount(mount_point,fs_card);
    fs_card = NULL;
    gpio_reset_pin((gpio_num_t)SD_CS);
    gpio_set_direction((gpio_num_t)SD_CS,GPIO_MODE_INPUT);
#elif defined(SDMMC_CLK)
    esp_vfs_fat_sdmmc_unmount();
    fs_card = NULL;
#endif
}
bool fs_get_device_id(char* out_id,size_t out_id_len) {
    FILE* f = fopen("/spiffs/id.txt","r");
    if(f==NULL) {
        return false;
    }
    fgets(out_id,out_id_len,f);
    fclose(f);
    return true;
}
static void gen_device_id() {
    char id[16];
    static const char* vowels = "aeiou";
    char tmp[2] = {0};
    size_t len = esp_random()%4+7;
    int salt = esp_random()%98+1;
    bool vowel = esp_random()&1?true:false;
    id[0]='\0';
    for(int i = 0;i<len;++i) {
        if(vowel) {
            int idx = esp_random()%5;
            tmp[0]=vowels[idx];
        } else {
            char ch = (esp_random()%26)+'a';
            while(strchr(vowels,ch)) {
                ch = (esp_random()%26)+'a';
            }
            tmp[0]=ch;
        }
        strcat(id,tmp);
        if(!vowel) {
            vowel=true;
        } else {
            vowel = esp_random()%10>6?true:false;
        }
    }
    itoa(salt,id+strlen(id),10);
    FILE *fid = fopen("/spiffs/id.txt","w");
    fputs(id,fid);
    fclose(fid);
}

bool fs_internal_init(void) {
    if(internal_init) {
        return true;
    }
    esp_vfs_spiffs_conf_t conf;
    memset(&conf, 0, sizeof(conf));
    conf.base_path = "/spiffs";
    conf.partition_label = NULL;
    conf.max_files = 5;
    conf.format_if_mount_failed = 1;
    if(ESP_OK!=esp_vfs_spiffs_register(&conf)) {
        return false;
    }
    FILE* f = fopen("/spiffs/id.txt","r");
    fclose(f);
    if(f==NULL) {
        gen_device_id();
    }
    internal_init = true;
    return true;
}
#endif