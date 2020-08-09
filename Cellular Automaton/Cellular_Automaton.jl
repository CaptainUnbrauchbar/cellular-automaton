using Dates # für die Zeit
using Images # zum Zeichnen
using SHA

include("./Random_fuer_CA.jl")
using .Random_fuer_CA


const XSIZE = 256

State = Union{Int64} #bin mir nicht sicher ob das mit typedef char State; übereinstimmt
Line = Array{State}(undef, XSIZE + 2)
# const STATE = ??? ich denke "#define STATE MPI_CHAR" braucht nicht übersetzt zu werden

function randInt(n, randomStructKontainer)
   return floor(Int64, nextRandomLEcuyer(randomStructKontainer) * n)
end

# wie ich das sehe dient displayConfig nur der Darstellung und das macht Julia etwas anders daher umgebaut

function displayConfig(buf::Array, lines::Int64)
   display(Gray.(buf[2:lines+1, 2:XSIZE+1]))
end

#random starting configuration */
function initConfig(buf::Array, lines::Int64)
   x::Int64 = 0
   y::Int64 = 0

   randomStructKontainer = initRandomLEcuyer(Int32(424243))
   for y = 1:lines
      for x =1:XSIZE
         buf[y + 1, x + 1] = randInt(100, randomStructKontainer) >= 50
      end
   end
end

const anneal = [0, 0, 0, 0, 1, 0, 1, 1, 1, 1]

function transition(a, x, y)
   return anneal[a[y-1,x-1]+a[y  ,x-1]+a[y+1,x-1]+a[y-1,x  ]+a[y  ,x  ]+a[y+1,x  ]+a[y-1,x+1]+a[y  ,x+1]+a[y+1,x+1]+1]
end

function boundary(buf::Array, lines::Int64)
   x::Int64 = 0
   y::Int64 = 0

   for y = 2:lines+1
      buf[y, 1] = buf[y, XSIZE+1]
      buf[y, XSIZE+2] = buf[y, 2]
   end
   for x = 1:XSIZE+2
      buf[1, x] = buf[lines+1, x]
      buf[lines+2, x] = buf[2, x]
   end

end

function simulate(from::Array, to::Array, lines::Int64)
   x::Int64 = 0
   y::Int64 = 0
   boundary(from, lines)
   for y = 2:lines+1
      for x = 2:XSIZE+1
         to[y, x] = transition(from, x, y)
      end
   end
end

function main(linesParameter::Int64, itsParameter::Int64, displayPeriodParameter::Int64, displayNameParameter::String)
   # von weiter Oben kopiert
   displayPeriod::Int64 = 0 # ::Int wäre auch zulässig aber eventuell(abhängig von der Hardware) etwas anders

   lines::Int64 = 0
   its::Int64 = 0
   i::Int64 = 0
   from = []
   to = []
   temp = []

   lines = linesParameter
   its = itsParameter
   displayPeriod = displayPeriodParameter
   from = zeros(Int64, lines+2, XSIZE+2)
   to = zeros(Int64, lines+2, XSIZE+2)
   initConfig(from, lines)

   println(sha2_256(string(from[2:lines + 1,2:XSIZE + 1])))
   boundary(from, lines)
   if displayPeriod != 0
      displayConfig(from, lines)
   end

   startTime = now()
   for i = 1:its
      simulate(from, to, lines)
      temp = from
      from = to
      to = temp
      if displayPeriod != 0
         if i % displayPeriod == 0
            displayConfig(from, lines)
         end
      end
   end
   endTime = now()
   print((lines*XSIZE*its / ((endTime - startTime).value/1000)), " cells per second ")
   if displayPeriod != 0
      print("(but the states have been displayed)")
   end
   println()
   println(lines, " lines, ", its, " iterations, display period=", displayPeriod)
   println(sha2_256(string(from[2:lines + 1,2:XSIZE + 1])))
end

#anstatt der externen Übergabeparameter
#lines # Dimension des Feldes
#its # Anzahl der Iterationen
#displayPeriod # Anzeigezeit
#Name # Name in der Anzeige
main(5000, 150, 0, "Test")
#main(500, 500, 100, "Test")
