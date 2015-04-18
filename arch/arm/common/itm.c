#include <stdint.h>

#include "itm.h"

#define DEMCR           0xe000edfc
#define DEMCR_TRCENA    (1 << 24)


#define ITM_BASE 0xe0000000
#define ITM_STIMULUS_PORT_BASE  (ITM_BASE + 0x0)
#define ITM_TRACE_ENABLE        (ITM_BASE + 0xe00)
#define ITM_TRACE_PRIVILEGE     (ITM_BASE + 0xe40)
#define ITM_TRACE_CTL           (ITM_BASE + 0xe80)
#define ITM_LOCK_ACCESS         (ITM_BASE + 0xfb0)

#define ITM_TRACE_CTL_ITMENA    (1 << 0)

#define read32(x) *((volatile uint32_t*) x)
#define write32(x, y) *((volatile uint32_t*) x) = y

void itm_init(void)
{
    read32(DEMCR) |= DEMCR_TRCENA; // activate trace

    write32(ITM_LOCK_ACCESS, 0xc5acce55); // unlock access to itm ctl registers
    read32(ITM_TRACE_CTL) |= ITM_TRACE_CTL_ITMENA;
    itm_enable_port(0);
    write32(ITM_LOCK_ACCESS, 0); // lock access to itm ctrl registers
}

void itm_putc(int port_id, uint32_t c)
{
    volatile uint32_t *port = (volatile uint32_t*) ITM_STIMULUS_PORT_BASE;

    while (!port[port_id])
        ;

    volatile uint32_t *port0 = port + port_id;
    volatile uint8_t  *port0_8b = (uint8_t*) port0;
    *port0_8b = (uint8_t)c;
}

size_t itm_puts(int port_id, const char *const str)
{
    size_t i;

    for (i = 0; str[i] != '\0'; i++) {
        itm_putc(port_id, str[i]); // FIXME: send 4-char at a time
    }

    return i;
}

void itm_enable_port(int port)
{
    read32(ITM_TRACE_ENABLE) |= (1 << port)
}

void itm_disable_port(int port)
{
    read32(ITM_TRACE_ENABLE) &= ~(1 << port);
}
