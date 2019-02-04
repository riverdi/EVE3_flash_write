#include "eve3-flash-utils.h"

/*
 *  FlashErase()
 */
bool_t
FlashErase (void)
{

  /* erase the flash */
  uint8_t status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);

  if (status == FLASH_STATUS_DETACHED)
    {
      Gpu_CoCmd_FlashAttach(phost);
      App_Flush_Co_Buffer(phost);
      Gpu_Hal_WaitCmdfifo_empty(phost);

      status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
      if (FLASH_STATUS_BASIC != status)
        {
          printf(" - Flash is not able to attach\n");
          return FALSE;
        }
    }

  Gpu_CoCmd_FlashErase(phost);
  App_Flush_Co_Buffer(phost);
  Gpu_Hal_WaitCmdfifo_empty(phost);

  /* check first 4KB */
  uint8_t *p_flashbuf = (uint8_t *)calloc(4 * 1024, sizeof(uint8_t));

  Gpu_CoCmd_FlashHelper_Read(phost, 0, 0, 4 * 1024, p_flashbuf);
  for (int i = 0; i < 4 * 1024; i++)
    {
      if (*(p_flashbuf++) != 0xFF)
        {
          return FALSE;
        }
    }

  return TRUE;
}


/*
 * Utils_Write_File_To_Flash()
 */
bool_t
Utils_Write_File_To_Flash (uchar8_t  *fileName,
                           uint32_t   addr,
                           uint32_t   tmp_ram_addr)
{
  FILE *fp;
  uint32_t fileLen, ret = 0;
  uint8_t pBuff[(1024 * 32)];

  fp = fopen(fileName, "rb+");
  if (fp)
    {

      fseek(fp, 0, SEEK_END);
      ret = fileLen = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      while (fileLen > 0)
        {
          int32_t ramlen = 0;
          tmp_ram_addr = 0;

          while ((ramlen < (1024 * 1024)) && (fileLen > 0))
            {
              uint32_t blocklen = fileLen > (1024 * 32) ? (1024 * 32) : fileLen;

              fread(pBuff, 1, blocklen, fp);
              fileLen -= blocklen;

              Gpu_Hal_WrMem(phost, tmp_ram_addr, pBuff, blocklen);

              tmp_ram_addr += blocklen;
              ramlen += blocklen;
            }

          ramlen = (ramlen + 4095) & (~4095);//to ensure 4KB alignment

          Gpu_CoCmd_FlashHelper_Update(phost, addr, 0, ramlen);
          addr += ramlen;
        }

      fclose(fp);
    }
  else
    {
      printf(" - Unable to open file: %s\n", fileName);
      return FALSE;
    }

  return TRUE;
}


/*
 * FlashWriteFirst()
 */
bool_t
FlashWriteFirst (const char  *pblob,
                 uint32_t     gramaddr)
{
  uint8_t status;

  {
    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);

    if (status == FLASH_STATUS_DETACHED)
      {
        Gpu_CoCmd_FlashAttach(phost);
        App_Flush_Co_Buffer(phost);
        Gpu_Hal_WaitCmdfifo_empty(phost);

        status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
        if (FLASH_STATUS_BASIC != status)
          {
            printf(" - Flash is not able to attach\n");
            return FALSE;
          }
       }

    /* write the blob image, location 0 is utilized here */
    if (Utils_Write_File_To_Flash((uchar8_t*)pblob, 0, gramaddr) != TRUE)
      return FALSE;

    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
    if (status == FLASH_STATUS_DETACHED)
      {
        Gpu_CoCmd_FlashAttach(phost);
        App_Flush_Co_Buffer(phost);
        Gpu_Hal_WaitCmdfifo_empty(phost);

        status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
        if (FLASH_STATUS_BASIC != status)
          {
            printf(" - Flash is not able to attach\n");
            return FALSE;
          }
      }

    /* try detaching and attaching the flash followed by fast mode */
    Gpu_CoCmd_FlashDetach(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
    if (FLASH_STATUS_DETACHED != status)
      {
        printf(" - Flash is not able to detatch\n");
        return FALSE;
      }

    Gpu_CoCmd_FlashAttach(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);

    if (FLASH_STATUS_BASIC != status)
      {
        printf(" - Flash is not able to attach\n");
        return FALSE;
      }

    Gpu_CoCmd_FlashFast(phost, 0);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
    if (FLASH_STATUS_FULL != status)
      {
        printf(" - Flash is not able to get into full mode\n");
        return FALSE;
      }

  }

  return TRUE;
}


/*
 * FlashVerify()
 */
bool_t
FlashVerify (const char  *pfile,
             uint8_t      gramaddr,
             uint32_t     addr,
             uint32_t     size)
{
  bool_t ret = TRUE;

  if ((gramaddr % 4) != 0)
    {
      printf(" - Destination address in main memory (gramaddr) must be 4-byte aligned");
      return FALSE;
    }

  if ((addr % 64) != 0)
    {
      printf(" - Source address in flash memory (flashaddr) must be 64-byte aligned");
      return FALSE;
    }

  if ((size % 4) != 0)
    {
      printf(" - Number of bytes to read must be multiple of 4 must be 4-byte aligned");
      return FALSE;
    }

  FILE *fp = fopen(pfile, "rb+");
  fseek(fp, 0, SEEK_END);

  uint32_t file_len = ftell(fp);

  if (size > file_len)
    {
      printf(" - Size > File Size (%d)", file_len);
      fclose(fp);
      return FALSE;
    }

  file_len = 16 * 1024 * 1024;
  if (size > file_len)
    {
      printf(" - Size > Flash Size (%d)", file_len);
      fclose(fp);
      return FALSE;
    }

  #define BLOCK (32 * 1024)

  uint8_t *p_filebuf = (uint8_t *)calloc(BLOCK, sizeof(uint8_t));
  uint8_t *p_flashbuf = (uint8_t *)calloc(BLOCK, sizeof(uint8_t));

  fseek(fp, addr, SEEK_SET);

  while (size > 0)
    {
      uint32_t blocklen = size > (BLOCK) ? (BLOCK) : size;

      Gpu_CoCmd_FlashHelper_Read(phost, gramaddr, addr, blocklen, p_flashbuf);

      fread(p_filebuf, 1, blocklen, fp);
      if (memcmp(p_filebuf, p_flashbuf, blocklen) != 0)
        {
          ret = FALSE;
          break;
        }

      size -= blocklen;
      addr += blocklen;
    }

  fclose(fp);
  free(p_filebuf);
  free(p_flashbuf);

  return ret;
}



