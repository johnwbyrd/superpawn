/**
 ** Chess.cpp
 ** A simple UCI-compatible chess engine
 **
 ** http://creativecommons.org/licenses/by/3.0/
 **/

/** \todo Check
 ** \todo Checkmate
 ** \todo Castling
 ** \todo Timers
 ** \todo En passant
 ** \todo Stalemate
 ** \todo Fifty-move clock
 ** \todo Threefold repetition
 ** \todo Draw due to material
 ** \todo Resignation?
 **/

/** Number of rows and columns on the board */
const unsigned int MAX_FILES = 8;
const unsigned int HIGHEST_FILE = MAX_FILES - 1;
const unsigned int MAX_SQUARES = MAX_FILES * MAX_FILES;

/** Maximum command length for UCI commands. */
const unsigned int MAX_COMMAND_LENGTH = 64 * 256;

/** Default search depth */
const unsigned int SEARCH_DEPTH = 6; //-V112

/** An estimate of a reasonable maximum of moves in any given position.  Not
 ** a hard bound.
 **/
const unsigned int DEFAULT_MOVES_SIZE = 2 << 6;

#include <time.h>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <memory>
#include <chrono>
#include <climits>
#include <ratio>
#include <atomic>

using namespace std;

typedef bool Color;
const Color BLACK = false;
const Color WHITE = true;

/** The base class for all objects in the program. */
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
class Move;
class Moves;
class Square;
class Interface;

/* Definitions specifically to speed along function creation in the Interface class. */
#define INTERFACE_FUNCTION_PARAMS const string &sParams
#define INTERFACE_FUNCTION_NO_PARAMS const string &
#define INTERFACE_FUNCTION_RETURN_TYPE void

#define INTERFACE_PROTOTYPE( FunctionName )  INTERFACE_FUNCTION_RETURN_TYPE FunctionName ( INTERFACE_FUNCTION_PARAMS )
#define INTERFACE_PROTOTYPE_NO_PARAMS( FunctionName )  INTERFACE_FUNCTION_RETURN_TYPE FunctionName ( INTERFACE_FUNCTION_NO_PARAMS )
#define INTERFACE_FUNCTION_TYPE( Variable ) INTERFACE_FUNCTION_RETURN_TYPE ( Interface::* Variable )( INTERFACE_FUNCTION_PARAMS )
#define INTERFACE_FUNCTION_TYPE_NO_PARAMS( Variable ) INTERFACE_FUNCTION_RETURN_TYPE ( Interface::* Variable )( INTERFACE_FUNCTION_NO_PARAMS )
#define INTERFACE_FUNCTION_ABSTRACT_TYPE (*( INTERFACE_FUNCTION_RETURN_TYPE )())

typedef INTERFACE_FUNCTION_RETURN_TYPE ( Interface::*InterfaceFunctionType )(
	INTERFACE_FUNCTION_PARAMS );

/** A centisecond wall clock. */
class Clock : Object
{
	public:

		typedef chrono::system_clock NativeClockType;
		typedef NativeClockType::duration NativeClockDurationType;
		typedef NativeClockType::time_point NativeTimePointType;

		typedef int64_t ChessTickType;
		typedef chrono::duration< ChessTickType, centi > Duration;

		Clock()
		{
			Reset();
		}

		void Reset()
		{
			m_Start = m_Clock.now();
		}

		Duration Get() const
		{
			NativeTimePointType timeNow;
			timeNow = m_Clock.now();

			Duration dur;
			dur = chrono::duration_cast< Duration >( timeNow - m_Start );

			return dur;
		}

		void Start()
		{
			Reset();
		}

		void Test()
		{
			for ( int t = 0; t < 100; t++ )
			{
				chrono::milliseconds delay( 500 );
				this_thread::sleep_for( delay );

				cout << "Duration is now: " << Get().count() << endl;
			}
		}

	protected:
		NativeClockType m_Clock;
		NativeTimePointType m_Start;

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

		virtual int PieceValue() const = 0;
		virtual Moves GenerateMoves( const Square& source,
									 const Board& board ) const = 0;
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
			if ( m_Color == BLACK )
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

		int PieceValue() const
		{
			return 0;
		}

		Moves GenerateMoves( const Square& source, const Board& board ) const;

};

class Pawn : public Piece
{
	public:
		Pawn( Color color ) : Piece( color )
		{
			m_PieceType = PAWN;
			m_Letter = 'p';
		}

		int PieceValue() const
		{
			return 100;
		}

		Moves GenerateMoves( const Square& source, const Board& board ) const;

		virtual void AddAndPromote( Moves& moves, Move& m,
									const bool bIsPromote ) const;
};

class Bishop : public Piece
{
	public:
		Bishop( Color color ) : Piece( color )
		{
			m_Letter = 'b';
			m_PieceType = BISHOP;
		}
		int PieceValue() const
		{
			return 300;
		}

