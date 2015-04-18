/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#ifndef __ITM_H__
#define __ITM_H__

#include <stddef.h>

void itm_init(void);
size_t itm_puts(int port_id, const char *const str);
void itm_enable_port(int port);

#endif /* __ITM_H__ */

