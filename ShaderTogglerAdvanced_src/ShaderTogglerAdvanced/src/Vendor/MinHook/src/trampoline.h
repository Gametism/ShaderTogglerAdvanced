/*
 *  MinHook - The Minimalistic API Hooking Library for x64/x86
 *  Copyright (C) 2009-2017 Tsuda Kageyu.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 *  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#pragma pack(push, 1)
typedef struct _JMP_REL_SHORT
{
    UINT8 opcode;
    UINT8 operand;
} JMP_REL_SHORT, *PJMP_REL_SHORT;
typedef struct _JMP_REL
{
    UINT8 opcode;
    UINT32 operand;
} JMP_REL, *PJMP_REL, CALL_REL;
typedef struct _JMP_ABS
{
    UINT8 opcode0;
    UINT8 opcode1;
    UINT32 dummy;
    UINT64 address;
} JMP_ABS, *PJMP_ABS;
typedef struct _CALL_ABS
{
    UINT8 opcode0;
    UINT8 opcode1;
    UINT32 dummy0;
    UINT8 dummy1;
    UINT8 dummy2;
    UINT64 address;
} CALL_ABS;
typedef struct _JCC_REL
{
    UINT8 opcode0;
    UINT8 opcode1;
    UINT32 operand;
} JCC_REL;
typedef struct _JCC_ABS
{
    UINT8 opcode;
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 dummy2;
    UINT32 dummy3;
    UINT64 address;
} JCC_ABS;
#pragma pack(pop)
typedef struct _TRAMPOLINE
{
    LPVOID pTarget;
    LPVOID pDetour;
    LPVOID pTrampoline;
#if defined(_M_X64) || defined(__x86_64__)
    LPVOID pRelay;
#endif
    BOOL patchAbove;
    UINT nIP;
    UINT8 oldIPs[8];
    UINT8 newIPs[8];
} TRAMPOLINE, *PTRAMPOLINE;
BOOL CreateTrampolineFunction(PTRAMPOLINE ct);
