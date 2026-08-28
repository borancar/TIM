# The C runtime, and what is not being reconstructed

The binary is Borland C++ large model, so a good deal of what the code map
finds is **Borland's runtime library, not the game**. Those routines are
deliberate non-goals:

- They are not the game's logic. Reconstructing `__lmul` tells you nothing
  about The Incredible Machine.
- They are Borland's code, not Dynamix's, so transcribing them from this binary
  is worse legally as well as pointless.
- The port is C. A 32-bit shift is `<<`; a long multiply is `*`. There is
  nothing to call.

So a caller that used a runtime helper is transcribed with the **operation**,
not with a call, and the helper's address is named in the caller's comment so
the line can still be argued back to a byte.

## Identified so far

Each was read before being classified; the idiom is the evidence.

| image | what it is | how it was recognised |
| --- | --- | --- |
| 0x0bd90, 0x0bd93, 0x0bd97, 0x0bd9f | long comparison helpers | a family of entry points that each load a small constant into CX and jump to one common body at 0x0bdad - the classic selector thunk for `<`, `<=`, `>`, `>=` on a `long` |
| 0x0be3e, 0x0be41 | long shift left | shifts the `dx:ax` pair by CL, with the cross-word carry done by shifting BX right by `16-cl` and OR-ing it in |
| 0x0c16e | long multiply | two 16x16 `mul`s accumulated into a 32-bit product, with `jcxz` skipping the high half |

This list grows as routines are read. **Nothing is classified as runtime
without having been read** - guessing here would quietly drop game code.

## Dispatch thunks

A `ljmp [DGROUP offset]` whose body is four bytes is the game's way of calling
the video driver through a vector the loader filled in - see
`docs/video-driver.md`. There are 33 of them.

They are **elided** in the port: a call through a thunk is transcribed as a
direct call to the routine the vector resolves to, and the thunk's own address
is recorded in the caller's comment. There is nothing else they could become -
the port has no vector table to fill in - and making each one a one-line
wrapper would add 33 functions that say nothing.
