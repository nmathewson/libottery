#!/usr/bin/python
#
#   Libottery by Nick Mathewson.
#
#   This software has been dedicated to the public domain under the CC0
#   public domain dedication.
#
#   To the extent possible under law, the person who associated CC0 with
#   libottery has waived all copyright and related or neighboring rights
#   to libottery.
#
#   You should have received a copy of the CC0 legalcode along with this
#   work in doc/cc0.txt.  If not, see
#      <http://creativecommons.org/publicdomain/zero/1.0/>.
#
# Here's a minimal chacha implementation, based on the paper, in
# Python.  Its only purpose here is to validate the output of the C
# implementation.  If you use this for crypto, you're a bad person.

from __future__ import print_function
import sys

import struct
import binascii

def ROTL(n, b):
    return ((n << b) | (n >> (32-b))) & 0xffffffff

def qround(A, a,b,c,d):
    A[a] += A[b]; A[a] &= 0xffffffff; A[d] ^= A[a]; A[d] = ROTL(A[d], 16)
    A[c] += A[d]; A[c] &= 0xffffffff; A[b] ^= A[c]; A[b] = ROTL(A[b], 12)
    A[a] += A[b]; A[a] &= 0xffffffff; A[d] ^= A[a]; A[d] = ROTL(A[d], 8)
    A[c] += A[d]; A[c] &= 0xffffffff; A[b] ^= A[c]; A[b] = ROTL(A[b], 7)

def chacha_block(position, nonce, key, nRounds):
    assert len(nonce) == 8
    assert len(key) == 32
    assert nRounds in (8,12,20)

    STATE = list(struct.unpack("<LLLL", b"expand 32-byte k"))
    STATE += struct.unpack("<LLLLLLLL", key)
    STATE.append(position & 0xffffffff)
    STATE.append(position >> 32)
    STATE += struct.unpack("<LL", nonce)

    ORIG = STATE[:]

    for _ in range(nRounds//2):
        qround(STATE, 0, 4, 8,12)
        qround(STATE, 1, 5, 9,13)
        qround(STATE, 2, 6,10,14)
        qround(STATE, 3, 7,11,15)
        qround(STATE, 0, 5,10,15)
        qround(STATE, 1, 6,11,12)
        qround(STATE, 2, 7, 8,13)
        qround(STATE, 3, 4, 9,14)

    OUT = [ ((i+j) & 0xffffffff) for (i,j) in zip(ORIG,STATE) ]

    return struct.pack("<16L", *OUT)

def experiment(key, nonce, skip, rounds=20):
    print("================================================================")
    p = skip//64
    out = b"".join(chacha_block(p+r, nonce, key, rounds) for r in range(8) )
    print("cipher: CHACHA%s"%rounds)
    print("   key: %s"%binascii.b2a_hex(key).decode())
    print(" nonce: %s"%binascii.b2a_hex(nonce).decode())
    print("offset: %s"%skip)
    for i in range(0, len(out), 32):
        print(binascii.b2a_hex(out[i:i+32]).decode())

def X(key,nonce,skip):
    experiment(key,nonce,skip,8)
    experiment(key,nonce,skip,12)
    experiment(key,nonce,skip,20)

if 1:
  X(b"helloworld!helloworld!helloworld",
    b"!hellowo", 0);
  X(b"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    b"\0\0\0\0\0\0\0\0", 0);
  X(b"helloworld!helloworld!helloworld",
    b"!hellowo", 8192);
  X(b"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    b"\0\0\0\0\0\0\0\0", 8192);
  X(b"Zombie ipsum reversus ab viral i", b"nferno, ", 128)
  X(b"nam rick grimes malum cerebro. D", b"e carne ", 512)
  X(b"lumbering animata corpora quaeri", b"tis. Sum", 640)
  X(b"mus brains sit, morbo vel malefi", b"cia? De ", 704)

