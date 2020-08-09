include("./Regel.jl")
using .Regel
include("./AnzahlNachbarn.jl")
using .AnzahlNachbarn
include("./BerechnerZelle.jl")
using .BerechnerZelle
include("./BerechnerAlles.jl")
using .BerechnerAlles

#Startparameter


dimensionX = 30
dimensionY = 30
torusX = true
torusY = true

#Umgebung
startNachbarschaft = zeros(Int64, dimensionX, dimensionY)
startNachbarschaft[3,5] = 1
startNachbarschaft[4,5] = 1
startNachbarschaft[5,5] = 1
startNachbarschaft[6,5] = 1
startNachbarschaft[7,5] = 1
startNachbarschaft[10,10] = 1

#Regeln

regelKontainer = initRegelsatz(3)
#Achtung Betrachtungsbereich sollte nicht größer sein als die Nachbarschaft
addRegel(1, torusX, torusY, "indirekt", 1., "eins", "kleiner", 2, 0, regelKontainer)
addRegel(1, torusX, torusY, "indirekt", 1., "eins", "größer", 3, 0, regelKontainer)
addRegel(0, torusX, torusY, "indirekt", 1., "eins", "gleich", 3, 1, regelKontainer)

#Testausgaben
x,y = size(startNachbarschaft)
#println(x)
#println(y)

#print("Nachbaranzahl = ")
#println(anzahlNachbarn(startNachbarschaft, 5, 5, "indirekt", 1., "gleich"))
#println(startNachbarschaft[4:6,4:6])
#println(berechneZelle(startNachbarschaft[3:5,4:6], 2, 2, regelKontainer.regelwerk[1]))
#println(startNachbarschaft)
#startNachbarschaft = circshift(startNachbarschaft, (1, 0))
println(startNachbarschaft)

letzterSchritt = start(1, startNachbarschaft, regelKontainer)
println(letzterSchritt)
letzterSchritt = start(1, letzterSchritt, regelKontainer)
println(letzterSchritt)
