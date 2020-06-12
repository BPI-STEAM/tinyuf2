/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ha Thach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>
#include "boards/boards.h"
#include "flash_hal.h"

#include "esp_log.h"
#include "esp_partition.h"

#include "spi_flash_chip_driver.h"

#define FLASH_CACHE_SIZE          4096
#define FLASH_CACHE_INVALID_ADDR  0xffffffff

static uint32_t _fl_addr = FLASH_CACHE_INVALID_ADDR;
static uint8_t _fl_buf[FLASH_CACHE_SIZE] __attribute__((aligned(4)));

// uf2 will always write to ota0 partition
static esp_partition_t const * _part_ota0 = NULL;

void flash_hal_init(void)
{
  _fl_addr = FLASH_CACHE_INVALID_ADDR;

  _part_ota0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
  assert(_part_ota0 != NULL);

  PRINT_INT(_part_ota0->type);
  PRINT_HEX(_part_ota0->subtype);
  PRINT_HEX(_part_ota0->address);
  PRINT_HEX(_part_ota0->size);
  PRINT_INT(_part_ota0->encrypted);

  esp_flash_t const * flash = _part_ota0->flash_chip;
  PRINT_HEX(flash->chip_id);
  PRINT_HEX(flash->size);

  uint32_t block_erase = flash->chip_drv->erase_block == NULL ? 0 : flash->chip_drv->block_erase_size;
  PRINT_HEX(block_erase);
}

uint32_t flash_hal_size(void)
{
  return _part_ota0->size;
}

void flash_hal_read(uint32_t addr, void* buffer, uint32_t len)
{
  esp_partition_read(_part_ota0, addr, buffer, len);
}

void flash_hal_flush(void)
{
  if ( _fl_addr == FLASH_CACHE_INVALID_ADDR ) return;

  // always write without check for contents match
  // since reading from spi flash is also equally slow
  PRINTF("Erase and Write at 0x%08X", _fl_addr);

  esp_partition_erase_range(_part_ota0, _fl_addr, FLASH_CACHE_SIZE);
  esp_partition_write(_part_ota0, _fl_addr, _fl_buf, FLASH_CACHE_SIZE);

  _fl_addr = FLASH_CACHE_INVALID_ADDR;
}

void flash_hal_write (uint32_t dst, void const *src, uint32_t len)
{
  uint32_t new_addr = dst & ~(FLASH_CACHE_SIZE - 1);

  if ( new_addr != _fl_addr )
  {
    flash_hal_flush();

    _fl_addr = new_addr;
    flash_hal_read(new_addr, _fl_buf, FLASH_CACHE_SIZE);
  }

  memcpy(_fl_buf + (dst & (FLASH_CACHE_SIZE - 1)), src, len);
}
