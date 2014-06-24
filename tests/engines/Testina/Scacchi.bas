Attribute VB_Name = "Scacchi"
'Testina chess engine - Sandro Corsini 2002
Option Explicit
Option Base 1

'COSTANTI
Public Const Ver = "Testina 2_2"
Const Infinito = 32767
Const WINDOW = 100
Const MaxPly = 50
Const CPedone = 100
Const CCavallo = 300
Const CAlfiere = 325
Const CTorre = 500
Const CDonna = 900
Const CRe = 10000
Const SECONDI = 24! * 60 * 60
Const BIANCHI = 1
Const NERI = 2
Const R = 2 'fattore di riduzione per null move heuristic
Const SICUREZZARE = -20 'penalita' vicinanza nemico al Re
'TIPI
Type MoveType
    CPar As Integer
    CArr As Integer
    Mosso As Integer
    Punteggio As Integer
    Catturato As Integer
    Algebrica As String
    Speciale As Integer '1=arrocco 2=promozione 3=en passant
    Promosso As Integer 'indica il pezzo a cui si promuove
    Enpassabile As Boolean 'Vero su mosse tipo e2e4,c2c4,d2d4..
    Illegale As Boolean
End Type

Type ArroccoType
    Re As Integer
    Torre7 As Integer
    Torre4 As Integer
End Type

Type BookType
    pos As String * 80 'la posizione
    Mossa As String * 50 'valutazione mosse
    'Mossa contiene in forma stringa il valore della mossa
    'generata da GeneraMosse es:
    'la 10a mossa di GeneraMosse ha il valore:
    'Cint(Mid$(Mossa,10,1))
End Type

'MOVIMENTO DEI PEZZI
'Diags contiene tutte le case attaccate da un pezzo che si
'muove sulle diagonali (es. alfiere o donna)
Dim Diags(1 To 64, 1 To 16) As Integer
'Cols contiene le case attaccate da un pezzo
'che si muove sulle colonne (torre o donna)
Dim Cols(1 To 64, 1 To 17) As Integer
'Cavs contiene le case attaccate dal cavallo
Dim Cavs(1 To 64, 1 To 8) As Integer
'Re contiene i movimenti del re
'bisogna gestire a parte l'arrocco
Dim Re(1 To 64, 1 To 8) As Integer
'per i pedoni si mantengono due array distinti
'PedB per i bianchi e PedN per i neri
'bisogna gestire a parte en-passant e promozione
Dim PedB(1 To 64, 1 To 5) As Integer
Dim PedN(1 To 64, 1 To 5) As Integer
'STRUTTURE GENERALI--------------------------------------------
'Conteggio dei nodi visitati
'il conteggio dei nodi e' piu' indicativo di quello delle
'posizioni perche' permette di capire se la ricerca e' stata
'migliorata e dove e' necessario arricchire la valutazione
'in caso di nodi eccessivi in una certa posizione
Dim Nodi As Double
'Rappresentazione della scacchiera
Public Board(1 To 64) As Integer
'rappresentazione della scacchiera in formato algebrico
Dim Coords(1 To 64) As String
'turno di gioco 1 bianco -1 nero
Public Turno As Integer
'contamosse conta le mosse per determinare lo stadio di gioco
Dim ContaMosse As Integer
'tipo di posizione 1)apertura 2)mediogioco 3)finale
Dim Stadio As Integer
'temporizzazione
Dim StartTime As Single 'si setta sul tempo iniziale
Dim Durata As Single 'tempo impiegato per la ricerca
Dim BestBianchi As Integer
Dim BestNeri As Integer
'strutture derivate dai tipi----------------------
Dim Mossa(1 To MaxPly, 1 To 80) As MoveType
Dim UltimaMossa As MoveType
Public ARROCCO(2) As ArroccoType
'STRUTTURE PER ORDINAMENTO E REFUTAZIONE
Dim History(64, 64) As Integer 'history heuristic
Dim Killer(2, MaxPly) As MoveType 'killer heuristic
Dim KillerScore(MaxPly) As Integer 'punteggio mosse killer
Public Storico(1 To 400) As MoveType
Dim UnaSolaMossa As Boolean 'true se ho una sola mossa legale
Dim NullMove As MoveType
Dim NullOk As Boolean
Dim NullSearching As Boolean
'variante principale
Dim PVLength(MaxPly) As Integer 'lunghezza variante principale
Dim PV(MaxPly, MaxPly) As MoveType 'array best mosse
Dim PVidx As Integer
Dim Variante(MaxPly) As MoveType 'qui si registra la PV
'----------------------------fine strutture da tipi
Dim Ply As Integer 'indica il Ply della mossa
Dim SearchPly As Integer 'il ply più elevato nella ricerca
Dim IndiceMossa As Integer 'punta la mossa utente
Dim Illegale As Integer
Dim Limite As Integer 'limite per quiescenza
'STRUTTURE DI VALUTAZIONE DELLA POSIZIONE---------------------
Dim Mobil(MaxPly) As Integer  'mobilità
'Tabelle per la valutazione della posizione
Dim Pedoni(64) As Integer 'PEDONI - tabella punti mosse di pedone
Dim Cavalli(64) As Integer 'CAVALLI-tabella punti mosse cavallo
Dim Alfieri(64) As Integer 'ALFIERI-tabella punti alfieri
Dim ReApe(64) As Integer 'tabella punti per i Re in apertura e mediogioco
Dim ReFinale(64) As Integer 'tabella punti Re nel finale
Dim Flip(64) As Integer 'scacchiera rovesciata
Dim Colonne(64) As Integer 'tabella per individuazione colonne
Dim PosReB As Integer, PosReN As Integer 'posizione dei Re
'Dim SELEZIONE As Integer 'regola la selezione delle mosse
'STRUTTURE PER RIPETIZIONE POSIZIONE
Dim RipBianco(64, 64) As Integer
Dim RipNero(64, 64) As Integer
Dim RipB As Integer, RipN As Integer
Dim CinquantaMosse As Byte 'controlla patta per la regola delle 50 mosse
'STRUTTURE PER GESTIONE BOOK MOVES
Public BookMode As Boolean
Public BookPos As String
Dim Book As BookType
Dim BookDisponibile As Boolean
Public ValoreRitorno As Integer
'STRUTTURE PER WINBOARD
Dim WInput As String 'Winboard Input
Public Termina As Boolean
Public Tempoxmossa As Single
Dim TestinaBianco As Boolean
Dim ICS As Boolean
Dim Forced As Boolean
Dim PostMode As Boolean


Function AlfaBeta(ByVal depth As Integer, ByVal Alfa As Integer, ByVal Beta As Integer) As Integer

Dim Indice As Integer, Punti As Integer, p As Integer
Dim j As Integer, Giocate As Byte

If depth <= 0 Then
    If SearchPly > 2 And Alfa > CTorre Then
        'posizione molto buona:non necessita quiescenza
        AlfaBeta = Eval
        Exit Function
    End If
    Ply = Ply + 40 'indicizzatore di mosse
    Limite = 35 'limite oltre il quale la quiescenza finisce
    AlfaBeta = Quies(Alfa, Beta)
    Ply = Ply - 40 'ripristino l'indicizzatore
    DoEvents
    Exit Function
End If
    
    'null move heuristic
    If Stadio <> 3 And depth = (SearchPly - 1) Then
       If NullOk = True Then
          If InScacco(50) = False Then
             NullOk = False
             NullSearching = True
             Punti = -AlfaBeta(depth - R - 1, -Beta, -Beta + 1)
             NullSearching = False
             If Punti >= Beta Then
                AlfaBeta = Beta
                Exit Function
             End If
          End If
       End If
       NullOk = True
    End If


If (depth = 1) And (depth = SearchPly) Then GeneraMosse (depth)
If (depth <> SearchPly) Then GeneraMosse (depth)

Indice = 1
PVLength(PVidx) = PVidx
Giocate = 0

Do
    If Mossa(depth, Indice).CPar = -100 Then Exit Do
    FaiMossa depth, Indice
    PVidx = PVidx + 1
    Giocate = Giocate + 1
    If InScacco(50) Then
        'se sono sotto scacco la mossa e' illegale
        Mossa(depth, Indice).Illegale = True
        DisfaiMossa depth, Indice
        PVidx = PVidx - 1
        Giocate = Giocate - 1
        GoTo NuovaMossa
    End If
    If CheckReps(depth, Indice) = True Then
        'controlla se c'e' una ripetizione
        Punti = 0
        GoTo ByPassPunteggio
    End If
    'razoring
    'If (depth < SearchPly) And (NullSearching = False) Then
    '   If InScacco(50) = False Then
    '      Punti = Eval
    '      If Beta >= Punti Then
    '         RipristinaReps depth, Indice
    '         DisfaiMossa depth, Indice
    '         Giocate = Giocate - 1
    '         PVidx = PVidx - 1
    '         AlfaBeta = Beta
    '         Exit Function
    '      End If
    '   End If
    'End If
    If Mossa(depth, Indice).Illegale = True Then
        RipristinaReps depth, Indice
        DisfaiMossa depth, Indice
        Giocate = Giocate - 1
        PVidx = PVidx - 1
        Mossa(depth, Indice).Punteggio = -Infinito
        GoTo NuovaMossa
    End If
    Mossa(depth, Indice).Punteggio = -32000
    Turno = Turno * -1
    Nodi = Nodi + 1
    Punti = -AlfaBeta(depth - 1, -Beta, -Alfa)
    Turno = Turno * -1
ByPassPunteggio:
    PVidx = PVidx - 1
    Mossa(depth, Indice).Punteggio = Punti
    RipristinaReps depth, Indice 'rimetto a posto la tabella ripetizione
    DisfaiMossa depth, Indice
    If Punti >= Beta Then
       AlfaBeta = Beta
       Exit Function
    End If
    If Punti > Alfa Then
        Alfa = Punti
        'impostiamo la PV
        PV(PVidx, PVidx) = Mossa(depth, Indice)
        For j = PVidx + 1 To PVLength(PVidx + 1) - 1
          PV(PVidx, j) = PV(PVidx + 1, j)
        Next j
        PVLength(PVidx) = PVLength(PVidx + 1)
        'aggiorno la History Table
        p = Mossa(depth, Indice).CPar
        j = Mossa(depth, Indice).CArr
        History(p, j) = History(p, j) + depth
        '....e visto che abbiamo un cut-off impostiamo la mossa killer
        If Punti > KillerScore(depth) Then
            KillerScore(depth) = Punti
            Killer(2, depth) = Killer(1, depth)
            Killer(1, depth) = Mossa(depth, Indice)
        End If
    End If
NuovaMossa:
    Indice = Indice + 1
Loop Until Mossa(depth, Indice).CPar = -100

Uscita_Sub:
If (Giocate = 1) And (depth = SearchPly) Then UnaSolaMossa = True
If Giocate = 0 Then
   If InScacco(50) Then
      Alfa = -Infinito
   Else
      Alfa = 0
   End If
End If
AlfaBeta = Alfa
End Function


Function BookMove() As Integer
'cerco una mossa nel book:se la mossa viene trovata la funzione
'ritorna l'indice delle mosse generate contenente la mossa da giocare
'in caso contrario ritorna 0 e BookDisponibile diventa false
Dim i As Long, pos As String, j As Byte, Valore As Byte
Dim Normale(20) As Byte, Buona(20) As Byte
Dim NormIdx As Byte, BuonIdx As Byte
Dim Trovata As Boolean

GeneraMosse (50)
Randomize Timer
pos = ConvBoard
NormIdx = 1: BuonIdx = 1
Trovata = False
Open "book.dat" For Random As #3 Len = Len(Book)
    i = 1
    Do
        Get #3, i, Book
        If Book.pos = pos Then
           'posizione trovata:cerco una buona mossa nella lista
           For j = 1 To 50
             Valore = CInt(Mid$(Book.Mossa, j, 1))
             Select Case Valore
                Case 2
                    Normale(NormIdx) = j
                    NormIdx = NormIdx + 1
                    Trovata = True
                Case 3
                    Buona(BuonIdx) = j
                    BuonIdx = BuonIdx + 1
                    Trovata = True
             End Select
             If Mossa(50, j).CPar = -100 Then Exit For
           Next j
           Exit Do
        End If
        i = i + 1
    Loop Until EOF(3)
Close #3
If Trovata = True Then
   'conto le mosse trovate
   NormIdx = 0: BuonIdx = 0
   For j = 1 To 5
     If Normale(j) <> 0 Then NormIdx = NormIdx + 1
     If Buona(j) <> 0 Then BuonIdx = BuonIdx + 1
   Next j
   If BuonIdx = 1 Then
     'ho una buona mossa:gioco quella
     BookMove = Buona(BuonIdx)
     Exit Function
   End If
   If BuonIdx > 1 Then
     'ho alcune buone mosse:ne scelgo una a caso
     j = Int(Rnd * BuonIdx) + 1
     BookMove = Buona(j)
     Exit Function
   End If
   If NormIdx = 1 Then
     'ho una mossa normale:gioco quella
     BookMove = Normale(NormIdx)
     Exit Function
   End If
   If NormIdx > 1 Then
     'ho alcune mosse normali:ne scelgo una a caso
     j = Int(Rnd * NormIdx) + 1
     BookMove = Normale(j)
     Exit Function
   End If