		Moves GenerateMoves( const Square& source, const Board& board ) const;

};

class Knight : public Piece
{
	public:
		Knight( Color color ) : Piece( color )
		{
			m_Letter = 'n';
			m_PieceType = KNIGHT;
		}

		int PieceValue() const
		{
			return 300;
		}

		Moves GenerateMoves( const Square& source, const Board& board ) const;

};

class Rook : public Piece
{
	public:
		Rook( Color color ) : Piece( color )
		{
			m_Letter = 'r';
			m_PieceType = ROOK;
		}

		int PieceValue() const
		{
			return 500;
		}

		Moves GenerateMoves( const Square& source, const Board& board ) const;

};

class Queen : public Piece
{
	public:
		Queen( Color color ) : Piece( color )
		{
			m_Letter = 'q';
			m_PieceType = QUEEN;
		}

		int PieceValue() const
		{
			return 900;
		}

		Moves GenerateMoves( const Square& source, const Board& board ) const;

};

class King : public Piece
{
	public:
		King( Color color ) : Piece( color )
		{
			m_Letter = 'k';
			m_PieceType = KING;
		}

		int PieceValue() const
		{
			return 1000000;
		}

		Moves GenerateMoves( const Square& source, const Board& board ) const;

	private:
		King();
};

/* PVS-Studio objects to this casting of bool to class type */
Pawn WhitePawn( WHITE ), BlackPawn( BLACK );        //-V601
Knight WhiteKnight( WHITE ), BlackKnight( BLACK );  //-V601
Bishop WhiteBishop( WHITE ), BlackBishop( BLACK );  //-V601
Rook WhiteRook( WHITE ), BlackRook( BLACK );        //-V601
Queen WhiteQueen( WHITE ), BlackQueen( BLACK );     //-V601
King WhiteKing( WHITE ), BlackKing( BLACK );        //-V601
NoPiece None;

class Square;

class Board : public Object
{
	public:
		Board()
		{
			Initialize();
		}

		void Initialize()
		{
			for ( unsigned int i = 0; i < MAX_FILES; i++ )
				for ( unsigned int j = 0; j < MAX_FILES; j++ )
				{
					Set( i, j, &None );
				}

		}

		const Piece* Set( int i, int j, const Piece* piece )
		{
			return ( Set( i + ( j << 3 ), piece ) );
		}

		const Piece* Set( int index, const Piece* piece )
		{
			return( m_Piece[ index ] = piece );
		}

		const Piece* Get( int i, int j ) const
		{
			return Get( i + ( j << 3 ) );
		}

		const Piece* Get( int index ) const
		{
			return ( m_Piece[ index ] );
		}

		const Piece* Set( const Square& s, const Piece* piece );
		const Piece* Get( const Square& s ) const;

		void Setup()
		{
			Initialize();

			for ( unsigned int i = 0 ; i < MAX_FILES; i ++ )
			{
				Set( i, 1, &WhitePawn );
				Set( i, 6, &BlackPawn );
			}

			Set( 0, 0, &WhiteRook );
			Set( 7, 0, &WhiteRook );
			Set( 0, 7, &BlackRook );
			Set( 7, 7, &BlackRook );

			Set( 1, 0, &WhiteKnight );
			Set( 6, 0, &WhiteKnight );
			Set( 1, 7, &BlackKnight );
			Set( 6, 7, &BlackKnight );

			Set( 2, 0, &WhiteBishop );
			Set( 5, 0, &WhiteBishop );
			Set( 2, 7, &BlackBishop );
			Set( 5, 7, &BlackBishop );

			Set( 3, 0, &WhiteQueen );
			Set( 4, 0, &WhiteKing ); //-V112
			Set( 3, 7, &BlackQueen );
			Set( 4, 7, &BlackKing ); //-V112
		}

		void Flip()
		{
			const Piece* pTemp;

			for ( unsigned int j = 0 ; j < ( MAX_FILES / 2 ); j++ )
				for ( unsigned int i = 0; i < MAX_FILES; i++ )
				{
					pTemp = Get( i, j );
					Set( i, j, Get( HIGHEST_FILE - i, HIGHEST_FILE - j ) );
					Set( HIGHEST_FILE - i, HIGHEST_FILE - j, pTemp );
				}
		}

		bool IsEmpty( const Square& square ) const;

