VERSION 5.00
Begin VB.Form Form2 
   Caption         =   "Salva file PGN"
   ClientHeight    =   3915
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   3690
   Icon            =   "pgn.frx":0000
   LinkTopic       =   "Form2"
   ScaleHeight     =   3915
   ScaleWidth      =   3690
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "Ok"
      Height          =   375
      Left            =   3000
      TabIndex        =   13
      Top             =   3360
      Width           =   495
   End
   Begin VB.OptionButton Option3 
      Caption         =   "Option3"
      Height          =   255
      Left            =   2400
      TabIndex        =   9
      Top             =   2880
      Width           =   255
   End
   Begin VB.OptionButton Option2 
      Caption         =   "Option2"
      Height          =   255
      Left            =   1320
      TabIndex        =   8
      Top             =   2880
      Width           =   255
   End
   Begin VB.OptionButton Option1 
      Caption         =   "Option1"
      Height          =   255
      Left            =   240
      TabIndex        =   7
      Top             =   2880
      Width           =   255
   End
   Begin VB.TextBox Text3 
      Height          =   285
      Left            =   1200
      TabIndex        =   5
      Top             =   1440
      Width           =   2055
   End
   Begin VB.TextBox Text2 
      Height          =   285
      Left            =   1200
      TabIndex        =   3
      Top             =   1080
      Width           =   2055
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   1200
      TabIndex        =   1
      Top             =   480
      Width           =   2055
   End
   Begin VB.Label Label7 
      Caption         =   "0-1"
      Height          =   255
      Left            =   2400
      TabIndex        =   12
      Top             =   2520
      Width           =   255
   End
   Begin VB.Label Label6 
      Caption         =   "1/2-1/2"
      Height          =   255
      Left            =   1200
      TabIndex        =   11
      Top             =   2520
      Width           =   615
   End
   Begin VB.Label Label5 
      Alignment       =   2  'Center
      Caption         =   "1-0"
      Height          =   255
      Left            =   120
      TabIndex        =   10
      Top             =   2520
      Width           =   495
   End
   Begin VB.Label Label4 
      Alignment       =   2  'Center
      Caption         =   "Risultato:"
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   2040
      Width           =   975
   End
   Begin VB.Label Label3 
      Alignment       =   2  'Center
      Caption         =   "NERO"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   1440
      Width           =   975
   End
   Begin VB.Label Label2 
      Alignment       =   2  'Center
      Caption         =   "BIANCO"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   1080
      Width           =   975
   End
   Begin VB.Label Label1 
      Alignment       =   2  'Center
      Caption         =   "Luogo"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   480
      Width           =   975
   End
End
Attribute VB_Name = "Form2"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Command1_Click()
    Scacchi.SalvaPgn
    Form1.SetFocus
End Sub
