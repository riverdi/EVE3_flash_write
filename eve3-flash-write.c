/*
 * Copyright (c) Riverdi Sp. z o.o. sp. k. <riverdi@riverdi.com>
 * Copyright (c) Skalski Embedded Technologies <contact@lukasz-skalski.com>
 */

#include "platform.h"
#include "App_Common.h"
#include "eve3-flash-utils.h"

Gpu_Hal_Context_t host, *phost;
static void show_animation_from_flash();

/*
 * main
 */
int main (int argc, char *argv[])
{
  uchar8_t blobfile[] = "blob.bin";
  int ret = 0;
  phost = &host;

  /* init HW Hal */
  printf ("Hardware init...\n");
  if (App_Common_Init(&host) != 0)
    {
      printf (" !!! ERROR !!!\n");
      ret = -1;
      goto out;
    }

  /* erase flash */
  printf ("Erasing Flash... (please wait)\n");
  if (!FlashErase())
    {
      printf (" !!! ERROR !!!\n");
      ret = -1;
      goto out;
    }

  /* write 'blob' data */
  printf ("Writing data from blob.bin file to Flash... (please wait)\n");
  if (!FlashWriteFirst(blobfile, RAM_G))
    {
      printf (" !!! ERROR !!!\n");
      ret = -1;
      goto out;
    }

  /* verify */
  printf ("Verifying... (please wait)\n");
  if (!FlashVerify(blobfile, RAM_G, 0, 4096))
    {
      printf (" !!! ERROR !!!\n");
      ret = -1;
      goto out;
    }

#ifdef TEST
  printf ("Animation from Flash...\n");
  show_animation_from_flash();
#endif

  printf (" !!! DONE !!!\n");

out:

  printf ("Press any key to continue\n");
  getchar();

  /* close all the opened handles */
  Gpu_Hal_Close(phost);
  Gpu_Hal_DeInit();

  return ret;
}


/*
 * show_animation_from_flash()
 */
static void
show_animation_from_flash()
{

/* address of "bicycle.anim.object" from Flash map after generating Flash */
#define ANIM_ADDR1 408576
#define FRAME_COUNT 25

  /* switch Flash to FULL Mode */
  Gpu_CoCmd_FlashHelper_SwitchFullMode(phost);

  uint16_t frame = 0;
  uint16_t loop = 10;

  while (1)
    {
      Gpu_CoCmd_Dlstart(phost);
      App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(0, 0, 0));
      App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
      Gpu_CoCmd_AnimFrame(phost, DispWidth / 2, DispHeight / 2, ANIM_ADDR1, frame);
      App_WrCoCmd_Buffer(phost, DISPLAY());
      Gpu_CoCmd_Swap(phost);
      App_Flush_Co_Buffer(phost);
      Gpu_Hal_WaitCmdfifo_empty(phost);

      frame++;
      if (frame == 25)
        {
          frame = 0;
          loop--;

          if (loop == 0)
            break;
        }
    }
}