		void Dump() const
		{
			/* Note this weird for loop, which terminates when an unsigned int
			 * goes below 0, i.e. gets real big
			 */
			for ( unsigned int j = ( MAX_FILES - 1 ); j < MAX_FILES; j-- ) //-V621
			{
				for ( unsigned int i = 0; i < MAX_FILES; i++ )
				{
					cout << Get( i, j )->Letter();
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
		const Piece* m_Piece[ MAX_FILES* MAX_FILES ];
};

class Square : public Object
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

		bool IsOnBoard() const
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

		operator string () const
		{
			string s;

			if ( IsOnBoard() )
			{
				s = ( char )( 'a' + i );
				s += ( char )( '1' + j );
			}
			else
			{ s = "-"; }

			return s;
		}

		void Dump() const
		{
			if ( IsOnBoard() )
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
			m_PromoteTo = &None;
			m_Score = 0;
		}

		Move( Piece* piece )
		{
			m_Piece = piece;
			m_PromoteTo = &None;
			m_Score = 0;
		}

		Move( const Piece* piece, const Square& source, const Square& dest )
		{
			m_Piece = piece;
			m_PromoteTo = &None;
			m_Source = source;
			m_Dest = dest;
			m_Score = 0;
		}

		Move( string sMove, Color color )
		{
			size_t moveLength = sMove.length();

			m_PromoteTo = &None;

			if ( moveLength != 4 && moveLength != 5 ) //-V112
			{ abort(); }

			m_Piece = &None;
			m_Source.I( sMove[0] - 'a' );
			m_Source.J( sMove[1] - '1' );
			m_Dest.I( sMove[2] - 'a' );
			m_Dest.J( sMove[3] - '1' );

			/* TODO: handle piece promotion -- we have to know this somehow
			 * from the color doing the moving
			 */
			if ( moveLength == 5 )
			{
				char cPromote = ( char ) tolower( ( int ) sMove[ 4 ] );

				switch ( cPromote )
				{

					case 'q' :
						m_PromoteTo = ( color == WHITE ) ? &WhiteQueen : &BlackQueen;
						break;

					case 'n' :
						m_PromoteTo = ( color == WHITE ) ? &WhiteKnight : &BlackKnight;
						break;

					case 'b' :
						m_PromoteTo = ( color == WHITE ) ? &WhiteBishop : &BlackBishop;
						break;

					case 'r' :
						m_PromoteTo = ( color == WHITE ) ? &WhiteRook : &BlackRook;
						break;

					default:
						break;
				}

			}

		}

		const Piece* GetPiece() const { return m_Piece; }
		void SetPiece( const Piece* val ) { m_Piece = val; }
		const Piece* GetPromoteTo() const { return m_PromoteTo; }

		void SetPromoteTo( const Piece* val )
		{
			m_PromoteTo = val;
			m_Score = val->PieceValue() ;
		}

		Square Source() const { return m_Source; }
		void Source( Square val ) { m_Source = val; }
		Square Dest() const { return m_Dest; }
		void Dest( Square val ) { m_Dest = val; }

		void Dump() const
		{
			if ( m_Piece == &None )
			{
				cout << "NoMove";
				return;
			}
			cout << m_Piece->Letter();
			m_Source.Dump();
			m_Dest.Dump();
			if ( m_PromoteTo != &None )
			{
				cout << tolower( m_PromoteTo->Letter() );
			}
		}

		operator string () const
		{
			return ( string )m_Source + ( string )m_Dest;
		}

		string TextWithPiece()
		{
			string letter;
			stringstream ss;

			ss << m_Piece->Letter();
			ss >> letter;

			return ( letter ) + ( string )m_Source + ( string )m_Dest;
		}

		int Score() const { return m_Score; }
		void Score( int val ) { m_Score = val; }

	protected:
		const Piece* m_Piece;
		Square m_Source, m_Dest;
		int m_Score;
		const Piece* m_PromoteTo;
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
			None.SetOtherColor( None ); //-V678

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
			m_Moves.reserve( DEFAULT_MOVES_SIZE );
		}

		void Add( const Move& move )
		{
			m_Moves.push_back( move );
		}

		size_t Count() const
		{
			return m_Moves.size();
		}

		void Make( const Move& move )
		{
			m_Moves.push_back( move );
		}

		void Unmake()
		{
			m_Moves.pop_back();
		}

		Moves operator+ ( const Moves& otherMoves )
		{
			m_Moves.insert( m_Moves.end(),
							otherMoves.m_Moves.begin(),
							otherMoves.m_Moves.end() );

			return *this;
		}

		void Sort()
		{
			sort( m_Moves.begin(), m_Moves.end() );
		}

		Move Random()
		{
			if ( m_Moves.size() == 0 )
			{ return NullMove; }

			return m_Moves.at( rand() % m_Moves.size() );
		}

		Move GetFirst() const
		{
			return m_Moves.front();
		}

		bool IsEmpty() const
		{
			return m_Moves.empty();
		}

		void Clear()
		{
			m_Moves.clear();
		}

		void Dump()
		{
			for ( auto move : m_Moves )
			{
				move.Dump();
				cout << " ";
			}
		}

		operator string () const
		{
			string s;

			for ( auto move : m_Moves )
			{
				s += ( string ) move;
				s += " ";
			}

			return s;
		}

		/** Attempt to add a particular attack to this Moves object.  If the
		 ** attempt succeeds, return true.
		 ** \param m The move with source information and piece information
		 ** \param board The board on which to make the move
		 ** \param id The row delta of the intended piece destination
		 ** \param jd The column delta of the intended piece destination
		 **/
		bool TryAttack( const Move& m, const Board& board, int id, int jd )
		{
			Move myMove = m;

			myMove.Dest( Square( id + myMove.Source().I(), jd + myMove.Source().J() ) );

			if ( myMove.Dest().IsOnBoard() )
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
					myMove.Score( None.PieceValue() );
					Add( myMove );
					return true;
				}
			}

