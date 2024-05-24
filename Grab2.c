/*
 *  Grab II - System sprite grabber
 *  Copyright (C) 2002  Chris Bazley
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licence as published by
 *  the Free Software Foundation; either version 2 of the Licence, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public Licence for more details.
 *
 *  You should have received a copy of the GNU General Public Licence
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* ANSI library files */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* RISC OS library files */
#include "toolbox.h"
#include "event.h"
#include "wimp.h"
#include "wimplib.h"
#include "menu.h"
#include "saveas.h"
#include "window.h"
#include "gadgets.h"
#include "iconbar.h"

/* CBLibrary headers */
#include "err.h"
#include "msgtrans.h"
#include "hourglass.h"
#include "Macros.h"
#include "sprformats.h"
#include "FileUtils.h"
#include "MessTrans.h"
#include "GadgetUtil.h"

/* Component IDs */
enum
{
  Menu_ComponentId_Help = 0x01,
  Menu_ComponentId_Quit = 0x02
};

/* Gadget IDs */
enum
{
  ComponentId_ROMSprites_Radio  = 0x00,
  ComponentId_RAMSprites_Radio  = 0x01,
  ComponentId_ToolSprites_Radio = 0x02,
  ComponentId_Cancel_ActButton  = 0x82bc02
};

#define APP_NAME "Grab2"
#define SQUASH_SPRITE_AREA 1

enum
{
  MinToolSpritesWimpVersion   = 321, /* Earliest version of window manager to
                                        support Wimp_ReadSysInfo 9. */
  MinExtErrorWimpVersion      = 321, /* Earliest version of window manager to
                                        support Wimp_ReportError extensions */
  ErrNum_ToSaveDrag           = 0x80b633,
  ErrNum_LockedFile           = 0x131c3,
  WimpReadSysInfo_ToolSprites = 9,
  IconbarGap                  = 96,
  MaxTaskNameLen              = 31,
  KnownWimpVersion            = 321 /* Latest version of window manager known */
};

static ObjectId save_id = NULL_ObjectId, underlying_win = NULL_ObjectId;
static int save_width = 0, save_height = 0;
static ComponentId radio_selected = NULL_ComponentId;
static int wimp_version;

/* ----------------------------------------------------------------------- */
/*                       Function prototypes                               */

static WimpMessageHandler wimp_quit_handler;
static ToolboxEventHandler auto_create_handler, error_handler,
                           menu_select_handler, save_handler, iconbar_handler,
                           savebox_button_click, savebox_about_to_be_shown,
                           savebox_save_complete;
static void initialise(void);

/* ----------------------------------------------------------------------- */
/*                         Public functions                                */

int main(int argc, char *argv[])
{
  int           event_code;
  WimpPollBlock poll_block;

  initialise();

  /*
   * poll loop
   */
  while (TRUE)
    event_poll (&event_code, &poll_block, 0);

}

/* ----------------------------------------------------------------------- */
/*                         Private functions                               */

