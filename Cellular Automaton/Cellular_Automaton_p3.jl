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

@everywhere function simulate(fromTeil::Array, anzahlZeilen::Int64, obererFromChannel, untererFromChannel, toChannel, obererToChannel, untererToChannel, iterationenProBlock, gesamtIterationen)
   println("Start simulate")
   x::Int64 = 0
   y::Int64 = 0
   indexIteration::Int64 = 0
   toTeil = zeros(Int64, anzahlZeilen, XSIZE)
   if iterationenProBlock == 0
      iterationenProBlock = gesamtIterationen
   end
   while gesamtIterationen > 0
      if iterationenProBlock > gesamtIterationen
         iterationenProBlock = gesamtIterationen
      end
      for indexIteration = 1:iterationenProBlock

         #Berechnung
         for y = 2:anzahlZeilen+1
            for x = 2:XSIZE+1
               toTeil[y-1, x-1] = transition(fromTeil, x, y)
            end
         end

         #Datenübergabe
         put!(obererToChannel,toTeil[1,1:XSIZE])
         put!(untererToChannel,toTeil[anzahlZeilen,1:XSIZE])
         fromTeil[2:anzahlZeilen + 1,2:XSIZE + 1] = toTeil
         #Ersatz für boundary
         #Teil 1 Randbereich aus Daten der anderen Threads erstellen, warten auf die Daten inclusive
         fromTeil[1,2:XSIZE + 1] = take!(obererFromChannel)
         fromTeil[anzahlZeilen + 2,2:XSIZE + 1] = take!(untererFromChannel)
         #Teil 2 Daten aus eigenen Daten erstellen
         fromTeil[1:anzahlZeilen + 2,1] = fromTeil[1:anzahlZeilen + 2,XSIZE + 1]
         fromTeil[1:anzahlZeilen + 2,XSIZE + 2] = fromTeil[1:anzahlZeilen + 2,2]

      end
      put!(toChannel, toTeil)
      gesamtIterationen = gesamtIterationen - iterationenProBlock
   end
   println("Ende simulate")
end

function indexeErzeugen(anzahl::Int64, jobs)
   for index in 1:anzahl
       put!(jobs, index)
   end
end

function main(linesParameter::Int64, itsParameter::Int64, displayPeriodParameter::Int64, threadAnzahlParameter::Int64, displayNameParameter::String)
   #Variablen definieren
   displayPeriod::Int64 = 0 # ::Int wäre auch zulässig aber eventuell(abhängig von der Hardware) etwas anders

   lines::Int64 = 0 #Anzahl der Linien
   its::Int64 = 0 #Anzahl der Durchläufe
   #Indexe für die For-Schleifen
   indexIts::Int64 = 0
   indexThread::Int64 = 0
   indexBild::Int64 = 0

   threadAnzahl::Int64 = 0 #Häufigkeit der Darstellungen
   zeilenProthread::Int64 = 0 #Zeilen je paralleler Teil eines Durchlaufs(außer dem letzten Teil)
   zeilenletzterthread::Int64 = 0 #Zeilen des letzten Teils eines Durchlaufs
   anzahlBilder::Int64 = 0 #Anzahl zu zeichender Bilder
   from = [] #Arbeitsmatrix
   to = [] #Ergebnis des Durchlaufs
   temp = [] #Zwischenspeicher für den Durchlaufwechsel

   #Variablen initialisieren
   lines = linesParameter #Zuweisung des Übergabeparameters
   its = itsParameter #Zuweisung des Übergabeparameters
   displayPeriod = displayPeriodParameter #Zuweisung des Übergabeparameters
   if(threadAnzahlParameter > lines) #Fallunterscheidung um bei kleiner Zeilenanzahl zu garantieren das jeder paralleler Teil eines Durchlaufs mindestend eine Zeile erhält
      threadAnzahl = lines
      zeilenProthread = 1
      zeilenletzterthread = 1
   else
      threadAnzahl = threadAnzahlParameter
      zeilenProthread = cld(lines, threadAnzahl)
      zeilenletzterthread = lines-(zeilenProthread*(threadAnzahl-1))
   end
   if displayPeriod > 0
      anzahlBilder = cld(its, displayPeriod)
   else
      anzahlBilder = 1
   end
   from = zeros(Int64, lines+2, XSIZE+2) #Speicherzuordnung und Leerinitialation
   initConfig(from, lines) #pseudozufälliger Startzustand

   kommunikationsChannelArray = Array{RemoteChannel}(undef, threadAnzahl * 2)
   toChannelArray = Array{RemoteChannel}(undef, threadAnzahl)

   for indexThread = 1:threadAnzahl
      kommunikationsChannelArray[indexThread] = RemoteChannel(()->Channel{Array}(threadAnzahl))
      kommunikationsChannelArray[indexThread + threadAnzahl] = RemoteChannel(()->Channel{Array}(threadAnzahl))
      toChannelArray[indexThread] = RemoteChannel(()->Channel{Array}(threadAnzahl))
   end

   boundary(from, lines)
   if displayPeriod != 0
      displayConfig(from, lines)
   end
   println(sha2_256(string(from[2:lines + 1,2:XSIZE + 1])))
   @elapsed startTime = now()

   indexThread = 1
   #Threads anlegen und starten
   for prozess in workers()
      if indexThread <= threadAnzahl
         if indexThread == threadAnzahl
            if indexThread == 1
               remote_do(simulate, prozess, from, zeilenletzterthread, kommunikationsChannelArray[1], kommunikationsChannelArray[2], toChannelArray[1], kommunikationsChannelArray[2], kommunikationsChannelArray[1], displayPeriod, its)
            else
               remote_do(simulate, prozess, from[((indexThread - 1) * zeilenProthread) + 1:lines + 2,1:XSIZE + 2], zeilenletzterthread, kommunikationsChannelArray[indexThread], kommunikationsChannelArray[threadAnzahl * 2], toChannelArray[indexThread], kommunikationsChannelArray[(threadAnzahl * 2) - 1], kommunikationsChannelArray[1], displayPeriod, its)
            end
         else
            if indexThread == 1
               remote_do(simulate, prozess, from[((indexThread - 1) * zeilenProthread) + 1:(indexThread * zeilenProthread) + 2,1:XSIZE + 2], zeilenProthread, kommunikationsChannelArray[indexThread], kommunikationsChannelArray[indexThread + threadAnzahl], toChannelArray[indexThread], kommunikationsChannelArray[threadAnzahl * 2], kommunikationsChannelArray[2], displayPeriod, its)
            else
               remote_do(simulate, prozess, from[((indexThread - 1) * zeilenProthread) + 1:(indexThread * zeilenProthread) + 2,1:XSIZE + 2], zeilenProthread, kommunikationsChannelArray[indexThread], kommunikationsChannelArray[indexThread + threadAnzahl], toChannelArray[indexThread], kommunikationsChannelArray[indexThread + threadAnzahl - 1], kommunikationsChannelArray[indexThread + 1], displayPeriod, its)
            end
         end
      end
      indexThread = indexThread + 1
   end
   for indexBild = 1:anzahlBilder
      if threadAnzahl > 1
         for indexThread = 1:threadAnzahl - 1
            from[(((indexThread - 1) * zeilenProthread) + 2):((indexThread * zeilenProthread) + 1),2:XSIZE + 1] = take!(toChannelArray[indexThread])
         end
      end
      from[(((threadAnzahl - 1) * zeilenProthread) + 2):lines + 1,2:XSIZE + 1] = take!(toChannelArray[threadAnzahl])
      if displayPeriod != 0
         displayConfig(from, lines)
      end
   end

   @elapsed endTime = now()

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
