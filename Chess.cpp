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

const int INFINITY = 9999999;


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
			return (m_tTotal + (clock() - m_tLastStart )) * 100 / CLOCKS_PER_SEC;

		return m_tTotal * 100 / CLOCKS_PER_SEC;
	}

	operator int ()
	{
		return Get();
	}

	void Start()
	{
		if ( m_bIsRunning )
			return;

		m_tLastStart = clock();

		m_bIsRunning = true;
	}

	void Stop()
	{
		m_bIsRunning = false;
		m_tTotal +=  (clock() - m_tLastStart );
		m_tLastStart = (clock_t)0;

	}

protected:
	clock_t m_tLastStart;
	clock_t m_tTotal;
	bool m_bIsRunning;
};

Clock gClock;

class Lock : Object
{
};

class Thread : Object
{
public:
	void *m_pContext;
	bool IsRunning( const bool bRunning = false )
	{
		return bRunning;
	}
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
	virtual Moves GenerateMoves( const Square &source, const Board &board ) = 0;
	virtual bool IsDifferent( const Square &dest, const Board &board ) const;
	virtual bool IsDifferentOrEmpty( const Square &dest, const Board &board ) const;

	void SetOtherColor( Piece &otherPiece )
	{
		m_pOtherColor = &otherPiece;
	}

	Piece *InvertColor()
	{
		return m_pOtherColor;
	}

	char Letter() const { 
		// in Forsyth-Edwards notation, white pieces are uppercase
		if ( !m_Color )
			return m_Letter;

		return (char) toupper( m_Letter );
	}

	Color GetColor() const
	{ 
		return m_Color;
	}

	void SetColor(Color val)
	{ 
		m_Color = val;
	}


protected:
	char	m_Letter;

	Color	m_Color;
	Piece	*m_pOtherColor;
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

	Moves GenerateMoves( const Square &source, const Board &board );

};

class Pawn : public Piece
{
public:
	Pawn(Color color) : Piece( color )
	{
		m_PieceType = PAWN;
		m_Letter = 'p';
	}

	int PieceValue()
	{
		return 100;
	}

	Moves GenerateMoves( const Square &source, const Board &board );

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

	Moves GenerateMoves( const Square &source, const Board &board );

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

	Moves GenerateMoves( const Square &source, const Board &board );

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

	Moves GenerateMoves( const Square &source, const Board &board );

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

	Moves GenerateMoves( const Square &source, const Board &board );

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

	Moves GenerateMoves( const Square &source, const Board &board );
};

Pawn WhitePawn( WHITE ), BlackPawn( BLACK );
Knight WhiteKnight( WHITE ), BlackKnight( BLACK );
Bishop WhiteBishop( WHITE), BlackBishop( BLACK );
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
		for (int i = 0; i < MAX_FILES; i++ )
			for ( int j = 0; j < MAX_FILES; j++ )
				m_Piece[ i ][ j ] = &None;

	}

	Piece *Set( int i, int j, Piece *piece )
	{
		return ( m_Piece[ i ][ j] = piece );
	}

	Piece *Get( int i, int j ) const
	{
		return m_Piece[ i ][ j ];
	}

	Piece *Set( const Square &s, Piece *piece );
	Piece *Get( const Square &s ) const;

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
		Piece *pTemp;
		

		for ( int j = 0 ; j < ( MAX_FILES / 2 ); j++ )
			for ( int i = 0; i < MAX_FILES; i++ )
			{
				pTemp = m_Piece[i][j];
				m_Piece[i][j] = m_Piece[HIGHEST_FILE - i][HIGHEST_FILE - j];
				m_Piece[HIGHEST_FILE - i][HIGHEST_FILE - j] = pTemp;
			}
	}

	bool IsEmpty( const Square &square ) const;

	void Dump()
	{
		for ( int j = ( MAX_FILES - 1); j >= 0; j-- )
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
	Piece *m_Piece[ MAX_FILES ][ MAX_FILES ];
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

	Square( const string &s )
	{
		i = s[0] - '0' + 1;
		j = s[1] - 'a';
	}

	bool IsLegal() const
	{
		return ( (( i & ~7) == 0 ) && ((j & ~7) == 0));
	}

	int I() const
	{ 
		return i;
	}
	
	void I(int val)
	{ 
		i = val;
	}

	int J() const 
	{ 
		return j;
	}

	void J(int val)
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
			s = (char)('a' + i);
			s += (char)('1' + j);
		}
		else
			s = "-";

		return s;
	}

	void Dump()
	{
		if ( IsLegal() )
		{
			cout << (char)('a' + i);
			cout << (char)('1' + j);
		}
		else
			cout << "??";
	}

	Square Change( int ip, int jp )
	{
		i += ip;
		j += jp;

		return *this;
	}

	Square Change( const Square &s )
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

	Square Add( const Square &s ) const
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
	int	i; // file
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

	Move( Piece *piece )
	{
		m_Piece = piece;
		m_Score = 0;
	}

	Move( Piece *piece, const Square source, const Square dest )
	{
		m_Piece = piece;
		m_Source = source;
		m_Dest = dest;
		m_Score = 0;
	}

	Piece *GetPiece() const { return m_Piece; }
	void SetPiece(Piece * val) { m_Piece = val; }
	Square Source() const { return m_Source; }
	void Source(Square val) { m_Source = val; }
	Square Dest() const { return m_Dest; }
	void Dest(Square val) { m_Dest = val; }

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

		s = (string)m_Source + (string)m_Dest;

		return s;
	}

	int Score() const { return m_Score; }
	void Score(int val) { m_Score = val; }

