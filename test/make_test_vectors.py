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

def X(key, nonce, skip, rounds=20):
    p = skip//64
    out = "".join(chacha_block(p+r, nonce, key, rounds) for r in (0,1,2,3) )
    print "   key: %s"%binascii.b2a_hex(key)
    print " nonce: %s"%binascii.b2a_hex(nonce)
    print "@%5d:"%skip ,
    for i in xrange(0, len(out), 32):
        print binascii.b2a_hex(out[i:i+32])

if 1:
  X("helloworld!helloworld!helloworld",
    "!hellowo", 0);
  X("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    "\0\0\0\0\0\0\0\0", 0);
  X("helloworld!helloworld!helloworld",
    "!hellowo", 8192);
  X("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
    "\0\0\0\0\0\0\0\0", 8192);


