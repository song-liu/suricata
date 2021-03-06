/* Copyright (C) 2013 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * \file
 *
 * \author Ken Steele <suricata@tilera.com>

 * Included by util-mpm-ac-tile.c with different SLOAD, SINDEX and
 * FUNC_NAME
 *
 */

// Hint to compiler to expect L2 hit latency for Load int16_t
#undef SLOAD
#define SLOAD(x) __insn_ld2s_L2((STYPE* restrict)(x))


/* This function handles (ctx->state_count < 32767) */
uint32_t FUNC_NAME(SCACTileSearchCtx *ctx, MpmThreadCtx *mpm_thread_ctx,
                   PatternMatcherQueue *pmq, uint8_t *buf, uint16_t buflen)
{
    int i = 0;
    int matches = 0;

    uint8_t* restrict xlate = ctx->translate_table;
    STYPE *state_table = (STYPE*)ctx->state_table;
    STYPE state = 0;
    int c = xlate[buf[0]];
    /* If buflen at least 4 bytes and buf 4-byte aligned. */
    if (buflen >= 4 && ((uint64_t)buf & 0x3) == 0) {
        BTYPE data = *(BTYPE* restrict)(&buf[0]);
        uint64_t index = 0;
        /* Process 4*floor(buflen/4) bytes. */
        i = 0;
        while (i < (buflen & ~0x3)) {
            BTYPE data1 = *(BTYPE* restrict)(&buf[i + 4]);
            index = SINDEX(index, state);
            state = SLOAD(state_table + index + c);
            c = xlate[BYTE1(data)];
            if (unlikely(SCHECK(state))) {
                matches = CheckMatch(ctx, pmq, buf, buflen, state, i, matches);
            }
            i++;
            index = SINDEX(index, state);
            state = SLOAD(state_table + index + c);
            c = xlate[BYTE2(data)];
            if (unlikely(SCHECK(state))) {
                matches = CheckMatch(ctx, pmq, buf, buflen, state, i, matches);
            }
            i++;
            index = SINDEX(index, state);
            state = SLOAD(state_table + index + c);
            c = xlate[BYTE3(data)];
            if (unlikely(SCHECK(state))) {
                matches = CheckMatch(ctx, pmq, buf, buflen, state, i, matches);
            }
            data = data1;
            i++;
            index = SINDEX(index, state);
            state = SLOAD(state_table + index + c);
            c = xlate[BYTE0(data)];
            if (unlikely(SCHECK(state))) {
                matches = CheckMatch(ctx, pmq, buf, buflen, state, i, matches);
            }
            i++;
        }
    }
    /* Process buflen % 4 bytes. */
    for (; i < buflen; i++) {
        uint64_t index = 0 ;
        index = SINDEX(index, state);
        state = SLOAD(state_table + index + c);
        c = xlate[buf[i+1]];
        if (unlikely(SCHECK(state))) {
            matches = CheckMatch(ctx, pmq, buf, buflen, state, i, matches);
        }
    } /* for (i = 0; i < buflen; i++) */

    return matches;
}
