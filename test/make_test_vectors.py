#!/usr/bin/python
#
# Here's a minimal chacha implementation, based on the paper, in
# Python.  Its only purpose here is to validate the output of the C
# implementation.  If you use this for crypto, you're a bad person.

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

    STATE = list(struct.unpack("<LLLL", "expand 32-byte k"))
    STATE += struct.unpack("<LLLLLLLL", key)
    STATE.append(position & 0xffffffff)
    STATE.append(position >> 32)
    STATE += struct.unpack("<LL", nonce)

    ORIG = STATE[:]

    for _ in xrange(nRounds//2):
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
    print "================================================================"
    p = skip//64
    out = "".join(chacha_block(p+r, nonce, key, rounds) for r in xrange(8) )
    print "cipher: ChaCha%s"%rounds
    print "   key: %s"%binascii.b2a_hex(key)
    print " nonce: %s"%binascii.b2a_hex(nonce)
    print "offset: %s"%skip
    for i in xrange(0, len(out), 32):
        print binascii.b2a_hex(out[i:i+32])

def X(key,nonce,skip):
    experiment(key,nonce,skip,8)
    experiment(key,nonce,skip,12)
    experiment(key,nonce,skip,20)

if 1:
  X("helloworld!helloworld!helloworld",
    "!hellowo", 0);
  X("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    "\0\0\0\0\0\0\0\0", 0);
  X("helloworld!helloworld!helloworld",
    "!hellowo", 8192);
  X("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    "\0\0\0\0\0\0\0\0", 8192);
  X("Zombie ipsum reversus ab viral i", "nferno, ", 128)
  X("nam rick grimes malum cerebro. D", "e carne ", 512)
  X("lumbering animata corpora quaeri", "tis. Sum", 640)
  X("mus brains sit, morbo vel malefi", "cia? De ", 704)

