module BerechnerZelle

include("./Regel.jl")
using .Regel
include("./AnzahlNachbarn.jl")
using .AnzahlNachbarn

    export berechneZelle
    function berechneZelle(nachbarschaft::Array, positionX::Int64, positionY::Int64, regel)
         returnWert = NaN
         selbst = nachbarschaft[positionX, positionY]
         if(regel.wennSelbst == selbst)
             nachbarZahl = anzahlNachbarn(nachbarschaft, positionX, positionY, regel.abstandsArt, regel.maxAbstand, regel.relationstyp)
             if(regel.verhaeltnisArt == "kleiner")
                 if(nachbarZahl < regel.vergleichswert)
                     returnWert = regel.neuesSelbst
                 end
             elseif (regel.verhaeltnisArt == "größer")
                 if(nachbarZahl > regel.vergleichswert)
                     returnWert = regel.neuesSelbst
                 end
             elseif (regel.verhaeltnisArt == "gleich")
                 if(nachbarZahl == regel.vergleichswert)
                     returnWert = regel.neuesSelbst
                 end
             end
         end
         return returnWert
    end

end
