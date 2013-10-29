#include <time.h>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <unordered_map>

using namespace std;

const unsigned int MAX_FILES = 8;
const unsigned int HIGHEST_FILE = MAX_FILES - 1;

typedef bool Color;

const Color BLACK = 0;
const Color WHITE = 1;

class Object
{
};

enum PieceType
{
	NONE = 0,
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING
};

class PieceInitializer;
class Board;
class Moves;
class Square;

// A centisecond wall clock.
class Clock : Object
{
	public:
		Clock()
		{
			Reset();
		}

		void Reset()
		{
			m_bIsRunning = false;
			m_tTotal = 0;
			m_tLastStart = 0;
		}

		int Get()
		{
			if ( m_bIsRunning )
			{ return ( m_tTotal + ( clock() - m_tLastStart ) ) * 100 / CLOCKS_PER_SEC; }

			return m_tTotal * 100 / CLOCKS_PER_SEC;
		}

		operator int ()
		{
			return Get();
		}

		void Start()
		{
			if ( m_bIsRunning )
			{ return; }

			m_tLastStart = clock();

			m_bIsRunning = true;
		}

		void Stop()
		{
			m_bIsRunning = false;
			m_tTotal +=  ( clock() - m_tLastStart );
			m_tLastStart = ( clock_t )0;

		}

	protected:
		clock_t m_tLastStart;
		clock_t m_tTotal;
		bool m_bIsRunning;
};

class Piece : Object
{
		friend class PieceInitializer;

	public:
		Piece()
		{
			m_Color = BLACK;
			m_PieceType = NONE;
			m_pOtherColor = NULL;
		}

		Piece( Color color )
		{
			m_Color = color;
			m_PieceType = NONE;
			m_pOtherColor = NULL;
		}

		virtual int PieceValue() = 0;
		virtual Moves GenerateMoves( const Square& source, const Board& board ) = 0;
		virtual bool IsDifferent( const Square& dest, const Board& board ) const;
		virtual bool IsDifferentOrEmpty( const Square& dest, const Board& board ) const;

		void SetOtherColor( Piece& otherPiece )
		{
			m_pOtherColor = &otherPiece;
		}

		Piece* InvertColor()
		{
			return m_pOtherColor;
		}

		char Letter() const
		{
			// in Forsyth-Edwards notation, white pieces are uppercase
			if ( !m_Color )
			{ return m_Letter; }

			return ( char ) toupper( m_Letter );
		}

		Color GetColor() const
		{
			return m_Color;
		}

		void SetColor( Color val )
		{
			m_Color = val;
		}


	protected:
		char    m_Letter;

		Color   m_Color;
		Piece*   m_pOtherColor;
		PieceType m_PieceType;
};

class NoPiece : public Piece
{
	public:
		NoPiece()
		{
			m_Letter = '.';
			m_PieceType = NONE;
			m_pOtherColor = this;
		}

		int PieceValue()
		{
			return 0;
		}

		Moves GenerateMoves( const Square& source, const Board& board );

};

class Pawn : public Piece
{
	public:
		Pawn( Color color ) : Piece( color )
		{
			m_PieceType = PAWN;
			m_Letter = 'p';
		}

		int PieceValue()
		{
			return 100;
		}

		Moves GenerateMoves( const Square& source, const Board& board );

};

class Bishop : public Piece
{
	public:
		Bishop( Color color ) : Piece( color )
		{
			m_Letter = 'b';
			m_PieceType = BISHOP;
		}
		int PieceValue()
		{
			return 300;
		}

		Moves GenerateMoves( const Square& source, const Board& board );

};

class Knight : public Piece
{
	public:
		Knight( Color color ) : Piece( color )
		{
			m_Letter = 'n';
			m_PieceType = KNIGHT;
		}

		int PieceValue()
		{
			return 300;
		}

		Moves GenerateMoves( const Square& source, const Board& board );

};

class Rook : public Piece
{
	public:
		Rook( Color color ) : Piece( color )
		{
			m_Letter = 'r';
			m_PieceType = ROOK;
		}

		int PieceValue()
		{
			return 500;
		}

		Moves GenerateMoves( const Square& source, const Board& board );

};

class Queen : public Piece
{
	public:
		Queen( Color color ) : Piece( color )
		{
			m_Letter = 'q';
			m_PieceType = QUEEN;
		}

		int PieceValue()
		{
			return 900;
		}

		Moves GenerateMoves( const Square& source, const Board& board );

};

class King : public Piece
{
	public:
		King( Color color ) : Piece( color )
		{
			m_Letter = 'k';
			m_PieceType = KING;
		}

		int PieceValue()
		{
			return 100000;
		}

		Moves GenerateMoves( const Square& source, const Board& board );
};

Pawn WhitePawn( WHITE ), BlackPawn( BLACK );
Knight WhiteKnight( WHITE ), BlackKnight( BLACK );
Bishop WhiteBishop( WHITE ), BlackBishop( BLACK );
Rook WhiteRook( WHITE ), BlackRook( BLACK );
Queen WhiteQueen( WHITE ), BlackQueen( BLACK );
King WhiteKing( WHITE ), BlackKing( BLACK );
NoPiece None;

