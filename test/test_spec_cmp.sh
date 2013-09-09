#!/bin/sh

test -f test/test_spec.output        || exit 77
test -f test/hs/test_ottery.output   || exit 77

cmp test/test_spec.output test/hs/test_ottery.output || exit 1

