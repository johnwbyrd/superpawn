Attribute VB_Name = "IOBas"
Option Explicit

'==================================================
'IOBas:
'
'Modulo di comunicazione con Winboard
'==================================================

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

Public hStdIn         As Long   ' handle dello Standard Input
Public hStdOut        As Long   ' handle dello Standard Output
'---------------------------------------------------------------------------
'PollCommand() - Verifica la presenza di dati nel buffer dello standard input
'
'restituisce TRUE se sono disponibili nuovi dati
'---------------------------------------------------------------------------
Function PollCommand() As Boolean

#If DEBUGMODE Then
    PollCommand = FakeInputState
#Else
    Dim sBuff       As String
    Dim lBytesRead  As Long
    Dim lTotalBytes As Long
    Dim lAvailBytes As Long
    Dim rc          As Long
    
    sBuff = String(2048, Chr$(0))
    rc = PeekNamedPipe(hStdIn, ByVal sBuff, 2048, lBytesRead, lTotalBytes, lAvailBytes)
    
    PollCommand = CBool(rc And lBytesRead > 0)
#End If

End Function
'---------------------------------------------------------------------------
'ReadCommand() - Legge una stringa dallo standard input
'---------------------------------------------------------------------------
Function ReadCommand() As String

    Dim sBuff      As String
    Dim lBytesRead As Long
    Dim rc         As Long
    
    sBuff = String(2048, Chr$(0))
    rc = ReadFile(hStdIn, ByVal sBuff, 2048, lBytesRead, ByVal 0&)
    'ToDo: gestione degli errori se rc=0
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
    Dim rc              As Long
    
    'secondo la documentazione di WinBoard il LineFeed PRIMA del comando
    'non dovrebbe essere inviato, pero' senza quello WinBoard legge solo
    'la prima stringa che mandiamo e poi cessa di rispondere.
    'Probabilmente c'e' qualcosa che mi sfugge nella documentazione oppure c'e'
    'qualche "inghippo" con WriteFile e la gestione delle stringhe in VB;
    'comunque questa "patch" fa funzionare il tutto e sembra non infastidire WinBoard.
    
    sCommand = vbLf & sCommand & vbLf
    
    lBytes = Len(sCommand)
    
    rc = WriteFile(hStdOut, ByVal sCommand, lBytes, lBytesWritten, ByVal 0&)
    'ToDo: gestione degli errori se rc=0
SendCommand = sCommand

End Function