class Square;

class Board : Object
{
	public:
		Board()
		{
			Initialize();
		}

		void Initialize()
		{
			for ( int i = 0; i < MAX_FILES; i++ )
				for ( int j = 0; j < MAX_FILES; j++ )
				{ m_Piece[ i ][ j ] = &None; }

		}

		Piece* Set( int i, int j, Piece* piece )
		{
			return ( m_Piece[ i ][ j] = piece );
		}

		Piece* Get( int i, int j ) const
		{
			return m_Piece[ i ][ j ];
		}

		Piece* Set( const Square& s, Piece* piece );
		Piece* Get( const Square& s ) const;

		void Setup()
		{
			for ( int i = 0 ; i < MAX_FILES; i ++ )
			{
				m_Piece[i][1] = &WhitePawn;
				m_Piece[i][6] = &BlackPawn;
			}

			m_Piece[0][0] = m_Piece[7][0] = &WhiteRook;
			m_Piece[0][7] = m_Piece[7][7] = &BlackRook;

			m_Piece[1][0] = m_Piece[6][0] = &WhiteKnight;
			m_Piece[1][7] = m_Piece[6][7] = &BlackKnight;

			m_Piece[2][0] = m_Piece[5][0] = &WhiteBishop;
			m_Piece[2][7] = m_Piece[5][7] = &BlackBishop;

			m_Piece[3][0] = &WhiteQueen;
			m_Piece[4][0] = &WhiteKing;
			m_Piece[3][7] = &BlackQueen;
			m_Piece[4][7] = &BlackKing;
		}

		void Flip()
		{
			Piece* pTemp;


			for ( int j = 0 ; j < ( MAX_FILES / 2 ); j++ )
				for ( int i = 0; i < MAX_FILES; i++ )
				{
					pTemp = m_Piece[i][j];
					m_Piece[i][j] = m_Piece[HIGHEST_FILE - i][HIGHEST_FILE - j];
					m_Piece[HIGHEST_FILE - i][HIGHEST_FILE - j] = pTemp;
				}
		}

		bool IsEmpty( const Square& square ) const;

		void Dump()
		{
			for ( int j = ( MAX_FILES - 1 ); j >= 0; j-- )
			{
				for ( int i = 0; i < MAX_FILES; i++ )
				{
					cout << m_Piece[ i ][ j ]->Letter();
				}

				cout << endl;
			}
		}

		void Test()
		{
			Setup();
			Dump();
			Flip();
			Dump();
			Flip();
			Dump();
		}

	protected:
		Piece* m_Piece[ MAX_FILES ][ MAX_FILES ];
};

class Square : Object
{
	public:
		Square()
		{
			i = j = 0;
		}

		Square( int rank, int file )
		{
			i = rank;
			j = file;
		}

		Square( const string& s )
		{
			i = s[0] - '0' + 1;
			j = s[1] - 'a';
		}

		bool IsLegal() const
		{
			return ( ( ( i & ~7 ) == 0 ) && ( ( j & ~7 ) == 0 ) );
		}

		int I() const
		{
			return i;
		}

		void I( int val )
		{
			i = val;
		}

		int J() const
		{
			return j;
		}

		void J( int val )
		{
			j = val;
		}

		void Set( int ip, int jp )
		{
			i = ip;
			j = jp;
		}

		operator string ()
		{
			string s;

			if ( IsLegal() )
			{
				s = ( char )( 'a' + i );
				s += ( char )( '1' + j );
			}
			else
			{ s = "-"; }

			return s;
		}

		void Dump()
		{
			if ( IsLegal() )
			{
				cout << ( char )( 'a' + i );
				cout << ( char )( '1' + j );
			}
			else
			{ cout << "??"; }
		}

		Square Change( int ip, int jp )
		{
			i += ip;
			j += jp;

			return *this;
		}

		Square Change( const Square& s )
		{
			i += s.i;
			j += s.j;

			return *this;
		}

		Square Add( int ip, int jp ) const
		{
			Square s( i + ip, j + jp );
			return s;
		}

		Square Add( const Square& s ) const
		{
			Square s1( i + s.I(), j + s.I() );
			return s1;
		}

		void Test()
		{
			I( 3 );
			J( 4 );
			Dump();
			I( 12 );
			J( 0 );
			Dump();

		}

	protected:
		int i; // file
		int j; // rank
};

class Move : Object
{
	public:
		Move()
		{
			m_Piece = &None;
			m_Score = 0;
		}

		Move( Piece* piece )
		{
			m_Piece = piece;
			m_Score = 0;
		}

		Move( Piece* piece, const Square& source, const Square dest )
		{
			m_Piece = piece;
			m_Source = source;
			m_Dest = dest;
			m_Score = 0;
		}

		Piece* GetPiece() const { return m_Piece; }
		void SetPiece( Piece* val ) { m_Piece = val; }
		Square Source() const { return m_Source; }
		void Source( Square val ) { m_Source = val; }
		Square Dest() const { return m_Dest; }
		void Dest( Square val ) { m_Dest = val; }