static const _kernel_oserror *save_sprite_area(const SpriteAreaHeader *area,
                                               const char *fn)
{
#if SQUASH_SPRITE_AREA
  const _kernel_oserror *e = NULL;
  FILE *f;

  _kernel_last_oserror(); /* reset SCL's error recording */

  f = fopen(fn, "wb");
  if (f == NULL)
  {
    e = _kernel_last_oserror();
    if (e == NULL)
      e = msgs_error_subn(DUMMY_ERRNO, "OpenOutFail", 1, fn);
  }
  else
  {
    size_t written;
    const SpriteHeader *sprite;
    uint32_t valid_count = 0, valid_size = 0, n;

    /* Leave room for a header at the start of the file */
    written = !fseek(f,
                     sizeof(SpriteAreaHeader) - offsetof(SpriteAreaHeader, sprite_count),
                     SEEK_SET);

    for (sprite = (const SpriteHeader *)((const char *)area + area->first), n = area->sprite_count;
         (n > 0) && (written == 1);
         sprite = (const SpriteHeader *)((const char *)sprite + sprite->size), n--)
    {
      /* RISC OS 6 does a weird thing to some (deleted?) sprites in its RAM pool
         so skip invalid sprites. */
      if (sprite->name[0] != '\0')
      {
        valid_count++;
        valid_size += sprite->size;
        written = fwrite(sprite, sprite->size, 1, f);
      }
    }

    if (written == 1)
    {
      /* Write replacement header to file (without first word). */
      SpriteAreaHeader new_hdr;

      new_hdr.sprite_count = valid_count;
      new_hdr.first = sizeof(new_hdr);
      new_hdr.used = sizeof(new_hdr) + valid_size;

      rewind(f);
      written = fwrite((char *)&new_hdr + offsetof(SpriteAreaHeader, sprite_count),
                       sizeof(new_hdr) - offsetof(SpriteAreaHeader, sprite_count),
                       1, f);
    }

    if (written != 1)
    {
      e = _kernel_last_oserror();
      if (e == NULL)
        e = msgs_error_subn(DUMMY_ERRNO, "WriteFail", 1, fn);
    }

    /* Close output file */
    fclose(f);

    if (e == NULL)
      e = set_file_type(fn, FileType_Sprite);

    if (e != NULL)
      remove(fn);
  }

  return e;
#else
  return _swix(OS_SpriteOp, _INR(0, 2),
               SPRITEOP_USERAREA_SPRNAME + SPRITEOP_SAVE_AREA, area, fn);
#endif
}

/* ----------------------------------------------------------------------- */

static void simple_exit(const _kernel_oserror *e)
{
  /* Limited amount we can do with no messages file... */
  assert(e != NULL);
  wimp_report_error((_kernel_oserror *)e, Wimp_ReportError_Cancel, APP_NAME);
  exit(EXIT_FAILURE);
}

/* ----------------------------------------------------------------------- */

static void initialise(void)
{
  int    toolbox_events = 0,
         wimp_messages = 0;
  static MessagesFD mfd;
  static IdBlock id_block;
  char taskname[MaxTaskNameLen];
  const _kernel_oserror *e;

  hourglass_on();
  /*
   * register ourselves with the Toolbox.
   */
  e = toolbox_initialise(0, KnownWimpVersion, &wimp_messages, &toolbox_events,
                         "<"APP_NAME"Res$Dir>", &mfd, &id_block, &wimp_version,
                         NULL, NULL);
  if (e != NULL)
    simple_exit(e);

  /*
   * Look up the localised task name and use it to initialise the error
   * reporting module.
   */
  e = messagetrans_lookup(&mfd, "_TaskName", taskname, sizeof(taskname), NULL, 0);
  if (e != NULL)
    simple_exit(e);

  e = err_initialise(taskname, wimp_version >= MinExtErrorWimpVersion, &mfd);
  if (e != NULL)
    simple_exit(e);

  /*
   * Initialise the message lookup module.
   */
  EF(msgs_initialise(&mfd));

  /*
   * initialise the event library.
   */

  EF(event_initialise (&id_block));
  EF(event_set_mask (Wimp_Poll_NullMask |
                     Wimp_Poll_PointerLeavingWindowMask |
                     Wimp_Poll_PointerEnteringWindowMask |
                     Wimp_Poll_KeyPressedMask | /* Dealt with by Toolbox */
                     Wimp_Poll_LoseCaretMask |
                     Wimp_Poll_GainCaretMask));

  EF(event_register_toolbox_handler(-1, Toolbox_ObjectAutoCreated,
                                    auto_create_handler, 0));

  EF(event_register_toolbox_handler(-1, Toolbox_Error, error_handler, 0));
  EF(event_register_message_handler(Wimp_MQuit, wimp_quit_handler, 0));

  hourglass_off();
}

/* ----------------------------------------------------------------------- */

static int wimp_quit_handler(WimpMessage *message,void *handle)
{
  exit(EXIT_SUCCESS);
  return 1; /* claim event */
}

/* ----------------------------------------------------------------------- */

