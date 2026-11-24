#ifndef RR_AUTOUPDATE_H
#define RR_AUTOUPDATE_H

/* Resolves every generated offset and method RVA against the live runtime and
 * writes %LOCALAPPDATA%\RobRunInternal\generated_offsets.h. Runs only when the
 * build ID does not match the compiled-in image. Nothing is written that was
 * not confirmed: unresolved entries keep their current value and are flagged. */
void rr_autoupdate_run(void);

#endif