		void Dump()
		{
			if ( m_Piece == &None )
			{
				cout << "NoMove";
				return;
			}
			cout << m_Piece->Letter();
			m_Source.Dump();
			m_Dest.Dump();
		}

		operator string ()
		{
			string s;

			s = ( string )m_Source + ( string )m_Dest;

			return s;
		}

		int Score() const { return m_Score; }
		void Score( int val ) { m_Score = val; }

	protected:
		Piece* m_Piece;
		Square m_Source, m_Dest;
		int m_Score;
};

bool operator< ( const Move& left, const Move& right )
{
	if  ( left.Score() > right.Score() ) { return true; }
	return false;
}

Move NullMove;

class PieceInitializer : Object
{
	public:
		PieceInitializer()
		{
			WhitePawn.SetOtherColor( BlackPawn );
			BlackPawn.SetOtherColor( WhitePawn );
			WhiteKnight.SetOtherColor( BlackKnight );
			BlackKnight.SetOtherColor( WhiteKnight );
			WhiteBishop.SetOtherColor( BlackBishop );
			BlackBishop.SetOtherColor( WhiteBishop );
			WhiteRook.SetOtherColor( BlackRook );
			BlackRook.SetOtherColor( WhiteRook );
			WhiteQueen.SetOtherColor( BlackQueen );
			BlackQueen.SetOtherColor( WhiteQueen );
			WhiteKing.SetOtherColor( BlackKing );
			BlackKing.SetOtherColor( WhiteKing );
			None.SetOtherColor( None );

			NullMove.Source( Square( -99, -99 ) );
			NullMove.Dest( Square( -99, -99 ) );
		}
};

class Moves : Object
{
	public:

		Moves()
		{
			Initialize();
		}

		void Initialize()
		{
			m_Moves.clear();
			m_Moves.reserve( 32 );
		}

		void Add( const Move& move )
		{
			m_Moves.push_back( move );
		}

		Moves operator+ ( Moves otherMoves )
		{
			m_Moves.insert( m_Moves.end(),
							otherMoves.m_Moves.begin(),
							otherMoves.m_Moves.end() );

			return *this;
		}

		Move Random()
		{
			if ( m_Moves.size() == 0 )
			{ return NullMove; }

			return m_Moves.at( rand() % m_Moves.size() );
		}

		Move GetFirst()
		{
			return m_Moves.front();
		}

		void Dump()
		{
			vector< Move >::iterator it;

			for ( it = m_Moves.begin(); it != m_Moves.end(); it++ )
			{
				( *it ).Dump();
				cout << " ";
			}
		}

		operator string ()
		{
			string s;

			MovesInternalType::iterator it;

			for ( it = m_Moves.begin(); it != m_Moves.end(); it++ )
			{
				s += ( string ) * it;
				s += " ";
			}

			return s;
		}

		bool TryAttack( const Move& m, const Board& board, int id, int jd )
		{
			Move myMove = m;

			myMove.Dest( Square( id + myMove.Source().I(), jd + myMove.Source().J() ) );

			if ( myMove.Dest().IsLegal() )
			{
				// Captures are more interesting than moves.
				if ( myMove.GetPiece()->IsDifferent( myMove.Dest(), board ) )
				{
					myMove.Score( board.Get( myMove.Dest() )->PieceValue() );
					Add( myMove );
					return true;
				}
				else if ( board.IsEmpty( myMove.Dest() ) )
				{
					Add( myMove );
					return true;
				}
			}

			return false;
		}

		unsigned int TryRayAttack( const Move& m, const Board& board, int id, int jd )
		{
			int nAttacks = 0;

			Square sAttacked;

			int i = id;
			int j = jd;

			while ( TryAttack( m, board, i, j ) )
			{
				sAttacked.Set( m.Source().I() + i, m.Source().J() + j );

				if ( board.Get( sAttacked ) != &None )
				{ break; }  // attack is over; we hit a piece of this or the other color

				i += id;
				j += jd;
				nAttacks++;
			}

			return nAttacks;
		}

	protected:
		typedef vector< Move > MovesInternalType;
		MovesInternalType m_Moves;
};

class Position : Object
{
	public:
		Position()
		{
			Initialize();
		}

		void Initialize()
		{
			m_ColorToMove = WHITE;
			m_nPly = 0;
			m_nScore = 0;
			m_nHalfMoves = 0;
			m_bBKR = m_bBQR = m_bWKR = m_bWQR = true;
			m_sEnPassant.Set( -1, -1 );
		}

		Position( bool colorToMove )
		{
			Initialize();
			m_ColorToMove = colorToMove;
		}

		Position( const string& sFEN )
		{
			Initialize();
			SetFEN( sFEN );
		}

		Position( const Position& position, const Move& move )
		{
			Initialize();
			m_Board = position.m_Board;
			m_nPly = position.m_nPly + 1;

			if ( &move == &NullMove )
			{ return; }

			m_nScore = position.Score() + ( m_Board.Get( move.Dest() )->PieceValue() ) *
					   ( m_ColorToMove ? -1 : 1 );

			m_Board.Set( move.Source().I(), move.Source().J(), &None );
			m_Board.Set( move.Dest().I(), move.Dest().J(), move.GetPiece() );

			m_ColorToMove = !position.m_ColorToMove;
		}

