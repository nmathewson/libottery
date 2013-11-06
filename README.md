libottery -- drop-in secure replacement for your RNG.
=====================================================

This project is meant to provide a drop-in replacement for the horrible
random number generator you are using today.

The RNG you are using today is probably horrible because:

  - If it's fast, it's probably very insecure.  If you have ever gotten
    bytes from random() or rand() or mt_rand() or whatever and exposed
    them to the world, you have typically just let the world know what
    bytes you're going to be using from now to the end of time.

  - If it's secure, it's probably slow.  So you probably have an
    insecure one that you use when you need random numbers fast.

  - If it's arc4random, then you probably think it's secure, but it
    isn't, really.  Even after you do the "discard the first N bytes"
    trick, RC4 still has detectable statistical biases.

This tool aims to be a speed-competitive replacement for arc4random, for
your libc's random() or rand() function.  It aims to be so fast that you
will never need to think "do I need to use a secure random number
generator here, or can I use a fast one?"

It currently uses Dan Bernstein's ChaCha stream cipher as its base.
When it goes fast, it goes fast because of Ted Krovetz's implementation.


WARNING WARNING WARNING
-----------------------

YOU WOULD HAVE TO BE SOME KIND OF LUNATIC TO USE THIS IN PRODUCTION CODE
RIGHT NOW.  It is so alpha that it begins the Greek alphabet.  It is so
alpha that Jean-Luc Godard is filming there.  It is so alpha that it's
64-bit RISC from the 1990s.  It's so alpha that it'll try to tell you
that you belong to everyone else.  It's so alpha that when you turn it
sideways, it looks like an ox.  It's so alpha that the Planck constant
wobbles a little bit whenever I run the unit tests.

I *will* break backward compatibility more than once. (Probably.)

I *will* change something you were depending on. (Or at least, I won't
promise not to.)

There *might* be horrible security bugs left in it. If there are, I
won't be very embarrassed: I told you not to use it yet!

If it breaks, you *don't* get to keep both pieces!  I will come over and
break the pieces into even smaller pieces, then break something else
that you actually liked, then point at them and laugh and laugh and
laugh.



            DO YOU BEGIN TO GRASP THE TRUE MEANING OF "ALPHA"?



