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

-- Compute a chacha double-round on a 16-tuple of words.
tworounds :: (Word32, Word32, Word32, Word32, Word32, Word32, Word32, Word32,
          Word32, Word32, Word32, Word32, Word32, Word32, Word32, Word32)->(
          Word32, Word32, Word32, Word32, Word32, Word32, Word32, Word32,
          Word32, Word32, Word32, Word32, Word32, Word32, Word32, Word32)

tworounds (x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15) =
      let (x0' , x4' , x8' ,  x12')  = qround (x0 , x4 , x8 , x12)
          (x1' , x5' , x9' ,  x13')  = qround (x1 , x5 , x9 , x13)
          (x2' , x6' , x10',  x14')  = qround (x2 , x6 , x10, x14)
          (x3' , x7' , x11',  x15')  = qround (x3 , x7 , x11, x15)

          (x0'', x5'', x10'', x15'') = qround (x0',x5',x10',x15')
          (x1'', x6'', x11'', x12'') = qround (x1',x6',x11',x12')
          (x2'', x7'', x8'',  x13'') = qround (x2',x7',x8', x13')
          (x3'', x4'', x9'',  x14'') = qround (x3',x4',x9', x14')

       in (x0'',x1'',x2'',x3'',x4'',x5'',x6'',x7'',
           x8'',x9'',x10'',x11'',x12'',x13'',x14'',x15'')

-- add two 16-word tuples.
plus (x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15) (
     y0,y1,y2,y3,y4,y5,y6,y7,y8,y9,y10,y11,y12,y13,y14,y15) =
     (x0+y0,x1+y1,x2+y2,x3+y3,
      x4+y4,x5+y5,x6+y6,x7+y7,
      x8+y8,x9+y9,x10+y10,x11+y11,
      x12+y12,x13+y13,x14+y14,x15+y15)

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
         dd = fromIntegral c
         cc = fromIntegral d
         num = aa + bb * 2^8 + cc * 2^16 + dd * 2^24
      in num : bytesToWords rest

-- Convert a list of words to a list of bytes, in little-endian order
wordsToBytes :: [Word32] -> [Word8]
wordsToBytes [] = []
wordsToBytes (w:rest) =
     let a = fromIntegral (w .&. 255)
         b = fromIntegral ((w `shiftR` 8) .&. 255)
         c = fromIntegral ((w `shiftR` 16) .&. 255)
         d = fromIntegral ((w `shiftR` 24) .&. 255)
      in a:b:c:d : wordsToBytes rest

-- Given a number of rounds, a chacha key-and-nonce (as a 10-tuple of Word32),
-- and a block counter, return a list of Word32 for the block.
chachaWords :: Integer -> (Word32,Word32,Word32,Word32,Word32,Word32,Word32,Word32,Word32,Word32) -> Word32 -> [Word32]
chachaWords rounds (k0,k1,k2,k3,k4,k5,k6,k7,n0,n1) ctr =
     let sigma0 = 1634760805 :: Word32
         sigma1 = 857760878 :: Word32
         sigma2 = 2036477234 :: Word32
         sigma3 = 1797285236 :: Word32
         inp = (sigma0, sigma1, sigma2, sigma3,
                k0,k1,k2,k3,
                k4,k5,k6,k7,
                ctr,0,n0,n1)
         (o0,o1,o2,o3,o4,o5,o6,o7,
          o8,o9,o10,o11,o12,o13,o14,o15) = chachafn rounds inp
      in [o0,o1,o2,o3,o4,o5,o6,o7,o8,o9,o10,o11,o12,o13,o14,o15]

-- Hex encoding/decoding. Is there a library for this?  There really
-- should be.

hexval '0' = 0
hexval '1' = 1
hexval '2' = 2
hexval '3' = 3
hexval '4' = 4
hexval '5' = 5
hexval '6' = 6
hexval '7' = 7
hexval '8' = 8
hexval '9' = 9
hexval 'a' = 10
hexval 'b' = 11
hexval 'c' = 12
hexval 'd' = 13
hexval 'e' = 14
hexval 'f' = 15

hexDigit 0 = '0'
hexDigit 1 = '1'
hexDigit 2 = '2'
hexDigit 3 = '3'
hexDigit 4 = '4'
hexDigit 5 = '5'
hexDigit 6 = '6'
hexDigit 7 = '7'
hexDigit 8 = '8'
hexDigit 9 = '9'
hexDigit 10 = 'a'
hexDigit 11 = 'b'
hexDigit 12 = 'c'
hexDigit 13 = 'd'
hexDigit 14 = 'e'
hexDigit 15 = 'f'

-- Convert a hex string to a list of bytes
fromHex :: String -> [Word8]
fromHex "" = []
fromHex (a:b:rest) = ((hexval a) * 16 + hexval b) : fromHex rest

-- Convert a list of bytes to a hex string.
toHex :: [Word8] -> String
toHex [] = ""
toHex (byte:rest) = hexDigit (byte `div` 16) : hexDigit (byte .&. 15) : toHex rest

{- Now define an intermediate PRF type.

   A PRF has a a given key
   length, an output length, a PRF function that takes a key and a
   counter to make a block, and a key setup function that converts a
   bytestring into a key.
-}

data PRF a = PRF { keyLen :: Int,
	  	   bytesPerBlock :: Int,
		   func :: ( a -> Word32 -> [Word8] ),
		   keyFunc :: ( [Word8] -> a )
                 }

{-
  Wrap the ChaCha block function up as a PRF.
-}