		Moves GenerateMoves() const
		{
			Piece* pPiece;
			Moves moves;
			for ( int j = 0; j < MAX_FILES; j++ )
				for ( int i = 0; i < MAX_FILES; i++ )
				{
					pPiece = m_Board.Get( i, j );

					if ( ( pPiece != &None ) && ( pPiece->GetColor() == m_ColorToMove ) )
					{
						moves = moves + pPiece->GenerateMoves( Square( i, j ), m_Board );
					}
				}

			return moves;
		}

		Board GetBoard() const
		{
			return m_Board;
		}

		void SetBoard( Board val )
		{
			m_Board = val;
		}

		void Setup()
		{
			Initialize();
			m_Board.Setup();
			m_ColorToMove = WHITE;
		}

		void Dump()
		{
			m_Board.Dump();

			stringstream ss;

			ss << "Ply: " << m_nPly << endl;
			cout << endl;
			cout << "Ply: ";
			cout << m_nPly;
			cout << endl;
		}

		int SetFEN( const string& sFEN )
		{
			stringstream ss;

			ss.str( sFEN );

			string sBoard, sToMove, sVirgins, sEnPassant;
			int nMoves;

			ss >> sBoard >> sToMove >> sVirgins >> sEnPassant >> m_nHalfMoves >> nMoves;

			int j = MAX_FILES - 1;
			int i = 0;
			char c;

			stringstream ssBoard( sBoard );
			m_Board.Initialize();

			while ( ssBoard >> c )
			{
				switch ( c )
				{
					case '1' :
					case '2' :
					case '3' :
					case '4' :
					case '5' :
					case '6' :
					case '7' :
					case '8' :
						i += c - '0';
						break;

					case 'r' :
						m_Board.Set( i++, j, &BlackRook );
						break;

					case 'n' :
						m_Board.Set( i++, j, &BlackKnight );
						break;

					case 'b' :
						m_Board.Set( i++, j, &BlackBishop );
						break;

					case 'q' :
						m_Board.Set( i++, j, &BlackQueen );
						break;

					case 'k' :
						m_Board.Set( i++, j, &BlackKing );
						break;

					case 'p' :
						m_Board.Set( i++, j, &BlackPawn );
						break;

					case 'R' :
						m_Board.Set( i++, j, &WhiteRook );
						break;

					case 'N' :
						m_Board.Set( i++, j, &WhiteKnight );
						break;

					case 'B' :
						m_Board.Set( i++, j, &WhiteBishop );
						break;

					case 'Q' :
						m_Board.Set( i++, j, &WhiteQueen );
						break;

					case 'K' :
						m_Board.Set( i++, j, &WhiteKing );
						break;

					case 'P' :
						m_Board.Set( i++, j, &WhitePawn );
						break;

					case '/' :
						i = 0;
						j--;
						break;

					default :
						cerr << "Unknown character in FEN board position";
						break;

				}
			}

			m_ColorToMove = ( sToMove == "w" );

			stringstream ssVirgins( sVirgins );

			m_bWKR = m_bWQR = m_bBKR = m_bBQR = false;

			while ( ssVirgins >> c )
			{
				switch ( c )
				{
					case '-' :
						break;

					case 'K':
						m_bWKR = true;
						break;

					case 'Q':
						m_bWQR = true;
						break;

					case 'k':
						m_bBKR = true;
						break;

					case 'q':
						m_bBQR = true;
						break;
				}
			}

			Square s( sEnPassant );
			m_sEnPassant = s;
			m_nPly = ( nMoves - 1 ) * 2 + ( m_ColorToMove ? 0 : 1 );

			return 0;
		}

		string GetFEN()
		{
			string s;
			Piece* pPiece;
			int nSpaces = 0;

			for ( int j = MAX_FILES - 1; j >= 0; j-- )
			{
				for ( int i = 0; i < MAX_FILES; i++ )
				{
					pPiece = m_Board.Get( i, j );

					if ( ( pPiece != &None ) && ( nSpaces > 0 ) )
					{
						s += ( char )nSpaces + '0';
						nSpaces = 0;
					}

					if ( pPiece != &None )
					{ s += pPiece->Letter(); }
					else
					{ nSpaces++; }

				}

				if ( nSpaces > 0 )
				{ s += ( char ) nSpaces + '0'; }

				nSpaces = 0;
				if ( j != 0 )
				{ s += '/'; }
			}

			if ( m_ColorToMove )
			{ s += " w "; }
			else
			{ s += " b "; }

			if ( !( m_bWKR || m_bWQR || m_bBKR || m_bBQR ) )
			{ s += "-"; }
			else
			{
				if ( m_bWKR )
				{ s += "K"; }
				if ( m_bWQR )
				{ s += "Q"; }
				if ( m_bBKR )
				{ s += "k"; }
				if ( m_bBQR )
				{ s += "q"; }
			}

			stringstream ss;

			ss << " ";
			ss << ( string )m_sEnPassant;
			ss << " ";
			ss << m_nHalfMoves;
			ss << " ";
			ss << m_nPly / 2 + 1;

			s += ss.str();

			return s;
		}