Else
    'non ci sono mosse nella posizione o manca la posizione
    'BookDisponibile = False
    BookMove = 0
End If

End Function

Function Checkposizione() As Integer
'questa funzione riporta un valore a secondo del tipo di
'posizione: 1) apertura 2) mediogioco 3)finale
'la funzione si applica alla parte che deve muovere

Dim p As Integer, Conto As Integer


'verifico apertura
'la posizione è di apertura se chi muove ha più di quattro
'pezzi nelle case di partenza
Conto = 0
If Turno = 1 Then
    For p = 57 To 64
        If Sgn(Board(p)) = Sgn(Turno) Then
            Conto = Conto + 1
        End If
    Next p
    If Conto > 4 Then Checkposizione = 1: Exit Function
End If
If Turno = -1 Then
    For p = 1 To 8
        If Sgn(Board(p)) = Sgn(Turno) Then
            Conto = Conto + 1
        End If
    Next p
    If Conto > 4 Then Checkposizione = 1: Exit Function
End If
'se il test apertura fallisce bisogna determinare se siamo
'nel mediogioco o nel finale:se il conto dei pezzi
'(escluso i re) è inferiore a 15 siamo nel finale
Conto = 0
For p = 1 To 64
    If Board(p) <> 0 And Board(p) <> 6 * Turno Then
        If Board(p) < 0 Then
           Conto = Conto + Board(p) * -1
        End If
        If Board(p) > 0 Then
           Conto = Conto + Board(p)
        End If
    End If
Next p
If Conto < 15 Then Checkposizione = 3: Exit Function

Checkposizione = 2
End Function

Function CheckPromozione(pos As Integer) As Boolean
'la funzione ritorna true se il pedone viene promosso
CheckPromozione = False
If Turno = 1 Then
    If pos > 0 And pos < 9 Then CheckPromozione = True
Else
    If pos > 56 And pos < 65 Then CheckPromozione = True
End If
End Function




Function CheckReps(ByVal Ply As Integer, ByVal Indice As Integer) As Boolean
'controlla la ripetizione di posizione durante la ricerca
'se c'e' ripetizione ritorna TRUE altrimenti FALSE
Dim j As Byte, i As Byte, Pezzo As Integer

CheckReps = False
    Pezzo = Mossa(Ply, Indice).Mosso
    j = Mossa(Ply, Indice).CPar
    i = Mossa(Ply, Indice).CArr
If Pezzo = 1 Or Pezzo = -1 Then Exit Function
If Mossa(Ply, Indice).Catturato <> 0 Then Exit Function
    If Turno = 1 Then
        RipBianco(j, i) = RipBianco(j, i) + 1
        RipB = RipBianco(j, i)
    Else
        RipNero(j, i) = RipNero(j, i) + 1
        RipN = RipNero(j, i)
    End If
    If RipB > 2 And RipN > 2 Then
        CheckReps = True
        Exit Function
    End If
    
CheckReps = False
End Function

Function ConvBoard() As String
'converte l'array scacchiera in una stringa
Dim pos As String

Dim i As Byte
For i = 1 To 64
    pos = pos + CStr(Board(i))
Next i
'uniformo la lunghezza di Pos con Book.Pos
Do
    If Len(pos) < 80 Then pos = pos + " "
Loop Until Len(pos) = 80

ConvBoard = pos
End Function

Sub DebugGenMosse(Pezzo As Integer)
'routine per verificare il funzionamento del generatore
'di mosse:altamente di debug
Dim i As Byte, j As Byte, p As Byte
Init
Open CStr(Pezzo) + ".debug" For Output As #2
For i = 1 To 64
    'svuoto la board()
    For p = 1 To 64
       Board(p) = 0
    Next p
    Board(i) = Pezzo
    GeneraMosse (50)
    j = 1
    Do
        Board(Mossa(1, j).CArr) = 100
        j = j + 1
    Loop Until Mossa(1, j).CPar = -100
    PrintScacchiera
    ReportMoves (1)
    'cancello il pezzo
    Board(i) = 0
Next i
Close #2
End Sub

Sub Debugger()
    'debug
    Dim p As Integer
    Dim i As Integer
    
    p = 1
    For i = 1 To 64
        Debug.Print Scacchi.Board(i);
        If p = 8 Then
            Debug.Print
            p = 0
        End If
        p = p + 1
    Next i
    Debug.Print "----------------------"
End Sub

Sub DisfaiMossa(Ply As Integer, idx As Integer)

If Mossa(Ply, idx).Speciale = 0 Then
   Board(Mossa(Ply, idx).CPar) = Mossa(Ply, idx).Mosso
   Board(Mossa(Ply, idx).CArr) = Mossa(Ply, idx).Catturato
   If Mossa(Ply, idx).Mosso = 7 Then ARROCCO(1).Torre7 = ARROCCO(1).Torre7 - 1
   If Mossa(Ply, idx).Mosso = 4 Then ARROCCO(1).Torre4 = ARROCCO(1).Torre4 - 1
   If Mossa(Ply, idx).Mosso = -7 Then ARROCCO(2).Torre7 = ARROCCO(2).Torre7 - 1
   If Mossa(Ply, idx).Mosso = -4 Then ARROCCO(2).Torre4 = ARROCCO(2).Torre4 - 1
   If Mossa(Ply, idx).Mosso = 6 Then ARROCCO(1).Re = ARROCCO(1).Re - 1
   If Mossa(Ply, idx).Mosso = -6 Then ARROCCO(2).Re = ARROCCO(2).Re - 1
End If
If Mossa(Ply, idx).Speciale = 1 Then
   'arrocco
   If Turno = 1 Then
      If Mossa(Ply, idx).Algebrica = "e1g1" And Mossa(Ply, idx).Mosso = 6 Then
        ARROCCO(1).Re = 0
        ARROCCO(1).Torre4 = 0
        Board(61) = 6: Board(63) = 0: Board(62) = 0: Board(64) = 4
      End If
      If Mossa(Ply, idx).Algebrica = "e1c1" And Mossa(Ply, idx).Mosso = 6 Then
        ARROCCO(1).Re = 0
        ARROCCO(1).Torre7 = 0
        Board(57) = 7: Board(60) = 0: Board(61) = 6: Board(59) = 0
      End If
   End If
   If Turno = -1 Then
      If Mossa(Ply, idx).Algebrica = "e8g8" And Mossa(Ply, idx).Mosso = -6 Then
        ARROCCO(2).Re = 0
        ARROCCO(2).Torre4 = 0
        Board(5) = -6: Board(7) = 0: Board(6) = 0: Board(8) = -4
      End If
      If Mossa(Ply, idx).Algebrica = "e8c8" And Mossa(Ply, idx).Mosso = -6 Then
        ARROCCO(2).Re = 0
        ARROCCO(2).Torre7 = 0
        Board(1) = -7: Board(4) = 0: Board(5) = -6: Board(3) = 0
      End If
   End If
End If
If Mossa(Ply, idx).Speciale = 2 Then
   'promozione pedone
    Board(Mossa(Ply, idx).CPar) = Mossa(Ply, idx).Mosso
    Board(Mossa(Ply, idx).CArr) = Mossa(Ply, idx).Catturato
End If
If Mossa(Ply, idx).Speciale = 3 Then
  'cattura en-passant
  Board(Mossa(Ply, idx).CPar) = Mossa(Ply, idx).Mosso
  Board(Mossa(Ply, idx).CArr) = 0
  Board(Mossa(Ply, idx).Catturato) = Mossa(Ply, idx).Mosso * -1
End If
If Mossa(Ply, idx).Mosso = 6 Then PosReB = Mossa(Ply, idx).CPar
If Mossa(Ply, idx).Mosso = -6 Then PosReN = Mossa(Ply, idx).CPar
UltimaMossa = NullMove

End Sub

Function DoScacco(ByVal Ply As Integer) As Boolean
'controlla se la parte che ha mosso da scacco
'genero le mosse dell'attuale giocatore e
'controllo se c'e' una mossa che cattura il re
Dim idx As Integer

GeneraMosse (Ply)
idx = 1
Do
    If Mossa(Ply, idx).Catturato = 6 * (-Turno) Then
        DoScacco = True
        Exit Function
    End If
    idx = idx + 1
Loop Until Mossa(Ply, idx).CPar = -100
DoScacco = False

End Function

Sub EditBoard(FEN As String)
'imposta Board() con la stringa FEN inviata da Winboard
Dim pos As Byte, FENPos As Byte, Char As String, Vuoti As Byte
Dim i As Byte 'contatore

pos = 1
    For FENPos = 1 To Len(FEN)
        Char = Mid$(FEN, FENPos, 1)
        'analizzo il contenuto di fen
        Select Case Char
            Case "1" To "8"
                Vuoti = CInt(Char)
                For i = pos To (pos + Vuoti) - 1
                    Board(i) = 0
                Next i
                pos = i
            Case "r"
                If pos = 1 Then
                    Board(pos) = -7: pos = pos + 1
                Else
                   Board(pos) = -4: pos = pos + 1
                End If
            Case "n"
                Board(pos) = -2: pos = pos + 1
            Case "b"
                Board(pos) = -3: pos = pos + 1
            Case "q"
                Board(pos) = -5: pos = pos + 1
            Case "k"
                PosReN = pos
                Board(pos) = -6: pos = pos + 1
            Case "p"
                Board(pos) = -1: pos = pos + 1
            Case "R"
                If pos = 57 Then
                   Board(pos) = 7: pos = pos + 1
                Else
                   Board(pos) = 4: pos = pos + 1
                End If
            Case "N"
                Board(pos) = 2: pos = pos + 1
            Case "B"
                Board(pos) = 3: pos = pos + 1
            Case "Q"
                Board(pos) = 5: pos = pos + 1
            Case "K"
                PosReB = pos
                Board(pos) = 6: pos = pos + 1
            Case "P"
                Board(pos) = 1: pos = pos + 1
            Case " "
                pos = FENPos + 1
                Exit For
        End Select
    Next FENPos
'imposto colore che deve muovere
Char = Mid$(FEN, pos, 1)
Select Case Char
    Case "w"
        Turno = 1
    Case "b"
        Turno = -1
End Select

End Sub

Function Eval() As Integer
'costanti
Const PMOB = 10
Const PEDONE_PASSATO = 20
Const PEDONE_DOPPIATO = -10
Const PEDONE_ISOLATO = -8
'variabili
Dim i As Integer, Score(BIANCHI To NERI) As Integer
Dim ColPedB(8) As Integer 'incolonnamento pedoni bianchi
Dim ColPedN(8) As Integer 'incolonnamento pedoni neri

'mobilita'
'Score(BIANCHI) = Score(BIANCHI) + (PMOB * MobB(Ply))
'Score(NERI) = Score(NERI) + (PMOB * MobN(Ply))
'valori posizionali e materiali
For i = 1 To 64
   If Board(i) = 0 Then GoTo Skippa
   Select Case Board(i)
        Case Is = 1
            Score(BIANCHI) = Score(BIANCHI) + Pedoni(i)
            Score(BIANCHI) = Score(BIANCHI) + CPedone
            ColPedB(Colonne(i)) = ColPedB(Colonne(i)) + 1
            Score(NERI) = Score(NERI) + EvalReNero(i)
        Case 2
            Score(BIANCHI) = Score(BIANCHI) + Cavalli(i)
            Score(BIANCHI) = Score(BIANCHI) + CCavallo
            Score(NERI) = Score(NERI) + EvalReNero(i)
        Case 3
            Score(BIANCHI) = Score(BIANCHI) + Alfieri(i)
            Score(BIANCHI) = Score(BIANCHI) + CAlfiere
            Score(NERI) = Score(NERI) + EvalReNero(i)
        Case 4
            Score(BIANCHI) = Score(BIANCHI) + CTorre
            Score(NERI) = Score(NERI) + EvalReNero(i)
        Case 5
            Score(BIANCHI) = Score(BIANCHI) + CDonna
            Score(NERI) = Score(NERI) + EvalReNero(i)
        Case 6
            If Stadio = 3 Then
                Score(BIANCHI) = Score(BIANCHI) + ReFinale(i)
            Else
                Score(BIANCHI) = Score(BIANCHI) + ReApe(i)
            End If
            Score(BIANCHI) = Score(BIANCHI) + CRe
            Score(NERI) = Score(NERI) + EvalReNero(i)
        Case 7
            Score(BIANCHI) = Score(BIANCHI) + CTorre
            Score(NERI) = Score(NERI) + EvalReNero(i)
        Case Is = -1
            Score(NERI) = Score(NERI) + Pedoni(Flip(i))
            Score(NERI) = Score(NERI) + CPedone
            ColPedN(Colonne(i)) = ColPedN(Colonne(i)) + 1
            Score(BIANCHI) = Score(BIANCHI) + EvalReBianco(i)
        Case -2
            Score(NERI) = Score(NERI) + Cavalli(Flip(i))
            Score(NERI) = Score(NERI) + CCavallo
            Score(BIANCHI) = Score(BIANCHI) + EvalReBianco(i)
        Case -3
            Score(NERI) = Score(NERI) + Alfieri(Flip(i))
            Score(NERI) = Score(NERI) + CAlfiere
            Score(BIANCHI) = Score(BIANCHI) + EvalReBianco(i)
        Case -4
            Score(NERI) = Score(NERI) + CTorre
            Score(BIANCHI) = Score(BIANCHI) + EvalReBianco(i)
        Case -5
            Score(NERI) = Score(NERI) + CDonna
            Score(BIANCHI) = Score(BIANCHI) + EvalReBianco(i)
        Case -6
            If Stadio = 3 Then
                Score(NERI) = Score(NERI) + ReFinale(Flip(i))
            Else
                Score(NERI) = Score(NERI) + ReApe(Flip(i))
            End If
            Score(NERI) = Score(NERI) + CRe
            Score(BIANCHI) = Score(BIANCHI) + EvalReBianco(i)
        Case -7
            Score(NERI) = Score(NERI) + CTorre
            Score(BIANCHI) = Score(BIANCHI) + EvalReBianco(i)
   End Select