protected:
	Piece *m_Piece;
	Square m_Source, m_Dest;
	int m_Score;
};

bool operator< (const Move& left, const Move& right)
{
	if  (left.Score() > right.Score()) return true;
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

		NullMove.Source( Square( -99, -99 ));
		NullMove.Dest( Square( -99, -99 ));
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
	}

	void Add( const Move &move )
	{
		m_Moves.push_back( move );
	}

	Moves Moves::operator+ ( Moves otherMoves )
	{
		m_Moves.insert( m_Moves.end(), 
			otherMoves.m_Moves.begin(), 
			otherMoves.m_Moves.end());

		return *this;
	}

	Move Random()
	{
		if ( m_Moves.size() == 0 )
			return NullMove;

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
			(*it).Dump();
			cout << " ";
		}
	}

	operator string ()
	{
		string s;

		vector< Move >::iterator it;

		for ( it = m_Moves.begin(); it != m_Moves.end(); it++ )
		{
			s += (string)*it;
			s += " ";
		}

		return s;
	}

	bool TryAttack( const Move &m, const Board &board, int id, int jd )
	{
		Move myMove = m;

		myMove.Dest( Square( id + myMove.Source().I(), jd + myMove.Source().J() ));

		if ( myMove.Dest().IsLegal() )
		{
			// Captures are more interesting than moves.
			if ( myMove.GetPiece()->IsDifferent( myMove.Dest(), board ))
			{
				myMove.Score( board.Get( myMove.Dest())->PieceValue() );
				Add( myMove );
				return true;
			}
			else if ( board.IsEmpty( myMove.Dest() ))
			{
				Add( myMove );
				return true;
			}
		}

		return false;
	}

	unsigned int TryRayAttack( const Move &m, const Board &board, int id, int jd )
	{
		int nAttacks = 0;

		Square sAttacked;

		int i = id;
		int j = jd;

		while ( TryAttack( m, board, i, j ) )
		{
			sAttacked.Set( m.Source().I() + i, m.Source().J() + j );

			if ( board.Get( sAttacked ) != &None )
				break;  // attack is over; we hit a piece of this or the other color

			i += id;
			j += jd;
			nAttacks++;
		}

		return nAttacks;
	}

	vector< Move > GetMoves() const { return m_Moves; }
	void SetMoves(vector< Move > val) { m_Moves = val; }