		operator string ()
		{
			return GetFEN();
		}

		unsigned int UpperBound() const { return m_nUpperBound; }
		void UpperBound( unsigned int val ) { m_nUpperBound = val; }

		unsigned int LowerBound() const { return m_nLowerBound; }
		void LowerBound( unsigned int val ) { m_nLowerBound = val; }

		Color ColorToMove() const { return m_ColorToMove; }
		void ColorToMove( Color val ) { m_ColorToMove = val; }

		int Score() const { return m_nScore; }
		void Score( int val ) { m_nScore = val; }

	protected:
		Board   m_Board;
		Color   m_ColorToMove;
		unsigned int    m_nPly;
		int m_nLowerBound;
		int m_nUpperBound;
		int m_nScore;
		int m_nHalfMoves;
		// Virgin rooks; can tell whether any of the four rooks has been moved
		bool m_bWKR, m_bWQR, m_bBKR, m_bBQR;
		Square m_sEnPassant;
};

class Searcher : Object
{
	public:

		Searcher() :
			m_nNodesSearched( 0 )
		{
		}

		virtual int Search( const Position& pos,
							Moves& mPrincipalVariation ) = 0;

		virtual int Evaluate( const Position& pos )
		{
			int nScore = 0;

			for ( int i = 0; i < MAX_FILES; i++ )
				for ( int j = 0; j < MAX_FILES; j++ )
				{
					Piece* piece;

					piece = pos.GetBoard().Get( i, j );

					nScore += ( piece->PieceValue() *
								( ( piece->GetColor() == WHITE ) ? 1 : -1 ) );

				}

			int nBias = ( pos.ColorToMove() == WHITE ) ? 1 : -1;

			return nScore * nBias;
		}

	protected:
		int m_nNodesSearched;
};

Piece* Board::Set( const Square& s, Piece* piece )
{
	return ( m_Piece[ s.I() ][ s.J() ] = piece );
}

Piece* Board::Get( const Square& s ) const
{
	return m_Piece[ s.I() ][ s.J()];
}

bool Board::IsEmpty( const Square& square ) const
{
	return ( m_Piece[ square.I() ][ square.J()] == &None );
}

bool Piece::IsDifferent( const Square& dest, const Board& board ) const
{
	Piece* piece = board.Get( dest );

	if ( piece == &None )
	{ return false; }

	return ( m_Color != piece->GetColor() );
}

bool Piece::IsDifferentOrEmpty( const Square& dest, const Board& board ) const
{
	Piece* piece = board.Get( dest );

	if ( piece == &None )
	{ return true; }

	return ( m_Color != piece->GetColor() );
}

Moves NoPiece::GenerateMoves( const Square& source, const Board& board )
{
	source;
	board;

	Moves moves;
	return moves;
}

Moves Pawn::GenerateMoves( const Square& source, const Board& board )
{
	Moves moves;
	Square dest = source;

	int d = m_Color ? 1 : -1;

	Move m( this, source, source );

	// Generate forward sliding moves
	dest.Change( 0, d );
	m.Dest( dest );

	if ( board.IsEmpty( m.Dest() ) )
	{
		moves.Add( m );

		// Two-square slide only from initial square
		if ( ( ( source.J() == 1 ) && ( GetColor() == WHITE ) ) ||
				( ( source.J() == 6 ) && ( GetColor() == BLACK ) ) )
		{
			dest.Change( 0, d );
			m.Dest( dest );

			if ( board.IsEmpty( m.Dest() ) )
			{ moves.Add( m ); }

		}
	}

	// Generate capture moves
	dest = source.Add( -1, d );
	if ( dest.IsLegal() && IsDifferent( dest, board ) )
	{
		m.Dest( dest );
		moves.Add( m );
	}

	dest = source.Add( 1, d );
	if ( dest.IsLegal() && IsDifferent( dest, board ) )
	{
		m.Dest( dest );
		moves.Add( m );
	}

	return moves;
}

Moves Knight::GenerateMoves( const Square& source, const Board& board )
{
	Move m( this, source, source );
	Moves moves;

	moves.TryAttack( m, board, 1, 2 );
	moves.TryAttack( m, board, -1, 2 );
	moves.TryAttack( m, board, 1, -2 );
	moves.TryAttack( m, board, -1, -2 );

	moves.TryAttack( m, board, 2, 1 );
	moves.TryAttack( m, board, -2, 1 );
	moves.TryAttack( m, board, 2, -1 );
	moves.TryAttack( m, board, -2, -1 );

	return moves;
}

Moves Bishop::GenerateMoves( const Square& source, const Board& board )
{
	Moves moves;
	Move m( this, source, source );

	moves.TryRayAttack( m, board, 1, 1 );
	moves.TryRayAttack( m, board, 1, -1 );
	moves.TryRayAttack( m, board, -1, 1 );
	moves.TryRayAttack( m, board, -1, -1 );

	return moves;
}