static int auto_create_handler(int event_code, ToolboxEvent *event, IdBlock *id_block, void *handle)
{
  /* Catch auto-created objects and initialise handlers etc. */
  ToolboxObjectAutoCreatedEvent *toace = (ToolboxObjectAutoCreatedEvent *)event;

  if (strcmp(toace->template_name, "Menu") == 0)
  {
    /* Listen for iconbar menu selections */
    EF(event_register_toolbox_handler(id_block->self_id, Menu_Selection,
                                      menu_select_handler, NULL));

    return 1; /* claim event */
  }

  if (strcmp(toace->template_name, "Iconbar") == 0)
  {
    /* Listen for mouse clicks on application icon */
    EF(event_register_toolbox_handler(id_block->self_id, Iconbar_Clicked,
                                      iconbar_handler, NULL));
    return 1; /* claim event */
  }

  if (strcmp(toace->template_name, "SaveAs") == 0)
  {
    WimpGetWindowStateBlock winstate;

    /* Participate in saving */
    save_id = id_block->self_id;
    EF(event_register_toolbox_handler(save_id, SaveAs_SaveToFile, save_handler,
                                      NULL));

    /* Find dimensions of savebox */
    EF(saveas_get_window_id(0, save_id, &underlying_win));
    EF(window_get_wimp_handle(0, underlying_win, &(winstate.window_handle)));
    EF(wimp_get_window_state(&winstate));
    save_width = winstate.visible_area.xmax - winstate.visible_area.xmin;
    save_height = winstate.visible_area.ymax - winstate.visible_area.ymin;

    EF(saveas_set_file_type(0, save_id, FileType_Sprite));

    EF(event_register_toolbox_handler(save_id, SaveAs_AboutToBeShown,
                                      savebox_about_to_be_shown, NULL));

    EF(event_register_toolbox_handler(underlying_win, ActionButton_Selected,
                                      savebox_button_click, NULL));

    EF(event_register_toolbox_handler(save_id, SaveAs_SaveCompleted,
                                      savebox_save_complete, NULL));

    /* Read default state of radio buttons */
    EF(radiobutton_get_state(0, underlying_win, ComponentId_ROMSprites_Radio,
                             NULL, &radio_selected));

    /* Grey out 'toolsprites' option if Window Manager too old */
    set_gadget_faded(underlying_win, ComponentId_ToolSprites_Radio,
                     wimp_version < MinToolSpritesWimpVersion);

    return 1; /* claim event */
  }

  return 0; /* event not handled */
}

/* ----------------------------------------------------------------------- */

static int error_handler(int event_code, ToolboxEvent *event, IdBlock *id_block, void *handle)
{
  ToolboxErrorEvent *totee = (ToolboxErrorEvent *)event;

  /* Treat "To save drag..." or locked file less harshly */
  if(totee->errnum == ErrNum_ToSaveDrag || totee->errnum == ErrNum_LockedFile)
    err_report(totee->errnum, totee->errmess);
  else
    err_complain(totee->errnum, totee->errmess);

  return 1; /* claim event */
}

/* ----------------------------------------------------------------------- */

static int menu_select_handler(int event_code, ToolboxEvent *event, IdBlock *id_block, void *handle)
{
  /* Handle click on icon bar menu */
  switch (id_block->self_component)
  {
    case Menu_ComponentId_Help:
      /* load help */
      if(_kernel_oscli("Filer_Run Grab2Res:Help") == _kernel_ERROR)
        err_check_rep(_kernel_last_oserror());
      return 1; /* claim event */

    case Menu_ComponentId_Quit:
      /* quit program */
      exit(EXIT_SUCCESS);
      return 1; /* claim event */
  }
  return 0; /* not handled */
}

/* ----------------------------------------------------------------------- */