protected:
	vector< Move > m_Moves;
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

	Position( const string &sFEN )
	{
		Initialize();
		SetFEN( sFEN );
	}

	Position( const Position &position, const Move &move )
	{
		Initialize();
		m_Board = position.m_Board;
		m_nPly = position.m_nPly + 1;

		if ( &move == &NullMove )
			return;

		m_nScore = position.Score() + ( m_Board.Get( move.Dest())->PieceValue()) * 
			( m_ColorToMove ? -1 : 1);

		m_Board.Set( move.Source().I(), move.Source().J(), &None );
		m_Board.Set( move.Dest().I(), move.Dest().J(), move.GetPiece());

		m_ColorToMove = !position.m_ColorToMove;
	}

	Moves GenerateMoves() const
	{
		Piece *pPiece;
		Moves moves;
		for ( int j = 0; j < MAX_FILES; j++ )
			for ( int i = 0; i < MAX_FILES; i++ )
			{
				pPiece = m_Board.Get( i, j );

				if (( pPiece != &None ) && ( pPiece->GetColor() == m_ColorToMove ))
				{
					moves = moves + pPiece->GenerateMoves( Square( i, j), m_Board );
				}
			}

		vector <Move> vMove = moves.GetMoves();
		sort( vMove.begin(), vMove.end() );
		moves.SetMoves( vMove );

		return moves;
	}

	Board GetBoard() const
	{ 
		return m_Board;
	}
	
	void SetBoard(Board val)
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
		cout << endl;
		cout << "Ply: ";
		cout << m_nPly;
		cout << endl;
	}

	int SetFEN( const string &sFEN )
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

		m_ColorToMove = ( sToMove == "w");

		stringstream ssVirgins( sVirgins );

		m_bWKR = m_bWQR = m_bBKR = m_bBQR = false;

		while (ssVirgins >> c )
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
		m_nPly = (nMoves - 1) * 2 + ( m_ColorToMove ? 0 : 1 );

		return 0;
	}

	string GetFEN()
	{
		string s;
		Piece *pPiece;
		int nSpaces = 0;

		for ( int j = MAX_FILES - 1; j >= 0; j--)
		{
			for ( int i = 0; i < MAX_FILES; i++ )
			{
				pPiece = m_Board.Get( i, j );

				if (( pPiece != &None ) && ( nSpaces > 0 ))
				{
					s += (char)nSpaces + '0';
					nSpaces = 0;
				}

				if ( pPiece != &None )
					s += pPiece->Letter();
				else
					nSpaces++;

			}

			if ( nSpaces > 0 )
				s += (char) nSpaces + '0';

			nSpaces = 0;
			if ( j != 0 )
				s += '/';
		}

		if ( m_ColorToMove )
			s += " w ";
		else
			s += " b ";

		if ( !( m_bWKR || m_bWQR || m_bBKR || m_bBQR ))
			s += "-";
		else 
		{
			if ( m_bWKR )
				s += "K";
			if ( m_bWQR )
				s += "Q";
			if ( m_bBKR )
				s += "k";
			if ( m_bBQR )
				s += "q";
		}

		stringstream ss;

		ss << " ";
		ss << (string)m_sEnPassant;
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
	void UpperBound(unsigned int val) { m_nUpperBound = val; }

	unsigned int LowerBound() const { return m_nLowerBound; }
	void LowerBound(unsigned int val) { m_nLowerBound = val; }

	Color ColorToMove() const { return m_ColorToMove; }
	void ColorToMove(Color val) { m_ColorToMove = val; }

	int Score() const { return m_nScore; }
	void Score(int val) { m_nScore = val; }

protected:
	Board	m_Board;
	Color	m_ColorToMove;
	unsigned int	m_nPly;
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
	  m_pThinker( NULL ),
	  m_nNodesSearched( 0 )
	{
	
	}

	Searcher( Thread *pThinker ) :
		m_nNodesSearched( 0 ),
		m_pThinker( pThinker )
	{

	}
	
	virtual int Search( const Position &pos, 
		Moves &mPrincipalVariation ) = 0;

	virtual bool IsRunning()
	{
		static int nDelay = 0;

		if ( ++nDelay < 10 )
			return true;

		nDelay = 0;

		if ( m_pThinker == NULL )
			return false;

		return ( m_pThinker->IsRunning() );
	}

	virtual int Evaluate( const Position &pos )
	{
		int nScore = 0;

		for ( int i = 0; i < MAX_FILES; i++ )
			for ( int j = 0; j < MAX_FILES; j++ )
			{
				Piece *piece;

				piece = pos.GetBoard().Get( i, j );

				nScore += (piece->PieceValue() *
					(( piece->GetColor() == WHITE) ? 1 : -1 ));

			}

		int nBias = ( pos.ColorToMove() == WHITE ) ? 1 : -1;

		return nScore * nBias;
	}

protected:
	int m_nNodesSearched;
	Thread *m_pThinker;

};