Moves Rook::GenerateMoves( const Square& source, const Board& board )
{
	Moves moves;
	Move m( this, source, source );

	moves.TryRayAttack( m, board, 0, 1 );
	moves.TryRayAttack( m, board, 0, -1 );
	moves.TryRayAttack( m, board, -1, 0 );
	moves.TryRayAttack( m, board, 1, 0 );

	return moves;
}

Moves King::GenerateMoves( const Square& source, const Board& board )
{
	Move m( this, source, source );
	Moves moves;

	moves.TryAttack( m, board, 1, 0 );
	moves.TryAttack( m, board, -1, 0 );
	moves.TryAttack( m, board, 0, 1 );
	moves.TryAttack( m, board, 0, -1 );

	moves.TryAttack( m, board, 1, 1 );
	moves.TryAttack( m, board, -1, 1 );
	moves.TryAttack( m, board, 1, -1 );
	moves.TryAttack( m, board, -1, -1 );

	return moves;
}

Moves Queen::GenerateMoves( const Square& source, const Board& board )
{
	Moves moves;
	Move m( this, source, source );

	moves.TryRayAttack( m, board, 1, 1 );
	moves.TryRayAttack( m, board, 1, -1 );
	moves.TryRayAttack( m, board, -1, 1 );
	moves.TryRayAttack( m, board, -1, -1 );

	moves.TryRayAttack( m, board, 0, 1 );
	moves.TryRayAttack( m, board, 0, -1 );
	moves.TryRayAttack( m, board, -1, 0 );
	moves.TryRayAttack( m, board, 1, 0 );

	return moves;
}

class Game : Object
{
	public:
		void New()
		{
			m_Time = m_OTime = 0;
			m_nMovesPerBaseTime = 40;
			m_nIncrementTime = 0;
			m_nBaseTime = 5 * 60;
			m_Position.Setup();
		}

		int Time() const { return m_Time; }
		void Time( int val ) { m_Time = val; }
		int OTime() const { return m_OTime; }
		void OTime( int val ) { m_OTime = val; }
		Position* GetPosition() { return &m_Position; }
		void SetPosition( Position& pos ) { m_Position = pos; }

	protected:
		friend class Interface;
		int m_Time, m_OTime; // engine time and opponent time, in centiseconds
		int m_nMovesPerBaseTime; // number of moves expected in the base time period
		int m_nBaseTime; // amount of time in seconds in the base time period
		int m_nIncrementTime; // amount of time in seconds to increment after every move

		Position m_Position;
};

class Interface;

#define INTERFACE_FUNCTION_PARAMS const string &sParams
#define INTERFACE_FUNCTION_RETURN_TYPE void

#define INTERFACE_PROTOTYPE( FunctionName )  INTERFACE_FUNCTION_RETURN_TYPE FunctionName ( INTERFACE_FUNCTION_PARAMS )
#define INTERFACE_FUNCTION_TYPE( Variable ) INTERFACE_FUNCTION_RETURN_TYPE ( Interface::* Variable )( INTERFACE_FUNCTION_PARAMS )
#define INTERFACE_FUNCTION_ABSTRACT_TYPE (*( INTERFACE_FUNCTION_RETURN_TYPE )())

typedef INTERFACE_FUNCTION_RETURN_TYPE ( Interface::*InterfaceFunctionType )(
	INTERFACE_FUNCTION_PARAMS );

class Interface : Object
{
	public:

		Interface( istream* in = &cin, ostream* out = &cout )
		{
			m_In = in;
			m_Out = out;
			m_pGame = new Game;
			m_bShowThinking = false;
		}

		~Interface()
		{
			delete  m_pGame;
		}

		void RegisterCommand( const string& sCommand,
							  INTERFACE_FUNCTION_TYPE( pfnCommand ) )
		{
			m_CommandMap[ sCommand ] = pfnCommand;
		};

		INTERFACE_PROTOTYPE( Notify )
		{
			switch ( m_Protocol )
			{
				case PROTOCOL_XBOARD:
					( *m_Out ) << "# " << sParams << endl;
					break;

				case PROTOCOL_UCI:
					( *m_Out ) << "info string " << sParams << endl;
					break;

				default:
					( *m_Out ) << sParams;
			}
		}

		INTERFACE_PROTOTYPE( Instruct )
		{
			( *m_Out ) << sParams << endl;
		}

		INTERFACE_PROTOTYPE( Xboard )
		{
			RegisterXboard( sParams );
			Notify( "Xboard commands registered" );
		}

		INTERFACE_PROTOTYPE( UCI )
		{
			RegisterUCI( sParams );
			Instruct( "id name Ippon" );
			Instruct( "id author John Byrd" );
			Instruct( "uciok" );
			Notify( "UCI commands registered" );
		}

