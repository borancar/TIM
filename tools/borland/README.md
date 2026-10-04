# Borland C++ 3.0's run-time library, transcribed

The routines of `CM.LIB` that TIM.EXE links - the near heap, stdio, the
`printf` engine, the DOS calls, the long arithmetic - transcribed from the
image while the port still had to reproduce the guest's memory byte for byte.

**Not built.** Since 2026-09-28 the game calls the C library by its own
names: Borland's own under Borland C++ (linked from `CM.LIB`, which
`tools/link.py` proves), libc on the host, and `reconstruct/hostlib.c` for the
few that libc has no counterpart for. These files are kept to be extracted for other
Turbo C / Borland C++ DOS programs. `borland.h` has the declarations that left
the port's headers; `check_printf.py` checked the `printf` engine against libc
through a library built from them. They include the port's headers, so
they build with `-I reconstruct` from the repository's root.