class SearcherAlphaBeta : Searcher
{
public:

	SearcherAlphaBeta()
	{

	}

	SearcherAlphaBeta( Thread *pThinker ) :
	  Searcher( pThinker )
	  {

	  }

	virtual int AlphaBeta( const Position &pos, 
		unsigned int depth,
		int alpha,
		int beta,
		vector< Move > &mPrincipalVariation )
	{
		int nEval = Evaluate( pos );

		Moves mCurrentMoves = pos.GenerateMoves();
		vector< Move > vCurrentMoves = mCurrentMoves.GetMoves();

		if ( depth == 0 )
		{
			m_nNodesSearched++;
			return nEval + vCurrentMoves.size();
		}

		if ( nEval > 8000 )
			return 100000;

		if ( nEval < -8000 )
			return -100000;

		if ( vCurrentMoves.size() == 0 )
			return -WhiteKing.PieceValue();

		vector< Move > mCurrentVariation, mBestVariation;

		for ( vector< Move >::iterator it = vCurrentMoves.begin(); it != vCurrentMoves.end(); it++ )
		{
			Position posNext( pos, *it );

			int nBlackOrWhite;
			nBlackOrWhite = ( (*it).GetPiece()->GetColor() == WHITE ) ? 1 : 0;

			mCurrentVariation = mPrincipalVariation;
			mCurrentVariation.push_back( *it );

			// A high score here is good for the currently moving piece
			int score = -AlphaBeta( posNext, depth - 1, -beta, -alpha, mCurrentVariation );

			if ( mBestVariation.empty() )
				mBestVariation = mCurrentVariation;

			if ( IsRunning() == false )
				break;

			bool bBetaCutoff, bAlphaCutoff;

			bAlphaCutoff = false;

			if ( score > alpha )
			{
				alpha = score;
				bAlphaCutoff = true;
			}

			bBetaCutoff = ( beta <= alpha );

			if ( bBetaCutoff )
				break;

			if ( bAlphaCutoff )
			{
				alpha = score; // alpha acts like max in MiniMax
				mBestVariation = mCurrentVariation;

				// reporting
				Moves moves;
				moves.SetMoves( mBestVariation );
				if (depth > 2)
				{
					cout << depth << " " << score << " " << gClock.Get();
					cout << " " << m_nNodesSearched << " " << (string)moves << endl;
					if ( moves.GetMoves().size() == 0 )
					{
						cout << "Nonexistent move list " << endl;
					}
				}
			}
		}

		mPrincipalVariation = mBestVariation;
		return alpha;
	}

	virtual int Search( const Position &pos, 
		Moves &mPrincipalVariation )
	{
		vector <Move> m = mPrincipalVariation.GetMoves();
		int value = AlphaBeta( pos, 5, -INFINITY, INFINITY, m );
		mPrincipalVariation.SetMoves( m );
		return value;
	}

};

class SearcherMTD : Searcher
{

	virtual int AlphaBetaWithMemory( const Position &pos, 
		int alpha, 
		int beta,
		int d,
		Moves &mPrincipalVariation )
	{
		// TBD

		pos;
		alpha;
		beta;
		d;
		mPrincipalVariation;

		return 0;
	}


