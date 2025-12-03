#ifndef _PFDI_SERVICE_H
#define _PFDI_SERVICE_H

#define PFDI_REGISTER	0xC2000012
#define PFDI_DONE		0xC2000013

void e2000_system_off_entry(void);
void e2000_system_reset_entry(void);
void e2000_suspend_finish_entry(void);
void e2000_suspend_end_entry(void);
void e2000_suspend_start_entry(void);
void register_pfdi(void);

#endif
