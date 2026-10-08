#ifndef WIZ_WAIT_H
#define WIZ_WAIT_H
/* Bounded waits for socket.c and w5500.c (PLCBA_CPU, OTA-reseau spec
 * Sec 3.4.1). Every loop that waits on the chip gives up after WIZ_WAIT_MS,
 * or at once if the port has flagged a fault. The first timeout latches the
 * port fault: every later wait gives up at once, so one W5500Task tour waits
 * WIZ_WAIT_MS at most in total, not once per loop (socket() alone holds three
 * loops, close() inside it included). A wait longer than WIZ_SPIN_MS sleeps
 * one tick per check (wiz_port_yield): the calling task has priority 32 and
 * must not starve lower-priority tasks for up to 3 s (review 2026-10-03,
 * R2). Deadline expiry returns WIZ_ERR_DEADLINE, distinct from the chip's own
 * ARP/TCP timeout (SOCKERR_TIMEOUT). The hooks are implemented by the board
 * port (PLCBA_CPU/W5500Driver). */
#include <stdint.h>

uint32_t wiz_port_now_ms(void);
int      wiz_port_faulted(void);
void     wiz_port_set_fault(void);
void     wiz_port_yield(void);

#define WIZ_WAIT_MS       3000U   /* > RTR x (RCR+1) = 1.8 s (one ARP resolution) */
#define WIZ_SPIN_MS       1U      /* busy-poll this long, then sleep between checks */
#define WIZ_ERR_DEADLINE  (-100)  /* fits int8_t and int32_t returns */

#define WIZ_DEADLINE_DECL()       const uint32_t wiz_t0_ = wiz_port_now_ms()
#define WIZ_DEADLINE_CHECK_RET(v) do { \
        const uint32_t wiz_dt_ = wiz_port_now_ms() - wiz_t0_; \
        if (wiz_port_faulted() || (wiz_dt_ > WIZ_WAIT_MS)) { wiz_port_set_fault(); return (v); } \
        if (wiz_dt_ >= WIZ_SPIN_MS) { wiz_port_yield(); } \
    } while (0)
#define WIZ_DEADLINE_CHECK()      WIZ_DEADLINE_CHECK_RET(WIZ_ERR_DEADLINE)
/* Simple form: wait while cond is true. */
#define WIZ_WAIT(cond)        do { WIZ_DEADLINE_DECL(); \
                                   while (cond) { WIZ_DEADLINE_CHECK(); } } while (0)
#endif
