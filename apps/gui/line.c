/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Thomas Martitz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include "line.h"
#include "gcc_extensions.h"
#include "icon.h"
#include "screens.h"
#include "settings.h"

/* A line consists of:
 * |[Ss]|[i]|[Ss]|[t]|, where s is empty space (pixels), S is empty space
 * (n space characters), i is an icon and t is the text.
 *
 * All components are optional. However, even if none are specified the whole
 * line will be cleared and redrawn.
 *
 * For empty space with the width of an icon use i and pass Icon_NOICON as
 * corresponding argument.
 */
static void vput_line(struct screen *display,
                      int x, int y, struct line_desc *line,
                      const char *fmt, va_list ap)
{
    const char *str;
    int ch, num, height;
    int xpos = x;
    int icon_y, icon_h, icon_w;
    enum themable_icons icon;
    char tempbuf[128];
    int tempbuf_idx;

    height = line->height == -1 ? display->getcharheight() : line->height;
    icon_h = get_icon_height(display->screen_type);
    icon_w = get_icon_width(display->screen_type);
    tempbuf_idx = 0;
    /* vertically center string on the line
     * x/2 - y/2 rounds up compared to (x-y)/2 if one of x and y is odd */
    icon_y = y + height/2 - icon_h/2;
    y += height/2 - display->getcharheight()/2;

    /* parse format string */
    while (1)
    {
        ch = *fmt++;
        if (ch == '$')
        {
            num = -1;
next:
            ch = *fmt++;
            switch(ch)
            {
                case '*': /* num from parameter list */
                    num = va_arg(ap, int);
                    goto next;

                case 'i': /* icon */
                    icon = va_arg(ap, int);
                    /* draw it, then skip over */
                    if (icon != Icon_NOICON)
                        screen_put_iconxy(display, xpos, icon_y, icon);
                    xpos += icon_w;
                    break;

                case 'S':
                    if (num > 0)
                        xpos += num * display->getcharwidth();
                    break;

                case 's':
                    if (num > 0)
                        xpos += num;
                    break;

                case 't':
                    str = va_arg(ap, const char *);
                    /* TODO: scrolling */
                    display->putsxy(xpos, y, str);
                    return;
                default:
                    if (LIKELY(isdigit(ch)))
                    {
                        num = 10*num + ch - '0';
                        goto next;
                    }
                    else
                    {
                        /* any other character here is an erroneous format string */
                        sprintf(tempbuf, "<E:%c>", ch);
                        display->putsxy(xpos, y, tempbuf);
                        /* Don't consider going forward, fix the caller */
                        return;
                    }
            }
        }
        else
        {   /* handle string constant in format string */
            tempbuf[tempbuf_idx++] = ch;
            if (!ch)
            {   /* end of string. put it online */
                display->putsxy(xpos, y, tempbuf); /* TODO: scrolling */
                return;
            }
        }
    }
}

static void style_line(struct screen *display,
                       int x, int y, struct line_desc *line)
{
    int style = line->style;
    int width = display->getwidth();
    int height = line->height == -1 ? display->getcharheight() : line->height;

    if (style & STYLE_COLORED)
    {
        if (style & STYLE_INVERT)
            display->set_background(style & STYLE_COLOR_MASK);
        else
            display->set_foreground(style & STYLE_COLOR_MASK);
    }

    /* mask out gradient and colorbar styles for non-color displays */
    if (display->depth < 16 && (style & (STYLE_COLORBAR|STYLE_GRADIENT)))
    {
        style &= ~(STYLE_COLORBAR|STYLE_GRADIENT);
        style |= STYLE_INVERT;
    }
    
    if (style & STYLE_GRADIENT)
    {
        display->set_drawmode(DRMODE_FG);
        display->gradient_fillrect(x, y, width, height,
                                   global_settings.lss_color,
                                   global_settings.lse_color);
        display->set_foreground(global_settings.lst_color);
    }
    else if (style & STYLE_COLORBAR)
    {
        display->set_drawmode(DRMODE_FG);
        display->set_foreground(global_settings.lss_color);
        display->fillrect(x, y, width - x, height);
        display->set_foreground(global_settings.lst_color);
    }
    else if (style & STYLE_INVERT)
    {
        display->set_drawmode(DRMODE_SOLID);
        display->fillrect(x, y, width - x, height);
        display->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    }
    else
    {
        display->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
        display->fillrect(x, y, width - x, height);
        display->set_drawmode(DRMODE_SOLID);
    }
    /* fg color, bg color and drawmode are left as-is for text drawing */
}

void put_line(struct screen *display,
              int x, int y, struct line_desc *line,
              const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    style_line(display, x, y, line);
    vput_line(display, x, y, line, fmt, ap);
    if (display->depth > 1)
        display->set_foreground(global_settings.fg_color);
    va_end(ap);
}