Skippa:
Next i
'valutazione dei pedoni in base al contenuto di ColPedB o ColPedN
For i = 1 To 8
    If ColPedB(i) > 0 And ColPedN(i) = 0 Then
        'il bianco ha un pedone passato
        Score(BIANCHI) = Score(BIANCHI) + PEDONE_PASSATO
    End If
    If ColPedB(i) > 1 Then
        'il bianco ha uno o piu' pedoni doppiati
        Score(BIANCHI) = Score(BIANCHI) + _
                        (ColPedB(i) * PEDONE_DOPPIATO)
    End If
    If i = 1 Then
        If ColPedB(i) <> 0 And ColPedB(i + 1) = 0 Then
            'il pedone nella colonna A e' isolato
            Score(BIANCHI) = Score(BIANCHI) + PEDONE_ISOLATO
        End If
    End If
    If i = 8 Then
        If ColPedB(i) <> 0 And ColPedB(i - 1) = 0 Then
            'il pedone nella colonna H e' isolato
            Score(BIANCHI) = Score(BIANCHI) + PEDONE_ISOLATO
        End If
    End If
    If i > 1 And i < 8 Then
        If ColPedB(i) <> 0 And ColPedB(i + 1) = 0 And ColPedB(i - 1) = 0 Then
            'il bianco ha un pedone isolato
            Score(BIANCHI) = Score(BIANCHI) + PEDONE_ISOLATO
        End If
    End If
    'Pedoni neri
    If ColPedN(i) > 0 And ColPedB(i) = 0 Then
        'il nero ha un pedone passato
        Score(NERI) = Score(NERI) + PEDONE_PASSATO
    End If
    If ColPedN(i) > 1 Then
        'il nero ha uno o piu' pedoni doppiati
        Score(NERI) = Score(NERI) + (ColPedN(i) * PEDONE_DOPPIATO)
    End If
    If i = 1 Then
        If ColPedN(i) <> 0 And ColPedN(i + 1) = 0 Then
            'il pedone nella colonna A e' isolato
            Score(NERI) = Score(NERI) + PEDONE_ISOLATO
        End If
    End If
    If i = 8 Then
        If ColPedN(i) <> 0 And ColPedN(i - 1) = 0 Then
            'il pedone nella colonna H e' isolato
            Score(NERI) = Score(NERI) + PEDONE_ISOLATO
        End If
    End If
    If i > 1 And i < 8 Then
        If ColPedN(i) <> 0 And ColPedN(i + 1) = 0 And ColPedN(i - 1) = 0 Then
            'il nero ha un pedone isolato
            Score(NERI) = Score(NERI) + PEDONE_ISOLATO
        End If
    End If
Next i
'sicurezza del Re
If Stadio = 1 Or Stadio = 2 Then
'considerazioni per apertura
    If ARROCCO(1).Re > 90 Then
       'tolgo 20 punti per ogni pedone mosso davanti
       'al Re arroccato
       If PosReB = 63 Then
          If Board(54) <> 1 Then Score(BIANCHI) = Score(BIANCHI) - 20
          If Board(55) <> 1 Then Score(BIANCHI) = Score(BIANCHI) - 20
          If Board(56) <> 1 Then Score(BIANCHI) = Score(BIANCHI) - 20
       End If
       If PosReB = 59 Or PosReB = 58 Then
          If Board(49) <> 1 Then Score(BIANCHI) = Score(BIANCHI) - 20
          If Board(50) <> 1 Then Score(BIANCHI) = Score(BIANCHI) - 20
          If Board(51) <> 1 Then Score(BIANCHI) = Score(BIANCHI) - 20
       End If
    End If
    If ARROCCO(2).Re > 90 Then
      If PosReN = 7 Then
          If Board(14) <> -1 Then Score(NERI) = Score(NERI) - 20
          If Board(15) <> -1 Then Score(NERI) = Score(NERI) - 20
          If Board(16) <> -1 Then Score(NERI) = Score(NERI) - 20
       End If
       If PosReN = 3 Or PosReN = 2 Then
          If Board(9) <> -1 Then Score(NERI) = Score(NERI) - 20
          If Board(10) <> -1 Then Score(NERI) = Score(NERI) - 20
          If Board(11) <> -1 Then Score(NERI) = Score(NERI) - 20
       End If
    End If
End If

If Turno = 1 Then
    Eval = Score(BIANCHI) - Score(NERI)
Else
    Eval = Score(NERI) - Score(BIANCHI)
End If
End Function


Function EvalReBianco(pos As Integer) As Integer
'ritorna un valore di sicurezza del Re basato dalla vicinanza dei pezzi
'avversari:il pezzo avversario e posto in Pos
Dim Valore As Integer

'sottraggo la posizione del Re dal pezzo avversario
Valore = PosReB - pos
'divido il valore per 8
Valore = Int(Valore / 8)
'se il numero e' negativo lo rendo positivo
If Valore < 0 Then Valore = Valore * -1
'aggiungo il valore ottenuto dalla penalita' vicinanza al Re:
'piu' vicino un pezzo nemico e' al Re peggiore e' la valutazione
'es: DISTANZA DAL RE         PENALITA'-DISTANZA
'          6                         -14
'          5                         -15
'          2                         -18
Valore = SICUREZZARE + Valore
EvalReBianco = Valore

End Function

Function EvalReNero(pos As Integer) As Integer
'ritorna un valore di sicurezza del Re basato dalla vicinanza dei pezzi
'avversari:il pezzo avversario e posto in Pos
Dim Valore As Integer

'sottraggo la posizione del Re dal pezzo avversario
Valore = PosReN - pos
'divido il valore per 8
Valore = Int(Valore / 8)
'se il numero e' negativo lo rendo positivo
If Valore < 0 Then Valore = Valore * -1
'aggiungo il valore ottenuto dalla penalita' vicinanza al Re:
'piu' vicino un pezzo nemico e' al Re peggiore e' la valutazione
'es: DISTANZA DAL RE         PENALITA'-DISTANZA
'          6                         -14
'          5                         -15
'          2                         -18
Valore = SICUREZZARE + Valore
EvalReNero = Valore
End Function

Sub FaiMossa(Ply As Integer, idx As Integer)

Mossa(Ply, idx).Illegale = False
If Mossa(Ply, idx).Speciale = 0 Then
   Board(Mossa(Ply, idx).CPar) = 0
   Board(Mossa(Ply, idx).CArr) = Mossa(Ply, idx).Mosso
   If Mossa(Ply, idx).Mosso = 7 Then ARROCCO(1).Torre7 = ARROCCO(1).Torre7 + 1
   If Mossa(Ply, idx).Mosso = 4 Then ARROCCO(1).Torre4 = ARROCCO(1).Torre4 + 1
   If Mossa(Ply, idx).Mosso = -7 Then ARROCCO(2).Torre7 = ARROCCO(2).Torre7 + 1
   If Mossa(Ply, idx).Mosso = -4 Then ARROCCO(2).Torre4 = ARROCCO(2).Torre4 + 1
   If Mossa(Ply, idx).Mosso = 6 Then ARROCCO(1).Re = ARROCCO(1).Re + 1
   If Mossa(Ply, idx).Mosso = -6 Then ARROCCO(2).Re = ARROCCO(2).Re + 1
End If
If Mossa(Ply, idx).Speciale = 1 Then
   'arrocco
   If Turno = 1 Then
      'se il Re e' in scacco o se deve transitare su case
      'in scacco la mossa non e' valida:per controllare
      'questa possibilita' si aggiungono dei Re virtuali
      'sulle case dove il Re deve muovere
      If Mossa(Ply, idx).Algebrica = "e1g1" Then
        Board(62) = 6: Board(63) = 6
      End If
      If Mossa(Ply, idx).Algebrica = "e1c1" Then
        Board(60) = 6: Board(59) = 6: Board(58) = 6
      End If
      If InScacco(MaxPly) = True Then
         Mossa(Ply, idx).Illegale = True
         Illegale = 2
      End If
      'rimuovo i Re fittizi
      If Mossa(Ply, idx).Algebrica = "e1g1" Then
        Board(62) = 0: Board(63) = 0
      End If
      If Mossa(Ply, idx).Algebrica = "e1c1" Then
        Board(60) = 0: Board(59) = 0: Board(58) = 0
      End If
      If Mossa(Ply, idx).Illegale = True Then GoTo Esce_sub
      If Mossa(Ply, idx).Algebrica = "e1g1" And Mossa(Ply, idx).Mosso = 6 Then
        ARROCCO(1).Re = ARROCCO(1).Re + 100
        ARROCCO(1).Torre4 = ARROCCO(1).Torre4 + 1
        Board(61) = 0: Board(63) = 6: Board(62) = 4: Board(64) = 0
      End If
      If Mossa(Ply, idx).Algebrica = "e1c1" And Mossa(Ply, idx).Mosso = 6 Then
         ARROCCO(1).Re = ARROCCO(1).Re + 100
         ARROCCO(1).Torre7 = ARROCCO(1).Torre7 + 1
         Board(57) = 0: Board(60) = 7: Board(61) = 0: Board(59) = 6
      End If
   End If
   If Turno = -1 Then
      'se il Re e' in scacco o se deve transitare su case
      'in scacco la mossa non e' valida:per controllare
      'questa possibilita' si aggiungono dei Re virtuali
      'sulle case dove il Re deve muovere
      If Mossa(Ply, idx).Algebrica = "e8g8" Then
        Board(6) = -6: Board(7) = -6
      End If
      If Mossa(Ply, idx).Algebrica = "e8c8" Then
        Board(4) = -6: Board(3) = -6: Board(2) = -6
      End If
      If InScacco(MaxPly) = True Then
         Mossa(Ply, idx).Illegale = True
         Illegale = 2
      End If
      'rimuovo i Re fittizi
      If Mossa(Ply, idx).Algebrica = "e8g8" Then
        Board(6) = 0: Board(7) = 0
      End If
      If Mossa(Ply, idx).Algebrica = "e8c8" Then
        Board(4) = 0: Board(3) = 0: Board(2) = 0
      End If
      If Mossa(Ply, idx).Illegale = True Then GoTo Esce_sub
      If Mossa(Ply, idx).Algebrica = "e8g8" And Mossa(Ply, idx).Mosso = -6 Then
        ARROCCO(2).Re = ARROCCO(2).Re + 100
        ARROCCO(2).Torre4 = ARROCCO(2).Torre4 + 1
        Board(5) = 0: Board(7) = -6: Board(6) = -4: Board(8) = 0
      End If
      If Mossa(Ply, idx).Algebrica = "e8c8" And Mossa(Ply, idx).Mosso = -6 Then
        ARROCCO(2).Re = ARROCCO(2).Re + 100
        ARROCCO(2).Torre7 = ARROCCO(2).Torre7 + 1
        Board(1) = 0: Board(4) = -7: Board(5) = 0: Board(3) = -6
      End If
   End If
End If
If Mossa(Ply, idx).Speciale = 2 Then
   'promozione pedone
      Board(Mossa(Ply, idx).CPar) = 0
      Board(Mossa(Ply, idx).CArr) = Mossa(Ply, idx).Promosso
End If
If Mossa(Ply, idx).Speciale = 3 Then
  'cattura en-passant
  Board(Mossa(Ply, idx).CPar) = 0
  Board(Mossa(Ply, idx).CArr) = Mossa(Ply, idx).Mosso
  Board(Mossa(Ply, idx).Catturato) = 0
