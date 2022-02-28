/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "los_dynlink.h"
#include "It_los_dynlink.h"

/* Test symbol not found for reloc, it's upto shared library */
STATIC UINT32 TestCase(VOID)
{
    VOID *handle = NULL;
    CHAR *dsoNameUndVal = DSO_FULL_PATH("dynamic_undval.so");
    CHAR *dsoNameUndFunc = DSO_FULL_PATH("dynamic_undfunc.so");

    handle = (VOID *)LOS_SoLoad(dsoNameUndVal, NULL);
    ICUNIT_ASSERT_EQUAL(handle, NULL, handle);

    handle = (VOID *)LOS_SoLoad(dsoNameUndFunc, NULL);
    ICUNIT_ASSERT_EQUAL(handle, NULL, handle);

    return LOS_OK;
}

VOID ItLosDynlink006(VOID)
{
    TEST_ADD_CASE("ItLosDynlink006", TestCase, TEST_LOS, TEST_DYNLINK, TEST_LEVEL1, TEST_FUNCTION);
}