(To people without my particular sense of humor: the purpose of this
section is to make you not use libottery in production code yet, because
it isn't ready. If it makes you nervous about using this version of the
software in production: good!  That's the point.)

How to build it
---------------

    ./autogen.sh
    ./configure
    make
    make check

If that doesn't work, debug the program.

Yes, I know autotools is a pain, but I've outgrown what I'm happy doing
in gmake alone. I welcome ports to other build tools, but only if they
get the full functionality of the current build system.

I've tested with clang and gcc, on Linux and OSX.  I've done most of the
testing on Intel x86_64 chips, and a little bit on ARM.  I will try
other places eventually.  I especially value interoperability with free
platforms.

How to use it (the basics)
--------------------------

    /* Include the main header. */
    #include <ottery.h>

    /* Get a random value between 0 and UINT_MAX, inclusive. */
    unsigned u = ottery_rand_unsigned();

    /* Get a random value between 0 and 1337, inclusive. */
    unsigned u2 = ottery_rand_range(1337);

    /* Get 128 random bytes. */
    unsigned char buf[128];
    ottery_rand_bytes(buf, sizeof(buf));


You can build your program with:

    gcc -Wall -O2 -g myprog.c -o myprog -lottery

See the comments in ottery.h and ottery_st.h for more information.


Details
-------

A libottery PRNG seeds itself from your operating system's (hopefully)
secure random number generator.  This seed is used as the key and nonce
for a (hopefully) strong, fast stream cipher.  Every time it generates a
"block" (about 1024 bytes), it uses a portion of its own output to
replace its key and nonce, so that the key it used to make those bytes
is unrecoverable.  After it extracts bytes from the RNG, it clears them
from its internal buffers.  (This is based on a construction described
in XXX, as summarized by DJB.)

By default, I'm trying to make libottery as safe to use as possible.
Therefore, I am making it threadsafe by default, and forksafe by
default.  (That is, you don't need to tell it to lock itself to tolerate
concurrent access, and you don't need to explicitly reinitialize it
after each fork().)  It also tries to make it impossible to accidentally
use it unseeded.  Later versions will try to resist even more common
mistakes.

Getting good performance for large-sized requests is a simple matter of
choosing a fast secure stream generator.  Fortunately, we have a bunch
of those.  Right now, libottery uses Dan Bernstein's ChaCha cipher,
where even a naive implementation is pretty fast, and an optimized one
can be downright scary.

For small request performance (say, somebody wants to generate a bunch
of random 4-byte unsigned ints), the overhead dominates. Right now, for
a 4-byte request, the majority of our time goes into locking, checking
whether we've forked, and making sure that the RNG is really
initialized.

[1] http://csrc.nist.gov/groups/ST/toolkit//documents/rng/HashBlockCipherDRBG.pdf

Speed comparison
----------------

On my Core i7 laptop running OSX: If you're only generating a 4-byte
unsigned, the ChaCha8 implementation is currently 8% slower than
arc4random(), and the ChaCha20 implementation is 25% slower.  We're
faster than arc4random() once you're generating at least 8 bytes per go.
We're faster than libc random() once you're generating at least 16 bytes
per go.

In one test, I turned off all the safety features for a laugh (NOT
RECOMMENDED!), and libottery was faster than libc random() even for 4-
byte requests.

At this point, I'm not sweating performance too badly.  Eventually, I'll
add a section about how to get better performance with compiler choices
and different options, but for now, don't sweat it.

One area where I *do* want to do better in the future is performance in
high-contention scenarios where the PRNG is getting used by many threads
at once.


Digression: What's wrong with arc4random()?
-------------------------------------------

It's a broken old cipher: Go read the wikipedia page.  Discarding the
first N bytes of its output mitigates the very worst attacks against it,
but it still has detectable statistical biases.

Also, nearly any implementation of it does secret-dependent data
lookups. That's a big no-no in modern cryptographic design.

Least of all, it's not really performance-competitive any more. But if
we can go faster and more secure, why wouldn't we?

The cipher might have been a good choice back in the mid-90s, when
OpenBSD first added arc4random.  But nowadays, we can do better, and
should. In fact OpenBSD replaced rc4 with ChaCha20 in the last version.

Digression: What's wrong with libc's random()/rand() calls?
-----------------------------------------------------------

It's probably a linear congruential generator, depending on what your
operating system provides.  That's completely insecure: anybody who sees
one or two outputs from it can predict all past and future outputs.  Or
they could search the seed-space, which is not very big. Worse, it's
probably slower than a well-designed modern stream cipher, so it doesn't
even have an excuse for being insecure.


Digression: What's wrong with your favorite cryptography library's PRNG?
------------------------------------------------------------------------

Probably nothing in terms of its security, assuming it does a reasonable
job of seeding itself.  But odds are good that it's pretty slow, which
can make trouble for programmers: in performance critical code that eats
a ton of random numbers, they're going to find themselves tempted to
just use random(), because really, what could it matter?

Intellectual Property Notices
-----------------------------

There are no known present or future claims by a copyright holder that
the distribution of this software infringes the copyright. In
particular, the author of the software is not making such claims and
does not intend to make such claims.  Additionally, Nick Mathewson has
dedicated his code here to the public domain using the CC0 public domain
dedication; see doc/cc0.txt or
<http://creativecommons.org/publicdomain/zero/1.0/> for full details.

There are no known present or future claims by a patent holder that the
use of this software infringes the patent. In particular, the author of
the software is not making such claims and does not intend to make such
claims.

This code is in the public domain.
