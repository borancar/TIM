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
| 0x0be62 | long shift right | shifts `dx:ax` right by CL with `sar` on the high half, and a separate path for shifts of 16 or more that moves DX into AX and sign-extends |
| 0x0bd0d | far pointer compare | normalises two `seg:off` pairs by folding `off >> 4` into the segment and masking the offset to four bits, then compares - which is only meaningful for pointers |
| 0x0c7c4 | stack overflow check | compares against `sp - 0x200` and stores error code 8; a near `ret`, unlike everything the game's own large-model code uses |
| 0x0d543 | `memset` | stores a byte at an odd destination first to reach an even address, duplicates the value into both halves of AX, then `rep stosw` and one trailing byte if the count was odd - the standard shape of an optimised `memset`, and it takes destination, count and value in that order |
| 0x00274 | write to stderr | `mov ah,0x40 / mov bx,2 / int 21h` and nothing else - DOS handle 2 is stderr, and this is what the runtime's error messages go through |
| 0x0c7e6 | heap extension | adds to the break at DGROUP 0x9c, refuses if the result would come within 0x200 bytes of SP, and stores error code 8 on failure - the same error code and the same margin as the stack check at 0x0c7c4 |
| 0x0dd55 | case-insensitive string compare | loads `0x617a` into CX so that CH is `'a'` and CL is `'z'`, and folds each character into that range before comparing |
| 0x0ca39 | `malloc` | asks 0x0c7e6 for the memory, answers 0 when that fails, links the block onto the list at DGROUP 0x4e36 through a `next` at +2, stores the size at +0, and answers the address four bytes past the header |
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
