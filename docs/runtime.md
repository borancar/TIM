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
| 0x0bd90, 0x0bd93, 0x0bd97, 0x0bd9f | long **divide and modulo** | a family of entry points that each load a small constant into CX and jump to one common body at 0x0bdad. *First recorded here as long comparisons, which was wrong*: the body takes two 32-bit arguments from the stack, keeps the selector in DI, tests its bit 0 to decide signedness, and negates the operands by sign before dividing. The selector picks signed or unsigned and quotient or remainder. Corrected after 0x02ac0 was seen calling 0x0bd90 to divide |
| 0x0be3e, 0x0be41 | long shift left | shifts the `dx:ax` pair by CL, with the cross-word carry done by shifting BX right by `16-cl` and OR-ing it in |
| 0x0be62 | long shift right | shifts `dx:ax` right by CL with `sar` on the high half, and a separate path for shifts of 16 or more that moves DX into AX and sign-extends |
| 0x0bd0d | far pointer compare | normalises two `seg:off` pairs by folding `off >> 4` into the segment and masking the offset to four bits, then compares - which is only meaningful for pointers |
| 0x0c7c4 | stack overflow check | compares against `sp - 0x200` and stores error code 8; a near `ret`, unlike everything the game's own large-model code uses |
| 0x0d543 | `memset` | stores a byte at an odd destination first to reach an even address, duplicates the value into both halves of AX, then `rep stosw` and one trailing byte if the count was odd - the standard shape of an optimised `memset`, and it takes destination, count and value in that order |
| 0x00274 | write to stderr | `mov ah,0x40 / mov bx,2 / int 21h` and nothing else - DOS handle 2 is stderr, and this is what the runtime's error messages go through |
| 0x0c7e6 | heap extension | adds to the break at DGROUP 0x9c, refuses if the result would come within 0x200 bytes of SP, and stores error code 8 on failure - the same error code and the same margin as the stack check at 0x0c7c4 |
| 0x0dd55 | case-insensitive string compare | loads `0x617a` into CX so that CH is `'a'` and CL is `'z'`, and folds each character into that range before comparing |
| 0x0ca39 | `malloc` | asks 0x0c7e6 for the memory, answers 0 when that fails, links the block onto the list at DGROUP 0x4e36 through a `next` at +2, stores the size at +0, and answers the address four bytes past the header |
| 0x0bfcd, 0x0c006 | error reporting - `__IOerror` and its wrapper | stores the DOS error code at DGROUP 0x4d34, maps it through the byte table at 0x4d36 into DGROUP **0x94**, and answers -1. 0x94 is `errno`, which is corroborated: the stack check at 0x0c7c4 and the heap extension at 0x0c7e6 both store 8 there. Both are **pascal** convention - they end in `ret 2`, so the callee clears its own argument - which the game's own cdecl code never does |
| 0x0cd3d | `chmod` | INT 21h AH=43h, and on carry hands the DOS error to `__IOerror` |
| 0x0c0c3 | `lseek` | INT 21h AH=42h, clearing a bit in the handle-flags table at DGROUP 0x4d06 first and reporting failure through `__IOerror` |
| 0x0c185 | `read` | INT 21h AH=3Fh, refusing with errno 5 when the handle's entry in the same table has bit 1 set |
| 0x0c16e | long multiply | two 16x16 `mul`s accumulated into a 32-bit product, with `jcxz` skipping the high half |
| 0x0dd95 | `strlen` | one `repne scasb` over 0xffff bytes, then `not` and `dec` on what is left of the counter - the count of bytes *not* scanned, complemented, less the NUL it stopped on |
| 0x0dcce | `strchr` | reads a **word** at a time once the pointer is even, testing both halves, with one `lodsb` first to align an odd start. The two exits differ by the `inc si` that makes `[si-2]` name the high half rather than the low |
| 0x0dd04 | `strcmp` | measures the **second** string with `repne scasb`, then runs `repe cmpsb` for that length, and answers the difference of the last two bytes compared - so it stops at the second string's NUL whichever is shorter |
| 0x0dddb | `strnicmp` | seeds `dx` with 0x617a - `a` in `dh`, `z` in `dl` - and folds to **upper** case by subtracting 0x20, but only after the raw comparison has already failed |
| 0x0de4e | `strupr` | one unsigned test rather than two: `b - 'a'` compared against 0x19, so anything below `a` wraps past it |
| 0x0de1e | `strrev` | length from `repne scasb`, and the guard is `cx == -2` - the value after scanning exactly one byte, the terminator - so an empty string is left alone rather than having its pointers cross |
| 0x0c293 | `tolower` | `EOF` compared against -1 as a *word* passes through; anything else indexes the ctype table at DGROUP **0x4ab7** and adds 0x20 when bit 2, "this is upper case", is set |
| 0x0d524 | `memcpy` | `rep movsw` for the pairs and one `movsb` for an odd byte, the carry out of `shr cx,1` deciding whether there is one. Answers the destination |
| 0x0d584 | `creat` | INT 21h AH=3Ch with the attribute in CX, and `ret 4` - **pascal**, so its two arguments sit at `[bp+4]` and `[bp+6]` rather than `[bp+6]` and `[bp+8]` |
| 0x0d59d | the **truncating** write | INT 21h AH=40h with CX and DX both zeroed before the call, so it always writes zero bytes, which DOS reads as "cut the file here". `ret 2`: nothing but the handle is needed to say that |
| 0x0df7a | `write` | INT 21h AH=40h, refusing with errno 5 when bit 0 of the handle's entry in the 0x4d06 table is set, and setting 0x1000 there on success - the "has been written" bit `fclose` looks at |
| 0x0de6e | the runtime's `_write` | the handle checked against DGROUP **0x4d04**, the table's size; a length test of `(count + 1) < 2` unsigned, which rejects 0 and 0xffff together; an append seek for 0x800; then either `write` for a binary handle or a `\n`-to-`\r\n` expansion for a 0x4000 one |
| 0x0b755, 0x0b794 | `chdir` and `unlink` | INT 21h AH=3Bh and AH=41h, each with `xor ax,ax` **before** the call and again after it, only the carry flag choosing which zero survives - so a DOS that leaves rubbish in AX cannot make either look like a failure |
| 0x0b819 | `setdisk` | INT 21h AH=0Eh with the drive from a *letter*: `and al,0x5f` clears bit 5 **and bit 7**, so a letter with the high bit set still lands on a drive |
| 0x0b6b7, 0x0b6d3 | `findfirst` and `findnext` | INT 21h AH=4Eh and AH=4Fh, identical but for the function number - `findnext` loads DS:DX and CX the same way even though AH=4Fh reads neither. Both then call 0x0b6ef, which asks AH=2Fh where the DTA is and lifts the attribute, size and name out of it |

This list grows as routines are read. **Nothing is classified as runtime
without having been read** - guessing here would quietly drop game code.

**DGROUP 0x4d06 is the handle-flags table**, indexed by file handle doubled.
Several of the routines above touch it, which is part of what identifies them.

## Dispatch thunks

A `ljmp [DGROUP offset]` whose body is four bytes is the game's way of calling
the video driver through a vector the loader filled in - see
`docs/video-driver.md`. There are 33 of them.

They are **elided** in the port: a call through a thunk is transcribed as a
direct call to the routine the vector resolves to, and the thunk's own address
is recorded in the caller's comment. There is nothing else they could become -
the port has no vector table to fill in - and making each one a one-line
wrapper would add 33 functions that say nothing.
