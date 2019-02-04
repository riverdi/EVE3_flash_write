/*
 * Copyright (c) Riverdi Sp. z o.o. sp. k. <riverdi@riverdi.com>
 * Copyright (c) Skalski Embedded Technologies <contact@lukasz-skalski.com>
 */

#ifndef _EVE3_FLASH_UTILS_H_
#define _EVE3_FLASH_UTILS_H_

#include "platform.h"
#include "App_Common.h"

extern Gpu_Hal_Context_t host, *phost;

bool_t FlashVerify (const char *pfile, uint8_t gramaddr, uint32_t addr, uint32_t size);
bool_t FlashErase (void);

bool_t FlashWriteFirst (const char *pblob, uint32_t gramaddr);
bool_t Utils_Write_File_To_Flash (uchar8_t *fileName, uint32_t addr, uint32_t tmp_ram_addr);

#endif /* _EVE3_FLASH_UTILS_H_ */
