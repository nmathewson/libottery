{-
   This software has been dedicated to the public domain under the CC0
   public domain dedication.

   To the extent possible under law, the person who associated CC0 with
   libottery has waived all copyright and related or neighboring rights
   to libottery.

   You should have received a copy of the CC0 legalcode along with this
   work in doc/cc0.txt.  If not, see
      <http://creativecommons.org/publicdomain/zero/1.0/>.
-}

-- An attempt to reimplement the libottery spec in Haskell.

-- I'm really bad at Haskell, so watch out.  It's here for testing.
-- If you use this for real crypto, you're building your edifice on a
-- foundation of sticks and thatch and inefficiency and side-channel
-- attacks.

-- (I'm using Haskell here because it's a good way to turn specs into code.
-- Though I'm probably doing that wrong.)

module ChaCha (
   wordsToBytes,
   bytesToWords,
   chachaWords,
   ChaChaKey(ChaChaKey)
) where

import Data.Word
import Data.Bits

{-
   Here's the ChaCha block function that implements the ChaCha8,
   ChaCha12, and ChaCha20 stream ciphers.  See above about not using
   this in production environments.
-}


-- Apply the function 'fn' to 'x' N times.
nTimes fn 0 x = x
nTimes fn n x = nTimes fn (n - 1) (fn x)

-- Compute the ChaCha quarter-round function on a 4-tuple of words.
qround :: (Word32, Word32, Word32, Word32)->(Word32, Word32, Word32, Word32)
qround (a,b,c,d) =
       let a'  = a + b
           d'  = rotate (d `xor` a') 16
           c'  = c + d'
           b'  = rotate (b `xor` c') 12
           a'' = a' + b'
           d'' = rotate (d' `xor` a'') 8
           c'' = c' + d''
           b'' = rotate (b' `xor` c'') 7
        in (a'', b'', c'', d'')

data ChaChaState = ChaChaState (
          Word32, Word32, Word32, Word32, Word32, Word32, Word32, Word32,
          Word32, Word32, Word32, Word32, Word32, Word32, Word32, Word32)

listToState [x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15] =
   ChaChaState (x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15)

stateToList s =
    [x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15]
    where
      ChaChaState (x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15) = s

-- Compute a chacha double-round on a 16-tuple of words.
tworounds :: ChaChaState -> ChaChaState
tworounds state =
      let
          ChaChaState (x0,x1,x2,x3,x4,x5,x6,x7,
                       x8,x9,x10,x11,x12,x13,x14,x15) = state
          (x0' , x4' , x8' ,  x12')  = qround (x0 , x4 , x8 , x12)
          (x1' , x5' , x9' ,  x13')  = qround (x1 , x5 , x9 , x13)
          (x2' , x6' , x10',  x14')  = qround (x2 , x6 , x10, x14)
          (x3' , x7' , x11',  x15')  = qround (x3 , x7 , x11, x15)

          (x0'', x5'', x10'', x15'') = qround (x0',x5',x10',x15')
          (x1'', x6'', x11'', x12'') = qround (x1',x6',x11',x12')
          (x2'', x7'', x8'',  x13'') = qround (x2',x7',x8', x13')
          (x3'', x4'', x9'',  x14'') = qround (x3',x4',x9', x14')

       in ChaChaState (x0'',x1'',x2'',x3'',x4'',x5'',x6'',x7'',
           x8'',x9'',x10'',x11'',x12'',x13'',x14'',x15'')

-- add two ChaCha states.
plus x y =
  listToState [ x+y | (x,y) <- zip (stateToList x) (stateToList y) ]

-- Compute N rounds of the ChaCha function on a 16-tuple of words.
chachafn rounds input =
    let result = nTimes tworounds (rounds `div` 2) input
     in plus result input

-- Convert a list of bytes to a list of words, in little-endian order
bytesToWords :: [Word8] -> [Word32]
bytesToWords [] = []
bytesToWords (a:b:c:d:rest) =
     let aa = fromIntegral a
         bb = fromIntegral b
         cc = fromIntegral c
         dd = fromIntegral d
         num = aa + bb * 2^8 + cc * 2^16 + dd * 2^24
      in num : bytesToWords rest
bytesToWords [a,b,c] = error "Only three elements"
bytesToWords [a,b] = error "Only two elements"
bytesToWords [a] = error "Only one elements"

-- Convert a list of words to a list of bytes, in little-endian order
wordsToBytes :: [Word32] -> [Word8]
wordsToBytes [] = []
wordsToBytes (w:rest) =
     let a = fromIntegral (w .&. 255)
         b = fromIntegral ((w `shiftR` 8) .&. 255)
         c = fromIntegral ((w `shiftR` 16) .&. 255)
         d = fromIntegral ((w `shiftR` 24) .&. 255)
      in a:b:c:d : wordsToBytes rest

data ChaChaKey = ChaChaKey (Word32,Word32,Word32,Word32,Word32,Word32,Word32,Word32,Word32,Word32)

-- Given a number of rounds, a chacha key-and-nonce (as a 10-tuple of Word32),
-- and a block counter, return a list of Word32 for the block.
chachaWords :: Integer -> ChaChaKey -> Word32 -> [Word32]
chachaWords rounds key ctr =
      stateToList (chachafn rounds inp)
      where
         ChaChaKey (k0,k1,k2,k3,k4,k5,k6,k7,n0,n1) = key
         sigma0 = 1634760805 :: Word32
         sigma1 = 857760878 :: Word32
         sigma2 = 2036477234 :: Word32
         sigma3 = 1797285236 :: Word32
         inp = ChaChaState (sigma0, sigma1, sigma2, sigma3,
                k0,k1,k2,k3,
                k4,k5,k6,k7,
                ctr,0,n0,n1)