End If
If Mossa(Ply, idx).Mosso = 6 Then PosReB = Mossa(Ply, idx).CArr
If Mossa(Ply, idx).Mosso = -6 Then PosReN = Mossa(Ply, idx).CArr
UltimaMossa = Mossa(Ply, idx)
Esce_sub:
End Sub

Function Fill64(Dest() As Integer, ParamArray Sorg() As Variant)
'invece di usare le tabelle memorizzate in variabili variant
'si usa questa funzione per popolare un array di tipo integer
'questa tecnica e' stata utilizzata da Luca Dormio in Larsen
Dim i As Integer
Erase Dest
For i = 1 To 64
    Dest(i) = Sorg(i - 1)
Next i
End Function

Sub GenAlfiere(pos As Integer, moveidx As Integer, Ply As Integer)

Dim i As Integer
Dim cambio As Boolean

cambio = False
For i = 1 To 16
    If cambio = True And Diags(pos, i) <> 0 Then GoTo Altro
    If cambio = True And Diags(pos, i) = 0 Then
        cambio = False
        GoTo Altro
    End If
    If Diags(pos, i) = 0 Then GoTo Altro
    If Diags(pos, i) = 99 Then Exit For
    If Board(Diags(pos, i)) = 0 Then
        Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(Diags(pos, i))
        Mossa(Ply, moveidx).CArr = Diags(pos, i)
        Mossa(Ply, moveidx).Catturato = 0
        Mossa(Ply, moveidx).CPar = pos
        Mossa(Ply, moveidx).Mosso = Board(pos)
        Mossa(Ply, moveidx).Speciale = 0
        Mossa(Ply, moveidx).Illegale = False
        Mossa(Ply, moveidx).Promosso = 0
        moveidx = moveidx + 1
    End If
    If Board(Diags(pos, i)) <> 0 And Sgn(Board(Diags(pos, i))) <> Sgn(Board(pos)) Then
        Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(Diags(pos, i))
        Mossa(Ply, moveidx).CArr = Diags(pos, i)
        Mossa(Ply, moveidx).Catturato = Board(Diags(pos, i))
        Mossa(Ply, moveidx).CPar = pos
        Mossa(Ply, moveidx).Mosso = Board(pos)
        Mossa(Ply, moveidx).Speciale = 0
        Mossa(Ply, moveidx).Illegale = False
        Mossa(Ply, moveidx).Promosso = 0
        moveidx = moveidx + 1
        cambio = True
    End If
    If Board(Diags(pos, i)) <> 0 And Sgn(Board(Diags(pos, i))) = Sgn(Board(pos)) Then
        cambio = True
    End If
    
Altro:
Next i

End Sub

Sub GenCavallo(pos As Integer, moveidx As Integer, Ply As Integer)
'genera le mosse di cavallo

Dim i As Integer

For i = 1 To 8
    If Cavs(pos, i) = 99 Then Exit Sub
    If Cavs(pos, i) <> 99 Then
        If Board(Cavs(pos, i)) = 0 Then
            Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(Cavs(pos, i))
            Mossa(Ply, moveidx).CArr = Cavs(pos, i)
            Mossa(Ply, moveidx).Catturato = 0
            Mossa(Ply, moveidx).CPar = pos
            Mossa(Ply, moveidx).Mosso = Board(pos)
            Mossa(Ply, moveidx).Speciale = 0
            Mossa(Ply, moveidx).Illegale = False
            Mossa(Ply, moveidx).Promosso = 0
            moveidx = moveidx + 1
        End If
        If Board(Cavs(pos, i)) <> 0 And Sgn(Board(Cavs(pos, i))) <> Sgn(Board(pos)) Then
            Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(Cavs(pos, i))
            Mossa(Ply, moveidx).CArr = Cavs(pos, i)
            Mossa(Ply, moveidx).Catturato = Board(Cavs(pos, i))
            Mossa(Ply, moveidx).CPar = pos
            Mossa(Ply, moveidx).Mosso = Board(pos)
            Mossa(Ply, moveidx).Speciale = 0
            Mossa(Ply, moveidx).Illegale = False
            Mossa(Ply, moveidx).Promosso = 0
            moveidx = moveidx + 1
        End If
    End If
Next i

End Sub

Sub GeneraMosse(Ply As Integer)
'generatore di mosse: si scandisce la scacchiera e quando
'si trova un pezzo della parte che deve muovere si
'generano le mosse archiviandole nella tabella
'Mossa(Ply,Indice)

Dim i As Integer, Counter As Integer
Counter = 1
For i = 1 To 64
    If Sgn(Board(i)) = Sgn(Turno) Then
        If Board(i) = 1 Or Board(i) = -1 Then GenPedone i, Counter, Ply
        If Board(i) = 2 Or Board(i) = -2 Then GenCavallo i, Counter, Ply
        If Board(i) = 3 Or Board(i) = -3 Then GenAlfiere i, Counter, Ply
        If Board(i) = 4 Or Board(i) = -4 Then GenTorre i, Counter, Ply
        If Board(i) = 7 Or Board(i) = -7 Then GenTorre i, Counter, Ply
        If Board(i) = 5 Or Board(i) = -5 Then
            GenAlfiere i, Counter, Ply
            GenTorre i, Counter, Ply
        End If
        If Board(i) = 6 Or Board(i) = -6 Then GenRe i, Counter, Ply
    End If
Next i
'pongo il marcatore di fine mosse
Mossa(Ply, Counter).CPar = -100
Mossa(Ply, Counter).Algebrica = "finemosse"
Mossa(Ply, Counter).Punteggio = -32768
'conto le mosse in mob() (mobilità)
Mobil(Ply) = Counter - 1
OrdinamentoTattico Ply, Counter - 1
End Sub


Sub GenPedone(pos As Integer, moveidx As Integer, Ply As Integer)
'genera le mosse di pedone posizionato nella casa Pos

Dim i As Integer, Cattura As Boolean, Pezzo As Integer
Dim PezzoString(2 To 5) As String
Dim MossadiDue As Integer

i = 1
PezzoString(2) = "n": PezzoString(3) = "b": PezzoString(4) = "r"
PezzoString(5) = "q"

'CONTROLLO EN-PASSANT (speciale 3)
If Turno = 1 Then
   If UltimaMossa.Mosso = -1 And _
      (UltimaMossa.CArr = pos - 1 Or UltimaMossa.CArr = pos + 1) _
       And (UltimaMossa.CPar > 8 And UltimaMossa.CPar < 17) Then
      If UltimaMossa.Enpassabile = True Then
         If UltimaMossa.CArr = pos - 1 And (UltimaMossa.CArr > 24 And UltimaMossa.CArr < 33) Then
            If pos <> 33 Then
              Mossa(Ply, moveidx).CPar = pos
              Mossa(Ply, moveidx).CArr = pos - 9
              Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(pos - 9)
              'catturato qui indica la casa e non il pezzo
              Mossa(Ply, moveidx).Catturato = pos - 1
              Mossa(Ply, moveidx).Mosso = Board(pos)
              Mossa(Ply, moveidx).Speciale = 3
              Mossa(Ply, moveidx).Illegale = False
              Mossa(Ply, moveidx).Enpassabile = False
              moveidx = moveidx + 1
            End If
         End If
         If UltimaMossa.CArr = pos + 1 And (UltimaMossa.CArr > 24 And UltimaMossa.CArr < 33) Then
            If pos <> 40 Then
              Mossa(Ply, moveidx).CPar = pos
              Mossa(Ply, moveidx).CArr = pos - 7
              Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(pos - 7)
              'catturato qui indica la casa e non il pezzo
              Mossa(Ply, moveidx).Catturato = pos + 1
              Mossa(Ply, moveidx).Mosso = Board(pos)
              Mossa(Ply, moveidx).Speciale = 3
              Mossa(Ply, moveidx).Illegale = False
              Mossa(Ply, moveidx).Enpassabile = False
              moveidx = moveidx + 1
            End If
         End If
      End If
   End If
End If
If Turno = -1 Then
   If UltimaMossa.Mosso = 1 And _
      (UltimaMossa.CArr = pos - 1 Or UltimaMossa.CArr = pos + 1) _
       And (UltimaMossa.CPar > 48 And UltimaMossa.CPar < 57) Then
      If UltimaMossa.Enpassabile = True Then
         If UltimaMossa.CArr = pos - 1 And (UltimaMossa.CArr > 32 And UltimaMossa.CArr < 41) Then
            Mossa(Ply, moveidx).CPar = pos
            Mossa(Ply, moveidx).CArr = pos + 7
            Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(pos + 7)
            'catturato qui indica la casa e non il pezzo
            Mossa(Ply, moveidx).Catturato = pos - 1
            Mossa(Ply, moveidx).Mosso = Board(pos)
            Mossa(Ply, moveidx).Speciale = 3
            Mossa(Ply, moveidx).Illegale = False
            Mossa(Ply, moveidx).Enpassabile = False
            moveidx = moveidx + 1
         End If
         If UltimaMossa.CArr = pos + 1 And (UltimaMossa.CArr > 32 And UltimaMossa.CArr < 41) Then
            If pos <> 32 Then
             Mossa(Ply, moveidx).CPar = pos
             Mossa(Ply, moveidx).CArr = pos + 9
             Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(pos + 9)
             'catturato qui indica la casa e non il pezzo
             Mossa(Ply, moveidx).Catturato = pos + 1
             Mossa(Ply, moveidx).Mosso = Board(pos)
             Mossa(Ply, moveidx).Speciale = 3
             Mossa(Ply, moveidx).Illegale = False
             Mossa(Ply, moveidx).Enpassabile = False
             moveidx = moveidx + 1
            End If
         End If
      End If
   End If
End If

'MOSSE NORMALI
Cattura = False
'se tocca al bianco...
If Turno = 1 Then
    For i = 1 To 5
        If Cattura = False Then
           If PedB(pos, i) <> 0 And PedB(pos, i) <> 99 Then
              Pezzo = 2 'il pezzo della promozione
Rifai1:
              If Board(PedB(pos, i)) = 0 Then
                 Mossa(Ply, moveidx).CPar = pos
                 Mossa(Ply, moveidx).CArr = PedB(pos, i)
                 Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(PedB(pos, i))
                 Mossa(Ply, moveidx).Catturato = 0
                 Mossa(Ply, moveidx).Mosso = Board(pos)
                 Mossa(Ply, moveidx).Speciale = 0
                 Mossa(Ply, moveidx).Illegale = False
                 'se muovo di due case setto enpassabile
                 If Mossa(Ply, moveidx).CArr = Mossa(Ply, moveidx).CPar - 16 Then
                    MossadiDue = Mossa(Ply, moveidx).CArr
                    If Board(MossadiDue - 1) = -1 Or Board(MossadiDue + 1) = -1 Then
                       Mossa(Ply, moveidx).Enpassabile = True
                    End If
                 End If
                 'controllo se promozione
                 If CheckPromozione(PedB(pos, i)) = True And Pezzo < 6 Then
                    Mossa(Ply, moveidx).Catturato = 0
                    Mossa(Ply, moveidx).Promosso = Pezzo
                    Mossa(Ply, moveidx).Speciale = 2
                    Mossa(Ply, moveidx).Algebrica = _
                        Mossa(Ply, moveidx).Algebrica + PezzoString(Pezzo)
                    Pezzo = Pezzo + 1
                    moveidx = moveidx + 1
                    GoTo Rifai1
                 End If
                 If CheckPromozione(PedB(pos, i)) = True And Pezzo = 6 Then
                    moveidx = moveidx - 1
                 End If
                 moveidx = moveidx + 1
                 If PedB(pos, i) = pos - 16 And Board(pos - 8) <> 0 Then
                    'se muovo di due case il pedone in apertura
                    'e la prima casa è occupata mossa=illegale
                    moveidx = moveidx - 1
                 End If
              End If
           End If
           If PedB(pos, i) = 99 Then Cattura = True
        End If
        If Cattura = True Then
           If PedB(pos, i) <> 0 And PedB(pos, i) <> 99 Then
              Pezzo = 2 'il pezzo della promozione
Rifai2:
              If Board(PedB(pos, i)) <> 0 And Sgn(Board(PedB(pos, i))) <> Sgn(Board(pos)) Then
                 Mossa(Ply, moveidx).CPar = pos
                 Mossa(Ply, moveidx).CArr = PedB(pos, i)
                 Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(PedB(pos, i))
                 Mossa(Ply, moveidx).Catturato = Board(PedB(pos, i))
                 Mossa(Ply, moveidx).Mosso = Board(pos)
                 Mossa(Ply, moveidx).Speciale = 0
                 Mossa(Ply, moveidx).Illegale = False
                 Mossa(Ply, moveidx).Enpassabile = False
                 'controllo se promozione
                 If CheckPromozione(PedB(pos, i)) = True And Pezzo < 6 Then
                    Mossa(Ply, moveidx).Catturato = Board(PedB(pos, i))
                    Mossa(Ply, moveidx).Promosso = Pezzo
                    Mossa(Ply, moveidx).Speciale = 2
                    Mossa(Ply, moveidx).Algebrica = _
                        Mossa(Ply, moveidx).Algebrica + PezzoString(Pezzo)
                    Pezzo = Pezzo + 1
                    moveidx = moveidx + 1
                    GoTo Rifai2
                 End If
                 If CheckPromozione(PedB(pos, i)) = True And Pezzo = 6 Then
                    moveidx = moveidx - 1
                 End If
                 moveidx = moveidx + 1
              End If
           End If
        End If
    Next i