		INTERFACE_PROTOTYPE( RegisterXboard )
		{
			sParams;
			m_Protocol = PROTOCOL_XBOARD;
			RegisterCommand( "accepted",    &Interface::Accepted );
			RegisterCommand( "new",         &Interface::New );
			RegisterCommand( "protover",    &Interface::Protover );
			RegisterCommand( "ping",        &Interface::Ping );
			RegisterCommand( "usermove",    &Interface::Usermove );
			RegisterCommand( "setboard",    &Interface::Setboard );
			RegisterCommand( "level",       &Interface::Level );
			RegisterCommand( "st",          &Interface::St );
			RegisterCommand( "time",        &Interface::Time );
			RegisterCommand( "otim",        &Interface::OTim );
			RegisterCommand( "?",           &Interface::MoveNow );
			RegisterCommand( "post",        &Interface::Post );
			RegisterCommand( "nopost",      &Interface::NoPost );
			RegisterCommand( "hard",        &Interface::Hard );
			RegisterCommand( "easy",        &Interface::Easy );
			RegisterCommand( "force",       &Interface::Force );
			RegisterCommand( "go",          &Interface::XboardGo );
			RegisterCommand( "black",       &Interface::Black );
			RegisterCommand( "white",       &Interface::White );
		}

		INTERFACE_PROTOTYPE( RegisterUCI )
		{
			sParams;
			m_Protocol = PROTOCOL_UCI;
			RegisterCommand( "debug",       &Interface::DebugUCI );
			RegisterCommand( "isready",     &Interface::IsReady );
			RegisterCommand( "setoption",   &Interface::SetOption );
			RegisterCommand( "ucinewgame",  &Interface::New );
			RegisterCommand( "position",    &Interface::UCIPosition );
			RegisterCommand( "go",          &Interface::UCIGo );
			RegisterCommand( "stop",        &Interface::Stop );
			RegisterCommand( "ponderhit",   &Interface::Ponderhit );
		}

		void RegisterAll()
		{
			RegisterCommand( "xboard",  &Interface::Xboard );
			RegisterCommand( "uci",     &Interface::UCI );
			RegisterCommand( "quit",    &Interface::Quit );
		}

		INTERFACE_PROTOTYPE( DebugUCI )
		{
			sParams;
		}

		INTERFACE_PROTOTYPE( IsReady )
		{
			// stop any pondering or loading
			sParams;
			Instruct( "readyok" );
		}

		INTERFACE_PROTOTYPE( SetOption )
		{
			sParams;
		}

		INTERFACE_PROTOTYPE( UCIGo )
		{
			sParams;
		}

		INTERFACE_PROTOTYPE( UCIPosition )
		{
			sParams;
		}

		INTERFACE_PROTOTYPE( Stop )
		{
			sParams;
		}

		INTERFACE_PROTOTYPE( Ponderhit )
		{
			sParams;
		}

		INTERFACE_PROTOTYPE( Quit )
		{
			sParams;
			Notify( "Engine exiting" );
			exit( 0 );
		}

		INTERFACE_PROTOTYPE( Ping )
		{
			stringstream ss;
			ss.str( sParams );

			int n;
			ss >> n;

			*( m_Out ) << "pong " << n << endl;
		}

		INTERFACE_PROTOTYPE( StopThinking )
		{
			sParams;

			Notify( "Thinking stopped" );

		}

		INTERFACE_PROTOTYPE( Black )
		{
			sParams;
			Position* pPosition = m_pGame->GetPosition();
			pPosition;
		}

		INTERFACE_PROTOTYPE( White )
		{
			sParams;
			Position* pPosition = m_pGame->GetPosition();
			pPosition;
		}

		INTERFACE_PROTOTYPE( Force )
		{
			StopThinking( sParams );
			Time( "0" );
			OTim( "0" );

			Notify( "Force" );
		}

		INTERFACE_PROTOTYPE( MoveNow )
		{
			StopThinking( sParams );
			Move myMove;
			myMove = ( m_PrincipalVariation ).GetFirst();

			*( m_Out ) << "move " << ( string )myMove << endl;
			*( m_Out ) << "# PV " << ( string )m_PrincipalVariation << endl;
		}

		INTERFACE_PROTOTYPE( Post )
		{
			sParams;
			m_bShowThinking = true;
			Notify( "Thinking will be shown" );
		}

		INTERFACE_PROTOTYPE( NoPost )
		{
			sParams;
			m_bShowThinking = false;
			Notify( "Thinking will not be shown" );
		}

		INTERFACE_PROTOTYPE( Hard )
		{
			sParams;
			m_bPonder = true;
			Notify( "Engine will ponder on opponent's time" );
		}

		INTERFACE_PROTOTYPE( Easy )
		{
			sParams;
			m_bPonder = false;
			Notify( "Engine will not ponder on opponent's time" );
		}

		INTERFACE_PROTOTYPE( Protover )
		{
			stringstream ss;
			ss.str( sParams );

			ss >> m_Protover;

			if ( m_Protover >= 2 )
			{
				string sResponse;

				*( m_Out ) << "feature ping=1 setboard=1 playother=1 usermove=1 analyze=0" <<
						   endl;
			}

		}

		INTERFACE_PROTOTYPE( Accepted )
		{
			sParams;
			*( m_Out ) << "# Unknown parameter to accepted" << endl;
		}

		INTERFACE_PROTOTYPE( New )
		{
			sParams;
			m_pGame->New();
			Notify( "New game" );
		}

