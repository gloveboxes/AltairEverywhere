/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "mbed.h"
#include "sgp30.h"

#include <stdio.h>  // printf

#define EOL "\r\n"
Serial pc(USBTX, USBRX);

int main(void) {
    uint16_t i = 0;
    int16_t err;
    uint16_t tvoc_ppb, co2_eq_ppm;
    uint32_t iaq_baseline;
    uint16_t ethanol_signal, h2_signal;

    while (sgp_probe() != NO_ERROR) {
        pc.printf("SGP sensor probing failed" EOL);
        sensirion_sleep_usec(1000000);
    }
    pc.printf("SGP sensor probing successful" EOL);

    /* Read signals */
    err = sgp_measure_signals_blocking_read(&ethanol_signal, &h2_signal);
    if (err == NO_ERROR) {
        /* Print ethanol signal and h2 signal */
        pc.printf("Ethanol signal: %u" EOL, ethanol_signal);
        pc.printf("H2 signal: %u" EOL, h2_signal);
    } else {
        pc.printf("error reading signals" EOL);
    }

    /* Consider the two cases (A) and (B):
     * (A) If a recent baseline, less than one week old, is not available it
     *     must be reset */
    err = sgp_iaq_init();
    /* (B) If a recent baseline is available, set it here for faster start-up */
    /* IMPLEMENT: retrieve iaq_baseline from presistent storage;
     * err = sgp_set_iaq_baseline(iaq_baseline);
     */

    /* Run periodic IAQ measurements at defined intervals */
    while (1) {
        err = sgp_measure_iaq_blocking_read(&tvoc_ppb, &co2_eq_ppm);
        if (err == NO_ERROR) {
            pc.printf(
                "tVOC  Concentration: %dppb, CO2eq Concentration: %dppm" EOL,
                tvoc_ppb, co2_eq_ppm);
        } else {
            pc.printf("error reading IAQ values" EOL);
        }

        /* Persist the current baseline every hour */
        if (++i % 3600 == 3599) {
            err = sgp_get_iaq_baseline(&iaq_baseline);
            if (err == NO_ERROR) {
                /* IMPLEMENT: store baseline to presistent storage */
            }
        }

        sensirion_sleep_usec(1000000);
    }
    return 1;
}