End If

'se tocca al nero...
If Turno = -1 Then
    For i = 1 To 5
        If Cattura = False Then
           If PedN(pos, i) <> 0 And PedN(pos, i) <> 99 Then
              Pezzo = 2 'pezzo di promozione
Rifai3:
              If Board(PedN(pos, i)) = 0 Then
                 Mossa(Ply, moveidx).CPar = pos
                 Mossa(Ply, moveidx).CArr = PedN(pos, i)
                 Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(PedN(pos, i))
                 Mossa(Ply, moveidx).Catturato = 0
                 Mossa(Ply, moveidx).Mosso = Board(pos)
                 Mossa(Ply, moveidx).Speciale = 0
                 Mossa(Ply, moveidx).Illegale = False
                 'se muovo di due case setto enpassabile
                 If Mossa(Ply, moveidx).CArr = Mossa(Ply, moveidx).CPar + 16 Then
                    MossadiDue = Mossa(Ply, moveidx).CArr
                    If Board(MossadiDue - 1) = 1 Or Board(MossadiDue + 1) = 1 Then
                       Mossa(Ply, moveidx).Enpassabile = True
                    End If
                 End If
                 'controllo se promozione
                 If CheckPromozione(PedN(pos, i)) = True And Pezzo < 6 Then
                    Mossa(Ply, moveidx).Catturato = 0
                    Mossa(Ply, moveidx).Promosso = -Pezzo
                    Mossa(Ply, moveidx).Speciale = 2
                    Mossa(Ply, moveidx).Algebrica = _
                        Mossa(Ply, moveidx).Algebrica + PezzoString(Pezzo)
                    Pezzo = Pezzo + 1
                    moveidx = moveidx + 1
                    GoTo Rifai3
                 End If
                 If CheckPromozione(PedN(pos, i)) = True And Pezzo = 6 Then
                    moveidx = moveidx - 1
                 End If
                 moveidx = moveidx + 1
                 If PedN(pos, i) = pos + 16 And Board(pos + 8) <> 0 Then
                   'se muovo di due case il pedone in apertura
                   'e la prima casa è occupata mossa=illegale
                   moveidx = moveidx - 1
                 End If
              End If
           End If
           If PedN(pos, i) = 99 Then Cattura = True
        End If
        If Cattura = True Then
           If PedN(pos, i) <> 0 And PedN(pos, i) <> 99 Then
              Pezzo = 2 'pezzo di promozione
Rifai4:
              If Board(PedN(pos, i)) <> 0 And Sgn(Board(PedN(pos, i))) <> Sgn(Board(pos)) Then
                 Mossa(Ply, moveidx).CPar = pos
                 Mossa(Ply, moveidx).CArr = PedN(pos, i)
                 Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(PedN(pos, i))
                 Mossa(Ply, moveidx).Catturato = Board(PedN(pos, i))
                 Mossa(Ply, moveidx).Mosso = Board(pos)
                 Mossa(Ply, moveidx).Speciale = 0
                 Mossa(Ply, moveidx).Illegale = False
                 Mossa(Ply, moveidx).Enpassabile = False
                 'controllo se promozione
                 If CheckPromozione(PedN(pos, i)) = True And Pezzo < 6 Then
                    Mossa(Ply, moveidx).Catturato = Board(PedN(pos, i))
                    Mossa(Ply, moveidx).Promosso = -Pezzo
                    Mossa(Ply, moveidx).Speciale = 2
                    Mossa(Ply, moveidx).Algebrica = _
                        Mossa(Ply, moveidx).Algebrica + PezzoString(Pezzo)
                    Pezzo = Pezzo + 1
                    moveidx = moveidx + 1
                    GoTo Rifai4
                 End If
                 If CheckPromozione(PedN(pos, i)) = True And Pezzo = 6 Then
                    moveidx = moveidx - 1
                 End If
                 moveidx = moveidx + 1
              End If
           End If
        End If
    Next i
End If

GenPedone_Uscita:
End Sub


Sub GenRe(pos As Integer, moveidx As Integer, Ply As Integer)
Dim i As Integer

For i = 1 To 8
    If Re(pos, i) = 99 Then Exit For
    If Board(Re(pos, i)) = 0 Then
        Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(Re(pos, i))
        Mossa(Ply, moveidx).CArr = Re(pos, i)
        Mossa(Ply, moveidx).Catturato = 0
        Mossa(Ply, moveidx).CPar = pos
        Mossa(Ply, moveidx).Mosso = Board(pos)
        Mossa(Ply, moveidx).Speciale = 0
        Mossa(Ply, moveidx).Illegale = False
        moveidx = moveidx + 1
    End If
    If Board(Re(pos, i)) <> 0 And Sgn(Board(Re(pos, i))) <> Sgn(Board(pos)) Then
        Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(Re(pos, i))
        Mossa(Ply, moveidx).CArr = Re(pos, i)
        Mossa(Ply, moveidx).Catturato = Board(Re(pos, i))
        Mossa(Ply, moveidx).CPar = pos
        Mossa(Ply, moveidx).Mosso = Board(pos)
        Mossa(Ply, moveidx).Speciale = 0
        Mossa(Ply, moveidx).Illegale = False
        moveidx = moveidx + 1
    End If
Next i
'determino possibilità di arrocco
If Turno = 1 Then 'arrocco del bianco
    'arrocco corto
    If ARROCCO(1).Re = 0 And ARROCCO(1).Torre4 = 0 And _
       Board(61) = 6 And Board(64) = 4 Then
        If Board(62) = 0 And Board(63) = 0 Then
           Mossa(Ply, moveidx).Algebrica = "e1g1"
           Mossa(Ply, moveidx).CArr = 63
           Mossa(Ply, moveidx).Catturato = 0
           Mossa(Ply, moveidx).CPar = pos
           Mossa(Ply, moveidx).Mosso = Board(pos)
           Mossa(Ply, moveidx).Speciale = 1
           Mossa(Ply, moveidx).Illegale = False
           moveidx = moveidx + 1
        End If
    End If
    'arrocco lungo
    If ARROCCO(1).Re = 0 And ARROCCO(1).Torre7 = 0 And _
       Board(61) = 6 And Board(57) = 7 Then
        If Board(58) = 0 And Board(59) = 0 And Board(60) = 0 Then
           Mossa(Ply, moveidx).Algebrica = "e1c1"
           Mossa(Ply, moveidx).CArr = 59
           Mossa(Ply, moveidx).Catturato = 0
           Mossa(Ply, moveidx).CPar = pos
           Mossa(Ply, moveidx).Mosso = Board(pos)
           Mossa(Ply, moveidx).Speciale = 1
           Mossa(Ply, moveidx).Illegale = False
           moveidx = moveidx + 1
        End If
    End If
End If
If Turno = -1 Then 'arrocco del nero
    'arrocco corto
    If ARROCCO(2).Re = 0 And ARROCCO(2).Torre4 = 0 And _
       Board(5) = -6 And Board(8) = -4 Then
        If Board(6) = 0 And Board(7) = 0 Then
           Mossa(Ply, moveidx).Algebrica = "e8g8"
           Mossa(Ply, moveidx).CArr = 7
           Mossa(Ply, moveidx).Catturato = 0
           Mossa(Ply, moveidx).CPar = pos
           Mossa(Ply, moveidx).Mosso = Board(pos)
           Mossa(Ply, moveidx).Speciale = 1
           Mossa(Ply, moveidx).Illegale = False
           moveidx = moveidx + 1
        End If
    End If
    'arrocco lungo
    If ARROCCO(2).Re = 0 And ARROCCO(2).Torre7 = 0 And _
       Board(5) = -6 And Board(1) = -7 Then
        If Board(2) = 0 And Board(3) = 0 And Board(4) = 0 Then
           Mossa(Ply, moveidx).Algebrica = "e8c8"
           Mossa(Ply, moveidx).CArr = 3
           Mossa(Ply, moveidx).Catturato = 0
           Mossa(Ply, moveidx).CPar = pos
           Mossa(Ply, moveidx).Mosso = Board(pos)
           Mossa(Ply, moveidx).Speciale = 1
           Mossa(Ply, moveidx).Illegale = False
           moveidx = moveidx + 1
        End If
    End If
End If

End Sub

Sub GenTorre(pos As Integer, moveidx As Integer, Ply As Integer)
Dim i As Integer
Dim cambio As Boolean

cambio = False
For i = 1 To 17
    If cambio = True And Cols(pos, i) <> 0 Then GoTo Altro
    If cambio = True And Cols(pos, i) = 0 Then
        cambio = False
        GoTo Altro
    End If
    If Cols(pos, i) = 0 Then GoTo Altro
    If Cols(pos, i) = 99 Then Exit For
    If Board(Cols(pos, i)) = 0 Then
        Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(Cols(pos, i))
        Mossa(Ply, moveidx).CArr = Cols(pos, i)
        Mossa(Ply, moveidx).Catturato = 0
        Mossa(Ply, moveidx).CPar = pos
        Mossa(Ply, moveidx).Mosso = Board(pos)
        Mossa(Ply, moveidx).Speciale = 0
        Mossa(Ply, moveidx).Illegale = False
        moveidx = moveidx + 1
    End If
    If Board(Cols(pos, i)) <> 0 And Sgn(Board(Cols(pos, i))) <> Sgn(Board(pos)) Then
        Mossa(Ply, moveidx).Algebrica = Coords(pos) + Coords(Cols(pos, i))
        Mossa(Ply, moveidx).CArr = Cols(pos, i)
        Mossa(Ply, moveidx).Catturato = Board(Cols(pos, i))
        Mossa(Ply, moveidx).CPar = pos
        Mossa(Ply, moveidx).Mosso = Board(pos)
        Mossa(Ply, moveidx).Speciale = 0
        Mossa(Ply, moveidx).Illegale = False
        moveidx = moveidx + 1
        cambio = True
    End If
    If Board(Cols(pos, i)) <> 0 And Sgn(Board(Cols(pos, i))) = Sgn(Board(pos)) Then
        cambio = True
    End If

Altro:
Next i

End Sub

Sub Init()
'avvia l'engine caricando tutte le tabelle di dati
Dim x As Integer, Y As Integer, num As Integer

'Colonne=tabella contenente un numero riferito alla colonna
Fill64 Colonne, 1, 2, 3, 4, 5, 6, 7, 8, _
               1, 2, 3, 4, 5, 6, 7, 8, _
               1, 2, 3, 4, 5, 6, 7, 8, _
               1, 2, 3, 4, 5, 6, 7, 8, _
               1, 2, 3, 4, 5, 6, 7, 8, _
               1, 2, 3, 4, 5, 6, 7, 8, _
               1, 2, 3, 4, 5, 6, 7, 8, _
               1, 2, 3, 4, 5, 6, 7, 8

'Flip=scacchiera rovesciata per valutare i pezzi neri
Fill64 Flip, 57, 58, 59, 60, 61, 62, 63, 64, _
            49, 50, 51, 52, 53, 54, 55, 56, _
            41, 42, 43, 44, 45, 46, 47, 48, _
             33, 34, 35, 36, 37, 38, 39, 40, _
             25, 26, 27, 28, 29, 30, 31, 32, _
             17, 18, 19, 20, 21, 22, 23, 24, _
              9, 10, 11, 12, 13, 14, 15, 16, _
              1, 2, 3, 4, 5, 6, 7, 8

Fill64 Pedoni, 0, 0, 0, 0, 0, 0, 0, 0, _
            5, 10, 15, 20, 20, 15, 10, 5, _
            4, 8, 12, 16, 16, 12, 8, 4, _
            3, 6, 9, 12, 12, 9, 6, 3, _
              2, 4, 6, 8, 8, 6, 4, 2, _
            1, 2, 3, -10, -10, 3, 2, 1, _
            0, 0, 0, -40, -40, 0, 0, 0, _
            0, 0, 0, 0, 0, 0, 0, 0
      
Fill64 Cavalli, -10, -10, -10, -10, -10, -10, -10, -10, _
    -10, 0, 0, 0, 0, 0, 0, -10, _
    -10, 0, 5, 5, 5, 5, 0, -10, _
    -10, 0, 5, 10, 10, 5, 0, -10, _
    -10, 0, 5, 10, 10, 5, 0, -10, _
    -10, 0, 5, 5, 5, 5, 0, -10, _
    -10, 0, 0, 0, 0, 0, 0, -10, _
    -10, -30, -10, -10, -10, -10, -30, -10
    
