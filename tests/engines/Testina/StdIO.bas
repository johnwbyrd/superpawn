Attribute VB_Name = "IOBas"
Option Explicit

'====================================================
' MODULO DI COMUNICAZIONE WINBOARD <---> VISUAL BASIC
' by Luca Dormio
'====================================================

'----------------------------------------------------
' Dichiarazioni API
'----------------------------------------------------
Declare Function GetStdHandle Lib "kernel32" _
    (ByVal nStdHandle As Long) As Long
Declare Function CloseHandle Lib "kernel32" _
    (ByVal hObject As Long) As Long
Declare Function PeekNamedPipe Lib "kernel32" _
    (ByVal hNamedPipe As Long, lpBuffer As Any, _
    ByVal nBufferSize As Long, lpBytesRead As Long, _
    lpTotalBytesAvail As Long, lpBytesLeftThisMessage As Long) As Long
Declare Function ReadFile Lib "kernel32" _
    (ByVal hFile As Long, _
    lpBuffer As Any, _
    ByVal nNumberOfBytesToRead As Long, _
    lpNumberOfBytesRead As Long, _
    lpOverlapped As Any) As Long
Declare Function WriteFile Lib "kernel32" _
    (ByVal hFile As Long, _
    ByVal lpBuffer As String, _
    ByVal nNumberOfBytesToWrite As Long, _
    lpNumberOfBytesWritten As Long, _
    lpOverlapped As Any) As Long
Public Const STD_INPUT_HANDLE = -10&
Public Const STD_OUTPUT_HANDLE = -11&
'----------------------------------------------------
' Fine Dichiarazioni API
'----------------------------------------------------

Public hStdIn         As Long   ' var. globale con l'handle dello Standard Input attivo
Public hStdOut        As Long   ' var. globale con l'handle dello Standard Output attivo
Sub Main()

Dim sInput As String

'Apriamo i canali dello standard input/output in avvio di programma,
'all'uscita dell'applicazione andranno chiusi con:
'
'CloseHandle hStdIn
'CloseHandle hStdOut

hStdIn = GetStdHandle(STD_INPUT_HANDLE)
hStdOut = GetStdHandle(STD_OUTPUT_HANDLE)

'Versione 1 - con il polling dello standard input
'Do
'    If PollCommand Then
'        sInput = ReadCommand
'
'        'a questo punto sInput contiene la stringa inviataci da Winboard;
'        'possiamo iniziare il parsing dei comandi ecc...
'        Debug.Print sInput
'    End If
'    DoEvents
'Loop
Open Ver + ".log" For Output As #2

'Versione 2 - senza polling
Do
    'Il processo rimane bloccato fintanto che non vengono
    'ricevuti dati sullo standard input.
    sInput = ReadCommand
    'a questo punto sInput contiene la stringa inviataci da Winboard;
    'possiamo iniziare il parsing dei comandi ecc...
    DoEvents
    ParseComando (sInput)
    If Termina = True Then Exit Do
Loop
CloseHandle hStdIn
CloseHandle hStdOut
Print #2, "testina terminato correttamente"
Close #2

End Sub
Sub ParseComando(Comando As String)
'effettua il parsing del comando ricevuto da winboard
Dim i As Integer, Stringa As String, Risposta As String
Do
    'estraggo un comando alla volta
    Stringa = ""
    For i = 1 To Len(Comando)
        If Asc(Mid$(Comando, i, 1)) > 13 Then
            Stringa = Stringa + Mid$(Comando, i, 1)
        Else
            Comando = Mid$(Comando, i + 1, Len(Comando))
            If Comando = Chr$(13) + Chr$(10) Then Comando = ""
            Exit For
        End If
    Next i
    DoEvents
    'invio il comando al programma
    If Stringa = "" Then Exit Do
    Print #2, "winboard :" + Stringa
    Form1.Text1.Text = Stringa + vbCrLf
    If Stringa = "quit" Then
        Termina = True
        Exit Sub
    End If
    If Tempoxmossa = 0 Then Tempoxmossa = 5
    Risposta = Scacchi.Main(Stringa, Tempoxmossa)
    'invio la risposta a winboard
    Select Case Risposta
        Case "OK"
            Form1.Text1.Text = "regolazione ok" + vbCrLf
        Case "input vuoto"
            'non fare niente
        Case Else
            If Left$(Risposta, 1) = "?" Then
               Print #2, "testina: comando sconosciuto " + Risposta
               GoTo Uscita_Select
            End If
            SendCommand ("move " & Risposta)
Uscita_Select:
    End Select
    If Comando = "" Then Exit Do
AltroLoop:
Loop
DoEvents
End Sub

'---------------------------------------------------------------------------
'PollCommand() - Verifica la presenza di dati nel buffer dello standard input
'
'restituisce TRUE se sono disponibili nuovi dati
'---------------------------------------------------------------------------
Function PollCommand() As Boolean

Dim sBuff       As String
Dim lBytesRead  As Long, lTotalBytes As Long, lAvailBytes As Long
Dim ReturnCode  As Long

sBuff = String(256, Chr$(0))
ReturnCode = PeekNamedPipe(hStdIn, ByVal sBuff, 256, lBytesRead, lTotalBytes, lAvailBytes)

PollCommand = CBool(ReturnCode And lBytesRead > 0)

End Function
'---------------------------------------------------------------------------
'ReadCommand() - Legge una stringa dallo standard input
'---------------------------------------------------------------------------
Function ReadCommand() As String

Dim sBuff       As String
Dim lBytesRead  As Long
Dim ReturnCode  As Long

sBuff = String(256, Chr$(0))
ReturnCode = ReadFile(hStdIn, ByVal sBuff, 256, lBytesRead, ByVal 0&)

'Da fare: eventuale gestione degli errori se ReturnCode=0
ReadCommand = Left$(sBuff, lBytesRead)

End Function
'---------------------------------------------------------------------------
'SendCommand() - Invia una stringa sullo standard output
'
'restituisce la stringa effettivamente inviata (compresi i LineFeeds)
'---------------------------------------------------------------------------
Function SendCommand(ByVal sCommand As String) As String

Dim lBytesWritten   As Long
Dim lBytes          As Long
Dim ReturnCode      As Long

'secondo la documentazione di WinBoard il LineFeed PRIMA del comando
'non dovrebbe essere inviato, pero' senza quello WinBoard legge solo
'la prima stringa che mandiamo e poi cessa di rispondere.
'Probabilmente c'e' qualcosa che mi sfugge nella documentazione oppure c'e'
'qualche inghippo con WriteFile e la gestione delle stringhe in VB;
'comunque questa "patch" fa funzionare il tutto e sembra non infastidire WinBoard.
sCommand = vbLf & sCommand & vbLf
lBytes = LenB(sCommand)

ReturnCode = WriteFile(hStdOut, ByVal sCommand, lBytes, lBytesWritten, ByVal 0&)

'Da fare: eventuale gestione degli errori se ReturnCode=0
SendCommand = sCommand

End Function
