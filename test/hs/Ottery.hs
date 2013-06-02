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

module Ottery (
   ChaChaPRNG,
   initChaCha8PRNG,
   initChaCha12PRNG,
   initChaCha20PRNG,
   addSeed,
   getU32,
   getU64,
   getBytes,
) where

import Data.Word
import Data.Bits
import ChaCha

{- Now define an intermediate PRF type.

   A PRF has a a given key
   length, an output length, a PRF function that takes a key and a
   counter to make a block, and a key setup function that converts a
   bytestring into a key.
-}

data PRF a = PRF { keyLen :: Int,
	  	   bytesPerBlock :: Int,
		   func :: a -> Word32 -> [Word8],
		   keyFunc :: [Word8] -> a
                 }

{-
  Wrap the ChaCha block function up as a PRF.
-}

-- Convert a 40-byte sequence into a 10-tuple of Word32 as used for a key
-- by chaChaWords.
chachaKey :: [Word8] -> ChaChaKey
chachaKey bytes =
	  let
              [k0,k1,k2,k3,k4,k5,k6,k7,n0,n1] = bytesToWords bytes
	   in
              ChaChaKey (k0,k1,k2,k3,k4,k5,k6,k7,n0,n1)

-- A ChaCha-based PRF.
chachaPRF rounds = PRF { keyLen = 40,
		         bytesPerBlock = 64,
		         func = \k ctr -> wordsToBytes (chachaWords 8 k ctr),
		         keyFunc = chachaKey
		       }

chacha8PRF = chachaPRF 8
chacha12PRF = chachaPRF 12
chacha20PRF = chachaPRF 20

{-
   And now at last, our PRNG.  A PRNG state has a current key (which changes
   a lot), a number of blocks to extract every time we invoke the PRF
   (which parameterizes the forward-secrecy vs efficiency tradeoff), a PRF,
   and a buffer of bytes that we've generated but which we haven't given
   to the user yet.
-}

data PRNG a = PRNG { key :: a,
                     blocksPerCall :: Int,
                     prf :: PRF a,
                     buf :: [Word8] }

{-
  Internal prng functions
-}

updatePRNG prng newKey newBuf =
     PRNG { key=newKey,
            blocksPerCall=blocksPerCall prng,
	    prf=prf prng,
	    buf=newBuf }

-- Yield nBlocks blocks of data from the PRF.  It's vital to re-key after
-- calling this function.
generate prng nBlocks =
     concat [ func (prf prng) (key prng) (fromIntegral ctr) | ctr <- [0..nBlocks-1] ]

-- Helper: how many bytes should we get getting per call to generate?
bytesPerCall prng = blocksPerCall prng * bytesPerBlock (prf prng)

-- Pull n buffered bytes from a prng, and return the PRNG's new state and the
-- returned bytes.  Gives an error if the n bytes are not buffered.
extract :: PRNG a -> Int -> (PRNG a, [Word8])
extract prng n
	| n <= length (buf prng) =
                 let (rv, nb) = splitAt n (buf prng)
                     nprng = updatePRNG prng (key prng) nb
                  in (nprng, rv)
        | otherwise = error "Tried to extract too many bytes!"


-- Get a small number of bytes from the prng, stirring first if they aren't
-- thre.  This is how libottery implements requests for uint32 and uint64.
-- Return the new prng state and the requested bytes.
getTiny :: PRNG a -> Int -> (PRNG a, [Word8])
getTiny prng n
	| n <= length (buf prng) = extract prng n
        | otherwise = extract (stir prng) n

-- Get a few bytes from the prng, extracting greedily from the buffer
-- at first, and then stirring to make more. This how libottery
-- answers small requests for a number of bytes.
-- Return the new prng state and the requested bytes.
getSmall :: PRNG a -> Int -> (PRNG a, [Word8])
getSmall prng n
        | n <= length (buf prng) = extract prng n
	| otherwise =
             let oldBuf = buf prng
	         prng' = stir prng
		 (prng'', remainder) = extract prng' (n - length oldBuf)
	      in (prng'', oldBuf ++ remainder)