Fill64 Alfieri, -10, -10, -10, -10, -10, -10, -10, -10, _
    -10, 0, 0, 0, 0, 0, 0, -10, _
    -10, 2, 5, 5, 5, 5, 2, -10, _
    -10, 0, 5, 10, 10, 5, 0, -10, _
    -10, 0, 5, 10, 10, 5, 0, -10, _
    -10, 0, 5, 5, 5, 5, 0, -10, _
    -10, 2, 0, 0, 0, 0, 2, -10, _
    -10, -10, -20, -10, -10, -20, -10, -10
    
Fill64 ReApe, -40, -40, -40, -40, -40, -40, -40, -40, _
              -40, -40, -40, -40, -40, -40, -40, -40, _
              -40, -40, -40, -40, -40, -40, -40, -40, _
              -40, -40, -40, -40, -40, -40, -40, -40, _
              -40, -40, -40, -40, -40, -40, -40, -40, _
              -40, -40, -40, -40, -40, -40, -40, -40, _
              -20, -20, -20, -20, -20, -20, -20, -20, _
               0, 20, 40, -20, 0, -20, 40, 20
      
Fill64 ReFinale, 0, 10, 20, 30, 30, 20, 10, 0, _
     10, 20, 30, 40, 40, 30, 20, 10, _
     20, 30, 40, 50, 50, 40, 30, 20, _
     30, 40, 50, 60, 60, 50, 40, 30, _
     30, 40, 50, 60, 60, 50, 40, 30, _
     20, 30, 40, 50, 50, 40, 30, 20, _
     10, 20, 30, 40, 40, 30, 20, 10, _
      0, 10, 20, 30, 30, 20, 10, 0
'movimenti cavallo
Open "cavs.dat" For Input As #1
For x = 1 To 64
   For Y = 1 To 8
        Input #1, Cavs(x, Y)
   Next Y
Next x
Close #1

'movimenti torre e donna
Open "cols.dat" For Input As #1
For x = 1 To 64
    For Y = 1 To 17
        Input #1, Cols(x, Y)
    Next Y
Next x
Close #1

'movimenti alfiere e donna
Open "diags.dat" For Input As #1
For x = 1 To 64
    For Y = 1 To 16
        Input #1, Diags(x, Y)
    Next Y
Next x
Close #1

'movimenti re
Open "re.dat" For Input As #1
For x = 1 To 64
    For Y = 1 To 8
        Input #1, Re(x, Y)
    Next Y
Next x
Close #1

'movimenti pedone (pedb=bianchi pedn=neri)
Open "pedb.dat" For Input As #1
For x = 1 To 64
    For Y = 1 To 5
        Input #1, PedB(x, Y)
    Next Y
Next x
Close #1

Open "pedn.dat" For Input As #1
For x = 1 To 64
    For Y = 1 To 5
        Input #1, PedN(x, Y)
    Next Y
Next x
Close #1

'coordinate scacchiera in forma algebrica
Open "coords.dat" For Input As #1
    For x = 1 To 64
        Input #1, Coords(x)
    Next x
Close #1

Turno = 1 ' turno al bianco
Stadio = 1 'apertura
ContaMosse = 1
End Sub

Sub ListaMosseBook()
'questa routine si attiva quando BookMode=1
'invia sulla ListBox sul Form l'elenco delle mosse legali
'nella posizione corrente
Dim i As Byte
GeneraMosse (49)
i = 1
Do
    Form1.List1.AddItem (Mossa(49, i).Algebrica)
    i = i + 1
Loop Until Mossa(49, i).CPar = -100

End Sub

Function Main(MossaUtente As String, Tempo As Single) As String
'Programma principale: riceve in input la mossa in forma
'algebrica e risponde con una mossa in forma algebrica o
'con un comando convenzionale.
Dim Risposta As String, Best As Integer, i As Integer
Dim Indice As Integer, Profo As Integer, idx As Integer
Dim j As Integer, Stringa As String
Dim Alfa As Integer, Beta As Integer
Dim Mosso As Integer, Catturato As Integer

'variabili per il parsing del tempo
Dim Mps As Long 'mosse per secondi
Ply = 1
Profo = 1 'profondità di inizio ricerca

If MossaUtente = "" Then Main = "input vuoto": Exit Function
'VERIFICA COMANDI VALIDI-------------------------------------
'COMANDI DA WINBOARD CONNESSO A INTERNET CHESS SERVER
If Len(MossaUtente) > 5 Then
    If Left$(MossaUtente, 4) = "name" Then
        'siamo collegati ad un server ICS
        ICS = True
        'leggo il nome dell'avversario
        Avversario = MossaUtente
        i = Len(Avversario)
        Avversario = Mid$(Avversario, 6, i)
        Print #2, "testina: gioco contro " + Avversario
        Main = "OK": Exit Function
    End If
    If Left$(MossaUtente, 5) = "ratin" Then
        'leggo l'elo dell'avversario
        EloAvversario = Right$(MossaUtente, 4)
        Print #2, "testina: rating avversario " + EloAvversario
        Main = "OK": Exit Function
    End If
End If
If MossaUtente = "computer" Then
    'l'avversario e' un computer
    Computer = True
     Main = "OK": Exit Function
End If
'COMANDI DA WINBOARD------------------------------------------
If MossaUtente = "white" Then
    'tocca al bianco:engine muove con il nero
    Forced = False
    TestinaBianco = False
    Turno = 1
    Print #2, "testina:gioco con il nero"
    Main = "OK": Exit Function
End If
If MossaUtente = "black" Then
    'tocca al nero:engine muove con il bianco
    Forced = False
    TestinaBianco = True
    Turno = -1
    Print #2, "testina:gioco con il bianco"
    Main = "OK": Exit Function
End If
If Left$(MossaUtente, 5) = "setbo" Then
   'winboard richiede di impostare una posizione
   j = Len(MossaUtente)
   Stringa = Right$(MossaUtente, j - 9)
   EditBoard (Stringa)
   Print #2, "testina:scacchiera impostata"
   PrintScacchiera
   Main = "OK": Exit Function
End If
If MossaUtente = "protover 2" Then
   'invio le features richieste da Winboard
   SendCommand ("feature " + " setboard=1 done=1")
   Main = "OK": Exit Function
End If
If Left$(MossaUtente, 5) = "accep" Then
   'feature accettata
   Print #2, "testina:feature accettata"
   Main = "OK": Exit Function
End If
If MossaUtente = "nopost" Then
    PostMode = False
    Print #2, "testina:non mostro linea di gioco"
    Main = "OK": Exit Function
End If
If MossaUtente = "post" Then
    PostMode = True
    Print #2, "testina: mostro la linea di gioco"
    Main = "OK": Exit Function
End If
If MossaUtente = "force" Then
    Print #2, "testina:engine disattivato"
    Forced = True
    Main = "OK": Exit Function
End If
If MossaUtente = "go" Then
    Print #2, "testina:engine attivato"
    Forced = False
    GoTo ComputerMuove
End If
If MossaUtente = "new" Then
    NewGame
    Print #2, "testina:nuova partita impostata"
    Main = "OK": Exit Function
End If
If Len(MossaUtente) > 5 Then
    If Left$(MossaUtente, 5) = "level" Then
        Print #2, "testina:considero solo time"
        Main = "OK": Exit Function
    End If
    If Left$(MossaUtente, 4) = "time" Then
        i = Len(MossaUtente)
        MossaUtente = Mid$(MossaUtente, 5, i)
        Mps = CLng(MossaUtente) / 100
        Tempoxmossa = Mps / 40
        Print #2, "testina: muovo in" + CStr(Tempoxmossa) + " secondi"
        Main = "OK": Exit Function
    End If
    If Left$(MossaUtente, 4) = "otim" Then
        Print #2, "testina: ignoro il tempo dell'avversario"
         Main = "OK": Exit Function
    End If
    If Left$(MossaUtente, 5) = "resul" Then
        'la partita e' finita
        Print #2, "testina:fine partita " + MossaUtente
        If TestinaBianco = True Then
            PgnData.Nero = Avversario
            PgnData.EloNero = EloAvversario
            PgnData.Bianco = Ver
            PgnData.EloBianco = ""
            If Computer = True Then PgnData.Nero = PgnData.Nero + "(C)"
        Else
            PgnData.Bianco = Avversario
            PgnData.EloBianco = EloAvversario
            PgnData.Nero = Ver
            PgnData.EloNero = ""
            If Computer = True Then PgnData.Bianco = PgnData.Bianco + "(C)"
        End If
        idx = Len(MossaUtente)
        MossaUtente = LTrim(Mid$(MossaUtente, 7, idx))
        idx = Len(MossaUtente)
        For idx = 1 To idx
            If Mid$(MossaUtente, idx, 1) = " " Then
                j = idx
                Exit For
            End If
        Next idx
        MossaUtente = RTrim(Mid$(MossaUtente, 1, j))
        PgnData.Risultato = MossaUtente
        If ICS = True Then
          SalvaPgn
        End If
        Avversario = "": EloAvversario = ""
        Erase Storico()
        ContaMosse = 1
        Computer = False
        TestinaBianco = False
        PgnData = VuotoPGN
        Main = "OK": Exit Function
    End If
End If
'FINE COMANDI------------------------------------------------
If MossaLegale(MossaUtente) = False Then
    Risposta = "? " + MossaUtente
    Main = Risposta
    Exit Function
Else
    FaiMossa 1, IndiceMossa
    If InScacco(MaxPly) = True Then
        Risposta = "illeg"
        DisfaiMossa 1, IndiceMossa
        Main = Risposta
        Exit Function
    End If
    'controllo ripetizione
    j = Mossa(1, IndiceMossa).CPar
    i = Mossa(1, IndiceMossa).CArr
    If Turno = 1 Then
        RipBianco(j, i) = RipBianco(j, i) + 1
        RipB = RipBianco(j, i)
        If Mossa(1, IndiceMossa).Catturato <> 0 Then
            Erase RipBianco()
        End If
    Else
        RipNero(j, i) = RipNero(j, i) + 1
        RipN = RipNero(j, i)
        If Mossa(1, IndiceMossa).Catturato <> 0 Then
            Erase RipNero()
        End If
    End If
    If RipB > 2 And RipN > 2 Then
        Storico(ContaMosse) = Mossa(1, IndiceMossa)
        Storico(ContaMosse + 1).CArr = -100
        ContaMosse = ContaMosse + 1
        SendCommand ("1/2-1/2 {Ripetizione}")
        Main = "OK": Exit Function
    End If
    'controllo patta per regola 50 mosse
    Mosso = Mossa(1, IndiceMossa).Mosso
    Catturato = Mossa(1, IndiceMossa).Catturato
    If Mosso < 0 Then Mosso = Mosso * -1
    If Mosso <> 1 And Catturato = 0 Then
        CinquantaMosse = CinquantaMosse + 1
    Else
        CinquantaMosse = 0
    End If
    If CinquantaMosse = 100 Then
        Storico(ContaMosse) = Mossa(1, IndiceMossa)
        Storico(ContaMosse + 1).CArr = -100
        ContaMosse = ContaMosse + 1
        SendCommand ("1/2-1/2 {50 mosse}")
        Main = "OK": Exit Function
    End If
    If Forced = True Then
        'siamo in force mode:l'engine esegue le mosse sulla scacchiera
        'senza attivarsi
        Turno = Turno * -1
        Main = "OK": Exit Function
    End If
    Storico(ContaMosse) = Mossa(1, IndiceMossa)
    Storico(ContaMosse + 1).CArr = -100
    ContaMosse = ContaMosse + 1
End If
Turno = Turno * -1
Stadio = Checkposizione
'Stadio = 1 Apertura
'Stadio = 2 Mediogioco
'Stadio = 3 Finale
ComputerMuove:
Nodi = 0
'RICERCA-----------------------------------------------------
'setto il tempo della ricerca
StartTime = Timer
Durata = Tempo
PVidx = 1
Alfa = -WINDOW
Beta = WINDOW
Erase History()
Erase Killer()
For i = 1 To MaxPly: KillerScore(i) = -Infinito: Next i
If Turno = 1 Then
    TestinaBianco = True
Else
    TestinaBianco = False
End If
If BookDisponibile = True Then
   'cerco mossa nel book
   j = BookMove
   If j > 0 Then
     FaiMossa 50, j
     Storico(ContaMosse) = Mossa(50, j)
     Storico(ContaMosse + 1).CArr = -100
     ContaMosse = ContaMosse + 1
     SendCommand "0 0 0 0 (Book.)"
     Risposta = Mossa(50, j).Algebrica
     Print #2, "testina: mossa book " + Mossa(50, j).Algebrica
     PrintScacchiera
     Turno = Turno * -1
     Main = Risposta: Exit Function
   End If
