import Data.Word
import Control.Monad.State

import qualified Ottery as O

-- Use some monads to make this testable.

type PRNGState = State O.ChaChaPRNG

u32 :: PRNGState Word32
u32 = do prng <- get
         let (prng', result) = O.getU32 prng
	 put prng'
	 return result

u64 :: PRNGState Word64
u64 = do prng <- get
         let (prng', result) = O.getU64 prng
	 put prng'
	 return result

nBytes n = do prng <- get
              let (prng', result) = O.getBytes prng n
	      put prng'
	      return result

stir :: PRNGState ()
stir = do prng <- get
          put (O.stir prng)
	  return ()

addSeed seed =
    do
       prng <- get
       let prng' = O.addSeed prng seed
       put prng'
       return ()

