/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdlib.h>
#include <applibs/log.h>

#ifdef PRINTF
#undef PRINTF
#endif // PRINTF

#define PRINTF Log_Debug

typedef unsigned short word16;
