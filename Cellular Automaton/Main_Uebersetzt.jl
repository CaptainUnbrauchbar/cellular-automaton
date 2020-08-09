using Random

const XSIZE = 256

State = Union{Int64} #bin mir nicht sicher ob das mit typedef char State; übereinstimmt
Line = Array{State}(undef, XSIZE + 2)
# const STATE = ??? ich denke "#define STATE MPI_CHAR" braucht nicht übersetzt zu werden, da a. die mpi Bibilotek wohl zum Anzeigen genutzt wird und Julia das selbst machen wird und b. der Wert nie genutzt wird

displayPeriod::Int64 # ::Int wäre auch zulässig aber eventuell etwas anders

function randInt(n)
   return floor(Int64, nextRandomLEcuyer)
end

# anstatt der Übergabeparameter
lines = 50 # Dimension des Feldes
its  = 50 # Anzahl der Iterationen
displayPeriod = 20 # Anzeigezeit

#random starting configuration */
function initConfig(Line::struct, lines::Int64)
   rng = MersenneTwister(424243)
   for y = 1:lines
      for x = 1:lines
         buf[y, x] = rand(rng)*100 >= 50
      end
   end
end

anneal = [0, 0, 0, 0, 1, 0, 1, 1, 1, 1]



# aus Übergabeparametern
lines = 50 # Dimension des Feldes
its  = 50 # Anzahl der Iterationen
displayPeriod = 20 # Anzeigezeit

initConfig(from, lines);
if (displayPeriod) bitmapInit(XSIZE, lines, argv[4]);

/* measurement loop */
startTime = MPI_Wtime();
for (i = 0;  i < its;  i++) {
   simulate(from, to, lines);
   temp = from;  from = to;  to = temp;
   if (displayPeriod)
      if (i % displayPeriod == 0)
         displayConfig(from, lines);
}

printf("%f cells per second %s\n",
      lines*XSIZE*its / (MPI_Wtime() - startTime),
      displayPeriod?"(but the states have been displayed)":"");
printf("%d lines, %d iterations, display period=%d.\n",
         lines, its, displayPeriod);
if (displayPeriod) {
   puts("Press Enter to exit from program.");
   scanf("%c", &i);
}