static int iconbar_handler(int event_code, ToolboxEvent *event, IdBlock *id_block, void *handle)
{
  /* Handle click on icon bar */
  const _kernel_oserror *e = NULL;

  if (TEST_BITS(event->hdr.flags, Iconbar_Clicked_Select))
  {
    WimpGetPointerInfoBlock pointer_info;

    /* Open window horizontally centred on pointer, above iconbar */
    e = wimp_get_pointer_info(&pointer_info);
    if (e == NULL)
    {
      WindowShowObjectBlock show_info;

      show_info.visible_area.xmin = pointer_info.x - save_width / 2;
      show_info.visible_area.ymin = IconbarGap + save_height;

      e = toolbox_show_object(Toolbox_ShowObject_AsMenu, save_id,
                              Toolbox_ShowObject_TopLeft, &show_info,
                              id_block->self_id, id_block->self_component);
    }
  }
  ON_ERR_RPT(e);
  return 1; /* claim event */
}

/* ----------------------------------------------------------------------- */

int savebox_about_to_be_shown(int event_code, ToolboxEvent *event, IdBlock *id_block, void *handle)
{
  /* Dialogue box opening - set the appropriate radio button */
  ON_ERR_RPT(radiobutton_set_state(0, underlying_win, radio_selected, 1));
  return 1; /* claim event */
}

/* ----------------------------------------------------------------------- */

int savebox_button_click(int event_code, ToolboxEvent *event, IdBlock *id_block, void *handle)
{
  /* When the Cancel button has been clicked with ADJUST we should reset
     the state of our extra radiobutton gadgets. Note that the SaveAs module
     should do likewise with the file name, but existing versions do not. */
  ActionButtonSelectedEvent *abse = (ActionButtonSelectedEvent *)event;

  if (id_block->self_component != ComponentId_Cancel_ActButton ||
      !TEST_BITS(abse->hdr.flags, ActionButton_Selected_Adjust))
    return 0; /* not interested */

  /* Cancel button clicked on underlying window */
  ON_ERR_RPT(radiobutton_set_state(0, id_block->self_id, radio_selected, 1));

  return 1; /* claim event */
}

/* ----------------------------------------------------------------------- */

static int save_handler(int event_code, ToolboxEvent *event, IdBlock *id_block, void *handle)
{
  /* Save sprite pool to file, when SaveAs object tells us to */
  SaveAsSaveToFileEvent *save_to_file_block = (SaveAsSaveToFileEvent *)event;
  const _kernel_oserror *e;
  int gadget_selected;
  unsigned int flags = 0;

  hourglass_on();

  /* Which should we save? */
  e = radiobutton_get_state(0, underlying_win, ComponentId_ROMSprites_Radio, 0,
                            &gadget_selected);
  if (e == NULL)
  {
    void *rom, *ram;

    switch (gadget_selected)
    {
      case ComponentId_ROMSprites_Radio:
      case ComponentId_RAMSprites_Radio:
        /* Get addresses of Wimp sprite pool areas */
        e = wimp_base_of_sprites(&rom, &ram);
        if (e != NULL)
          break;

        e = save_sprite_area(
              gadget_selected == ComponentId_ROMSprites_Radio ? rom : ram,
              save_to_file_block->filename);
        break;

      case ComponentId_ToolSprites_Radio:
        if (wimp_version >= MinToolSpritesWimpVersion)
        {
          /* Get address of Wimp toolsprites */
          WimpSysInfo results;

          e = wimp_read_sys_info(WimpReadSysInfo_ToolSprites, &results);
          if (e != NULL)
            break;

          e = save_sprite_area((SpriteAreaHeader *)results.r0,
                               save_to_file_block->filename);
        }
        break;

      default:
        assert("Unknown radio button selected" == NULL);
        break;
    }
  }

  hourglass_off();

  if (e != NULL)
    err_report(0, msgs_lookup_subn("SaveFail", 1, e->errmess));
  else
    flags = SaveAs_SuccessfulSave;

  saveas_file_save_completed(flags, id_block->self_id,
                             save_to_file_block->filename);
  return 1; /* claim event */
}

/* ----------------------------------------------------------------------- */

int savebox_save_complete(int event_code, ToolboxEvent *event, IdBlock *id_block, void *handle)
{
  /* Whenever a save is successfully completed we record which of our
     extra radiobutton components is set */
  ON_ERR_RPT(radiobutton_get_state(0, underlying_win,
                                   ComponentId_ROMSprites_Radio, NULL,
                                   &radio_selected));
  return 1; /* claim event */
}
