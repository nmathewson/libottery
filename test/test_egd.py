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

import atexit
from binascii import b2a_hex
import os
import subprocess
import unittest

FAKE_EGD = "./test/fake_egd"
TEST_EGD = "./test/test_egd"
SOCKNAME = "./test-socket-%s" % os.getpid()

o_fortuna = (
  "Sors immanis et inanis rota tu volubilis status malus vana salus "
  "semper dissolubilis obumbrata et velata michi quoque niteris nunc "
  "per ludum dorsum nudum fero tui sceleris Sors salutis et virtutis "
  "michi nunc contraria est affectus et defectus semper in angaria")

class TestEGDDied(Exception):
    pass

class FakeEGDDied(Exception):
    pass

def cleanup():
    try:
        os.unlink(SOCKNAME)
    except OSError:
        pass

atexit.register(cleanup)

def run_egd(tst_args, egd_args):
    fake_egd = subprocess.Popen([FAKE_EGD]+egd_args, stdout=subprocess.PIPE)
    test_egd = subprocess.Popen([TEST_EGD]+tst_args, stdout=subprocess.PIPE)
    fe_output = fake_egd.stdout.read()
    te_output = test_egd.stdout.read()
    test_egd.wait()
    fake_egd.wait()
    if fake_egd.returncode != 0:
        print fe_output
        raise FakeEGDDied(fake_egd.returncode)
    if test_egd.returncode != 0:
        raise TestEGDDied(fake_egd.returncode)
    os.unlink(SOCKNAME)
    return parse_output(te_output)

def parse_output(te_output):
    res = {}
    for line in te_output.strip().split("\n"):
        k,v = line.split(":")
        res[k] = v
    return res

class EGDTests(unittest.TestCase):
    def tearDown(self):
        try:
            os.unlink(SOCKNAME)
        except OSError:
            pass

    def test_succeed_16(self):
        d = run_egd(["16", SOCKNAME], [SOCKNAME])
        self.assertEquals(d['FLAGS'], '801')
        self.assertEquals(d['BYTES'], b2a_hex(o_fortuna[:16]))

    def test_succeed_0(self):
        d = run_egd(["0", SOCKNAME], [SOCKNAME])
        self.assertEquals(d['FLAGS'], '801')
        self.assertEquals(d['BYTES'], b2a_hex(o_fortuna[:0]))

    def test_succeed_255(self):
        d = run_egd(["255", SOCKNAME], [SOCKNAME])
        self.assertEquals(d['FLAGS'], '801')
        self.assertEquals(d['BYTES'], b2a_hex(o_fortuna[:255]))

    def test_fail_noegd(self):
        cleanup()
        p = subprocess.Popen([TEST_EGD, "16", SOCKNAME], stdout=subprocess.PIPE)
        o = p.stdout.read()
        p.wait()
        d = parse_output(o)
        self.assertEquals(d['ERR'], '3') #init_strong_rng

    def test_fail_badaddr(self):
        cleanup()
        p = subprocess.Popen([TEST_EGD, "16", "_BAD_FAMILY_"],
                             stdout=subprocess.PIPE)
        o = p.stdout.read()
        p.wait()
        d = parse_output(o)
        self.assertEquals(d['ERR'], '3') #init_strong_rng

    def test_fail_toolong_256(self):
        cleanup()
        p = subprocess.Popen([TEST_EGD, "256", SOCKNAME],
                             stdout=subprocess.PIPE)
        o = p.stdout.read()
        p.wait()
        d = parse_output(o)
        self.assertEquals(d['ERR'], '4') #access_strong_rng

    def test_fail_short(self):
        d = run_egd(["16", SOCKNAME], [SOCKNAME, "--short-output"])
        self.assertEquals(d['ERR'], '4') #access_strong_rng

    def test_fail_no_out(self):
        d = run_egd(["16", SOCKNAME], [SOCKNAME, "--no-output"])
        self.assertEquals(d['ERR'], '4') #access_strong_rng

    def test_fail_truncated(self):
        d = run_egd(["16", SOCKNAME], [SOCKNAME, "--truncate-output"])
        self.assertEquals(d['ERR'], '4') #access_strong_rng

    def test_fail_close_early(self):
        d = run_egd(["16", SOCKNAME], [SOCKNAME, "--close-before-read"])
        self.assertEquals(d['ERR'], '4') #access_strong_rng

    def test_fail_close_early(self):
        d = run_egd(["16", SOCKNAME], [SOCKNAME, "--close-after-read"])
        self.assertEquals(d['ERR'], '4') #access_strong_rng



if __name__ == '__main__':
    unittest.main()
