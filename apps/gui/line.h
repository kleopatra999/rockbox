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

#ifndef __LINE_H__
#define __LINE_H__

#include <stdint.h>
#include <stdbool.h>
#include "screens.h"

struct line_desc {
    unsigned style;
    int height;
    int16_t line;
    int16_t nlines;
    bool scroll;
};

#define LINE_DESC_DEFINIT { .style = STYLE_DEFAULT, .height = -1, .line = 0, .nlines = 1, .scroll = false }

void put_line(struct screen *display,
              int x, int y, struct line_desc *line,
              const char *fmt, ...);

#endif /* __LINE_H__*/