	virtual int MTDF( const Position &pos, Moves &mPrincipalVariation, int f, int d )
	{
		int g = f;
		int upperbound = +INFINITY;
		int lowerbound = -INFINITY;
		int beta;

		do {
			if ( g == lowerbound )
				beta = g + 1;
			else
				beta = g;

			g = AlphaBetaWithMemory( pos, beta - 1, beta, d, mPrincipalVariation );

			if ( g < beta )
				upperbound = g;
			else
				lowerbound = g;

		} while ( lowerbound < upperbound );

		return g;
	}

	virtual int Search( const Position &pos, 
		Moves &mPrincipalVariation )
	{
		int nFirstGuess = 0;

		for ( int d = 1; d < 5; d++ )
		{
			nFirstGuess = MTDF( pos, mPrincipalVariation, nFirstGuess, d );
			// break early if time is up
		}

		return nFirstGuess;
	}
};

Piece *Board::Set( const Square &s, Piece *piece )
{
	return ( m_Piece[ s.I() ][ s.J() ] = piece );
}

Piece *Board::Get( const Square &s ) const
{
	return m_Piece[ s.I() ][ s.J()];
}


bool Board::IsEmpty( const Square &square ) const
{
	return ( m_Piece[ square.I() ][ square.J()] == &None );
}

bool Piece::IsDifferent( const Square &dest, const Board &board ) const
{
	Piece *piece = board.Get( dest );

	if ( piece == &None)
		return false;

	return ( m_Color != piece->GetColor() );
}

bool Piece::IsDifferentOrEmpty( const Square &dest, const Board &board ) const
{
	Piece *piece = board.Get( dest );

	if ( piece == &None)
		return true;

	return ( m_Color != piece->GetColor() );
}


Moves NoPiece::GenerateMoves( const Square &source, const Board &board )
{
	source;
	board;

	Moves moves;
	return moves;
}

Moves Pawn::GenerateMoves( const Square &source, const Board &board )
{
	Moves moves;
	Square dest = source;

	int d = m_Color ? 1 : -1;

	Move m( this, source, source );

	// Generate forward sliding moves
	dest.Change( 0, d );
	m.Dest( dest );

	if ( board.IsEmpty( m.Dest() ))
	{
		moves.Add( m );

		// Two-square slide only from initial square
		if ((( source.J() == 1) && (GetColor() == WHITE)) ||
			(( source.J() == 6) && (GetColor() == BLACK)))
		{
			dest.Change( 0, d );
			m.Dest( dest );

			if ( board.IsEmpty( m.Dest() ))
				moves.Add( m );

		}
	}

	// Generate capture moves
	dest = source.Add( -1, d );
	if ( dest.IsLegal() && IsDifferent( dest, board ))
	{
		m.Dest( dest );
		moves.Add( m );
	}

	dest = source.Add( 1, d );
	if ( dest.IsLegal() && IsDifferent( dest, board ))
	{
		m.Dest( dest );
		moves.Add( m );
	}

	return moves;
}

Moves Knight::GenerateMoves( const Square &source, const Board &board )
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

Moves Bishop::GenerateMoves( const Square &source, const Board &board )
{
	Moves moves;
	Move m( this, source, source );

	moves.TryRayAttack( m, board, 1, 1 );
	moves.TryRayAttack( m, board, 1, -1 );
	moves.TryRayAttack( m, board, -1, 1 );
	moves.TryRayAttack( m, board, -1, -1 );

	return moves;
}

Moves Rook::GenerateMoves( const Square &source, const Board &board )
{
	Moves moves;
	Move m( this, source, source );

	moves.TryRayAttack( m, board, 0, 1 );
	moves.TryRayAttack( m, board, 0, -1 );
	moves.TryRayAttack( m, board, -1, 0 );
	moves.TryRayAttack( m, board, 1, 0);

	return moves;
}

Moves King::GenerateMoves( const Square &source, const Board &board )
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

Moves Queen::GenerateMoves( const Square &source, const Board &board )
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
	moves.TryRayAttack( m, board, 1, 0);

	return moves;
}

class Interface;

#define CMD_PARAMS Interface *pSelf, string sParams

class InterfaceState 
{
public:
	Interface *m_pInterface;
	void (*m_fnCallback)(CMD_PARAMS);
};

