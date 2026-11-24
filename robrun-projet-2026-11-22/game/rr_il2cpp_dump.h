#ifndef RR_IL2CPP_DUMP_H
#define RR_IL2CPP_DUMP_H

/* Walks the live IL2CPP runtime through its exported embedding API and writes
 * field names, offsets and static slots for the types the offsets headers
 * depend on. Version-stable: no metadata parsing, so a Unity metadata bump
 * cannot break it. Output: %LOCALAPPDATA%\RobRunInternal\il2cpp_fields.log */
void rr_il2cpp_dump(void);

#endif