		INTERFACE_PROTOTYPE( Setboard )
		{
			Position* pPosition;

			pPosition = m_pGame->GetPosition();
			pPosition->SetFEN( sParams );
			string sFEN = pPosition->GetFEN();

			*( m_Out ) << "# New position: " << sFEN << endl;
		}

		INTERFACE_PROTOTYPE( Usermove )
		{
			Square sSrc, sDest;

			sSrc.Set( sParams[0] - 'a', sParams[1] - '1' );
			sDest.Set( sParams[2] - 'a', sParams[3] - '1' );

			Position* pPosition = m_pGame->GetPosition();

			Piece* pPiece = pPosition->GetBoard().Get( sSrc );

			Move move( pPiece, sSrc, sDest );

			Position nextPos( *pPosition, move );
			*pPosition = nextPos;

			*( m_Out ) << "# User move: " << ( string )move << endl;

			XboardGo( "Thinking Params Go Here" );
		}

		int TimeToSeconds( const string& sTime )
		{
			unsigned int nColon = 0;
			int nMinutes = 0, nSeconds = 0;

			nColon = sTime.find( ':' );

			if ( nColon == string::npos )
			{
				stringstream ss;
				ss.str( sTime );
				ss >> nMinutes;
			}
			else
			{
				stringstream sMin, sSec;
				sMin.str( sTime.substr( 0, nColon ) );
				sSec.str( sTime.substr( nColon + 1 ) );
				sMin >> nMinutes;
				sSec >> nSeconds;
			}

			return ( nMinutes * 60 + nSeconds );
		}

		INTERFACE_PROTOTYPE( Level )
		{
			stringstream ss;
			ss.str( sParams );

			string sBaseTime, sIncrementTime;

			ss >> m_pGame->m_nMovesPerBaseTime >> sBaseTime >> sIncrementTime;

			m_pGame->m_nBaseTime = TimeToSeconds( sBaseTime );
			m_pGame->m_nIncrementTime = TimeToSeconds( sIncrementTime );

			*( m_Out ) << "# Level: Moves per base: " << m_pGame->m_nMovesPerBaseTime <<
					   " Base: " << m_pGame->m_nBaseTime << " Inc: " << m_pGame->m_nIncrementTime <<
					   endl;

		}

		INTERFACE_PROTOTYPE( St )
		{
			stringstream ss;
			ss.str( sParams );

			ss >> m_pGame->m_nBaseTime;
			m_pGame->m_nMovesPerBaseTime = m_pGame->m_nIncrementTime = 0;

			*( m_Out ) << "# Level: Moves per base: " << m_pGame->m_nMovesPerBaseTime <<
					   " Base: " << m_pGame->m_nBaseTime << " Inc: " << m_pGame->m_nIncrementTime <<
					   endl;
		}

		INTERFACE_PROTOTYPE( XboardGo )
		{
			sParams;
			Notify( "Thinking initiated" );
		}

		INTERFACE_PROTOTYPE( Time )
		{
			stringstream ss;
			ss.str( sParams );

			ss >> m_pGame->m_Time;

			*( m_Out ) << "# Time: " << m_pGame->m_Time << endl;

		}

		INTERFACE_PROTOTYPE( OTim )
		{
			stringstream ss;
			ss.str( sParams );

			ss >> m_pGame->m_OTime;

			*( m_Out ) << "# Opponent time: " << m_pGame->m_OTime << endl;
		}


		static void Execute( void* pInterface, const string& sCommand )
		{
			( ( Interface* )pInterface )->Execute( sCommand );
		}

		void Execute( const string& sCommand )
		{
			string sParams, sVerb;

			stringstream ss;
			ss.str( sCommand );

			ss >> sVerb;

			if ( sVerb.length() < sCommand.length() )
			{ sParams = sCommand.substr( sVerb.length() + 1, 1024 ); }

			InterfaceFunctionType ic = m_CommandMap[ sVerb ];

			if ( ic )
			{ ( this->*ic )( sParams ); }
			else
			{
				stringstream unk;
				unk << "Unknown command: ";
				unk << sCommand << endl;
				Notify( unk.str() );
			}
		}


		void Run()
		{
			m_Out->setf( ios::unitbuf );

			string sInputLine;

			RegisterAll( );

			for ( ;; )
			{
				getline( *m_In, sInputLine );
				Execute( sInputLine );
			}
		}

		ostream* m_Out;
		istream* m_In;

		Game* m_pGame;

		Moves m_PrincipalVariation;

		enum ProtocolType
		{
			PROTOCOL_XBOARD,
			PROTOCOL_UCI
		};

		ProtocolType m_Protocol;

		int m_Protover;

		bool m_bShowThinking;
		bool m_bPonder;

	protected:
		unordered_map< string, InterfaceFunctionType > m_CommandMap;

};

int main( int argc, char* argv[] )
{
	srand ( ( unsigned int ) time( NULL ) );

	// clean up unreferenced warnings about parameters
	argc;
	argv;

	PieceInitializer pieceInitializer;

	Interface i;

	i.Run();

	return 0;
}

