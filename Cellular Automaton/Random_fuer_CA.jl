module Random_fuer_CA

const EPS = 1.2e-7
const RNMX = (1.0 - EPS)

const IM1 = 2147483563
const IM2 = 2147483399
const AM1 = convert(Float64, 1.0/IM1)
const IMM1 = IM1-1
const IA1 = 40014
const IA2 = 40692
const IQ1 = 53668
const IQ2 = 52774
const IR1 = 12211
const IR2 = 3791

const NTAB = 32
const NDIV = 1+IMM1/NTAB

   mutable struct randomStruct
      state1::Int32
      state2::Int32
      y::Int32
      v::Array
   end

   function initRandomSeedLEcuyer(seed::Int32, r::randomStruct)
      r.state1 = seed
      if (r.state1 == 0) r.state1 = 987654321 end
      r.state2 = r.state1
   end

   function initRandomTabLEcuyer(r::randomStruct)
      j::Int32 = 0
      k::Int32 = 0
      for j = NTAB+7:0
         k = r.state1/IQ1
         r.state1 = IA1*(r.state1-k*IQ1)-k*IR1
         if (r.state1 < 0) r.state1 += IM1 end
         if (j < NTAB) r.v[j+1] = r.state1 end
      end
      r.y = r.v[1]
   end

   export initRandomLEcuyer
   function initRandomLEcuyer(seed::Int32)
      #r = randomStruct(987654321, 0, 0, Array{Int32}(undef,NTAB)) ich weiß nicht ob das so sein sollte
      r = randomStruct(987654321, 0, 0, zeros(Int32,NTAB))
      initRandomSeedLEcuyer(seed, r)
      initRandomTabLEcuyer(r)
      return r
   end

   export nextRandomLEcuyer
   function nextRandomLEcuyer(r::randomStruct)
      k::Int32 = 0
      result::Float64 = NaN
      j::Int64 = 0

      k = round(r.state1/IQ1)# Mögliche Ursache für abweichungen
      r.state1 = IA1*(r.state1-k*IQ1)-k*IR1
      if(r.state1 < 0) r.state1 += IM1 end

      k = round(r.state2/IQ2)
      r.state2 = IA2*(r.state2-k*IQ2)-k*IR2
      if(r.state2 < 0) r.state2 += IM2 end

      j = floor(r.y/NDIV)
      r.y = r.v[j+1] - r.state2
      r.v[j+1] = r.state1

      if(r.y<1) r.y+= IMM1 end
      result = AM1*r.y
      if(result >= 1.0) result = RNMX end
      return result
   end
end
