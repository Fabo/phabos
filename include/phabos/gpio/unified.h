#ifndef __GPIO_UNIFIED_H__
#define __GPIO_UNIFIED_H__

#include <stdint.h>

#include <phabos/gpio.h>

int gpio_get_direction(unsigned int line);
void gpio_direction_in(unsigned int line);
void gpio_direction_out(unsigned int line, unsigned int value);
void gpio_activate(unsigned int line);
void gpio_deactivate(unsigned int line);
int gpio_get_value(unsigned int line);
int gpio_set_value(unsigned int line, unsigned int value);
int gpio_set_debounce(unsigned int line, uint16_t delay);
int gpio_irqattach(unsigned int line, gpio_irq_handler_t handler);
int gpio_set_triggering(unsigned int line, int trigger);
int gpio_mask_irq(unsigned int line);
int gpio_unmask_irq(unsigned int line);
int gpio_clear_interrupt(unsigned int line);

unsigned int gpio_line_count(void);

#endif /* __GPIO_UNIFIED_H__ */

