module Regel

mutable struct RegelZeile
    wennSelbst::Int64
    torusX::Bool
    torusY::Bool
    abstandsArt::String
    maxAbstand::Float64
    relationstyp::String
    verhaeltnisArt::String
    vergleichswert::Int64
    neuesSelbst::Int64
end

mutable struct RegelKontainer
    maxRegeln::Int64
    regelwerk::Array{RegelZeile}
    letzteRegel::Int64
end
    export initRegelsatz
    function initRegelsatz(anzahlRegeln::Int64)
        return RegelKontainer(anzahlRegeln, Array{RegelZeile}(undef, anzahlRegeln), 0)
    end

    export addRegel
    function addRegel(wennSelbst::Int64, torusX::Bool, torusY::Bool, abstandsArt::String, maxAbstand::Float64, relationstyp::String, verhaeltnisArt::String, vergleichswert::Int64, neuesSelbst::Int64, regelKontainer::RegelKontainer)
        returnWert = false
        if(regelKontainer.letzteRegel < regelKontainer.maxRegeln)
            regelKontainer.letzteRegel = regelKontainer.letzteRegel + 1
            regelKontainer.regelwerk[regelKontainer.letzteRegel] = RegelZeile(wennSelbst, torusX, torusY, abstandsArt, maxAbstand, relationstyp, verhaeltnisArt, vergleichswert, neuesSelbst)
            returnWert = true
        end
        return returnWert
    end

    export getRegelAnzahl
    function getRegelAnzahl(regelKontainer::RegelKontainer)
        return regelKontainer.letzteRegel
    end

    export getRegel
    function getRegel(index::Int64, regelKontainer::RegelKontainer)
        # index außerhalb wird nicht geprüft
        return regelKontainer.regelwerk[index]
    end
end
