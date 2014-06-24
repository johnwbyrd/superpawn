Attribute VB_Name = "PGN"
'il modulo PGN viene chiamato alla fine della partita per registrarla su disco
'in formato PGN.Introdotta nella versione 1.1 questo modulo e' fondamentale
'per giocare sull'Internet Chess Server

Type PGNType
    Evento As String
    Luogo As String
    Data As String
    Round As String
    Bianco As String
    Nero As String
    Risultato As String
    TimeControl As String
    EloBianco As String
    EloNero As String
End Type


Public PgnData As PGNType
Public VuotoPGN As PGNType

Public Avversario As String
Public EloAvversario As String
Public Computer As Boolean
Sub SalvaPgn()
'salva la partita in formato PGN
Dim MoveNo As Integer, p As Integer, i As Integer
Dim Nomefile As String, Stringa As String

Stringa = CStr(Trim(Now))
Nomefile = ""
p = Len(Stringa)
For i = 1 To p
    If Asc(Mid$(Stringa, i, 1)) > 47 And Asc(Mid$(Stringa, i, 1)) < 58 Then
        Nomefile = Nomefile + Mid$(Stringa, i, 1)
    End If
Next i
PgnData.Data = Date

Open Nomefile + ".pgn" For Output As #1
Print #1, "[Site """ + PgnData.Luogo + """]"
Print #1, "[Date """ + PgnData.Data + """]"
Print #1, "[Round ""-""]"
Print #1, "[White """ + PgnData.Bianco + """]"
Print #1, "[WhiteElo """ + PgnData.EloBianco + """]"
Print #1, "[Black """ + PgnData.Nero + """]"
Print #1, "[BlackElo """ + PgnData.EloNero + """]"
Print #1, "[Result """ + PgnData.Risultato + """]"
p = 1: MoveNo = 1
Do
    Print #1, Str(MoveNo) + "." + " ";
    Print #1, Storico(p).Algebrica + " ";
    p = p + 1
    If Storico(p).CArr = -100 Then Exit Do
    Print #1, Storico(p).Algebrica + " ";
    p = p + 1
    MoveNo = MoveNo + 1
Loop Until Storico(p).CArr = -100
Close #1
End Sub

