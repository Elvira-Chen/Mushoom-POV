#pragma once
#include <sys/stat.h>
#include "sdmmc_cmd.h"

sdmmc_card_t *sdcard_init(void);
esp_err_t sdcard_free(sdmmc_card_t *card);