End If
UnaSolaMossa = False
For SearchPly = Profo To MaxPly - 1 'INIZIO RICERCA ITERATIVA------
NullOk = True
Erase PV()
Best = AlfaBeta(SearchPly, Alfa, Beta)
'Aspiration Window
If (Best >= Beta) And (Best <> Infinito) Then
   'fallisce l'upper bound
   Alfa = -Best - WINDOW
   Beta = Infinito
   GoTo Ordinamento
Else
   If (Best <= Alfa) And (Best <> -Infinito) Then
      'fallisce lower bound
      Beta = Best + WINDOW
      Alfa = -Infinito
      GoTo Ordinamento
   End If
End If
idx = Best
If idx < 0 Then idx = idx * -1
If idx = Infinito Then GoTo Ordinamento
If Alfa = -Infinito Then Alfa = -idx
If Beta = Infinito Then Beta = idx
Alfa = Alfa - idx: Beta = Beta + idx
'scrivo sul file per debug il ply e i nodi visitati
'Print #2, "prof." + CStr(SearchPly) + " nodi " + CStr(Nodi)
'se ho solo una mossa disponibile la gioco subito
Ordinamento:
Ordinamosse (SearchPly): 'ordino mosse al ply corrente
If UnaSolaMossa = True Then Exit For

'>>>funzione di debug-disabilitarla con un commento
'ReportMoves (SearchPly)
'mostro la variante principale e la uso per il
'livello di analisi successivo
   Stringa = CStr(SearchPly) + " "
   Stringa = Stringa + CStr(Best) + " "
   Stringa = Stringa + CStr(Int(Timer + SECONDI - StartTime)) + " "
   Stringa = Stringa + CStr(Nodi) + " "
       idx = SearchPly
       For j = 1 To PVLength(1) - 1
         Stringa = Stringa + PV(1, j).Algebrica + " "
         Variante(idx) = PV(1, j)
         idx = idx - 1
         If idx = 0 Then idx = 40
       Next j
       If PostMode = True Then SendCommand (Stringa)
       
'ricopio la lista di mosse della profondita' corrente al ply successivo
Indice = 1
Do
    Mossa(SearchPly + 1, Indice) = Mossa(SearchPly, Indice)
    Indice = Indice + 1
Loop Until Mossa(SearchPly, Indice).CPar = -100
DoEvents
Mossa(SearchPly + 1, Indice) = Mossa(SearchPly, Indice) 'fine mosse

If TimeCheck = True Then Exit For

ReRicerca:
If (SearchPly > 2) And ((Best = Infinito) Or (Best = -Infinito)) Then Exit For
Next SearchPly
'FINE RICERCA--------------------------------------------------

'la mossa da giocare e' la prima
Indice = 1
Do
    Risposta = Mossa(SearchPly, Indice).Algebrica
    If Mossa(SearchPly, Indice).Illegale = True Then
        Risposta = ""
    Else
        Exit Do
    End If
    Indice = Indice + 1
Loop Until Mossa(SearchPly, Indice).CPar = -100
'se non ci sono mosse da giocare forse la partita e' finita
If Risposta = "" Then
    If Best = -Infinito Then
        If InScacco(50) = False Then
           SendCommand ("1/2-1/2 {Stallo}")
           Main = "OK": Exit Function
        Else
           If Turno = 1 Then
             SendCommand ("0-1 {Matto}")
             Main = "OK": Exit Function
           Else
             SendCommand ("1-0 {Matto}")
             Main = "OK": Exit Function
           End If
        End If
    Else
        SendCommand ("1/2-1/2 {Stallo}")
        Main = "OK": Exit Function
    End If
    Print #2, "testina: fine partita"
    Main = "OK": Exit Function
End If
'gioco la mossa
FaiMossa SearchPly, Indice
    'controllo ripetizione
    j = Mossa(SearchPly, Indice).CPar
    i = Mossa(SearchPly, Indice).CArr
    If Turno = 1 Then
        RipBianco(j, i) = RipBianco(j, i) + 1
        RipB = RipBianco(j, i)
        If Mossa(SearchPly, Indice).Catturato <> 0 Then
            Erase RipBianco()
        End If
    Else
        RipNero(j, i) = RipNero(j, i) + 1
        RipN = RipNero(j, i)
        If Mossa(SearchPly, Indice).Catturato <> 0 Then
            Erase RipNero()
        End If
    End If
    If RipB > 2 And RipN > 2 Then
        SendCommand ("move " + Risposta)
        Storico(ContaMosse) = Mossa(SearchPly, Indice)
        Storico(ContaMosse + 1).CArr = -100
        ContaMosse = ContaMosse + 1
        SendCommand ("1/2-1/2 {Ripetizione}")
        Main = "OK": Exit Function
    End If
    'controllo patta per regola 50 mosse
    Mosso = Mossa(SearchPly, Indice).Mosso
    Catturato = Mossa(SearchPly, Indice).Catturato
    If Mosso < 0 Then Mosso = Mosso * -1
    If Mosso <> 1 And Catturato = 0 Then
        CinquantaMosse = CinquantaMosse + 1
    Else
        CinquantaMosse = 0
    End If
    If CinquantaMosse = 100 Then
        SendCommand ("move " + Risposta)
        Storico(ContaMosse) = Mossa(SearchPly, Indice)
        Storico(ContaMosse + 1).CArr = -100
        ContaMosse = ContaMosse + 1
        SendCommand ("1/2-1/2 {50 mosse}")
        Main = "OK": Exit Function
    End If
'se il punteggio e' uguale a +Infinito e Searchply=3 allora
'testina ha dato scaccomatto
If Best = Infinito And SearchPly <= 3 Then
   If DoScacco(50) = True Then 'per sicurezza ma non e' necessario
      If Turno = 1 Then
        SendCommand ("move " + Risposta)
        Storico(ContaMosse) = Mossa(SearchPly, Indice)
        Storico(ContaMosse + 1).CArr = -100
        ContaMosse = ContaMosse + 1
        SendCommand ("1-0 {Matto}")
        Main = "OK": Exit Function
      Else
        SendCommand ("move " + Risposta)
        Storico(ContaMosse) = Mossa(SearchPly, Indice)
        Storico(ContaMosse + 1).CArr = -100
        ContaMosse = ContaMosse + 1
        SendCommand ("0-1 {Matto}")
        Main = "OK": Exit Function
      End If
   End If
End If
Print #2, "testina: " + Risposta
'questa funzione di debug scrive la scacchiera
PrintScacchiera
'scrivo su file LOG la mossa,profondita' e nodi
Print #2, " Nodi " + CStr(Nodi) + " Prof. " + CStr(SearchPly) _
      + " Punti " + CStr(Best)
Storico(ContaMosse) = Mossa(SearchPly, Indice)
Storico(ContaMosse + 1).CArr = -100
ContaMosse = ContaMosse + 1
Uscita:
Turno = Turno * -1
Main = Risposta
End Function
Function MossaLegale(MossaUtente As String) As Boolean
'Determina se la mossa inserita dall'utente è legale
Dim Indice As Integer, Trovata As Boolean

Ply = 1
Trovata = False
Indice = 0
GeneraMosse (Ply)

'cerco nella lista delle mosse se esiste la mossa utente
Do
    Indice = Indice + 1
    If Mossa(Ply, Indice).Algebrica = MossaUtente Then
        Trovata = True
        IndiceMossa = Indice
        Exit Do
    End If
Loop Until Mossa(Ply, Indice).CPar = -100

MossaLegale = Trovata
End Function





Sub NewGame()
'legge i dati negli array ed imposta la scacchiera alla
'posizione iniziale

Dim x As Integer, Y As Integer
Dim pos As Integer

Fill64 Board, -7, -2, -3, -5, -6, -3, -2, -4, _
             -1, -1, -1, -1, -1, -1, -1, -1, _
             0, 0, 0, 0, 0, 0, 0, 0, _
             0, 0, 0, 0, 0, 0, 0, 0, _
             0, 0, 0, 0, 0, 0, 0, 0, _
             0, 0, 0, 0, 0, 0, 0, 0, _
             1, 1, 1, 1, 1, 1, 1, 1, _
             7, 2, 3, 5, 6, 3, 2, 4


Turno = 1: ' tocca al bianco
For x = 1 To 2
    ARROCCO(x).Re = 0
    ARROCCO(x).Torre4 = 0
    ARROCCO(x).Torre7 = 0
Next x
'reset variabili
Erase Mossa
Erase ARROCCO
Erase Storico
UltimaMossa = NullMove
BookDisponibile = True
Forced = False
CinquantaMosse = 0
PosReB = 61: PosReN = 5 'posizione dei Re
End Sub


Sub OrdinamentoTattico(Ply As Integer, Counter As Integer)
'ordinamento mosse per incrementare la velocita di ricerca

Dim i As Integer, Conto As Integer, idx As Integer
Dim Preso As Integer, Catturante As Integer, j As Integer

If Ply >= 49 Then Exit Sub 'ply 50 e 49 usati per utilita'

        i = 1
        Do
        Select Case Mossa(Ply, i).Mosso
            Case Is = 1
                Mossa(Ply, i).Punteggio = Pedoni(Mossa(Ply, i).CArr) - _
                    Pedoni(Mossa(Ply, i).CPar)
            Case 2
                Mossa(Ply, i).Punteggio = Cavalli(Mossa(Ply, i).CArr) - _
                    Cavalli(Mossa(Ply, i).CPar)
            Case 3
                Mossa(Ply, i).Punteggio = Alfieri(Mossa(Ply, i).CArr) - _
                    Alfieri(Mossa(Ply, i).CPar)
            Case 4
                'per la torre uso la tabella del cavallo
                Mossa(Ply, i).Punteggio = Cavalli(Mossa(Ply, i).CArr) - _
                    Cavalli(Mossa(Ply, i).CPar)
            Case 5
                'per la donna uso la tabella del cavallo
                Mossa(Ply, i).Punteggio = Cavalli(Mossa(Ply, i).CArr) - _
                    Cavalli(Mossa(Ply, i).CPar)
            Case 6
                If Stadio = 1 Or Stadio = 2 Then
                   Mossa(Ply, i).Punteggio = ReApe(Mossa(Ply, i).CArr) - _
                   ReApe(Mossa(Ply, i).CPar)
                Else
                   Mossa(Ply, i).Punteggio = ReFinale(Mossa(Ply, i).CArr) - _
                   ReFinale(Mossa(Ply, i).CPar)
                End If
            Case 7
                'per la torre uso la tabella del cavallo
                Mossa(Ply, i).Punteggio = Cavalli(Mossa(Ply, i).CArr) - _
                    Cavalli(Mossa(Ply, i).CPar)
            Case Is = -1
                Mossa(Ply, i).Punteggio = Pedoni(Flip(Mossa(Ply, i).CArr)) - _
                    Pedoni(Flip(Mossa(Ply, i).CPar))
            Case -2
                Mossa(Ply, i).Punteggio = Cavalli(Flip(Mossa(Ply, i).CArr)) - _
                    Cavalli(Flip(Mossa(Ply, i).CPar))
            Case -3
                Mossa(Ply, i).Punteggio = Alfieri(Flip(Mossa(Ply, i).CArr)) - _
                    Alfieri(Flip(Mossa(Ply, i).CPar))
            Case -4
                'per la torre uso la tabella del cavallo
                Mossa(Ply, i).Punteggio = Cavalli(Flip(Mossa(Ply, i).CArr)) - _
                    Cavalli(Flip(Mossa(Ply, i).CPar))
            Case -5
                'per la donna uso la tabella del cavallo
                Mossa(Ply, i).Punteggio = Cavalli(Flip(Mossa(Ply, i).CArr)) - _
                    Cavalli(Flip(Mossa(Ply, i).CPar))
            Case -6
                If Stadio = 1 Or Stadio = 2 Then
                   Mossa(Ply, i).Punteggio = ReApe(Flip(Mossa(Ply, i).CArr)) - _
                   ReApe(Flip(Mossa(Ply, i).CPar))
                Else
                   Mossa(Ply, i).Punteggio = ReFinale(Flip(Mossa(Ply, i).CArr)) - _
                   ReFinale(Flip(Mossa(Ply, i).CPar))
                End If
            Case -7
                'per la torre uso la tabella del cavallo
                Mossa(Ply, i).Punteggio = Cavalli(Flip(Mossa(Ply, i).CArr)) - _
                    Cavalli(Flip(Mossa(Ply, i).CPar))
        End Select
        Conto = Conto + 1
        i = i + 1
    Loop Until Mossa(Ply, i).CPar = -100
    
    'fase 2 - analizzo le mosse
    For i = 1 To Conto
        If Mossa(Ply, i).Catturato <> 0 Then
           'se la mossa generata e' una cattura si implementa
           'un meccanismo MVV/LVA:il valore del pezzo
           'catturato si sottrae al catturante.
           'Pedone (1) cattura Donna(5) 5-1=4
           'Donna(5) cattura Cavallo(2) 2-5=-3
           'Alfiere(3) cattura Torre(4) 4-3=1
           Preso = Mossa(Ply, i).Catturato
           Catturante = Mossa(Ply, i).Mosso
           If Preso > 10 Then Preso = 1 'cattura enpassant
           If Preso < 0 Then Preso = Preso * -1
           If Catturante < 0 Then Catturante = Catturante * -1
           If Preso = 7 Then Preso = 4
           If Catturante = 7 Then Catturante = 4
           'condizioni catture
           If Catturante < Preso Then 'cattura vincente
               Mossa(Ply, i).Punteggio = _
                             Mossa(Ply, i).Punteggio + 100
           End If
           If Catturante = Preso Then 'cattura normale
               Mossa(Ply, i).Punteggio = _
                             Mossa(Ply, i).Punteggio + 90
           End If
           If Catturante > Preso Then 'cattura da valutare
               Mossa(Ply, i).Punteggio = _
                             Mossa(Ply, i).Punteggio + 80
           End If
         End If
         'se la mossa e' nella variante principale deve essere
         'la prima in ogni caso quindi punti=10000
         If SearchPly >= Ply Then
           idx = (SearchPly - Ply) + 1
         Else
           idx = (40 - Ply) + 1 + SearchPly
         End If
         If Mossa(Ply, i).Algebrica = Variante(idx).Algebrica Then
            Mossa(Ply, i).Punteggio = Mossa(Ply, i).Punteggio + 10000
         End If
         'se la mossa e' nella history table deve essere
         'accreditata di 1 punto per ogni volta che e'
         'stata giocata e accettata in AlfaBeta
         j = Mossa(Ply, i).CPar: idx = Mossa(Ply, i).CArr
         Mossa(Ply, i).Punteggio = Mossa(Ply, i).Punteggio + History(j, idx)
         'se la mossa e' una mossa killer viene dato un bonus
         If Killer(1, Ply).CPar = Mossa(Ply, i).CPar Then
            If Killer(1, Ply).CArr = Mossa(Ply, i).CArr Then
               If Killer(1, Ply).Mosso = Mossa(Ply, i).Mosso Then
                  Mossa(Ply, i).Punteggio = Mossa(Ply, i).Punteggio + 1000
               End If
            End If
         End If
         If Killer(2, Ply).CPar = Mossa(Ply, i).CPar Then
            If Killer(2, Ply).CArr = Mossa(Ply, i).CArr Then
               If Killer(2, Ply).Mosso = Mossa(Ply, i).Mosso Then
                  Mossa(Ply, i).Punteggio = Mossa(Ply, i).Punteggio + 500
               End If
            End If
         End If
         'verifico mosse speciali
         Select Case Mossa(Ply, i).Speciale
            Case 1
                'arrocco
                Mossa(Ply, i).Punteggio = Mossa(Ply, i).Punteggio + 50
            Case 2
                'promozione
                Mossa(Ply, i).Punteggio = Mossa(Ply, i).Punteggio + 400
            Case 3
                'en passant
                Mossa(Ply, i).Punteggio = Mossa(Ply, i).Punteggio + 100
         End Select
    Next i
    
    Ordinamosse Ply, Conto
  
