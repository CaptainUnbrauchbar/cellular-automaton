using Dates # für die Zeit
using Distributed # für die Paraalelität
using Images # zum Zeichnen
using SHA

include("./Random_fuer_CA.jl")
using .Random_fuer_CA

addprocs(12)

@everywhere const XSIZE = 256

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

# random starting configuration */
function initConfig(buf::Array, lines::Int64)
   x::Int64 = 0
   y::Int64 = 0

   randomStructKontainer = initRandomLEcuyer(Int32(424243))
   for y = 1:lines
      for x = 1:XSIZE
         buf[y + 1, x + 1] = randInt(100, randomStructKontainer) >= 50
      end
   end
end

@everywhere const anneal = [0, 0, 0, 0, 1, 0, 1, 1, 1, 1]

@everywhere function transition(a, x, y)
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

@everywhere function simulate(fromChannel, toChannel)
   println("Start simulate")
   fromIndex::Int64 = -1 # Index des From-Arrays, spezielle Indexe: 0 entspricht Ende, -1 entspricht Anfang
   x::Int64 = 0
   y::Int64 = 0
   #erste Daten abrufen
   fromTeil, fromIndex, lines = take!(fromChannel)
   while fromIndex != 0
      toTeil = zeros(Int64, lines, XSIZE)
      for y = 2:lines+1
         for x = 2:XSIZE+1
            toTeil[y-1, x-1] = transition(fromTeil, x, y)
         end
      end
      put!(toChannel, (toTeil, fromIndex))
      #neue Daten abrufen
      fromTeil, fromIndex, lines = take!(fromChannel)
   end
   println("Ende simulate")
end

function main(linesParameter::Int64, itsParameter::Int64, displayPeriodParameter::Int64, treadAnzahlParameter::Int64, displayNameParameter::String)
   #Variablen definieren
   displayPeriod::Int64 = 0 # ::Int wäre auch zulässig aber eventuell(abhängig von der Hardware) etwas anders

   lines::Int64 = 0 #Anzahl der Linien
   its::Int64 = 0 #Anzahl der Durchläufe
   #Indexe für die For-Schleifen
   indexIts::Int64 = 0
   indexThread::Int64 = 0
   indexToTeil::Int64 = 0

   treadAnzahl::Int64 = 0 #Häufigkeit der Darstellungen
   zeilenProTread::Int64 = 0 #Zeilen je paralleler Teil eines Durchlaufs(außer dem letzten Teil)
   zeilenletzterTread::Int64 = 0 #Zeilen des letzten Teils eines Durchlaufs
   from = [] #Arbeitsmatrix
   to = [] #Ergebnis des Durchlaufs
   toTeil = [] #Ergebnis eines parallelen Teils eines Durchlaufs
   temp = [] #Zwischenspeicher für den Durchlaufwechsel

   #Variablen initialisieren
   lines = linesParameter #Zuweisung des Übergabeparameters
   its = itsParameter #Zuweisung des Übergabeparameters
   displayPeriod = displayPeriodParameter #Zuweisung des Übergabeparameters
   if(treadAnzahlParameter > lines) #Fallunterscheidung um bei kleiner Zeilenanzahl zu garantieren das jeder paralleler Teil eines Durchlaufs mindestend eine Zeile erhält
      treadAnzahl = lines
      zeilenProTread = 1
      zeilenletzterTread = 1
   else
      treadAnzahl = treadAnzahlParameter
      zeilenProTread = cld(lines, treadAnzahl)
      zeilenletzterTread = lines-(zeilenProTread*(treadAnzahl-1))
   end
   from = zeros(Int64, lines+2, XSIZE+2) #Speicherzuordnung und Leerinitialation
   to = zeros(Int64, lines+2, XSIZE+2) #Speicherzuordnung und Leerinitialation
   initConfig(from, lines) #pseudozufälliger Startzustand
   toChannel = RemoteChannel(()->Channel{Tuple}(treadAnzahl)) #Nachtrag Variablen definition/Rückgabedatentyp der parallelen Durchläufe
   fromChannel = RemoteChannel(()->Channel{Tuple}(treadAnzahl)) #Nachtrag Variablen definition/Rückgabedatentyp der parallelen Durchläufe

   #Threads anlegen
   for prozess in workers()
      remote_do(simulate, prozess, fromChannel, toChannel)
   end
   if displayPeriod != 0
      displayConfig(from, lines)
   end
   println(sha2_256(string(from[2:lines + 1,2:XSIZE + 1])))
   @elapsed startTime = now()
   for indexIts in 1:its
      boundary(from, lines)
      #Durchlauf starten
      #println("Durchlauf ", indexIts, " gestartet")
      for indexThread in 1:treadAnzahl
         if(indexThread < treadAnzahl)
            put!(fromChannel, (from[(((indexThread-1)*zeilenProTread)+1):((indexThread*zeilenProTread)+2),:], indexThread, zeilenProTread))
         else
            put!(fromChannel, (from[(((indexThread-1)*zeilenProTread)+1):lines+2,:], indexThread, zeilenletzterTread))
         end
      end
      #Durchlauf zusammenfassen
      #println("Durchlauf beim zusammenfassen")
      for indexThread in 1:treadAnzahl
         toTeil, indexToTeil = take!(toChannel)
         if(indexToTeil < treadAnzahl)
            to[(((indexToTeil-1)*zeilenProTread)+2):((indexToTeil*zeilenProTread)+1),2:XSIZE+1] = toTeil
         else
            to[(((indexToTeil-1)*zeilenProTread)+2):lines+1,2:XSIZE+1] = toTeil
         end
      end
      #Durchlauf beenden
      #println("Durchlauf beim beenden")
      temp = from
      from = to
      to = temp
      if displayPeriod != 0
         if iI % displayPeriod == 0
            displayConfig(from, lines)
         end
      end
   end
   @elapsed endTime = now()
   #Threads Endbefehl senden
   for prozess in workers()
      put!(fromChannel, (toTeil, indexThread, zeilenProTread))
   end

   #Auswertung
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
#ThreadAnzahl # Anzahl der genutzten Threads
#Name # Name in der Anzeige
#main(500, 5, 100, 4, "Test")
#main(500, 5, 100, 4, "Test")
#main(500, 6, 100, 4, "Test")
#main(500, 500, 100, 4, "Test")
main(5000, 150, 0, 8, "Test")
