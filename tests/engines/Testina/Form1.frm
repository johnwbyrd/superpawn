VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Book Editor"
   ClientHeight    =   4395
   ClientLeft      =   165
   ClientTop       =   735
   ClientWidth     =   5670
   Icon            =   "Form1.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   4395
   ScaleWidth      =   5670
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   ">>>"
      Height          =   615
      Left            =   1680
      TabIndex        =   2
      Top             =   1800
      Width           =   495
   End
   Begin VB.TextBox Text1 
      Height          =   3615
      Left            =   2400
      MultiLine       =   -1  'True
      TabIndex        =   1
      Top             =   360
      Width           =   3015
   End
   Begin VB.ListBox List1 
      Height          =   3570
      Left            =   120
      TabIndex        =   0
      Top             =   360
      Width           =   1335
   End
   Begin VB.Menu MnuOpzioni 
      Caption         =   "Opzioni"
      Begin VB.Menu mnurestart 
         Caption         =   "Restart"
      End
      Begin VB.Menu MnuEsci 
         Caption         =   "Esci"
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim Comando As String

Private Sub Command1_Click()
Dim idx As Byte, Mossa As String

idx = List1.ListIndex + 1
Scacchi.VerificaMossaBook (idx)
'copio la mossa scelta sulla textbox
Mossa = List1.List(idx - 1)
Text1.Text = Text1.Text + Mossa + " - "
'costruisco la lista di mosse successiva
List1.Clear
Scacchi.ListaMosseBook
End Sub

Private Sub Form_Load()
    Comando = LCase$(Command$)
    'DebugGenMosse (4): End
    'Comando = "book": 'forzatura per debug
    If Comando = "book" Then
        BookMode = True
        Scacchi.Init
        Scacchi.StartBook
        Scacchi.ListaMosseBook
    Else
        BookMode = False
        Scacchi.Init 'inizializza l'engine
        Scacchi.WinBoardLoop 'attiva routines per Winboard
        End
    End If
End Sub


Private Sub MnuEsci_Click()
    End
End Sub


Private Sub MnuRestart_Click()
        Text1.Text = ""
        List1.Clear
        Scacchi.StartBook
        Scacchi.ListaMosseBook
End Sub