End Sub

Sub Ordinamosse(Ply As Integer, Optional Conto As Integer)
'ordina le mosse al ply 1

Dim k As Integer, Best As Integer, j As Integer
Dim temp As MoveType
    
If Conto = 0 Then
'se conto=0 dobbiamo contare le mosse
j = 1
Do
    Conto = Conto + 1
    j = j + 1
Loop Until Mossa(Ply, j).Algebrica = "finemosse"
End If

   
 For k = 1 To Conto - 1
    Best = k
    For j = k + 1 To Conto
        If Mossa(Ply, j).Punteggio > Mossa(Ply, Best).Punteggio Then
           Best = j
        End If
    Next j
        'scambio (k) con (small)
        If Mossa(Ply, k).Punteggio <> Mossa(Ply, Best).Punteggio Then
           temp = Mossa(Ply, k)
           Mossa(Ply, k) = Mossa(Ply, Best)
           Mossa(Ply, Best) = temp
        End If
 Next k
   
End Sub


Sub PrintMoves()
'routine di debug:mostra le mosse possibili per il computer
Dim i As Integer

i = 1
Do
    Print #2, Mossa(1, i).Algebrica; " ";
    i = i + 1
Loop Until Mossa(1, i).CPar = -100
DoEvents
End Sub

Sub PrintScacchiera()
'scrive la scacchiera su file:necessaria per debug
'con winboard
Dim i As Integer, c As Integer
c = 0
For i = 1 To 64
    c = c + 1
    Select Case Board(i)
        Case 1
            Print #2, " P ";
        Case 2
            Print #2, " C ";
        Case 3
            Print #2, " A ";
        Case 4, 7
            Print #2, " T ";
        Case 5
            Print #2, " D ";
        Case 6
            Print #2, " R ";
        Case -1
            Print #2, " p ";
        Case -2
            Print #2, " c ";
        Case -3
            Print #2, " a ";
        Case -4, -7
            Print #2, " t ";
        Case -5
            Print #2, " d ";
        Case -6
            Print #2, " r ";
        Case 0
            Print #2, " . ";
        Case 100
            Print #2, " X ";
    End Select
    If c = 8 Then
        Print #2, "  "
        c = 0
    End If
Next i
Print #2, " "
End Sub

Function Quies(ByVal Alfa As Integer, ByVal Beta As Integer) As Integer
Dim Score As Integer, Indice As Integer, j As Integer
Dim Preso As Integer, Mosso As Integer, p As Integer

Score = Eval

'metto un limite alla quiescenza
If Ply = Limite Then
    Quies = Score - CTorre
    Exit Function
End If
Ply = Ply - 1
DoEvents
If (Score >= Beta) Then
   Quies = Beta
   Ply = Ply + 1
   Exit Function
End If
If (Score > Alfa) Then Alfa = Score
GeneraMosse (Ply)

Indice = 1
PVLength(PVidx) = PVidx

Do
  If Mossa(Ply, Indice).Algebrica = "finemosse" Then Exit Do
  Preso = Mossa(Ply, Indice).Catturato
  Mosso = Mossa(Ply, Indice).Mosso
  If Preso > 10 Then Preso = 1
  If Preso <> 0 Then
     FaiMossa Ply, Indice
     PVidx = PVidx + 1
     Turno = Turno * -1
     Nodi = Nodi + 1
     Score = -Quies(-Beta, -Alfa)
     Turno = Turno * -1
     DisfaiMossa Ply, Indice
     PVidx = PVidx - 1
Scoring:
     If Score >= Beta Then
        Quies = Beta
        Ply = Ply + 1
        Exit Function
     End If
     If (Score > Alfa) Then
        Alfa = Score
            'impostiamo la PV
            PV(PVidx, PVidx) = Mossa(Ply, Indice)
            For j = PVidx + 1 To PVLength(PVidx + 1) - 1
                PV(PVidx, j) = PV(PVidx + 1, j)
            Next j
            PVLength(PVidx) = PVLength(PVidx + 1)
            'aggiorno la History Table
            p = Mossa(Ply, Indice).CPar
            j = Mossa(Ply, Indice).CArr
            History(p, j) = History(p, j) + ((40 - Ply) + 1 + SearchPly)
            '....e visto che abbiamo un cut-off impostiamo la mossa killer
            If Score > KillerScore(Ply) Then
              KillerScore(Ply) = Score
              Killer(2, Ply) = Killer(1, Ply)
              Killer(1, Ply) = Mossa(Ply, Indice)
            End If
     End If
  End If
NuovaMossa:
  Indice = Indice + 1
Loop Until Mossa(Ply, Indice).CPar = -100

Quies = Alfa
Ply = Ply + 1
End Function


Function InScacco(ByVal Ply As Integer) As Boolean
' determino se la parte che deve muovere
'è sotto scacco:genero le mosse dell'avversario e
'controllo se c'e' una mossa che cattura il re
Dim idx As Integer

Turno = Turno * -1 'passo all'avversario
GeneraMosse (Ply)
idx = 1
Do
    If Mossa(Ply, idx).Catturato = 6 * (-Turno) Then
        InScacco = True
        Turno = Turno * -1 'rispristino il turno
        Exit Function
    End If
    idx = idx + 1
Loop Until Mossa(Ply, idx).CPar = -100
InScacco = False
Turno = Turno * -1 'ripristino il turno
End Function


Sub ReportMoves(Ply As Integer)
'questa e' una routine solo per debug
'scrive sul file log la lista delle mosse ed il punteggio
'serve a monitorare l'effettivo ordinamento delle mosse
Dim Indice As Byte, MossePerLinea As Byte
Indice = 1
MossePerLinea = 0
Do
    Print #2, Mossa(Ply, Indice).Algebrica + " " + CStr(Mossa(Ply, Indice).Punteggio) + "/ ";
    MossePerLinea = MossePerLinea + 1
    Indice = Indice + 1
    If MossePerLinea = 5 Then
       MossePerLinea = 0
       Print #2, " "
    End If
Loop Until Mossa(Ply, Indice).CPar = -100
Print #2, " "
Print #2, "---fine rapporto prof." + CStr(Ply) + "---"
End Sub

Sub RipristinaReps(ByVal Ply As Integer, ByVal Indice As Integer)
'siccome gli indici della ripetizione durante la ricerca sono
'stati modificati bisogna rimetterli a posto
Dim j As Byte, i As Byte, Pezzo As Integer

Pezzo = Mossa(Ply, Indice).Mosso
If Pezzo = 1 Or Pezzo = -1 Then Exit Sub
If Mossa(Ply, Indice).Catturato <> 0 Then Exit Sub

    j = Mossa(Ply, Indice).CPar
    i = Mossa(Ply, Indice).CArr
    If Turno = 1 Then
        RipBianco(j, i) = RipBianco(j, i) - 1
        RipB = RipBianco(j, i)
    Else
        RipNero(j, i) = RipNero(j, i) - 1
        RipN = RipNero(j, i)
    End If

End Sub

Sub StartBook()
'questa routine viene attivata se bookmode=true
'verifica la presenza del book-file e in caso contrario crea un
'nuovo file Book.Dat
Dim pos As String, i As Byte
NewGame
BookPos = ConvBoard

On Error GoTo Inesistente
Open "Book.dat" For Input As #3
Close #3
'tutto ok posso aprire il file in modalita' random
Exit Sub
Inesistente:
Close #3
Open "Book.dat" For Random As #3 Len = Len(Book)
'siccome il book e' stato appena creato bisogna memorizzare
'la posizione di partenza
Book.pos = BookPos
Book.Mossa = String(50, "0")
Put #3, 1, Book
Close #3
End Sub


Function TimeCheck() As Boolean
'verifica che non sia stato passato il limite di tempo
'altrimenti testina rischia di perdere per il tempo

TimeCheck = False
If (Timer + SECONDI - StartTime) Mod SECONDI >= Durata Then TimeCheck = True
If SearchPly > 2 Then
   If Durata / 3 > (Timer + SECONDI - StartTime) Mod SECONDI Then
      TimeCheck = False
   Else
      TimeCheck = True
   End If
End If
End Function

Sub VerificaMossaBook(idx As Integer)
'se la mossa puntata da idx ha gia' un punteggio allora la
'mossa viene effettuata sulla scacchiera e la posizione
'aggiornata.Altrimenti prima chiede una valutazione della mossa
Dim Valore As Integer, i As Long, Scelta As Byte
Dim Registrato As Boolean

Registrato = False
i = 1
Open "Book.dat" For Random As #3 Len = Len(Book)
Do
    Get #3, i, Book
    If Book.pos = BookPos Then
       Valore = CInt(Mid$(Book.Mossa, idx, 1))
       If Valore = 0 Then
          Form2.Visible = True
          Scelta = Form2.Modal
          Form2.Visible = False
          'associo il valore alla mossa
          Mid$(Book.Mossa, idx, 1) = CStr(Scelta)
          'riscrivo la posizione
          Put #3, i, Book
          Registrato = True
          Valore = Scelta
       End If
       If Valore <> 0 Then
          'la mossa e' gia' presente:la gioco sulla scacchiera
          FaiMossa 49, idx
          'aggiorno la posizione
          BookPos = ConvBoard
          Turno = Turno * -1
          Close #3
          Exit Sub
       End If
    End If
    i = i + 1
Loop Until EOF(3)
If Registrato = False Then
    'la posizione e' nuova e deve essere registrata
    Form2.Visible = True
    Scelta = Form2.Modal
    Form2.Visible = False
    Book.pos = BookPos
    Book.Mossa = String(50, "0")
    'associo il valore alla mossa
    Mid$(Book.Mossa, idx, 1) = CStr(Scelta)
    'riscrivo la posizione
    Put #3, , Book
    FaiMossa 49, idx
    'aggiorno la posizione
    BookPos = ConvBoard
    Turno = Turno * -1
End If
Close #3
End Sub

Sub WinBoardLoop()
'interfaccia winboard
NewGame
Termina = False
ICS = False
PostMode = False
Forced = False
Form1.Visible = False
IOBas.Main
End Sub