			return false;
		}

		/** Attempt to add a particular ray (sliding) attack to this Moves object.
		 ** Generates a move for every successful step of the slide.
		 ** \return The number of actual moves generated in that slide
		 ** direction.  May be zero.
		 ** \param m The move with source information and piece information
		 ** \param board The board on which to make the move
		 ** \param id The row delta of the intended piece destination
		 ** \param jd The column delta of the intended piece destination
		 **/
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

		typedef vector< Move > MovesInternalType;
		typedef MovesInternalType::iterator iterator;
		typedef MovesInternalType::const_iterator const_iterator;

		iterator begin() { return m_Moves.begin(); }
		const_iterator begin() const { return m_Moves.begin(); }
		iterator end() { return m_Moves.end(); }
		const_iterator end() const { return m_Moves.end(); }

	protected:
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
			m_nMaterialScore = 0;
			m_nHalfMoves = 0;
			m_bBKR = m_bBQR = m_bWKR = m_bWQR = true;
			m_sEnPassant.Set( -1, -1 );
			m_Moves.Clear();
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

		/** Generates a new Position based on a previous, existing Position
		 ** as well as a Move to apply to that previous Position.  A new
		 ** Position is generated; the original Position remains untouched.
		 **/
		Position( const Position& position, const Move& move )
		{
		    /*
			m_Board = position.m_Board;
			m_ColorToMove = !(position.m_ColorToMove);
			m_nPly = position.m_nPly;
			m_nPly++;
			m_nLowerBound = position.m_nLowerBound;
			m_nUpperBound = position.m_nUpperBound;
			m_nMaterialScore = position.m_nMaterialScore;
			
			m_bWKR = position.m_bWKR;
			m_bWQR = position.m_bWQR;
			m_bBKR = position.m_bBKR;
			m_bBQR = position.m_bBQR;
			*/
			*this = position;

			m_Moves.Clear();
			m_nPly++;

			if ( &move == &NullMove )
			{ 
				return;
			}

			/* Move piece and optionally promote */
			if ( move.GetPromoteTo() == &None )
		    {
				m_nMaterialScore = position.GetScore() + ( m_Board.Get(
					move.Dest() )->PieceValue() ) *
					( m_ColorToMove == WHITE ? 1 : -1 );

				m_Board.Set( move.Dest().I(), move.Dest().J(),
					m_Board.Get( move.Source() ) );

			}
			else
			{
				m_nMaterialScore = position.GetScore() + 
					( move.GetPromoteTo()->PieceValue() + 
					m_Board.Get( move.Dest() )->PieceValue() ) *
					( m_ColorToMove == WHITE ? 1 : -1 );

				m_Board.Set( move.Dest().I(), move.Dest().J(),
					move.GetPromoteTo() );
			}

			m_Board.Set( move.Source().I(), move.Source().J(), &None );

			m_ColorToMove = !m_ColorToMove;
		}

		void GenerateMoves()
		{
			const Piece* pPiece;

			for ( unsigned int j = 0; j < MAX_FILES; j++ )
				for ( unsigned int i = 0; i < MAX_FILES; i++ )
				{
					pPiece = m_Board.Get( i, j );

					if ( ( pPiece != &None ) && ( pPiece->GetColor() == m_ColorToMove ) )
					{
						m_Moves = m_Moves + pPiece->GenerateMoves( Square( i, j ), m_Board );
					}
				}

			m_Moves.Sort();
		}

		const Moves &GetMoves()
		{
			if ( m_Moves.IsEmpty() )
				GenerateMoves();

			return m_Moves;
		}

		size_t CountMoves()
		{
			GetMoves();
			return m_Moves.Count();
		
		}

		const Board& GetBoard() const
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

		void Dump() const
		{
			m_Board.Dump();

			cout << "FEN: " << GetFEN() << endl;

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

			UpdateScore();

			return 0;
		}

		/** Cause the material score for this Position to be recalculated from
		 ** the material on the Board (not from a delta from a previous
		 ** Position).
		 **/
		void UpdateScore();

		string GetFEN() const
		{
			string s;
			const Piece* pPiece;
			int nSpaces = 0;

			for ( unsigned int j = MAX_FILES - 1; j != 0; j-- )
			{
				for ( unsigned int i = 0; i < MAX_FILES; i++ )
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

		operator string () { return GetFEN(); }

		unsigned int UpperBound() const { return m_nUpperBound; }
		void UpperBound( unsigned int val ) { m_nUpperBound = val; }

		unsigned int LowerBound() const { return m_nLowerBound; }
		void LowerBound( unsigned int val ) { m_nLowerBound = val; }

		Color ColorToMove() const { return m_ColorToMove; }
		void ColorToMove( Color val ) { m_ColorToMove = val; }

		int GetScore() const
		{
			return m_nMaterialScore;
		}
		void SetScore( int val )
		{
			m_nMaterialScore = val;
		}

	protected:
		Board   m_Board;
		Color   m_ColorToMove;
		unsigned int    m_nPly;
		int m_nLowerBound;
		int m_nUpperBound;
		int m_nMaterialScore;
		int m_nHalfMoves;
		// Virgin rooks; can tell whether any of the four rooks has been moved
		bool m_bWKR, m_bWQR, m_bBKR, m_bBQR;
		Square m_sEnPassant;
		/** Cached generated moves. */
		Moves m_Moves;
};

class EvaluatorBase : public Object
{
	public:
		virtual int Evaluate( Position& pos ) const = 0;

	protected:
		virtual int Bias( const Position &pos, int nResult ) const
		{
			return ( pos.ColorToMove() == WHITE ? nResult : -nResult );
		}
};

class EvaluatorSlowMaterial : public EvaluatorBase
{
	public:
		virtual int Evaluate( Position& pos ) const
		{
			Board board = pos.GetBoard();
			const Piece* piece;

			int nScore = 0;

			for ( unsigned int i = 0; i < MAX_SQUARES; i++ )
			{
				piece = board.Get( i );
				if ( piece != &None )
				{
					nScore += ( piece->PieceValue() *
								( ( piece->GetColor() == WHITE ) ? 1 : -1 ) );
				}
			}

			return Bias( pos, nScore );
		}
};

class EvaluatorMaterial : public EvaluatorSlowMaterial
{
	public:
		virtual int Evaluate( Position& pos ) const
		{
			return Bias( pos, pos.GetScore() );
		}
};

class EvaluatorWeighted : public EvaluatorBase
{
	public:
		virtual int Evaluate( Position& pos ) const
		{
			if ( m_Evaluators.empty() )
			{ abort(); }

			WeightsType::const_iterator weightIter;
			weightIter = m_Weights.begin();

			int nScore = 0;

			for ( EvaluatorsType::const_iterator iter = m_Evaluators.begin();
					iter != m_Evaluators.end();
					++iter )
			{
				nScore += ( int ) ( ( *iter )->Evaluate( pos ) * ( *weightIter ) );
				++weightIter;
			}

			return nScore;
		}

		void Add( EvaluatorBase& eval, float weight = 1.0f )
		{
			m_Evaluators.push_back( &eval );
			m_Weights.push_back( weight );
		}

	protected:
		typedef vector<float> WeightsType;
		typedef vector<EvaluatorBase*> EvaluatorsType;

		WeightsType m_Weights;
		EvaluatorsType m_Evaluators;
};

class EvaluatorSimpleMobility : public EvaluatorBase
{
	virtual int Evaluate( Position& pos ) const
	{
		return pos.CountMoves() ;
	}
	
};

class EvaluatorStandard : public EvaluatorWeighted
{
	public:
		EvaluatorStandard()
		{
			m_Weighted.Add( m_Material );
			m_Weighted.Add( m_SimpleMobility );
		}

		virtual int Evaluate( Position& pos ) const
		{
			return m_Weighted.Evaluate( pos );
		}

		EvaluatorMaterial m_Material;
		EvaluatorSimpleMobility m_SimpleMobility;
		EvaluatorWeighted m_Weighted;
};

typedef EvaluatorStandard Evaluator;

void Position::UpdateScore()
{
	EvaluatorSlowMaterial slow;
	SetScore( slow.Evaluate( *this ));
}

class SearcherBase : Object
{
	public:
		SearcherBase( Interface& interface ) :
			m_nNodesSearched( 0 )
		{
			m_bTerminated = true;
			m_pInterface = &interface;
		}

		~SearcherBase()
		{
			Stop();
		}

		virtual void Start( const Position& /*pos*/,
							Moves& /*mPrincipalVariation*/  )
		{

		}

		virtual void Stop()
		{

		}

		virtual int Evaluate( Position& pos )
		{
			return m_Evaluator.Evaluate( pos );
		}

	protected:
		void Notify( const string& s ) const;
		void Instruct( const string& s ) const;
		void Bestmove( const string& s ) const;


		void SearchComplete( )
		{
			stringstream ss;

			ss << "Principal variation found: " << ( string ) m_Result;
			Notify( ss.str() );

			ss.str( "" );
			ss << ( string ) m_Result.GetFirst();
			Bestmove( ss.str() );

			m_bTerminated = true;
		}

		virtual int Search( Position& pos ) = 0;

		int m_nNodesSearched;
		mutex m_Lock;
		atomic_bool m_bTerminated;
		Interface* m_pInterface;
		thread m_Thread;
		Evaluator m_Evaluator;
		Clock m_Clock;

		Moves m_Result;
		int m_Score;

		SearcherBase( const SearcherBase& ) {};
		SearcherBase() {};
};

class SearcherReporting : public SearcherBase
{
	public:
		SearcherReporting( Interface& interface ) :
			SearcherBase( interface ) {};

		virtual void Report() const
		{

		}

};

class SearcherAlphaBeta : public SearcherReporting
{
	public:
		typedef lock_guard< mutex > SearchLockType;

		SearcherAlphaBeta( Interface& interface ) :
			SearcherReporting( interface )
		{ }

		virtual void Start( const Position& pos )
		{
			Stop();

			SearchLockType guard( m_Lock );

			if ( m_bTerminated == false )
			{ return; }

			m_Result.Clear();
			m_bTerminated = false;
			m_Thread = thread( &SearcherAlphaBeta::Search, this, pos );
		}

		virtual void Stop()
		{
			SearchLockType guard( m_Lock );

			bool bCanJoin = m_Thread.joinable();
			m_bTerminated = true;
			if ( bCanJoin )
			{ m_Thread.join(); }
		}

	protected:

		virtual int Search( Position& pos )
		{
			Moves PV;

			pos.Dump();

			m_Score = alphaBetaMax( INT_MIN, INT_MAX, SEARCH_DEPTH, pos,
									PV );

			m_Result = PV;
			SearchComplete();
			return m_Score;
		}

		virtual int alphaBetaMax( int alpha, int beta, int depthleft,
								  Position& pos, Moves& pv )
		{
			if ( depthleft == 0 )
			{
				return Evaluate( pos );
			}

			Moves bestPV, currentPV;

			const Moves myMoves = pos.GetMoves();

			if ( myMoves.IsEmpty() )
			{
				Move nullMove;
				pv.Make( nullMove );
				return beta;
			}

			for ( auto& move : myMoves )
			{
				currentPV = pv;
				currentPV.Make( move );
				Position nextPos( pos, move );

				int score = alphaBetaMin( alpha, beta, depthleft - 1,
										  nextPos, currentPV );
				if( score >= beta )
				{
					Move nullMove;
					pv.Make( nullMove );
					return beta;
				}
				if( score > alpha )
				{
					alpha = score;
					bestPV = currentPV;
				}

				if ( m_bTerminated )
				{ break; }
			}

			pv = bestPV;
			return alpha;
		}

		virtual int alphaBetaMin( int alpha, int beta, int depthleft,
								  Position& pos, Moves& pv )
		{
			if ( depthleft == 0 )
			{
				return Evaluate( pos );
			}

			Moves bestPV, currentPV;

			Moves myMoves = pos.GetMoves();

			if ( myMoves.IsEmpty() )
			{
				Move nullMove;
				pv.Make( nullMove );

				return alpha;
			}

			for ( auto& move : myMoves )
			{
				currentPV = pv;
				currentPV.Make( move );
				Position nextPos( pos, move );

				int score = alphaBetaMax( alpha, beta, depthleft - 1,
										  nextPos, currentPV );
				if( score <= alpha )
				{
					Move nullMove;
					pv.Make( nullMove );
					return alpha;
				}
				if( score < beta )
				{
					beta = score;
					bestPV = currentPV;
				}

				if ( m_bTerminated )
				{ break; }
			}

			pv = bestPV;
			return beta;
		}

	protected:
		SearcherAlphaBeta();

};

typedef SearcherAlphaBeta Searcher;

const Piece* Board::Set( const Square& s, const Piece* piece )
{
	return Set( s.I(), s.J(), piece );
}

const Piece* Board::Get( const Square& s ) const
{
	return Get( s.I(), s.J() );
}

bool Board::IsEmpty( const Square& square ) const
{
	return ( Get( square.I(), square.J() ) == &None );
}

bool Piece::IsDifferent( const Square& dest, const Board& board ) const
{
	const Piece* piece = board.Get( dest );

	if ( piece == &None )
	{ return false; }

	return ( m_Color != piece->GetColor() );
}

bool Piece::IsDifferentOrEmpty( const Square& dest, const Board& board ) const
{
	const Piece* piece = board.Get( dest );

	if ( piece == &None )
	{ return true; }

	return ( m_Color != piece->GetColor() );
}

Moves NoPiece::GenerateMoves( const Square& /*source*/,
							  const Board& /*board*/ ) const
{
	Moves moves;
	return moves;
}

void Pawn::AddAndPromote( Moves& moves, Move& m, const bool bIsPromote ) const
{
	if ( bIsPromote )
	{
		Color color = m.GetPiece()->GetColor();
		if ( color == WHITE )
		{
			m.SetPromoteTo( &WhiteQueen );
			moves.Add( m );
			m.SetPromoteTo( &WhiteKnight );
			moves.Add( m );
			m.SetPromoteTo( &WhiteBishop );
			moves.Add( m );
			m.SetPromoteTo( &WhiteRook );
			moves.Add( m );
		}
		else
		{
			m.SetPromoteTo( &BlackQueen );
			moves.Add( m );
			m.SetPromoteTo( &BlackKnight );
			moves.Add( m );
			m.SetPromoteTo( &BlackBishop );
			moves.Add( m );
			m.SetPromoteTo( &BlackRook );
			moves.Add( m );
		}
	}
	else
	{ moves.Add( m ); }
}

Moves Pawn::GenerateMoves( const Square& source, const Board& board ) const
{
	Moves moves;
	Square dest = source;
	Color movingColor = GetColor();

	int sourceJ;
	sourceJ = source.J();

	const int d = m_Color ? 1 : -1;
	const bool bIsPromote = ( ( sourceJ == 1 ) && ( movingColor == BLACK ) ) ||
							( ( sourceJ == 6 ) && ( movingColor == WHITE ) );

	Move m( this, source, source );

	// Generate forward sliding moves
	dest.Change( 0, d );
	m.Dest( dest );

	if ( dest.IsOnBoard() && board.IsEmpty( m.Dest() ) )
	{
		AddAndPromote( moves, m, bIsPromote );

		// Two-square slide only from initial square
		if ( ( ( sourceJ == 1 ) && ( movingColor == WHITE ) ) ||
				( ( sourceJ == 6 ) && ( movingColor == BLACK ) ) )
		{
			dest.Change( 0, d );
			m.Dest( dest );

			if ( board.IsEmpty( m.Dest() ) )
			{ AddAndPromote( moves, m, bIsPromote ); }
		}
	}

	// Generate capture moves
	dest = source.Add( -1, d );
	if ( dest.IsOnBoard() && IsDifferent( dest, board ) )
	{
		m.Dest( dest );
		AddAndPromote( moves, m, bIsPromote );
	}

	dest = source.Add( 1, d );
	if ( dest.IsOnBoard() && IsDifferent( dest, board ) )
	{
		m.Dest( dest );
		AddAndPromote( moves, m, bIsPromote );
	}

	return moves;
}

Moves Knight::GenerateMoves( const Square& source, const Board& board ) const
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

Moves Bishop::GenerateMoves( const Square& source, const Board& board ) const
{
	Moves moves;
	Move m( this, source, source );

	moves.TryRayAttack( m, board, 1, 1 );
	moves.TryRayAttack( m, board, 1, -1 );
	moves.TryRayAttack( m, board, -1, 1 );
	moves.TryRayAttack( m, board, -1, -1 );

	return moves;
}

Moves Rook::GenerateMoves( const Square& source, const Board& board ) const
{
	Moves moves;
	Move m( this, source, source );

	moves.TryRayAttack( m, board, 0, 1 );
	moves.TryRayAttack( m, board, 0, -1 );
	moves.TryRayAttack( m, board, -1, 0 );
	moves.TryRayAttack( m, board, 1, 0 );

	return moves;
}

Moves King::GenerateMoves( const Square& source, const Board& board ) const
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

Moves Queen::GenerateMoves( const Square& source, const Board& board ) const
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

class Interface : Object
{
	public:
		enum ProtocolType
		{
			PROTOCOL_XBOARD,
			PROTOCOL_UCI
		};

		Interface( istream* in = &cin, ostream* out = &cout ) :
			m_In( in ),
			m_Out( out ),
			m_bShowThinking( false ),
			m_pGame( new Game )
		{
			m_pSearcher = shared_ptr< Searcher >( new Searcher( *this ) );
		}

		~Interface()
		{
		}

		ostream* GetOut() const { return m_Out; }
		void SetOut( ostream* val ) { m_Out = val; }

		istream* GetIn() const { return m_In; }
		void SetIn( istream* val ) { m_In = val; }

		void Run()
		{
			m_Out->setf( ios::unitbuf );

			string sInputLine;

			RegisterAll( );

			for ( ;; )
			{
				getline( *m_In, sInputLine );

				if ( sInputLine.empty() )
				{ break; }

				LockGuardType guard( m_Lock );

				Execute( sInputLine );
			}
		}

		typedef lock_guard< mutex > LockGuardType;

		mutex& GetLock()
		{
			return m_Lock;
		}

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
					( *m_Out ) << sParams << endl;
			}
		}

		INTERFACE_PROTOTYPE( Instruct )
		{
			( *m_Out ) << sParams << endl;
		}

		INTERFACE_PROTOTYPE( Bestmove )
		{
			stringstream ss;

			ss << "bestmove " << sParams;
			Instruct( ss.str() );
		}

	protected:

		void RegisterCommand( const string& sCommand,
							  INTERFACE_FUNCTION_TYPE( pfnCommand ) )
		{
			m_CommandMap[ sCommand ] = pfnCommand;
		};

		void RegisterAll()
		{
			RegisterCommand( "uci",     &Interface::UCI );
			RegisterCommand( "quit",    &Interface::Quit );
		}

		INTERFACE_PROTOTYPE( UCI )
		{
			RegisterUCI( sParams );

			Instruct( "id name Ippon" );
			Instruct( "id author John Byrd" );
			Instruct( "uciok" );
		}

		INTERFACE_PROTOTYPE_NO_PARAMS( RegisterUCI )
		{
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

		INTERFACE_PROTOTYPE_NO_PARAMS( DebugUCI )
		{
			Notify( "DebugUCI not yet implemented" );
		}

		INTERFACE_PROTOTYPE_NO_PARAMS( IsReady )
		{
			// stop any pondering or loading
			Instruct( "readyok" );
		}

		INTERFACE_PROTOTYPE( SetOption )
		{
			stringstream ss;
			ss << "SetOption parameters: " << sParams;

			Notify( "SetOption not yet implemented" );
		}

		INTERFACE_PROTOTYPE( UCIGo )
		{
			stringstream ss;

			ss << "Go parameters: " << sParams;
			Notify( ss.str() );

			m_pSearcher->Start( *( m_pGame->GetPosition() ) );
		}

		INTERFACE_PROTOTYPE( UCIPosition )
		{
			stringstream ss( sParams );
			string sType;

			while ( ss >> sType )
			{

				if ( sType == "fen" )
				{
					string sArg, sFen;
					const int fenArgs = 6;

					for ( int t = 0; t < fenArgs; t++ )
					{
						ss >> sArg;
						if ( t != 0 )
						{
							sFen.append( " " );
						}
						sFen.append( sArg );
					}

					Position pos;
					pos.SetFEN( sFen );
					m_pGame->SetPosition( pos );

					Notify( "New position: " );
					Notify( sFen );
				}

				if ( sType == "startpos" )
				{
					m_pGame->New();
				}

				if ( sType == "moves" )
				{
					string sMove;

					while ( ss >> sMove )
					{
						Position* pLast = m_pGame->GetPosition();
						Move nextMove( sMove, pLast->ColorToMove() );

						Position nextPos( *pLast, nextMove );
						m_pGame->SetPosition( nextPos );
					}
				}
			}
		}

		INTERFACE_PROTOTYPE_NO_PARAMS( New )
		{
			m_pGame->New();
		}

		INTERFACE_PROTOTYPE_NO_PARAMS( Stop )
		{
			m_pSearcher->Stop();
		}

		INTERFACE_PROTOTYPE_NO_PARAMS( Ponderhit )
		{
			Notify( "Ponderhit not yet implemented" );
		}

		INTERFACE_PROTOTYPE_NO_PARAMS( Quit )
		{
			Notify( "Engine exiting" );
			exit( 0 );
		}

		int TimeToSeconds( const string& sTime )
		{
			size_t nColon = 0;
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

		void Execute( const string& sCommand )
		{
			string sParams, sVerb;

			stringstream ss;
			ss.str( sCommand );

			ss >> sVerb;

			if ( sVerb.length() < sCommand.length() )
			{ sParams = sCommand.substr( sVerb.length() + 1, MAX_COMMAND_LENGTH ); }

			InterfaceFunctionType ic = m_CommandMap[ sVerb ];

			if ( ic )
			{
				( this->*ic )( sParams );
			}
			else
			{
				stringstream unk;
				unk << "Unknown command: " << sCommand ;
				Notify( unk.str() );
			}
		}

	protected:
		istream* m_In;
		ostream* m_Out;

		mutex m_Lock;

		Moves m_PrincipalVariation;

		ProtocolType m_Protocol;
		int m_Protover;
		bool m_bShowThinking;
		bool m_bPonder;
		shared_ptr< Game > m_pGame;
		shared_ptr< Searcher > m_pSearcher;

	protected:
		unordered_map< string, InterfaceFunctionType > m_CommandMap;
};

void SearcherBase::Notify( const string& s ) const
{
	Interface::LockGuardType guard( m_pInterface->GetLock() );
	m_pInterface->Notify( s );
}

void SearcherBase::Instruct( const string& s ) const
{
	Interface::LockGuardType guard( m_pInterface->GetLock() );
	m_pInterface->Instruct( s );
}

void SearcherBase::Bestmove( const string& s ) const
{
	Interface::LockGuardType guard( m_pInterface->GetLock() );
	m_pInterface->Bestmove( s );
}


int main( int , char** )
{
	Clock c;
	PieceInitializer pieceInitializer;
	Interface i;


	/* 
	stringstream ss;

	ss.str("uci\nucinewgame\ngo\n");
	i.SetIn( &ss ); */

	i.Run();

	/*
	chrono::seconds um( 60 );
	this_thread::sleep_for( um );
	*/

	return 0;
}

