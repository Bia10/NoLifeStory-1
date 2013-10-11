/*
LZ4 - Fast LZ compression algorithm
Header File
Copyright (C) 2011-2012, Yann Collet.
BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You can contact the author at :
- LZ4 homepage : http://fastcompression.blogspot.com/p/lz4.html
- LZ4 source repository : http://code.google.com/p/lz4/
*/
// Modified by Peter Atashian
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstddef>
#include "lz4.hpp"
namespace {
    size_t const copylength {8u};
    size_t const mlbits {4u};
    size_t const mlmask {(1u << mlbits) - 1u};
    size_t const runbits {4u};
    size_t const runmask {(1u << runbits) - 1u};
    ptrdiff_t const stepsize {sizeof(size_t)};
    bool const arch64 {stepsize == 8};
    size_t const archshift {stepsize == 8 ? 3u : 2u};
    size_t const archadd {stepsize == 8 ? 7u : 3u};
    ptrdiff_t const dectable1[] {0, 3, 2, 3, 0, 0, 0, 0};
    ptrdiff_t const dectable2[] {0, 0, 0, -1, 0, 1, 2, 3};
}
namespace lz4 {
    void uncompress(void const * source, void * dest, size_t osize) {
        uint8_t const * ip {reinterpret_cast<uint8_t const *>(source)};
        uint8_t * op {reinterpret_cast<uint8_t *>(dest)};
        uint8_t const * const oend {op + osize};
        for (;;) {
            size_t const token {*ip++};
            {
                size_t length {token >> mlbits};
                if (length == runmask) {
                    size_t len {};
                    do {
                        len = *ip++;
                        length += len;
                    } while (len == 255);
                }
                uint8_t * const opc {op + length};
                uint8_t const * const ipc {ip + length};
                for (size_t i {(length + archadd) >> archshift}; i; --i) {
                    *reinterpret_cast<size_t *>(op) = *reinterpret_cast<size_t const *>(ip);
                    op += stepsize;
                    ip += stepsize;
                }
                op = opc;
                ip = ipc;
            }
            if (op > oend - copylength) return;
            uint8_t const * ref {op - *reinterpret_cast<uint16_t const *>(ip)};
            ip += 2;
            size_t length {token & mlmask};
            if (length == mlmask) {
                size_t len {};
                do {
                    len = *ip++;
                    length += len;
                } while (len == 255);
            }
            if (op - ref < stepsize) {
                ptrdiff_t const dec2 {arch64 ? dectable2[op - ref] : 0};
                op[0] = ref[0];
                op[1] = ref[1];
                op[2] = ref[2];
                op[3] = ref[3];
                op += 4;
                ref += 4;
                ref -= dectable1[op - ref];
                *reinterpret_cast<uint32_t *>(op) = *reinterpret_cast<uint32_t const *>(ref);
                op += stepsize - 4;
                ref -= dec2;
            } else {
                *reinterpret_cast<size_t *>(op) = *reinterpret_cast<size_t const *>(ref);
                op += stepsize;
                ref += stepsize;
            }
            length -= stepsize - 4;
            {
                uint8_t * const opc {op + length};
                for (size_t i {(length + archadd) >> archshift}; i; --i) {
                    *reinterpret_cast<size_t *>(op) = *reinterpret_cast<size_t const *>(ref);
                    op += stepsize;
                    ref += stepsize;
                }
                op = opc;
            }
        }
    }
}