-- Convert a 40-byte sequence into a 10-tuple of Word32 as used for a key
-- by chaChaWords.
chachaKey :: [Word8] -> (Word32,Word32,Word32,Word32,Word32,Word32,Word32,Word32,Word32,Word32)
chachaKey bytes =
	  let [k0,k1,k2,k3,k4,k5,k6,k7,n0,n1] = bytesToWords bytes
	   in (k0,k1,k2,k3,k4,k5,k6,k7,n0,n1)

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

-- Yield nBlocks blocks of data from the PRF.  It's vital to re-key after
-- calling this function.
generate prng nBlocks =
     concat [ (func (prf prng)) (key prng) (fromIntegral ctr) | ctr <- [0..nBlocks-1] ]

-- Helper: how many bytes should we get getting per call to generate?
bytesPerCall prng = (blocksPerCall prng) * (bytesPerBlock (prf prng))

-- Pull n buffered bytes from a prng, and return the PRNG's new state and the
-- returned bytes.  Gives an error if the n bytes are not buffered.
extract :: PRNG a -> Int -> (PRNG a, [Word8])
extract prng n
	| (n <= length (buf prng)) =
                 let (rv, nb) = splitAt n (buf prng)
                     nprng = PRNG {key=(key prng),
		                   blocksPerCall=(blocksPerCall prng),
				   prf=(prf prng),
				   buf=nb}
                  in (nprng, rv)
        | otherwise = error "Tried to extract too many bytes!"


-- Get a small number of bytes from the prng, stirring first if they aren't
-- thre.  This is how libottery implements requests for uint32 and uint64.
-- Return the new prng state and the requested bytes.
getTiny :: PRNG a -> Int -> (PRNG a, [Word8])
getTiny prng n
	| (n <= length (buf prng)) = extract prng n
        | otherwise = extract (stir prng) n

-- Get a few bytes from the prng, extracting greedily from the buffer
-- at first, and then stirring to make more. This how libottery
-- answers small requests for a number of bytes.
-- Return the new prng state and the requested bytes.
getSmall :: PRNG a -> Int -> (PRNG a, [Word8])
getSmall prng n
        | (n <= length (buf prng)) = extract prng n
	| otherwise =
             let oldBuf = buf prng
	         prng' = stir prng
		 (prng'', remainder) = extract prng' (n - length oldBuf)
	      in (prng'' , (oldBuf ++ remainder) )

-- Divide integer A by nonnegative integer B, rounding up.
divRoundUp a b = (a + b - 1) `div` b

-- Generate a bunch of bytes from the PRNG, extracting first from the buffer,
-- then generating blocks in bulk to answer the entire user request and
-- rekey.
-- Return the new prng state and the requested bytes.
getLots :: PRNG a -> Int -> (PRNG a, [Word8])
getLots prng n =
	let oldBuf = buf prng
	    bytesNeeded = n + (keyLen (prf prng)) - (length oldBuf)
	    bpc = blocksPerCall prng
	    blockLen = bytesPerCall prng
	    blocksNeeded = bytesNeeded `divRoundUp` blockLen
	    output = generate prng (blocksNeeded * bpc)
	    (middle, lastBlock) = splitAt (bpc * (blocksNeeded-1)) output
	    (newKeyBytes, newBuf) = splitAt (keyLen (prf prng)) lastBlock
	    newKey = (keyFunc (prf prng)) newKeyBytes
	    prng' = PRNG { key=newKey, blocksPerCall=bpc, prf=(prf prng), buf=newBuf}
	    (prng'', final) = extract prng' (n - (length oldBuf) - (length middle))
	 in (prng'', (oldBuf ++ middle ++ final))

-- Exposed PRNG functions

-- Construct a new PRNG.
initPRNG prf blocksPerCall initialkey =
	 let k = (keyFunc prf) initialkey
	  in stir PRNG { key=k, blocksPerCall=blocksPerCall, prf=prf, buf=[] }


-- Generate more buffered material for the PRNG, and rekey for forward
-- secrecy.
stir :: PRNG a -> PRNG a
stir prng =
     let newBlock = generate prng (blocksPerCall prng)
         (newKeyBytes, newBuf) = splitAt (keyLen (prf prng)) newBlock
	 newKey = (keyFunc (prf prng)) newKeyBytes
      in PRNG { key=newKey, blocksPerCall=(blocksPerCall prng), prf=(prf prng), buf=newBuf }

-- Add more entropy to the PRNG.
addSeed :: PRNG a -> [Word8] -> PRNG a
addSeed prng seed
     | (seed == []) = stir prng
     | otherwise =
        let klen = keyLen (prf prng)
            x = take klen (generate prng (blocksPerCall prng))
	    (y, rest) = splitAt (keyLen (prf prng)) seed
	    newKeyBytes = [ a^b | (a,b) <- (zip x y) ]
	    newKey = (keyFunc (prf prng)) newKeyBytes
	    prng' = PRNG {key=newKey,blocksPerCall=(blocksPerCall prng),prf=(prf prng),buf=[]}
	 in addSeed prng' rest

-- Return a random 32-bit integer from the PRNG. Return the new PRNG state
-- and the integer.
getU32 :: PRNG a -> (PRNG a, Word32)
getU32 prng =
      let (prng', bytes) = getTiny prng 4
          [w] = bytesToWords bytes
       in (prng', w)

-- Return a random list of n bytes from the PRNG. Return the new PRNG state
-- and the byte list.
getBytes :: PRNG a -> Int -> (PRNG a, [Word8])
getBytes prng n
     | n < ((length (buf prng)) + (bytesPerCall prng) - (keyLen (prf prng))) =
        getSmall prng n
     | otherwise = getLots prng n

