VERSION 5.00
Begin VB.Form Form2 
   Caption         =   "Richiesta Valore"
   ClientHeight    =   2595
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4560
   Icon            =   "RichValore.frx":0000
   LinkTopic       =   "Form2"
   ScaleHeight     =   2595
   ScaleWidth      =   4560
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "!"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   24
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   1095
      Index           =   2
      Left            =   3120
      TabIndex        =   3
      Top             =   960
      Width           =   1095
   End
   Begin VB.CommandButton Command1 
      Caption         =   "="
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   24
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   1095
      Index           =   1
      Left            =   1680
      TabIndex        =   2
      Top             =   960
      Width           =   1095
   End
   Begin VB.CommandButton Command1 
      Caption         =   "?"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   24
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   1095
      Index           =   0
      Left            =   240
      TabIndex        =   1
      Top             =   960
      Width           =   1095
   End
   Begin VB.Label Label1 
      Alignment       =   2  'Center
      Caption         =   "Scegli il valore di questa mossa"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   360
      Width           =   4215
   End
End
Attribute VB_Name = "Form2"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False


Function Modal() As Integer
'ferma l'esecuzione del programma finche' non si preme
'un pulsante di valutazione
Form1.Visible = False
ValoreRitorno = -100
Do
    DoEvents
    If ValoreRitorno > -1 Then Exit Do
Loop
Modal = ValoreRitorno + 1
Form1.Visible = True
End Function

Private Sub Command1_Click(Index As Integer)
    ValoreRitorno = Index
End Sub

