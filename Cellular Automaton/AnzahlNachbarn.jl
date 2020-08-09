module AnzahlNachbarn

    function abstandDirekt(positionX, positionY, indexLaenge, indexBreite)
        return abs(positionX - indexLaenge) + abs(positionY - indexBreite)
    end

    function abstandIndirekt(positionX, positionY, indexLaenge, indexBreite)
        return max(abs(positionX - indexLaenge), abs(positionY - indexBreite))
    end

    function abstandKreis(positionX, positionY, indexLaenge, indexBreite)
        return sqrt((positionX - indexLaenge)^2+(positionY - indexBreite)^2)
    end

    function relationsTest(selbst::Int64, nachbar::Int64, relationstyp::String)
        #im Fehlerfall wird falsch zurückgeliefert
        returnWert = false

        # bereinigen
        relationstyp = lowercase(relationstyp)

        if(relationstyp == "gleich")
            if(selbst == nachbar)
                returnWert = true
            end
        elseif(relationstyp == "ungleich")
            if(selbst != nachbar)
                returnWert = true
            end
        elseif(relationstyp == "größer")
            if(selbst > nachbar)
                returnWert = true
            end
        elseif(relationstyp == "kleiner")
            if(selbst < nachbar)
                returnWert = true
            end
        elseif(relationstyp == "null")
            if(nachbar == 0)
                returnWert = true
            end
        elseif(relationstyp == "eins")
            if(nachbar == 1)
                returnWert = true
            end
        # eventuell noch Abstand 1,2, ... aber was ist dann mit dem Überlauf, dann müsste ein max definiert werden und prüfen ob der Nachbar dann über den Überlauf gültig wird
        end
        return returnWert
    end

    export anzahlNachbarn
    function anzahlNachbarn(nachbarschaft::Array, positionX::Int64, positionY::Int64, abstandsArt::String, maxAbstand::Float64, relationstyp::String)
        # genutzte Variablen initialisieren
        returnWert = 0
        istAbstand = 0.

        # bereinigen
        abstandsArt = lowercase(abstandsArt)

        laenge, breite = size(nachbarschaft)
        for indexLaenge = 1:laenge
            for indexBreite = 1:breite
                istAbstand = 0 # notfalls erstmal die 0 erstellen für den Fehlerfall
                if(abstandsArt == "direkt")
                    istAbstand = abstandDirekt(positionX, positionY, indexLaenge, indexBreite)
                elseif(abstandsArt == "indirekt")
                    istAbstand = abstandIndirekt(positionX, positionY, indexLaenge, indexBreite)
                elseif(abstandsArt == "kreis")
                    istAbstand = abstandDirekt(positionX, positionY, indexLaenge, indexBreite)
                else
                    #sollte nicht vorkommen
                    #eventuell Fehler oder Enumeration
                end
                if(maxAbstand >= istAbstand)&(istAbstand != 0)
                    if(relationsTest(nachbarschaft[positionX, positionY], nachbarschaft[indexLaenge, indexBreite], relationstyp))
                        returnWert += 1
                    end
                end
            end
        end
        return returnWert
    end

end
