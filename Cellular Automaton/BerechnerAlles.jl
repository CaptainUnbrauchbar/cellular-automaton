module BerechnerAlles

    include("./BerechnerZelle.jl")
    using .BerechnerZelle
    using Plots

    function darstellen(aktuellerSchritt::Array)
        heatmap(aktuellerSchritt)
    end

    export start
    function start(anzahlIterationsschritte::Int64, startNachbarschaft::Array, regelKontainer)
        dimensionX = 0
        dimensionY = 0
        aktuellerSchritt = copy(startNachbarschaft)
        maxX, maxY = size(startNachbarschaft)
        anzahlRegeln = size(regelKontainer.regelwerk)[1]
        for iterationsschritt = 1:anzahlIterationsschritte
            #darstellen(aktuellerSchritt)
            neuerSchritt = copy(aktuellerSchritt)
            for indexRegel = 1:anzahlRegeln
                regel = regelKontainer.regelwerk[indexRegel]
                radius = trunc(Int64, regel.maxAbstand)
                if((regel.torusX == true) & (regel.torusY == true))
                    lauffeld = circshift(aktuellerSchritt, (radius, radius))
                    for indexX = 1:maxX
                        for indexY = 1:maxY
                            neueZelle = berechneZelle(lauffeld[1:radius*2+1,1:radius*2+1],1+radius,1+radius,regel)
                            if(isequal(neueZelle, NaN))
                            else
                                neuerSchritt[indexX, indexY] = neueZelle
                            end
                            lauffeld = circshift(lauffeld, (0, -1))
                        end
                        lauffeld = circshift(lauffeld, (-1, 0))
                    end
                end
            end
            aktuellerSchritt = copy(neuerSchritt)
        end
        #darstellen(aktuellerSchritt)
        return aktuellerSchritt
    end

end