-- Divide integer A by nonnegative integer B, rounding up.
divRoundUp a b = (a + b - 1) `div` b

-- Generate a bunch of bytes from the PRNG, extracting first from the buffer,
-- then generating blocks in bulk to answer the entire user request and
-- rekey.
-- Return the new prng state and the requested bytes.
getLots :: PRNG a -> Int -> (PRNG a, [Word8])
getLots prng n =
	let oldBuf = buf prng
	    bytesNeeded = n + keyLen (prf prng) - length oldBuf
	    bpc = blocksPerCall prng
	    blockLen = bytesPerCall prng
	    callsNeeded = bytesNeeded `divRoundUp` blockLen
	    output = generate prng (callsNeeded * bpc)
	    (middle, lastBlock) = splitAt (blockLen * (callsNeeded-1)) output
	    (newKeyBytes, newBuf) = splitAt (keyLen (prf prng)) lastBlock
	    newKey = keyFunc (prf prng) newKeyBytes
	    prng' = updatePRNG prng newKey newBuf
	    (prng'', final) = extract prng' (n - length oldBuf - length middle)
	 in (prng'', oldBuf ++ middle ++ final)

-- Exposed PRNG functions

-- Construct a new PRNG.
initPRNG prf blocksPerCall initialkey =
	 let k = keyFunc prf initialkey
	  in stir PRNG { key=k, blocksPerCall=blocksPerCall, prf=prf, buf=[] }


-- Generate more buffered material for the PRNG, and rekey for forward
-- secrecy.
stir :: PRNG a -> PRNG a
stir prng =
     let newBlock = generate prng (blocksPerCall prng)
         (newKeyBytes, newBuf) = splitAt (keyLen (prf prng)) newBlock
	 newKey = keyFunc (prf prng) newKeyBytes
      in updatePRNG prng newKey newBuf

zeropad :: Int -> [Word8] -> [Word8]
zeropad n lst = take n (lst ++ repeat 0)

-- Add more entropy to the PRNG.
addSeed :: PRNG a -> [Word8] -> PRNG a
addSeed prng seed
     | null seed = stir prng
     | otherwise =
        let klen = keyLen (prf prng)
            x = take klen (generate prng (blocksPerCall prng))
	    (y, rest) = splitAt klen seed
	    y' = zeropad klen y
	    newKeyBytes = [ a `xor` b | (a,b) <- zip x y' ]
	    newKey = keyFunc (prf prng) newKeyBytes
	    prng' = updatePRNG prng newKey []
	 in addSeed prng' rest

-- Return a random 32-bit integer from the PRNG. Return the new PRNG state
-- and the integer.
getU32 :: PRNG a -> (PRNG a, Word32)
getU32 prng =
      let (prng', bytes) = getTiny prng 4
          [w] = bytesToWords bytes
       in (prng', w)

getU64 :: PRNG a -> (PRNG a, Word64)
getU64 prng =
       let
          (prng', bytes) = getTiny prng 8
	  [w_lo, w_hi] = bytesToWords bytes
	  w64 = fromIntegral w_lo + (fromIntegral w_hi `shiftL` 32)
	in (prng', w64)

-- Return a random list of n bytes from the PRNG. Return the new PRNG state
-- and the byte list.
getBytes :: PRNG a -> Int -> (PRNG a, [Word8])
getBytes prng n
     | n < length (buf prng) + bytesPerCall prng - keyLen (prf prng) =
        getSmall prng n
     | otherwise = getLots prng n

-- Instantiate PRNG.

type ChaChaPRNG = PRNG ChaChaKey
initChaCha8PRNG = initPRNG chacha8PRF
initChaCha12PRNG = initPRNG chacha12PRF
initChaCha20PRNG = initPRNG chacha20PRF