class Interface;

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
	void Time(int val) { m_Time = val; }
	int OTime() const { return m_OTime; }
	void OTime(int val) { m_OTime = val; }
	Position *GetPosition() { return &m_Position; }
	void SetPosition( Position &pos ) { m_Position = pos; }

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

	Interface( istream *in = &cin, ostream *out = &cout )
	{
		m_In = in;
		m_Out = out;
		m_pThinker = NULL;
		m_pGame = new Game;
		m_bShowThinking = false;
	}

	~Interface()
	{
		delete  m_pGame;
	}

	static void RegisterCommand( const string &sCommand,
		Interface *iSelf,
		void (*cmd)(CMD_PARAMS) )
	{
		InterfaceState is;

		is.m_pInterface = iSelf;
		is.m_fnCallback = cmd;
		iSelf->m_CommandMap[ sCommand ] = is;
	};

	static void RegisterXboard( CMD_PARAMS )
	{
		RegisterCommand( "accepted",	pSelf,	Interface::Accepted );
		RegisterCommand( "new",			pSelf,	Interface::New );
		RegisterCommand( "protover",	pSelf,	Interface::Protover );
		RegisterCommand( "ping",		pSelf,	Interface::Ping );
		RegisterCommand( "usermove",	pSelf,	Interface::Usermove );
		RegisterCommand( "setboard",	pSelf,	Interface::Setboard );
		RegisterCommand( "level",		pSelf,	Interface::Level );
		RegisterCommand( "st",			pSelf,	Interface::St );
		RegisterCommand( "time",		pSelf,	Interface::Time );
		RegisterCommand( "otim",		pSelf,	Interface::OTim );
		RegisterCommand( "?",			pSelf,  Interface::MoveNow );
		RegisterCommand( "post",		pSelf,  Interface::Post );
		RegisterCommand( "nopost",		pSelf,  Interface::NoPost );
		RegisterCommand( "hard",		pSelf,  Interface::Hard );
		RegisterCommand( "easy",		pSelf,  Interface::Easy );
		RegisterCommand( "force",		pSelf,  Interface::Force );
		RegisterCommand( "go",			pSelf,	Interface::Go );
		RegisterCommand( "black",		pSelf,  Interface::Black );
		RegisterCommand( "white",		pSelf,  Interface::White );
	}

	static void RegisterAll( Interface *pSelf)
	{
		RegisterCommand( "xboard",	pSelf,	Interface::Xboard );
		RegisterCommand( "quit",	pSelf,	Interface::Quit );
	}

	static void Report( CMD_PARAMS )
	{
		*(pSelf->m_Out) << "# " << sParams << endl;
	}

	static void Ping( CMD_PARAMS )
	{
		stringstream ss;
		ss.str( sParams );

		int n;
		ss >> n;

		*(pSelf->m_Out) << "pong " << n << endl;
	}

	static void StopThinking( CMD_PARAMS )
	{
		Thread *pThinker = pSelf->m_pThinker;

		if ( pThinker == NULL )
		{
			Report(pSelf, "Not currently thinking");
		}
		else
		{
			pThinker->IsRunning( false );
			delete pThinker;
			pSelf->m_pThinker = NULL;
		}

		Report(pSelf, "Thinking stopped");

	}

	static void Black( CMD_PARAMS )
	{
		Game *pGame = pSelf->m_pGame;

		Position *pPosition = pGame->GetPosition();

		pPosition;
	}

	static void White( CMD_PARAMS )
	{
		Game *pGame = pSelf->m_pGame;

		Position *pPosition = pGame->GetPosition();

		pPosition;
	}


	static void Force( CMD_PARAMS )
	{
		StopThinking( pSelf, sParams );
		Time( pSelf, "0");
		OTim( pSelf, "0");

		Report(pSelf, "Force");
	}

	static void MoveNow( CMD_PARAMS )
	{
		StopThinking( pSelf, sParams );
		Move myMove;
		myMove = (pSelf->m_PrincipalVariation).GetFirst();

		*(pSelf->m_Out) << "move " << (string)myMove << endl;
		*(pSelf->m_Out) << "# PV " << (string)pSelf->m_PrincipalVariation << endl;
	}

	static void Post( CMD_PARAMS )
	{
		pSelf->m_bShowThinking = true;
		Report(pSelf, "Thinking will be shown");
	}

	static void NoPost( CMD_PARAMS )
	{
		pSelf->m_bShowThinking = false;
		Report(pSelf, "Thinking will not be shown");
	}

	static void Hard( CMD_PARAMS )
	{
		pSelf->m_bPonder = true;
		Report(pSelf, "Engine will ponder on opponent's time");
	}

	static void Easy( CMD_PARAMS )
	{
		pSelf->m_bPonder = false;
		Report(pSelf, "Engine will not ponder on opponent's time");
	}

	static void Protover( CMD_PARAMS )
	{
		stringstream ss;
		ss.str( sParams );

		ss >> pSelf->m_Protover;

		if ( pSelf->m_Protover >= 2 )
		{
			string sResponse; 

			*(pSelf->m_Out) << "feature ping=1 setboard=1 playother=1 usermove=1 analyze=0" << endl;
		}

	}

	static void Accepted( CMD_PARAMS )
	{
		*(pSelf->m_Out) << "# Unknown parameter to accepted" << endl;
	}

	static void New( CMD_PARAMS )
	{
		pSelf->m_pGame->New();
		Report(pSelf, "New game");
	}

	static void Setboard( CMD_PARAMS )
	{
		Position *pPosition;

		pPosition = pSelf->m_pGame->GetPosition();

		pPosition->SetFEN( sParams );

		string sFEN = pPosition->GetFEN();

		*(pSelf->m_Out) << "# New position: " << sFEN << endl;
	}

	static void Usermove( CMD_PARAMS )
	{
		Square sSrc, sDest;

		sSrc.Set( sParams[0] - 'a', sParams[1] - '1' );
		sDest.Set( sParams[2] - 'a', sParams[3] - '1' );

		Position *pPosition = pSelf->m_pGame->GetPosition();

		Piece *pPiece = pPosition->GetBoard().Get( sSrc );

		Move move( pPiece, sSrc, sDest );

		Position nextPos( *pPosition, move );
		*pPosition = nextPos;

		*(pSelf->m_Out) << "# User move: " << (string)move << endl;

		Go( pSelf, "Thinking Params Go Here");
	}

	static int TimeToSeconds( const string &sTime )
	{
		unsigned int nColon = 0;
		int nMinutes = 0, nSeconds = 0;

		nColon = sTime.find(':');

		if ( nColon == string::npos )
		{
			stringstream ss;
			ss.str( sTime );
			ss >> nMinutes;
		}
		else
		{
			stringstream sMin, sSec;
			sMin.str( sTime.substr( 0, nColon ));
			sSec.str( sTime.substr( nColon + 1 ));
			sMin >> nMinutes;
			sSec >> nSeconds;
		}

		return ( nMinutes * 60 + nSeconds );

	}

	static void Level( CMD_PARAMS )
	{
		Game *pGame = pSelf->m_pGame;

		stringstream ss;
		ss.str( sParams );

		string sBaseTime, sIncrementTime;

		ss >> pGame->m_nMovesPerBaseTime >> sBaseTime >> sIncrementTime;

		pGame->m_nBaseTime = TimeToSeconds( sBaseTime );
		pGame->m_nIncrementTime = TimeToSeconds( sIncrementTime );

		*(pSelf->m_Out) << "# Level: Moves per base: " << pGame->m_nMovesPerBaseTime << 
			" Base: " << pGame->m_nBaseTime << " Inc: " << pGame->m_nIncrementTime << endl;

	}

	static void St( CMD_PARAMS )
	{
		Game *pGame = pSelf->m_pGame;

		stringstream ss;
		ss.str( sParams );

		ss >> pGame->m_nBaseTime;
		pGame->m_nMovesPerBaseTime = pGame->m_nIncrementTime = 0;

		*(pSelf->m_Out) << "# Level: Moves per base: " << pGame->m_nMovesPerBaseTime << 
			" Base: " << pGame->m_nBaseTime << " Inc: " << pGame->m_nIncrementTime << endl;
	}

	static void Go( CMD_PARAMS )
	{
		if ( pSelf->m_pThinker )
			StopThinking( pSelf, sParams );

		// pSelf->m_pThinker = new Thread( &Interface::Search, pSelf );

		Report(pSelf, "Thinking initiated");
	}

	static void Time( CMD_PARAMS )
	{
		Game *pGame = pSelf->m_pGame;

		stringstream ss;
		ss.str( sParams );

		ss >> pGame->m_Time;

		*(pSelf->m_Out) << "# Time: " << pGame->m_Time << endl;
		
	}

	static void OTim( CMD_PARAMS )
	{
		Game *pGame = pSelf->m_pGame;

		stringstream ss;
		ss.str( sParams );

		ss >> pGame->m_OTime;

		*(pSelf->m_Out) << "# Opponent time: " << pGame->m_OTime << endl;
	}

	static int Search( Thread *pCaller )
	{
		gClock.Reset();
		gClock.Start();

		Interface *pSelf = (Interface *)pCaller->m_pContext;

		SearcherAlphaBeta searcher( pCaller );

		(pSelf->m_PrincipalVariation).Initialize();

		Position *pPosition = pSelf->m_pGame->GetPosition();

		searcher.Search( *pPosition, pSelf->m_PrincipalVariation );

		Move myMove;
		myMove = pSelf->m_PrincipalVariation.GetFirst();

		Position NextPosition( *pPosition, myMove );
		*pPosition = NextPosition;

		gClock.Stop();

		Report(pSelf, "Move search completed");

		return 0;
	}

	static void Xboard( CMD_PARAMS )
	{
		RegisterXboard( pSelf, sParams );
		*(pSelf->m_Out) << "# Xboard commands registered" << endl;
	}

	static void Quit( CMD_PARAMS )
	{
		pSelf; 
		*(pSelf->m_Out) << "# Engine exiting" << endl;
		exit( 0 );
	}

	void Execute( const string &sCommand )
	{
		string sParams, sVerb;

		stringstream ss;
		ss.str( sCommand );

		ss >> sVerb;

		if ( sVerb.length() < sCommand.length() )
			sParams = sCommand.substr( sVerb.length() + 1, 1024 );

		InterfaceState is = m_CommandMap[ sVerb ];

		if ( is.m_fnCallback )
			is.m_fnCallback( is.m_pInterface, sParams );
		else
			*m_Out << "# Unknown command: " << sCommand << endl;
	}

	static void Execute( void *pInterface, const string &sCommand )
	{
		((Interface *)pInterface)->Execute( sCommand );
	}

	void Run()
	{
		m_Out->setf(ios::unitbuf);

		string sInputLine;

		RegisterAll( this );

		for ( ;; )
		{
			getline( *m_In, sInputLine );
			Execute( this, sInputLine );
		}
	}

	ostream *m_Out;
	istream *m_In;

	Game *m_pGame;

	Moves m_PrincipalVariation;

	Thread *m_pThinker;
	Thread *m_pTimer;

	int	m_Protover;

	bool m_bShowThinking;
	bool m_bPonder;

protected:
	unordered_map< string, InterfaceState > m_CommandMap;

};

void TestSearch1()
{
	Position pos;
	SearcherAlphaBeta s;

	int score;

	for ( int i = 0; i < 100; i++ )
	{
		Move move;
		Moves moves;

		score = s.Search( pos, moves );

		cout << "PV: ";
		moves.Dump();
		cout << endl;

		cout << "Score: ";
		cout << score;
		cout << endl;

		vector< Move > vMove = moves.GetMoves();

		pos = Position( pos, vMove[0]);
		pos.Dump();
	}
}
int main(int argc, char* argv[])
{
	srand ( (unsigned int ) time(NULL) );

	// clean up unreferenced warnings about parameters
	argc;
	argv;

	PieceInitializer pieceInitializer;

	Interface i;

	i.Run();

	return 0;
}

