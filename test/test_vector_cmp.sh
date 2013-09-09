#!/bin/sh

test -f test/test_vectors.expected        || exit 77
test -f test/test_vectors.actual          || exit 77
test -f test/test_vectors.actual-midrange || exit 77
test -f test/test_vectors.actual-nosimd   || exit 77

cmp test/test_vectors.expected test/test_vectors.actual || exit 1
cmp test/test_vectors.expected test/test_vectors.actual-midrange || exit 1
cmp test/test_vectors.expected test/test_vectors.actual-nosimd || exit 1
