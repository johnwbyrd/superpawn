/**
 ** Chess.cpp
 ** A simple UCI-compatible chess engine
 ** http://chess.johnbyrd.org
 **
 ** astyle options:
 ** --align-pointer=name --align-reference=name --max-code-length=80 --pad-paren-in --pad-header --remove-brackets --convert-tabs --mode=c
 ** http://creativecommons.org/licenses/by/3.0/
 **/

/**
** \todo Don't keep searching deeper if mate is detected; move immediately
** \todo Distance to mate reporting is wrong
** \todo Reintroduce transposition table
** \todo Quiescent search
** \todo Parse clock requests from interface
** \todo Change search based on clock
** \todo Fifty-move clock
** \todo Take castling into account in computing hashes
**/

#include "BuildInfo.h"

#define WEB_URL "http://chess.johnbyrd.org"

/** Number of rows and columns on the board */
const unsigned int MAX_FILES = 8;

/** See the table in the PieceInitializer() constructor for more details on this */
const unsigned int NUM_PIECES = 13;
const unsigned int HIGHEST_FILE = MAX_FILES - 1;
const unsigned int MAX_SQUARES = MAX_FILES * MAX_FILES;

/** Amount of memory to dedicate to position hash table, must be a power of 2, currently max 2 GB */
const unsigned int HASH_TABLE_SIZE = 1 * 1024 * 1024;

/** Maximum command length for UCI commands. */
const unsigned int MAX_COMMAND_LENGTH = 64 * 256;

/** Default search depth */
const unsigned int DEFAULT_SEARCH_DEPTH = 6; //-V112

/** An estimate of a reasonable maximum of moves in any given position.  Not
 ** a hard bound.
 **/
const unsigned int DEFAULT_MOVES_SIZE = 2 << 6;

#include <time.h>
#include <string>
#include <iostream>
#include <vector>
#include <array>
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
#include <random>

/** A number which is larger than any board score and idempotent under double negation */
const int BIG_NUMBER = 1000000;

using namespace std;

void Die( const string &s );

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

const int NONE_VALUE = 0;
const int PAWN_VALUE = 100;
const int KNIGHT_VALUE = 300;
const int BISHOP_VALUE = 300;
const int ROOK_VALUE = 500;
const int QUEEN_VALUE = 900;
const int CHECKMATE_VALUE = 990000; // any abs. value greater than this is mate
const int KING_VALUE = 1000000;

class PieceInitializer;
class Board;
class Move;
class Moves;
class Square;
class Interface;
class Position;

/* Definitions specifically to speed along function creation in the Interface class. */
#define INTERFACE_FUNCTION_PARAMS const string &sParams
#define INTERFACE_FUNCTION_NO_PARAMS const string &
#define INTERFACE_FUNCTION_RETURN_TYPE void

#define INTERFACE_PROTOTYPE( FunctionName )  INTERFACE_FUNCTION_RETURN_TYPE FunctionName ( INTERFACE_FUNCTION_PARAMS )
#define INTERFACE_PROTOTYPE_NO_PARAMS( FunctionName )  INTERFACE_FUNCTION_RETURN_TYPE FunctionName ( INTERFACE_FUNCTION_NO_PARAMS )
#define INTERFACE_FUNCTION_TYPE( Variable ) INTERFACE_FUNCTION_RETURN_TYPE ( Interface::* Variable )( INTERFACE_FUNCTION_PARAMS )
#define INTERFACE_FUNCTION_TYPE_NO_PARAMS( Variable ) INTERFACE_FUNCTION_RETURN_TYPE ( Interface::* Variable )( INTERFACE_FUNCTION_NO_PARAMS )
#define INTERFACE_FUNCTION_ABSTRACT_TYPE (*( INTERFACE_FUNCTION_RETURN_TYPE )())

typedef INTERFACE_FUNCTION_RETURN_TYPE( Interface::*InterfaceFunctionType )(
    INTERFACE_FUNCTION_PARAMS );

typedef std::array<int, MAX_SQUARES> PieceSquareRawTableType;

PieceSquareRawTableType pstDefault =
{
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0
};


/* "A knight on the rim is grim." */
PieceSquareRawTableType pstKnight =
{
    -40, -30, -30, -30, -30, -30, -30, -40,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -30,   0,   5,  10,  10,   5,   0, -50,
    -30,  10,  15,  30,  30,  15,  10, -30,
    -30,  10,  15,  30,  30,  15,  10, -30,
    -30,   0,  10,  15,  15,  10,   0, -50,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -40, -30, -30, -30, -30, -30, -30, -40
};

PieceSquareRawTableType pstWhitePawn =
{
    0,   0,   0,   0,   0,   0,   0,   0,
    5,   5,  10, -20, -20,  10,   5,   5,
    5,  -5,  -5,   0,   0,  -5,  -5,   5,
    0,   0,  10,  40,  40,  10,   0,   0,
    20, 20,  30,  50,  50,  30,  20,  20,
    40, 40,  40,  60,  60,  40,  40,  40,
    80, 80,  80,  80,  80,  80,  80,  80,
    0,   0,   0,   0,   0,   0,   0,   0
};

PieceSquareRawTableType pstBishop =
{
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

PieceSquareRawTableType pstRook =
{
    0, 0, 0, 30, 10, 30, 0, 0
    - 5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    5, 10, 10, 10, 10, 10, 10, 5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 30, 10, 30, 0, 0
};

PieceSquareRawTableType pstWhiteKingEarly =
{
    0, 0, 40, 0, 0, 0, 40, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

PieceSquareRawTableType pstBlackKingEarly =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 60, 0, 0, 0, 60, 0
};


class PieceSquareTable : public Object
{
public:
    PieceSquareTable()
    {
        for ( unsigned int i = 0; i < MAX_SQUARES; i++ )
            m_Table[ i ] = 0;
    }

    PieceSquareTable( const PieceSquareRawTableType &table )
    {
        m_Table = table;
    }

    void InvertColor()
    {
        PieceSquareRawTableType temp;
        temp = m_Table;
        for ( unsigned int i = 0; i < MAX_FILES; i++ )
            for ( unsigned int j = 0; j < MAX_FILES; j++ )
            {
                m_Table[ i + j * MAX_FILES ] =
                    temp[ i + ( ( MAX_FILES - 1 ) - j ) * MAX_FILES];
            }
    }

    int Get( const Square &s ) const;

    int Get( unsigned int index ) const
    {
        return m_Table[ ( size_t ) index ];
    }

    PieceSquareRawTableType m_Table;
};

/** A centisecond wall clock. */
class Clock : Object
{
public:

    typedef chrono::system_clock NativeClockType;
    typedef NativeClockType::duration NativeClockDurationType;
    typedef NativeClockType::time_point NativeTimePointType;

    typedef int64_t ChessTickType;
    typedef chrono::duration<ChessTickType, milli> Duration;

    Clock()
    {
        Reset();
    }

    void Reset()
    {
        m_Start = m_Clock.now();
    }

    ChessTickType Get() const
    {
        NativeTimePointType timeNow;
        timeNow = m_Clock.now();

        Duration dur;
        dur = chrono::duration_cast<Duration> ( timeNow - m_Start );

        return dur.count();
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

            cout << "Duration is now: " << Get() << endl;
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
        m_PieceSquareTable = pstDefault;
    }

    Piece( Color color )
    {
        m_Color = color;
        m_PieceType = NONE;
        m_pOtherColor = NULL;
        m_PieceSquareTable = pstDefault;
    }

    virtual int PieceValue() const = 0;
    /** A unique index value for each piece. */
    virtual int Index() const
    {
        return m_nIndex;
    };
    virtual void SetIndex( int i )
    {
        m_nIndex = i;
    };
    virtual Moves GenerateMoves( const Square &source,
                                 const Position &pos ) const = 0;
    virtual bool IsDifferent( const Square &dest, const Board &board ) const;
    virtual bool IsDifferentOrEmpty( const Square &dest,
                                     const Board &board ) const;

    void SetOtherColor( Piece &otherPiece )
    {
        m_pOtherColor = &otherPiece;
    }

    Piece *InvertColor()
    {
        return m_pOtherColor;
    }

    char Letter() const
    {
        // in Forsyth-Edwards notation, white pieces are uppercase
        if ( m_Color == BLACK )
            return m_Letter;

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

    PieceType Type() const
    {
        return m_PieceType;
    }

    const PieceSquareTable &GetPieceSquareTable() const
    {
        return m_PieceSquareTable;
    }
    void SetPieceSquareTable( const PieceSquareTable &val )
    {
        m_PieceSquareTable = val;
    }

protected:
    char    m_Letter;
    Color   m_Color;
    Piece   *m_pOtherColor;
    PieceType m_PieceType;
    int     m_nIndex;
    PieceSquareTable m_PieceSquareTable;
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
        return NONE_VALUE;
    }

    Moves GenerateMoves( const Square &source, const Position &pos ) const;

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
        return PAWN_VALUE;
    }

    Moves GenerateMoves( const Square &source, const Position &pos ) const;
    void AddEnPassantMove( Move &m, const Square &dest, Moves &moves ) const;
    virtual void AddAndPromote( Moves &moves, Move &m,
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
        return BISHOP_VALUE;
    }

    Moves GenerateMoves( const Square &source, const Position &pos ) const;

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
        return KNIGHT_VALUE;
    }

    Moves GenerateMoves( const Square &source, const Position &pos ) const;

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
        return ROOK_VALUE;
    }

    Moves GenerateMoves( const Square &source, const Position &pos ) const;

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
        return QUEEN_VALUE;
    }

    Moves GenerateMoves( const Square &source, const Position &pos ) const;

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
        return KING_VALUE;
    }

    Moves GenerateCastlingMoves( const Square &source, const Position &pos ) const;
    Moves GenerateMoves( const Square &source, const Position &pos ) const;

private:
    King();
};

/* PVS-Studio objects to this casting of bool to class type */
Pawn WhitePawn( WHITE ), BlackPawn( BLACK );        //-V601
Knight WhiteKnight( WHITE ), BlackKnight( BLACK );   //-V601
Bishop WhiteBishop( WHITE ), BlackBishop( BLACK );   //-V601
Rook WhiteRook( WHITE ), BlackRook( BLACK );        //-V601
Queen WhiteQueen( WHITE ), BlackQueen( BLACK );     //-V601
King WhiteKing( WHITE ), BlackKing( BLACK );        //-V601
NoPiece None;

class BoardBase : public Object
{
public:
    BoardBase()
    {
        Initialize();
    }

    virtual const Piece *Set( int index, const Piece *piece )
    {
        return ( m_Piece[ index ] = piece );
    }

    virtual const Piece *Get( int index ) const
    {
        return ( m_Piece[ index ] );
    }

    virtual void Initialize()
    {
        for ( unsigned int i = 0; i < MAX_SQUARES; i++ )
            Set( i, &None );
    }

    virtual const Piece *Set( int i, int j, const Piece *piece )
    {
        return ( Set( i + ( j << 3 ), piece ) );
    }


    const Piece *Get( int i, int j ) const
    {
        return Get( i + ( j << 3 ) );
    }

    const Piece *Set( const Square &s, const Piece *piece );
    const Piece *Get( const Square &s ) const;

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
        Set( 4, 0, &WhiteKing );  //-V112
        Set( 3, 7, &BlackQueen );
        Set( 4, 7, &BlackKing );  //-V112
    }

    void Flip()
    {
        const Piece *pTemp;

        for ( unsigned int j = 0 ; j < ( MAX_FILES / 2 ); j++ )
            for ( unsigned int i = 0; i < MAX_FILES; i++ )
            {
                pTemp = Get( i, j );
                Set( i, j, Get( HIGHEST_FILE - i, HIGHEST_FILE - j ) );
                Set( HIGHEST_FILE - i, HIGHEST_FILE - j, pTemp );
            }
    }

    bool IsEmpty( const Square &square ) const;

    void Dump() const
    {
        /* Note this weird for loop, which terminates when an unsigned int
         * goes below 0, i.e. gets real big
         */
        for ( unsigned int j = ( MAX_FILES - 1 ); j < MAX_FILES; j-- ) //-V621
        {
            for ( unsigned int i = 0; i < MAX_FILES; i++ )
                cout << Get( i, j )->Letter();

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
    const Piece *m_Piece[ MAX_FILES *MAX_FILES ];
};

typedef uint64_t HashValue;
HashValue s_PiecePositionHash[ MAX_SQUARES ][ NUM_PIECES ];
HashValue s_PieceColorHash[ 2 ];

class BoardHashing : public BoardBase
{
    typedef BoardBase super;
public:
    BoardHashing() : BoardBase(), m_Hash( 0 )
    {
        Initialize();
    }

    virtual void Initialize() override
    {
        m_Hash = 0;
        super::Initialize();
    }

    virtual const Piece *Set( int index, const Piece *piece ) override
    {
        // first, erase the old piece if it is non-null
        const Piece *curPiece = Get( index );

        if ( curPiece != &None )
            m_Hash ^= s_PiecePositionHash[ index ][ curPiece->Index() ];

        // next, place the next piece if it is non-null
        if ( piece != &None )
            m_Hash ^= s_PiecePositionHash[ index ][ piece->Index() ];

        return super::Set( index, piece );
    }

    /* Because the compiler gets hung up on trying to match the above function
     * to the three-argument version of Set... sigh...
     */
    virtual const Piece *Set( int i, int j, const Piece *piece ) override
    {
        return super::Set( i, j, piece );
    }

    HashValue GetHash() const
    {
        return m_Hash;
    }

    HashValue m_Hash;
};

class BoardPieceSquare : public BoardHashing
{
public:
    virtual int GetPieceSquareValue( int index ) const
    {
        return Get( index )->
               GetPieceSquareTable().Get( index );
    }

    virtual int GetPieceSquareValue( const Square &s ) const
    {
        return Get( s )->
               GetPieceSquareTable().Get( s );
    }
};

class Board : public BoardPieceSquare {};

class HashInitializer
{
public:
    HashInitializer()
    {
        mt19937_64 mt;
        for ( unsigned int i = 0; i < MAX_SQUARES; i++ )
            for ( unsigned int j = 0; j < NUM_PIECES; j++ )
                s_PiecePositionHash[ i ][ j ] = mt();

        for ( unsigned int i = 0; i < 2; i++ )
            s_PieceColorHash[ i ] = mt();

    }
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

    Square( const string &s )
    {
        i = s.at( 0 ) - 'a';
        j = s.at( 1 ) - '1';
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

    operator string() const
    {
        string s;

        if ( IsOnBoard() )
        {
            s = ( char )( 'a' + i );
            s += ( char )( '1' + j );
        }
        else
            s = "-";

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

    bool operator== ( const Square &right ) const
    {
        return ( ( i == right.i ) && ( j == right.j ) );
    }

protected:
    int i; // file
    int j; // rank
};

Square A1( 0, 0 ), A2( 0, 1 ), A3( 0, 2 ), A4( 0, 3 ), A5( 0, 4 ), //-V112
       A6( 0, 5 ), A7( 0, 6 ), A8( 0, 7 );
Square B1( 1, 0 ), B2( 1, 1 ), B3( 1, 2 ), B4( 1, 3 ), B5( 1, 4 ), //-V112
       B6( 1, 5 ), B7( 1, 6 ), B8( 1, 7 );
Square C1( 2, 0 ), C2( 2, 1 ), C3( 2, 2 ), C4( 2, 3 ), C5( 2, 4 ), //-V112
       C6( 2, 5 ), C7( 2, 6 ), C8( 2, 7 );
Square D1( 3, 0 ), D2( 3, 1 ), D3( 3, 2 ), D4( 3, 3 ), D5( 3, 4 ), //-V112
       D6( 3, 5 ), D7( 3, 6 ), D8( 3, 7 );
Square E1( 4, 0 ), E2( 4, 1 ), E3( 4, 2 ), E4( 4, 3 ), E5( 4, 4 ), //-V112
       E6( 4, 5 ), E7( 4, 6 ), E8( 4, 7 ); //-V112
Square F1( 5, 0 ), F2( 5, 1 ), F3( 5, 2 ), F4( 5, 3 ), F5( 5, 4 ), //-V112
       F6( 5, 5 ), F7( 5, 6 ), F8( 5, 7 );
Square G1( 6, 0 ), G2( 6, 1 ), G3( 6, 2 ), G4( 6, 3 ), G5( 6, 4 ), //-V112
       G6( 6, 5 ), G7( 6, 6 ), G8( 6, 7 );
Square H1( 7, 0 ), H2( 7, 1 ), H3( 7, 2 ), H4( 7, 3 ), H5( 7, 4 ), //-V112
       H6( 7, 5 ), H7( 7, 6 ), H8( 7, 7 );

int PieceSquareTable::Get( const Square &s ) const
{
    return m_Table[ ( size_t ) ( s.I() + s.J() * MAX_FILES ) ];
}

class Move : Object
{
public:
    Move()
    {
        m_Piece = &None;
        m_PromoteTo = &None;
        m_Score = 0;
    }

    Move( Piece *piece )
    {
        m_Piece = piece;
        m_PromoteTo = &None;
        m_Score = 0;
    }

    Move( const Piece *piece, const Square &source, const Square &dest )
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
            Die( "Got an incoming Move string that had a weird length " );

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

    const Piece *GetPiece() const
    {
        return m_Piece;
    }
    void SetPiece( const Piece *val )
    {
        m_Piece = val;
    }
    const Piece *GetPromoteTo() const
    {
        return m_PromoteTo;
    }

    void SetPromoteTo( const Piece *val )
    {
        m_PromoteTo = val;
        /* Don't just overwrite -- this may be a promotion and a capture! */
        m_Score += val->PieceValue() ;
    }

    Square Source() const
    {
        return m_Source;
    }
    void Source( const Square &val )
    {
        m_Source = val;
    }
    Square Dest() const
    {
        return m_Dest;
    }
    void Dest( const Square &val )
    {
        m_Dest = val;
    }

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
            cout << tolower( m_PromoteTo->Letter() );
    }

    operator string() const
    {
        if ( m_Piece == &None )
            return ( string ) "null";

        if ( m_PromoteTo != &None )
        {
            string s;
            s.append( m_Source );
            s.append( m_Dest );
            s.push_back( ( char ) tolower( m_PromoteTo->Letter() ) );
            return s;
        }

        return ( string ) m_Source + ( string ) m_Dest;
    }

    string TextWithPiece()
    {
        string letter;
        stringstream ss;

        ss << m_Piece->Letter();
        ss >> letter;

        return ( letter ) + ( string ) m_Source + ( string ) m_Dest;
    }

    bool operator== ( const Move &right )
    {
        return ( ( m_Piece == right.m_Piece ) &&
                 ( m_Source == right.m_Source ) &&
                 ( m_Dest == right.m_Dest ) &&
                 ( m_PromoteTo == right.m_PromoteTo ) );
    }

    int Score() const
    {
        return m_Score;
    }
    void Score( int val )
    {
        m_Score = val;
    }

protected:
    const Piece *m_Piece;
    Square m_Source, m_Dest;
    int m_Score;
    const Piece *m_PromoteTo;
};

bool operator == ( const Move &left, const Move &right )
{
    if ( ( left.Source() == right.Source() ) &&
            ( left.Dest() == right.Dest() ) &&
            ( left.GetPiece() == right.GetPiece() ) &&
            ( left.GetPromoteTo() == right.GetPromoteTo() ) )
        return true;

    return false;
}

bool operator != ( const Move &left, const Move &right )
{
    return !( left == right );
}

bool operator< ( const Move &left, const Move &right )
{
    int leftscore = left.Score();
    int rightscore = right.Score();

    if ( ( leftscore != 0 ) && ( leftscore == rightscore ) )
    {
        /* Captured values are the same.  Choose the capturing piece with
         * the least value, MVV/LVA style.
         */

        if ( left.GetPiece()->PieceValue() < right.GetPiece()->PieceValue() )
            return true;

        return false;
    }

    if ( leftscore > rightscore )
        return true;

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
        None.SetOtherColor( None );  //-V678

        NullMove.Source( Square( -99, -99 ) );
        NullMove.Dest( Square( -99, -99 ) );

        None.SetIndex( 0 );
        WhitePawn.SetIndex( 1 );
        BlackPawn.SetIndex( 2 );
        WhiteKnight.SetIndex( 3 );
        BlackKnight.SetIndex( 4 ); //-V112
        WhiteBishop.SetIndex( 5 );
        BlackBishop.SetIndex( 6 );
        WhiteRook.SetIndex( 7 );
        BlackRook.SetIndex( 8 );
        WhiteQueen.SetIndex( 9 );
        BlackQueen.SetIndex( 10 );
        WhiteKing.SetIndex( 11 );
        BlackKing.SetIndex( 12 );

        WhitePawn.SetPieceSquareTable( pstWhitePawn );
        PieceSquareTable pstBlackPawn = pstWhitePawn;
        pstBlackPawn.InvertColor();
        BlackPawn.SetPieceSquareTable( pstBlackPawn );

        WhiteKnight.SetPieceSquareTable( pstKnight );
        BlackKnight.SetPieceSquareTable( pstKnight );

        WhiteBishop.SetPieceSquareTable( pstBishop );
        BlackBishop.SetPieceSquareTable( pstBishop );

        WhiteKing.SetPieceSquareTable( pstWhiteKingEarly );
        BlackKing.SetPieceSquareTable( pstBlackKingEarly );
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

    /** Find the move in the moves list, remove it and push it onto the
     ** front of the list.
     **/
    bool Bump( const Move &bump )
    {
        MovesInternalType::iterator it;

        it = m_Moves.begin();
        while ( it != m_Moves.end() )
        {
            if ( *it == bump )
            {
                Move tmp;
                tmp = * ( m_Moves.begin() );
                * ( m_Moves.begin() ) = *it;
                *it = tmp;
                return true;
            }
            ++it;
        }

        Die( "Expected to find the bump element in the array, but it didn't exist -- bad hash table?" );
        return false;
    }

    size_t Count() const
    {
        return m_Moves.size();
    }

    Move &GetLast()
    {
        return m_Moves.at( m_Moves.size() - 1 );
    }

    void Make( const Move &move )
    {
        m_Moves.push_back( move );
    }

    void Unmake()
    {
        m_Moves.pop_back();
    }

    Moves operator+ ( const Moves &otherMoves )
    {
        m_Moves.insert( m_Moves.end(),
                        otherMoves.m_Moves.begin(),
                        otherMoves.m_Moves.end() );

        return *this;
    }

    void Append( const Moves &&otherMoves )
    {
        m_Moves.insert( m_Moves.end(),
                        otherMoves.m_Moves.begin(),
                        otherMoves.m_Moves.end() );
    }

    void Sort()
    {
        sort( m_Moves.begin(), m_Moves.end() );
    }

    Move Random()
    {
        if ( m_Moves.size() == 0 )
            return NullMove;

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

    operator string() const
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
    bool TryAttack( const Move &m, const Board &board, int id, int jd )
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
                break;   // attack is over; we hit a piece of this or the other color

            i += id;
            j += jd;
            nAttacks++;
        }

        return nAttacks;
    }

    typedef vector<Move> MovesInternalType;
    typedef MovesInternalType::iterator iterator;
    typedef MovesInternalType::const_iterator const_iterator;

    iterator begin()
    {
        return m_Moves.begin();
    }
    const_iterator begin() const
    {
        return m_Moves.begin();
    }
    iterator end()
    {
        return m_Moves.end();
    }
    const_iterator end() const
    {
        return m_Moves.end();
    }

protected:
    MovesInternalType m_Moves;
};

class Position;

class PositionHasher : Object
{
public:

    PositionHasher( const Position &pPos ) :
        m_Hash( 0 )
    {
        m_pPosition = &pPos;
    }
    /** \todo Update this hash value based on all relevant
     ** position data including castling and other rights
     **/
    HashValue GetHash() const;

protected:
    HashValue m_Hash;
    const Position *m_pPosition;

private:
    PositionHasher();
};

enum HashEntryType
{
    HET_NONE = 0x0,
    /* PV-nodes (Knuth's Type 1) are nodes that have a score that ends
     * up being inside the window. So if the bounds passed are [a,b],
     * with the score returned s, a<s<b. These nodes have all moves searched,
     * and the value returned is exact (i.e., not a bound), which propagates
     * up to the root along with a principal variation.*/
    HET_PRINCIPAL_VARIATION = 0x1,
    /* Cut-nodes (Knuth's Type 2), otherwise known as fail-high nodes,
     * are nodes in which a beta-cutoff was performed. So with bounds
     * [a,b], s>=b. A minimum of one move at a Cut-node needs to be
     * searched. The score returned is a lower bound (might be greater)
     * on the exact score of the node*/
    HET_CUT_NODE = 0x2,
    /** All-nodes (Knuth's Type 3), otherwise known as fail-low nodes,
     * are nodes in which no move's score exceeded alpha. With bounds
     * [a,b], s<=a. Every move at an All-node is searched, and the score
     * returned is an upper bound, the exact score might be less. */
    HET_ALL_NODE = 0x4,
};

class PositionHashEntry : public Object
{
public :
    HashEntryType m_TypeBits;
    HashValue m_Hash;
    Move m_BestMove;
    int m_Depth;
    int m_Ply;
    int m_Score;

    PositionHashEntry() :
        m_Hash( 0 ),
        m_BestMove( 0 ),
        m_Depth( 0 ),
        m_Ply( 0 ),
        m_Score( 0 )
    {
        m_TypeBits = HET_NONE;
    }
};

class PositionHashTable;
PositionHashTable *s_pPositionHashTable = nullptr;

class PositionHashTable : public Object
{
public:
    PositionHashTable() :
        m_pEntries( nullptr ),
        m_SizeBytes( 0 ), m_SizeEntries( 0 ), m_SizeBytesMask( 0 ),
        m_CacheLookups( 0 ), m_CacheMisses( 0 ), m_CacheHits( 0 ),
        m_nEntriesInUse( 0 )
    {
        SetSize( HASH_TABLE_SIZE );
    }

    virtual ~PositionHashTable()
    {
        if ( m_SizeBytes )
            delete m_pEntries;
    }

    virtual void Insert( const PositionHashEntry &entry )
    {
        size_t loc = entry.m_Hash % m_SizeEntries;
        /** todo Insert logic for different strategies */
        PositionHashEntry *pHE = m_pEntries + loc;

        if ( pHE->m_TypeBits == HET_NONE )
            m_nEntriesInUse++;

        if ( pHE->m_Hash == entry.m_Hash )
        {
            /* Only overwrite if the search depth is farther */
            if ( pHE->m_Ply > entry.m_Ply )
                return;
        }
        m_pEntries[ loc ] = entry;
    }

    virtual const PositionHashEntry *LookUp( const HashValue &val )
    {
        m_CacheLookups++;
        size_t loc = val % m_SizeEntries;
        PositionHashEntry *pEntry = m_pEntries + loc;
        if ( val == pEntry->m_Hash )
        {
            m_CacheHits++;
            return pEntry;
        }

        m_CacheMisses++;
        return nullptr;
    }

    virtual size_t GetSize() const
    {
        return m_SizeBytes;
    }

    virtual void SetSize( size_t size )
    {
        if ( size == 0 )
            Die( "Size of hash table can't be zero" );

        /* modified from http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 --
         * should work to 2 GB */
        size--;
        size |= size >> 1;
        size |= size >> 2;
        size |= size >> 4;
        size |= size >> 8;
        size |= size >> 16;
        size++;

        m_SizeBytes = size;
        m_SizeBytesMask = size - 1;
        m_SizeEntries = m_SizeBytes / sizeof( PositionHashEntry );
        m_pEntries = new PositionHashEntry[ m_SizeEntries ];
    }

    /** Returns a value representing how full the cache is.  0 is empty.  1000 is full.  */
    virtual unsigned int GetHashFull()
    {
        return ( unsigned int )( 1000 * m_nEntriesInUse / m_SizeEntries );
    }

    PositionHashEntry *m_pEntries;
    size_t m_SizeBytes, m_SizeEntries, m_SizeBytesMask;
    uint64_t m_CacheLookups, m_CacheMisses, m_CacheHits;
    uint64_t m_nEntriesInUse;
};

class HashTableInitializer : public Object
{
public :
    HashTableInitializer()
    {
        s_pPositionHashTable = new PositionHashTable;
    }

    virtual ~HashTableInitializer()
    {
        delete s_pPositionHashTable;
        s_pPositionHashTable = nullptr;
    }
};

class Position : Object
{
    friend class PositionHasher;
    /* King moves need to see somewhat deeply inside the Position structure for
     * castling at the moment; this could be refactored I suppose
     */
    friend class King;

public:
    Position()
    {
        Initialize();
    }

    Position( bool colorToMove )
    {
        Initialize();
        SetColorToMove( colorToMove );
    }

    Position( const string &sFEN )
    {
        Initialize();
        SetFEN( sFEN );
    }

    /** Generates a new Position based on a previous, existing Position
     ** as well as a Move to apply to that previous Position.  A new
     ** Position is generated; the original Position remains untouched.
     **/
    Position( const Position &position, const Move &move )
    {
        CopyFrom( position );

        Square source = move.Source();
        DevirginizeRooks( source );

        const Piece *pPiece = GetBoard().Get( source );
        DevirginizeKing( pPiece );

        if ( &move == &NullMove )
        {
            SetColorToMove( !GetColorToMove() );
            return;
        }

        if ( pPiece == &None )
        {
            stringstream ss;
            ss << "Illegal move: no piece found at source location for move ";
            ss << ( string ) move;
            Die( ss.str() );
        }

        if ( move.GetPromoteTo() == &None )
            MovePiece( position, move );
        else
            PromotePiece( position, move );

        m_Board.Set( move.Source().I(), move.Source().J(), &None );
        SetColorToMove( !GetColorToMove() );

        PushHashInHistory();
    }

    void Initialize()
    {
        SetColorToMove( WHITE );
        m_nPly = 0;
        m_nMaterialScore = 0;
        m_bVirginH8 =
            m_bVirginA8 =
                m_bVirginH1 =
                    m_bVirginA1 =
                        m_bVirginBlackKing =
                            m_bVirginWhiteKing = true;
        m_sEnPassant.Set( -1, -1 );
        m_Moves.Clear();
        m_Board.Initialize();
        m_bIsCheckDetermined = false;
        m_bIsCheck = false;
        m_PreviousPositions.clear();
    }

    int GetColorBias() const
    {
        return ( m_ColorToMove == WHITE ? 1 : -1 );
    }

    PositionHashTable *GetHashTable() const
    {
        return s_pPositionHashTable;
    }

    /** Looks up this position in the hash table and returns a pointer
     ** to the corresponding PositionHashEntry if any.
     **/
    const PositionHashEntry *LookUp() const
    {
        PositionHashTable *pHT = GetHashTable();
        PositionHasher ph( *this );
        return pHT->LookUp( ph.GetHash() );
    }

    /** Inserts this position into the hash table.  Takes care of updating
     ** the hash value before insertion.
     **/
    void Insert( PositionHashEntry &pos )
    {
        PositionHashTable *pHT = GetHashTable();
        PositionHasher ph( *this );
        pos.m_Hash = ph.GetHash();
        pHT->Insert( pos );
    }

    void HandleEnPassant( const Move &move, const Position &position,
                          Square &captureSquare )
    {
        /* Did the pawn just move two spaces?  If so, record this fact in the position */
        int pawnMoveDistance;
        pawnMoveDistance = move.Dest().J() - move.Source().J();

        if ( abs( pawnMoveDistance ) > 1 )
        {
            /* Halfway between the start and the end */
            pawnMoveDistance = pawnMoveDistance >> 1;
            Square enPassant( move.Source().I(), move.Source().J() + pawnMoveDistance );
            m_sEnPassant = enPassant;
        }

        /* Did the pawn just move into the previous en passant square?  If so, capture */
        else if ( move.Dest() == position.m_sEnPassant )
        {
            /* Capture the pawn behind it */
            int d = position.GetColorToMove() ? -1 : 1;
            captureSquare = Square( move.Dest().I(), move.Dest().J() + d );
            /* FIXME This line doesn't seem to get the material right in the case of an en passant */
            CaptureMaterial( position, captureSquare );
            m_Board.Set( captureSquare.I(), captureSquare.J(), &None );
        }
        else
        {
            /* Not en passant.  Regardless of whether there's a capture or not,
            * bump the material score by the captured square
            */
            CaptureMaterial( position, captureSquare );
        }
    }

    void HandleCastling( const Move &move )
    {
        if ( abs( move.Source().I() - move.Dest().I() ) == 2 )
        {
            /* Move rook during castling */
            int rookISource = 0, rookIDest = 3, rookJ;
            rookJ = move.Source().J();
            if ( move.Dest().I() == 6 )
            {
                rookISource = 7;
                rookIDest = 5;
            }

            const Rook *pRook = dynamic_cast<const Rook *>( m_Board.Get( rookISource,
                                rookJ ) );
            if ( pRook == nullptr )
            {
                stringstream ss;
                ss << "Couldn't find rook during castling: ";
                ss << GetFEN();
                Die( ss.str() );
            }

            m_Board.Set( rookIDest, rookJ, pRook );
            m_Board.Set( rookISource, rookJ, &None );

            Square rookSource( rookISource, rookJ );
            DevirginizeRooks( rookSource );
        }
    }


    void PushHashInHistory()
    {
        PositionHasher ph( *this );
        m_PreviousPositions.push_back( ph.GetHash() );
    }

    void PopHashFromHistory()
    {
        m_PreviousPositions.pop_back();
    }

    unsigned int CountHashesInHistory( const HashValue &theHash ) const
    {
        unsigned int count = 0;
        for ( auto previous : m_PreviousPositions )
        {
            if ( previous == theHash )
                count++;
        }
        return count;
    }

    void CopyFrom( const Position &position )
    {
        m_Board = position.m_Board;
        m_ColorToMove = position.m_ColorToMove;
        m_nMaterialScore = position.m_nMaterialScore;
        m_PreviousPositions = position.m_PreviousPositions;

        m_nPly = position.m_nPly + 1;
        m_sEnPassant = Square( -1, -1 );

        m_bVirginH1 = position.m_bVirginH1;
        m_bVirginA1 = position.m_bVirginA1;
        m_bVirginH8 = position.m_bVirginH8;
        m_bVirginA8 = position.m_bVirginA8;
        m_bVirginBlackKing = position.m_bVirginBlackKing;
        m_bVirginWhiteKing = position.m_bVirginWhiteKing;

        m_bIsCheckDetermined = false;
        m_bIsCheck = false;
    }

    void MovePiece( const Position &position, const Move &move )
    {
        /* Move piece */
        const Piece *pSource = m_Board.Get( move.Source() );
        const PieceType sourceType = pSource->Type();

        Square captureSquare = move.Dest();

        if ( sourceType == PAWN )
            HandleEnPassant( move, position, captureSquare );
        else
            CaptureMaterial( position, captureSquare );

        /* Move the piece to the destination */
        m_Board.Set( move.Dest().I(), move.Dest().J(),
                     pSource );

        /* Handle castling */
        if ( sourceType == KING )
            HandleCastling( move );

    }

    void PromotePiece( const Position &position, const Move &move )
    {
        /* Promote piece */
        m_nMaterialScore = position.GetScore() +
                           ( move.GetPromoteTo()->PieceValue() +
                             m_Board.Get( move.Dest() )->PieceValue() ) *
                           GetColorBias();

        m_Board.Set( move.Dest().I(), move.Dest().J(),
                     move.GetPromoteTo() );
    }

    void DevirginizeKing( const Piece *pPiece )
    {
        if ( pPiece == &WhiteKing )
            m_bVirginWhiteKing = false;

        if ( pPiece == &BlackKing )
            m_bVirginBlackKing = false;
    }

    /* This is the sexiest function in the entire program. */
    void DevirginizeRooks( Square &source )
    {
        if ( source == A1 )
            m_bVirginA1 = false;

        if ( source == A8 )
            m_bVirginA8 = false;

        if ( source == H1 )
            m_bVirginH1 = false;

        if ( source == H8 )
            m_bVirginH8 = false;
    }

    void CaptureMaterial( const Position &position, const Square &captureSquare )
    {
        m_nMaterialScore = position.GetScore() + ( m_Board.Get(
                               captureSquare )->PieceValue() ) *
                           GetColorBias();

        if ( captureSquare == A1 )
            m_bVirginA1 = false;

        if ( captureSquare == A8 )
            m_bVirginA8 = false;

        if ( captureSquare == H1 )
            m_bVirginH1 = false;

        if ( captureSquare == H8 )
            m_bVirginH8 = false;
    }

    void GenerateMoves()
    {
        const Piece *pPiece;

        for ( unsigned int j = 0; j < MAX_FILES; j++ )
            for ( unsigned int i = 0; i < MAX_FILES; i++ )
            {
                pPiece = m_Board.Get( i, j );

                if ( ( pPiece != &None ) && ( pPiece->GetColor() == m_ColorToMove ) )
                    m_Moves.Append( pPiece->GenerateMoves( Square( i, j ), *this ) );
            }

        m_Moves.Sort();
    }

    const Moves &GetMoves()
    {
        if ( m_Moves.IsEmpty() )
            GenerateMoves();

        return m_Moves;
    }

    const Moves &GetCaptures()
    {
        if ( m_Captures.IsEmpty() )
        {
            if ( m_Moves.IsEmpty() )
                GenerateMoves();

            for ( auto &move : m_Moves )
            {
                if ( move.Score() > 0 )
                    m_Captures.Add( move );
                else
                    break;
            }
        }

        return m_Captures;
    }

    size_t CountMoves()
    {
        GetMoves();
        return m_Moves.Count();

    }

    bool CanKingBeCapturedNow()
    {
        Moves moves = GetMoves();

        if ( !moves.IsEmpty() )
        {
            Move bestMove = moves.GetFirst();
            if ( bestMove.Score() >= KING_VALUE )
                return true;
        }

        return false;
    }

    bool IsCheck()
    {
        if ( m_bIsCheckDetermined )
            return m_bIsCheck;

        /* To determine check, apply a null move to the current position and see if the result
         * permits the king to be captured.
         */
        Position tempPos( *this, NullMove );
        m_bIsCheck = tempPos.CanKingBeCapturedNow();
        m_bIsCheckDetermined = true;
        return m_bIsCheck;
    }

    bool IsStalemate()
    {
        if ( IsCheck() )
            return false;

        Moves moves = GetMoves();
        for ( auto move : moves )
        {
            Position nextPos( *this, move );
            if ( nextPos.CanKingBeCapturedNow() == false )
            {
                // terminate asap
                return false;
            }
        }

        return true;
    }

    const Board &GetBoard() const
    {
        return m_Board;
    }

    void SetBoard( const Board &val )
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

    int SetFEN( const string &sFEN )
    {
        Initialize();

        stringstream ss;

        ss.str( sFEN );

        string sBoard, sToMove, sVirgins, sEnPassant;
        int nMoves;

        ss >> sBoard >> sToMove >> sVirgins >> sEnPassant >> m_nPly >> nMoves;

        int j = MAX_FILES - 1;
        int i = 0;
        char c;

        stringstream ssBoard( sBoard );

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

        SetColorToMove( sToMove == "w" ? WHITE : BLACK );

        stringstream ssVirgins( sVirgins );

        m_bVirginH1 =
            m_bVirginA1 =
                m_bVirginH8 =
                    m_bVirginA8 =
                        m_bVirginBlackKing =
                            m_bVirginWhiteKing = false;

        while ( ssVirgins >> c )
        {
            switch ( c )
            {
            case '-' :
                break;

            case 'K':
                m_bVirginH1 = true;
                break;

            case 'Q':
                m_bVirginA1 = true;
                break;

            case 'k':
                m_bVirginH8 = true;
                break;

            case 'q':
                m_bVirginA8 = true;
                break;
            }
        }

        if ( sEnPassant != "-" )
        {
            Square s( sEnPassant );
            m_sEnPassant = s;
        }

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
        const Piece *pPiece;
        int nSpaces = 0;

        for ( unsigned int jj = 1; jj <= MAX_FILES; jj++ )
        {
            unsigned int j = MAX_FILES - jj;

            for ( unsigned int i = 0; i < MAX_FILES; i++ )
            {
                pPiece = m_Board.Get( i, j );

                if ( ( pPiece != &None ) && ( nSpaces > 0 ) )
                {
                    s += ( char ) nSpaces + '0';
                    nSpaces = 0;
                }

                if ( pPiece != &None )
                    s += pPiece->Letter();
                else
                    nSpaces++;

            }

            if ( nSpaces > 0 )
                s += ( char ) nSpaces + '0';

            nSpaces = 0;
            if ( j != 0 )
                s += '/';
        }

        if ( GetColorToMove() == WHITE )
            s += " w ";
        else
            s += " b ";

        if ( !( m_bVirginH1 || m_bVirginA1 || m_bVirginH8 || m_bVirginA8 ) )
            s += "-";

        else
        {
            if ( m_bVirginH1 )
                s += "K";

            if ( m_bVirginA1 )
                s += "Q";

            if ( m_bVirginH8 )
                s += "k";

            if ( m_bVirginA8 )
                s += "q";
        }

        stringstream ss;

        ss << " ";
        ss << ( string ) m_sEnPassant;
        ss << " ";
        ss << m_nPly;
        ss << " ";
        ss << m_nPly / 2 + 1;

        s += ss.str();

        return s;
    }

    operator string()
    {
        return GetFEN();
    }

    Color GetColorToMove() const
    {
        return m_ColorToMove;
    }
    void SetColorToMove( Color val )
    {
        m_ColorToMove = val;
    }

    int GetScore() const
    {
        return m_nMaterialScore;
    }
    void SetScore( int val )
    {
        m_nMaterialScore = val;
    }

    unsigned int GetPly() const
    {
        return m_nPly;
    }

    void SetPly( int val )
    {
        m_nPly = val;
    }

    Square EnPassant() const
    {
        return m_sEnPassant;
    }
    void EnPassant( const Square &val )
    {
        m_sEnPassant = val;
    }

    HashValue GetHash()
    {
        if ( m_PreviousPositions.size() == 0 )
        {
            // do it the hard way
            PushHashInHistory();
        }

        return *( m_PreviousPositions.end() - 1 );
    }


protected:
    Board   m_Board;
    Color   m_ColorToMove;
    unsigned int    m_nPly;
    int m_nMaterialScore;
    /** Virgin rooks; can tell whether any of the four rooks has been moved */
    bool m_bVirginH1, m_bVirginA1, m_bVirginH8, m_bVirginA8;
    bool m_bVirginWhiteKing, m_bVirginBlackKing;
    Square m_sEnPassant;
    /** Cached generated moves. */
    Moves m_Moves;
    Moves m_Captures;
    bool m_bIsCheckDetermined;
    bool m_bIsCheck;
    typedef std::vector< HashValue > PreviousPositionType;
    PreviousPositionType m_PreviousPositions;
};

HashValue PositionHasher::GetHash() const
{
    return ( m_pPosition->m_Board.GetHash() ^
             s_PieceColorHash[( int ) m_pPosition->m_ColorToMove ]
           );
}

class EvaluatorBase : public Object
{
public:
    virtual int Evaluate( Position &pos ) const = 0;

protected:
    virtual int Bias( const Position &pos, int nResult ) const
    {
        return ( pos.GetColorToMove() == WHITE ? nResult : -nResult );
    }
};

class EvaluatorSlowMaterial : public EvaluatorBase
{
public:
    virtual int Evaluate( Position &pos ) const
    {
        Board board = pos.GetBoard();
        const Piece *piece;

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

class EvaluatorPieceSquare : public EvaluatorBase
{
public:
    virtual int Evaluate( Position &pos ) const
    {
        Board board = pos.GetBoard();
        const Piece *piece;

        int nScore = 0;

        for ( unsigned int i = 0; i < MAX_SQUARES; i++ )
        {
            piece = board.Get( i );
            if ( piece != &None )
            {
                nScore += ( board.GetPieceSquareValue( i ) *
                            ( ( piece->GetColor() == WHITE ) ? 1 : -1 ) );
            }
        }

        return Bias( pos, nScore );
    }
};

class EvaluatorMaterial : public EvaluatorSlowMaterial
{
public:
    virtual int Evaluate( Position &pos ) const
    {
        return Bias( pos, pos.GetScore() );
    }
};

class EvaluatorWeighted : public EvaluatorBase
{
public:
    virtual int Evaluate( Position &pos ) const
    {
        if ( m_Evaluators.empty() )
            Die( "No evaluators have been defined" );

        WeightsType::const_iterator weightIter;
        weightIter = m_Weights.begin();

        int nScore = 0;

        for ( EvaluatorsType::const_iterator iter = m_Evaluators.begin();
                iter != m_Evaluators.end();
                ++iter )
        {
            nScore += ( int )( ( *iter )->Evaluate( pos ) * ( *weightIter ) );
            ++weightIter;
        }

        return nScore;
    }

    void Add( EvaluatorBase &eval, float weight = 1.0f )
    {
        m_Evaluators.push_back( &eval );
        m_Weights.push_back( weight );
    }

protected:
    typedef vector<float> WeightsType;
    typedef vector<EvaluatorBase *> EvaluatorsType;

    WeightsType m_Weights;
    EvaluatorsType m_Evaluators;
};

class EvaluatorSimpleMobility : public EvaluatorBase
{
    virtual int Evaluate( Position &pos ) const
    {
        return ( int ) pos.CountMoves() ;
    }
};

class EvaluatorStandard : public EvaluatorWeighted
{
public:
    EvaluatorStandard()
    {
        m_Weighted.Add( m_Material );
        m_Weighted.Add( m_SimpleMobility, 0.1f );
        m_Weighted.Add( m_PieceSquareEvaluator );
    }

    virtual int Evaluate( Position &pos ) const
    {
        return m_Weighted.Evaluate( pos );
    }

    EvaluatorMaterial m_Material;
    EvaluatorSimpleMobility m_SimpleMobility;
    EvaluatorPieceSquare m_PieceSquareEvaluator;
    EvaluatorWeighted m_Weighted;
};

typedef EvaluatorStandard Evaluator;

void Position::UpdateScore()
{
    EvaluatorSlowMaterial slow;
    SetScore( slow.Evaluate( *this ) );
}

/** Keeps track of the search and decides whether it should continue. */
class DirectorBase : Object
{
    friend class Interface;
public:
    DirectorBase()
    {
        Initialize();
    }

    virtual unsigned int GetDepth() const
    {
        return m_nDepth;
    }

    virtual unsigned int GetNodes() const
    {
        return m_nNodes;
    }

    virtual void Initialize()
    {
        m_SearchMoves.Clear();
        m_bPonder = false;
        m_WhiteTime = m_BlackTime = m_WhiteInc = m_BlackInc = 0;
        m_nMovesToGo =
            m_nNodes = m_nMateInMoves = m_nMoveTime = 0;
        m_nDepth = 0;
        m_bInfinite = false;
        m_SearchStopTime = 0;
    }

    virtual void Action()
    {
        m_SearchStopTime = 0;
    }

    virtual void Cut()
    {
        m_SearchStopTime = 0;
    }

    virtual void CalculateSearchStopTime(
        const Clock::ChessTickType /* currentTime */,
        const int /* nScore */,
        const int /* nDepthSearched */,
        const Position &rootPosition,
        const Moves & /* mPrincipalVariation */
    )
    {
        int ply = rootPosition.GetPly();
        Color sideToMove = rootPosition.GetColorToMove();
        Clock::ChessTickType timeLeft, timeInc;

        if ( sideToMove == WHITE )
        {
            timeLeft = m_WhiteTime;
            timeInc = m_WhiteInc;
        }
        else
        {
            timeLeft = m_BlackTime;
            timeInc = m_BlackInc;
        }

        /* Why does this time management formula sort of work?  I'm not really sure.
         * It seems to sort of resemble how Stockfish manages its time.
         * Rather than try to understand why it works, I'll leave it be for the time
         * being, until something better comes along.
         */
        int movesUntilTimeControl;
        if ( m_nMovesToGo != 0 )
            movesUntilTimeControl = m_nMovesToGo;
        else
            movesUntilTimeControl = ply + 25;

        Clock::ChessTickType factor = 3;

        m_SearchStopTime = ( timeLeft + timeInc ) / movesUntilTimeControl;
        m_SearchStopTime /= factor;

    }

    virtual bool ShouldCut(
        const Clock::ChessTickType currentTime,
        const int nScore,
        const unsigned int nDepthSearched,
        const Position &rootPosition,
        const Moves &mPrincipalVariation
    )
    {
        if ( m_nDepth != 0 )
            return ( nDepthSearched >= m_nDepth );

        if ( m_nMoveTime != 0 )
            return ( currentTime >= m_nMoveTime );

        if ( m_bInfinite )
            return false;

        if ( m_SearchStopTime == 0 )
            CalculateSearchStopTime( currentTime, nScore, nDepthSearched,
                                     rootPosition, mPrincipalVariation );

        return ( currentTime >= m_SearchStopTime );
    }

protected:
    Moves m_SearchMoves;
    bool m_bPonder;
    Clock::ChessTickType m_WhiteTime, m_BlackTime, m_WhiteInc, m_BlackInc;
    unsigned int m_nMovesToGo;
    unsigned int m_nDepth;
    unsigned int m_nNodes;
    unsigned int m_nMateInMoves;
    unsigned int m_nMoveTime;
    bool m_bInfinite;
    Clock::ChessTickType m_SearchStopTime;
};

typedef DirectorBase Director;


class SearcherBase : Object
{
public:
    SearcherBase( Interface &interface ) :
        m_nNodesSearched( 0 )
    {
        m_bTerminated = true;
        m_pInterface = &interface;
    }

    ~SearcherBase()
    {
        Stop();
    }

    bool IsRunning() const
    {
        return !m_bTerminated;
    }

    virtual void Start( const Position &pos )
    {
        m_Root = pos;
        m_nNodesSearched = 0;
        m_Clock.Reset();
        m_Clock.Start();
    }

    virtual void Stop()
    {

    }

    virtual void SetDirector( const Director &director )
    {
        m_Director = director;
    }

    virtual int Evaluate( Position &pos )
    {
        return m_Evaluator.Evaluate( pos );
    }

protected:
    void Notify( const string &s ) const;
    void Instruct( const string &s ) const;
    void Bestmove( const string &s ) const;

    void SearchComplete()
    {
        stringstream ss;

        ss.str( "" );
        ss << ( string ) m_Result.GetFirst();
        Bestmove( ss.str() );

        m_bTerminated = true;
    }

    /** Conducts a search starting at m_Root. */
    virtual int Search() = 0;

    int m_nNodesSearched;
    mutex m_Lock;
    atomic_bool m_bTerminated;
    Interface *m_pInterface;
    thread m_Thread;
    Evaluator m_Evaluator;
    Director m_Director;
    Clock m_Clock;

    Moves m_Result;
    int m_Score;

    SearcherBase() {};

    Position m_Root;
};

class SearcherReporting : public SearcherBase
{
public:
    SearcherReporting( Interface &interface ) :
        SearcherBase( interface )
    {
        m_tLastReport = m_Clock.Get();
    };

    virtual void Report( const Position &pos )
    {
        Clock::ChessTickType tMilliSinceStart = m_Clock.Get();

        if ( abs( tMilliSinceStart - m_tLastReport ) < 1000 )
            return;

        if ( tMilliSinceStart == 0 )
            return;

        m_tLastReport = tMilliSinceStart;

        uint64_t nodesPerSec = m_nNodesSearched * 1000 / tMilliSinceStart;

        int hashFull = pos.GetHashTable()->GetHashFull();

        stringstream ss;

        ss << "info time " << tMilliSinceStart
           << " nodes " << m_nNodesSearched
           << " nps " << nodesPerSec
           << " hashfull " << hashFull ;

        Instruct( ss.str() );
    }

    Clock::ChessTickType m_tLastReport;
};

class SearcherThreaded : public SearcherReporting
{
public:
    typedef SearcherReporting super;
    typedef lock_guard<mutex> SearchLockType;

    SearcherThreaded( Interface &interface ) :
        SearcherReporting( interface )
    { }

    virtual ~SearcherThreaded()
    {
        Stop();
    }

    virtual void Start( const Position &pos )
    {
        Stop();

        super::Start( pos );

        SearchLockType guard( m_Lock );

        if ( m_bTerminated == false )
            return;

        m_Result.Clear();
        m_bTerminated = false;
        m_Director.Action();
        m_Thread = thread( &SearcherThreaded::Search, this );
    }

    virtual void Stop()
    {
        SearchLockType guard( m_Lock );

        bool bCanJoin = m_Thread.joinable();
        m_bTerminated = true;
        if ( bCanJoin )
            m_Thread.join();

        m_Director.Cut();
    }

protected:

    virtual int Search()
    {
        m_Director.Action();
        int nCurrentDepth = 0;

        while ( !m_bTerminated )
        {
            nCurrentDepth++;
            Moves PV;
            if ( ( nCurrentDepth > 1 ) &&
                    m_Director.ShouldCut( m_Clock.Get(),
                                          m_Score,
                                          nCurrentDepth,
                                          m_Root,
                                          PV
                                        ) )
            {
                m_bTerminated = true;
                break;
            }

            m_Score = InternalSearch( -BIG_NUMBER, BIG_NUMBER,
                                      nCurrentDepth, m_Root, PV );
            m_Result = PV;
            if ( m_Result.Count() == 0 )
                Die( "Size of principal variation is zero!" );

            ReportCurrentPrincipalVariation( nCurrentDepth, PV );

            if ( m_bTerminated )
                break;
        }

        SearchComplete();
        return m_Score;
    }

    void ReportCurrentPrincipalVariation( unsigned int nCurrentDepth,
                                          const Moves &PV )
    {
        stringstream ss;
        ss << "info depth " << nCurrentDepth;
        ss << " pv " << ( string )PV;
        ss << " score ";

        int absScore;
        absScore = abs( m_Score );

        if ( absScore > CHECKMATE_VALUE )
        {
            int mate = ( 1 + KING_VALUE - absScore ) / 2;
            if ( m_Score < 0 )
                mate = -mate;

            ss << "mate " << mate;
        }
        else
            ss << "cp " << m_Score;

        Instruct( ss.str() );
    }

    virtual int InternalSearch( int alpha, int beta, int depthleft,
                                Position &pos, Moves &pv ) = 0;

protected:
    SearcherThreaded();
};

class SearcherPrincipalVariation : public SearcherThreaded
{
    typedef SearcherThreaded super;
public:
    SearcherPrincipalVariation( Interface &interface ) :
        super( interface )
    { }

protected:
    virtual int InternalSearch( int, int, int depth,
                                Position &pos, Moves &pv )
    {
        return SearchPrincipalVariation( -BIG_NUMBER, BIG_NUMBER, depth, pos, pv );
    }

    /* Returns true if no search is necessary at this node due to a transposition table hit. */
    virtual bool CheckTranspositionTable( Move &bestMove,
                                          int &nSearchResult,
                                          const Position &pos, const int alpha,
                                          const int beta, const int depth )
    {
        const PositionHashEntry *pEntry = pos.LookUp();
        bestMove = NullMove;

        /* Logic copied heavily from Bob Hyatt at http://www.open-chess.org/viewtopic.php?f=5&t=1872 */
        /* See if an entry in the hash table exists at this depth for this
        * position...
        */
        if ( pEntry )
        {
            /* 1. Is the draft >= remaining depth of search (was this hash entry stored
             * from a search at least as deep as the current one ? )
             */
            if ( pEntry->m_Depth >= depth )
            {
                /*
                2. If so, there are three types of entries with 3 different actions.  All of these
                depend on the hash entry "type"
                */
                switch ( pEntry->m_TypeBits )
                {
                /*
                a. EXACT. Return the score from the table.  When you get back to search,
                you do not need to search any further, you can use that score as the
                "search result" and return.
                */
                case HET_PRINCIPAL_VARIATION:
                    bestMove = pEntry->m_BestMove;
                    nSearchResult = pEntry->m_Score;
                    return true;

                /*
                b. UPPER.  If the value from the table, which is an "upper bound" (but is
                actually alpha at the time the entry was stored ) is less than or equal to
                the current alpha value, return a "fail low" indication to search which says
                "just return alpha, no need to search".  This test ensures that the stored
                "upper bound" is <= the current alpha value, otherwise we don't know whether
                to fail low or not.
                */
                case HET_ALL_NODE:
                    if ( pEntry->m_Score <= alpha )
                    {
                        bestMove = pEntry->m_BestMove;
                        nSearchResult = pEntry->m_Score;
                        return true;
                    }
                    break;

                /*
                c. LOWER.  If the value from the table, which is a "lower bound" ( but is actually
                beta at the time the entry was stored ) is >= beta, return a "fail high"
                indication to search which says "just return beta, no need to do a search." */
                case HET_CUT_NODE:
                    if ( pEntry->m_Score >= beta )
                    {
                        bestMove = pEntry->m_BestMove;
                        nSearchResult = beta;
                        return true;
                    }
                    break;

                default:
                    Die( "Unknown PositionHashEntry type; hash table corruption?" );
                };
            };

            /* We got an exact match but the search wasn't deep enough to
            * simply return.  So seed this search with the exact value
            * from the hash table.
            */
            bestMove = pEntry->m_BestMove;
        }

        /* The caller does need to do a full width search */
        return false;
    }

    virtual void GetMoves( Moves &myMoves, Position &pos, const int /*depth*/ )
    {
        myMoves = pos.GetMoves();
        if ( myMoves.IsEmpty() )
            Die( "No moves could be generated!" );
    }

    void FilterCheckResolvingMoves( Moves &myMoves, Position &pos )
    {
        Moves checkResolvingMoves;
        /* Filter out all moves to ones that resolve the check */
        Moves::iterator it = myMoves.begin();

        while ( it != myMoves.end() )
        {
            Position tempPos( pos, *it );
            if ( !tempPos.CanKingBeCapturedNow() )
                checkResolvingMoves.Add( *it );

            ++it;
        }

        myMoves = checkResolvingMoves;
    }

    bool IsEndOfGame( int &score, Position &pos, Moves &myMoves )
    {
        if ( pos.CanKingBeCapturedNow() )
        {
            score = KING_VALUE;
            return true;
        }

        if ( pos.IsCheck() )
        {
            FilterCheckResolvingMoves( myMoves, pos );
            if ( myMoves.Count() == 0 )
            {
                // checkmate, no move possible
                score = -KING_VALUE;
                return true;
            }
        }

        if ( pos.CountHashesInHistory( pos.GetHash() ) >= 3 )
        {
            // draw by repetition
            score = 0;
            return true;
        }

        if ( pos.IsStalemate() )
        {
            score = 0;
            return true;
        }

        return false;
    }

    int AttenuateForMate( int score )
    {
        if ( abs( score ) > CHECKMATE_VALUE )
        {
            int attenuate = ( score > 0 ) ? -1 : 1;
            score += attenuate;
        }
        return score;
    }

    virtual bool IsFrontier( int &score, Position &pos, int &/*alpha*/,
                             int /*beta*/, int depth )
    {
        if ( depth <= 0 )
        {
            Report( pos );
            score = Evaluate( pos );
            return true;
        }

        return false;
    }

    virtual int SearchNode( int beta, int alpha, int depth, Position &nextPos,
                            Moves &currentPV )
    {
        return -SearchPrincipalVariation( -beta, -alpha, depth - 1, nextPos,
                                          currentPV );
    }

    virtual bool CheckPreviousSearchResults( int &/*score*/, Position &/*pos*/,
            Moves &/*pv*/, int &/*alpha*/, int &/*beta*/, const int /*depth*/ )
    {
        return false;
    }

    virtual void CacheNodeType( const HashEntryType &/*het*/, Position &/*pos*/,
                                const int /*score*/,
                                const Move &/*move*/ )
    {}

    virtual int SearchPrincipalVariation( int alpha, int beta, int depth,
                                          Position &pos, Moves &pv )
    {
        /* Overall structure lifted egregiously from
         * http://chessprogramming.wikispaces.com/Principal+Variation+Search */
        /* Lots of other ideas from http://www.open-chess.org/viewtopic.php?f=5&t=1872
         */

        int score = 0;
        Move bestMove = NullMove;
        Moves bestPV, currentPV, myMoves;

        if ( CheckPreviousSearchResults( score, pos, pv, alpha, beta, depth ) )
            return score;

        if ( IsFrontier( score, pos, alpha, beta, depth ) )
            return score;

        m_nNodesSearched++;

        GetMoves( myMoves, pos, depth );

        if ( bestMove != NullMove )
        {
            /* We got a recommendation from the transposition table. */
            myMoves.Bump( bestMove );
        }

        if ( IsEndOfGame( score, pos, myMoves ) )
            return score;

        bool bFirstSearch = true;
        bool bAlphaExceeded = false;

        for ( auto &move : myMoves )
        {
            currentPV = pv;
            currentPV.Make( move );
            Position nextPos( pos, move );
            score = SearchNode( beta, alpha, depth, nextPos, currentPV );

            // Attenuate for distance from mate, so that mate in 2 is preferable to mate in 5
            score = AttenuateForMate( score );

            if ( bFirstSearch )
            {
                bestPV = currentPV;
                bestMove = move;
                bFirstSearch = false;
            }

            if ( score >= beta )
            {
                /* Hard beta cutoff of the search now.  This is a CUT node, and the hash entry
                * is called "LOWER" because the score you have is a lower bound, where the
                * real score is greater than or equal to beta...
                * Cut nodes(Knuth's Type 2), otherwise known as fail-high nodes, are nodes in which a
                * beta-cutoff was performed. So with bounds [a,b], s>=b. A minimum of one move at a
                * Cut-node needs to be searched. The score returned is a lower bound (might be
                * greater) on the exact score of the node.
                */
                pv = bestPV;
                CacheNodeType( HET_CUT_NODE, pos, score, bestPV.GetLast() );
                return beta;   // fail-high beta-cutoff
            }

            if ( score > alpha )
            {
                /* The score is between alpha and beta.  We have a new best move.  This could
                * be an exact entry in the hash table, if it survives the rest of the search at this level.
                */
                bAlphaExceeded = true;
                alpha = score; // alpha acts like max in MiniMax
                bestPV = currentPV;
                bestMove = move;
            }

            if ( m_bTerminated )
                break;
        }

        if ( bAlphaExceeded )
            CacheNodeType( HET_PRINCIPAL_VARIATION, pos, score, bestPV.GetLast() );
        else
            CacheNodeType( HET_ALL_NODE, pos, score, bestPV.GetLast() );

        pv = bestPV;
        return alpha;
    }

};

class SearcherQuiescence : public SearcherPrincipalVariation
{
    typedef SearcherPrincipalVariation super;

public:
    SearcherQuiescence( Interface &interface ) :
        super( interface )
    { }

    virtual bool IsFrontier( int &score, Position &pos, int &alpha, int beta,
                             int depth )
    {
        if ( depth <= 0 )
        {
            score = Evaluate( pos );
            if ( score >= beta )
            {
                score = beta;
                return true;
            }
            if ( alpha < score )
                alpha = score;
            Report( pos );
            Moves captures = pos.GetCaptures();
            if ( captures.IsEmpty() )
                return true;

            return false;
        }

        Report( pos );
        return false;
    }

    virtual void GetMoves( Moves &myMoves, Position &pos, const int depth )
    {
        if ( depth > 0 )
        {
            myMoves = pos.GetMoves();
            if ( myMoves.IsEmpty() )
                Die( "No moves could be generated!" );
        }
        else
        {
            myMoves = pos.GetCaptures();
            if ( myMoves.IsEmpty() )
                Die( "No captures could be generated!" );
        }
    }
};

typedef SearcherQuiescence Searcher;

const Piece *BoardBase::Set( const Square &s, const Piece *piece )
{
    return Set( s.I(), s.J(), piece );
}

const Piece *BoardBase::Get( const Square &s ) const
{
    return Get( s.I(), s.J() );
}

bool BoardBase::IsEmpty( const Square &square ) const
{
    return ( Get( square.I(), square.J() ) == &None );
}

bool Piece::IsDifferent( const Square &dest, const Board &board ) const
{
    const Piece *piece = board.Get( dest );

    if ( piece == &None )
        return false;

    return ( m_Color != piece->GetColor() );
}

bool Piece::IsDifferentOrEmpty( const Square &dest, const Board &board ) const
{
    const Piece *piece = board.Get( dest );

    if ( piece == &None )
        return true;

    return ( m_Color != piece->GetColor() );
}

Moves NoPiece::GenerateMoves( const Square & /*source*/,
                              const Position & /*board*/ ) const
{
    Moves moves;
    return moves;
}

void Pawn::AddAndPromote( Moves &moves, Move &m, const bool bIsPromote ) const
{
    if ( bIsPromote )
    {
        Color color = m.GetPiece()->GetColor();
        if ( color == WHITE )
        {
            Move m1;
            m1 = m;
            m1.SetPromoteTo( &WhiteQueen );
            moves.Add( m1 );

            m1 = m;
            m1.SetPromoteTo( &WhiteKnight );
            moves.Add( m1 );

            m1 = m;
            m1.SetPromoteTo( &WhiteBishop );
            moves.Add( m1 );

            m1 = m;
            m1.SetPromoteTo( &WhiteRook );
            moves.Add( m1 );
        }
        else
        {
            Move m1;
            m1 = m;
            m1.SetPromoteTo( &BlackQueen );
            moves.Add( m1 );

            m1 = m;
            m1.SetPromoteTo( &BlackKnight );
            moves.Add( m1 );

            m1 = m;
            m1.SetPromoteTo( &BlackBishop );
            moves.Add( m1 );

            m1 = m;
            m1.SetPromoteTo( &BlackRook );
            moves.Add( m1 );
        }
    }
    else
        moves.Add( m );
}

void Pawn::AddEnPassantMove( Move &m, const Square &dest, Moves &moves ) const
{
    Pawn *pPawn;
    pPawn = GetColor() ? &WhitePawn : &BlackPawn;
    m.Dest( dest );
    m.Score( pPawn->PieceValue() );
    AddAndPromote( moves, m, false );
}

Moves Pawn::GenerateMoves( const Square &source, const Position &pos ) const
{
    Moves moves;
    Square dest = source;
    Color movingColor = GetColor();
    const Board &board = pos.GetBoard();

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
                AddAndPromote( moves, m, bIsPromote );
        }
    }

    // Generate capture moves
    dest = source.Add( -1, d );
    if ( dest.IsOnBoard() && IsDifferent( dest, board ) )
    {
        m.Dest( dest );
        m.Score( board.Get( dest )->PieceValue() );
        AddAndPromote( moves, m, bIsPromote );
    }

    dest = source.Add( 1, d );
    if ( dest.IsOnBoard() && IsDifferent( dest, board ) )
    {
        m.Dest( dest );
        m.Score( board.Get( dest )->PieceValue() );
        AddAndPromote( moves, m, bIsPromote );
    }

    Square enPassant = pos.EnPassant();
    // Generate en passant moves
    if ( enPassant.IsOnBoard() )
    {
        dest = source.Add( -1, d );
        if ( dest == enPassant )
            AddEnPassantMove( m, dest, moves );

        dest = source.Add( 1, d );
        if ( dest == enPassant )
            AddEnPassantMove( m, dest, moves );
    }

    return moves;
}

Moves Knight::GenerateMoves( const Square &source, const Position &pos ) const
{
    Move m( this, source, source );
    Moves moves;
    const Board &board = pos.GetBoard();

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

Moves Bishop::GenerateMoves( const Square &source, const Position &pos ) const
{
    Moves moves;
    Move m( this, source, source );
    const Board &board = pos.GetBoard();

    moves.TryRayAttack( m, board, 1, 1 );
    moves.TryRayAttack( m, board, 1, -1 );
    moves.TryRayAttack( m, board, -1, 1 );
    moves.TryRayAttack( m, board, -1, -1 );

    return moves;
}

Moves Rook::GenerateMoves( const Square &source, const Position &pos ) const
{
    Moves moves;
    Move m( this, source, source );
    const Board &board = pos.GetBoard();

    moves.TryRayAttack( m, board, 0, 1 );
    moves.TryRayAttack( m, board, 0, -1 );
    moves.TryRayAttack( m, board, -1, 0 );
    moves.TryRayAttack( m, board, 1, 0 );

    return moves;
}

Moves King::GenerateCastlingMoves( const Square &source,
                                   const Position &pos ) const
{
    Moves moves;
    Board board = pos.GetBoard();

    if ( pos.GetColorToMove() == WHITE )
    {
        if ( pos.m_bVirginWhiteKing )
        {
            if ( pos.m_bVirginA1 &&
                    ( board.Get( A1 ) == &WhiteRook ) &&
                    ( board.Get( B1 ) == &None ) &&
                    ( board.Get( C1 ) == &None ) &&
                    ( board.Get( D1 ) == &None )
               )
            {
                Position nextPos( pos, NullMove );
                nextPos.m_bVirginBlackKing = false;
                nextPos.m_bVirginWhiteKing = false;

                Moves responses = nextPos.GetMoves();

                bool bCanCastle = true;

                for ( auto response : responses )
                {
                    if ( ( response.Dest() == C1 ) ||
                            ( response.Dest() == D1 ) ||
                            ( response.Dest() == E1 ) )
                    {
                        bCanCastle = false;
                        break;
                    }
                }

                if ( bCanCastle ) {
                    Move m( this, source, C1 );
                    moves.Add( m );
                }
            }

            if ( pos.m_bVirginH1 &&
                    ( board.Get( F1 ) == &None ) &&
                    ( board.Get( G1 ) == &None ) &&
                    ( board.Get( H1 ) == &WhiteRook )
               )
            {
                Position nextPos( pos, NullMove );
                nextPos.m_bVirginBlackKing = false;
                nextPos.m_bVirginWhiteKing = false;

                Moves responses = nextPos.GetMoves();

                bool bCanCastle = true;

                for ( auto response : responses )
                {
                    if ( ( response.Dest() == E1 ) ||
                            ( response.Dest() == F1 ) ||
                            ( response.Dest() == G1 ) )
                    {
                        bCanCastle = false;
                        break;
                    }
                }

                if ( bCanCastle ) {
                    Move m( this, source, G1 );
                    moves.Add( m );
                }
            }

        } // if ( m_bVirginWhiteKing )
    }
    else
    {
        if ( pos.m_bVirginBlackKing )
        {
            if ( pos.m_bVirginA8 &&
                    ( board.Get( A8 ) == &BlackRook ) &&
                    ( board.Get( B8 ) == &None ) &&
                    ( board.Get( C8 ) == &None ) &&
                    ( board.Get( D8 ) == &None )
               )
            {
                Position nextPos( pos, NullMove );
                nextPos.m_bVirginBlackKing = false;
                nextPos.m_bVirginWhiteKing = false;

                Moves responses = nextPos.GetMoves();

                bool bCanCastle = true;

                for ( auto response : responses )
                {
                    if ( ( response.Dest() == C8 ) ||
                            ( response.Dest() == D8 ) ||
                            ( response.Dest() == E8 ) )
                    {
                        bCanCastle = false;
                        break;
                    }
                }

                if ( bCanCastle ) {
                    Move m( this, source, C8 );
                    moves.Add( m );
                }
            }

            if ( pos.m_bVirginH8 &&
                    ( board.Get( F8 ) == &None ) &&
                    ( board.Get( G8 ) == &None ) &&
                    ( board.Get( H8 ) == &BlackRook )
               )
            {
                Position nextPos( pos, NullMove );
                nextPos.m_bVirginBlackKing = false;
                nextPos.m_bVirginWhiteKing = false;

                Moves responses = nextPos.GetMoves();

                bool bCanCastle = true;

                for ( auto response : responses )
                {
                    if ( ( response.Dest() == E8 ) ||
                            ( response.Dest() == F8 ) ||
                            ( response.Dest() == G8 ) )
                    {
                        bCanCastle = false;
                        break;
                    }
                }

                if ( bCanCastle ) {
                    Move m( this, source, G8 );
                    moves.Add( m );
                }
            }
        } // if ( m_bVirginBlackKing )
    }

    return moves;
}

Moves King::GenerateMoves( const Square &source, const Position &pos ) const
{
    Move m( this, source, source );
    Moves moves;

    const Board &board = pos.GetBoard();

    moves = GenerateCastlingMoves( source, pos );

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

Moves Queen::GenerateMoves( const Square &source, const Position &pos ) const
{
    Moves moves;
    Move m( this, source, source );
    const Board &board = pos.GetBoard();

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
        m_Position.Setup();
    }

    Position *GetPosition()
    {
        return &m_Position;
    }
    void SetPosition( Position &pos )
    {
        m_Position = pos;
    }

protected:
    friend class Interface;
    Position m_Position;
};

class Interface;
Interface *s_pDefaultInterface = NULL;

class Interface : Object
{
public:
    enum ProtocolType
    {
        PROTOCOL_XBOARD,
        PROTOCOL_UCI
    };

    Interface( istream *in = &cin, ostream *out = &cout ) :
        m_In( in ),
        m_Out( out ),
        m_bShowThinking( false ),
        m_bLogInputToFile( false ),
        m_pGame( new Game )
    {
        m_pSearcher = shared_ptr<Searcher> ( new Searcher( *this ) );
        s_pDefaultInterface = this;
    }

    ~Interface()
    {
    }

    void LogLineToFile( const string &line )
    {
// sorry, Windows only...
#ifdef WIN32
        const char *sFileName = "c:\\temp\\superpawn.log";
        string outLine = line;
        outLine.append( "\n" );
        FILE *fp;
        fopen_s( &fp, sFileName, "a" );
        fwrite( line.c_str(), sizeof( char ), outLine.length(), fp );
        fclose( fp );
#endif
    }

    ostream *GetOut() const
    {
        return m_Out;
    }
    void SetOut( ostream *val )
    {
        m_Out = val;
    }

    istream *GetIn() const
    {
        return m_In;
    }
    void SetIn( istream *val )
    {
        m_In = val;
    }

    void AnnounceSelf()
    {
        {
            stringstream ss;
            ss << "Superpawn build number " << BUILD_NUMBER << ", build ID " << BUILD_ID;
            Notify( ss.str() );
        }
        {
            stringstream ss;
            ss << "Superpawn built on " << __DATE__ << " " << __TIME__;
            Notify( ss.str() );
        }
        {
            stringstream ss;
            ss << "For more info: " << WEB_URL;
            Notify( ss.str() );
        }
    }

    void Run()
    {
        m_Out->setf( ios::unitbuf );
        string sInputLine;
        RegisterAll();

        AnnounceSelf();

        for ( ;; )
        {
            getline( *m_In, sInputLine );
            if ( m_bLogInputToFile )
                LogLineToFile( sInputLine );

            LockGuardType guard( m_Lock );
            Execute( sInputLine );
        }
    }

    typedef lock_guard<mutex> LockGuardType;

    mutex &GetLock()
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

    void RegisterCommand( const string &sCommand,
                          INTERFACE_FUNCTION_TYPE( pfnCommand ) )
    {
        m_CommandMap[ sCommand ] = pfnCommand;
    };

    void RegisterAll()
    {
        RegisterCommand( "uci",     &Interface::UCI );
        RegisterCommand( "quit",    &Interface::Quit );
        RegisterCommand( "testone", &Interface::TestOne );
        RegisterCommand( "test",  &Interface::Test );
    }

    INTERFACE_PROTOTYPE( UCI )
    {
        RegisterUCI( sParams );

        Instruct( "id name Superpawn" );
        Instruct( "id author John Byrd" );

        Instruct( "option name Hash type spin default 1 min 1 max 2048" );
        Instruct( "option name UCI_EngineAbout type string default " WEB_URL );

        string none;
        New( none );

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
        stringstream ss( sParams );
        string sParam, sName;
        size_t nValue = 1;

        while ( ss >> sParam )
        {
            if ( sParam == "name" )
                ss >> sName;
            else if ( sParam == "value" )
                ss >> nValue;
            else
            {
                stringstream sfail;
                sfail << "Unrecognized SetOption parameter: " << sParam;
                Notify( sfail.str() );
            }
        }

        if ( !sName.empty() )
        {
            if ( sName == "Hash" )
                s_pPositionHashTable->SetSize( nValue * 1024 * 1024 );
        }
        else
            Notify( "SetOption: Could not find name of the option to set" );
    }

    INTERFACE_PROTOTYPE( TestOne )
    {
        UCIPosition( sParams );
        UCIGo( "wtime 30000 btime 30000 winc 0 binc 0 movestogo 1" );
        do {
            chrono::milliseconds delay( 500 );
            this_thread::sleep_for( delay );
        } while ( m_pSearcher->IsRunning() );
    }

    INTERFACE_PROTOTYPE_NO_PARAMS( Test )
    {
        TestOne( "startpos moves g1f3 e7e6 e2e4 d8f6 b1c3 f6f4 d2d4 f4d6 e4e5 d6b4 f1d3 b4a5 e1g1 c7c5 d4c5 a5c5 c1e3 c5b4 c3b5 b8a6 a2a3 b4g4 b5a7 a8a7 e3a7 a6c7 d1d2 f7f6 d2c3 e8d8 a7b6 d8e7 c3c7 e7f7 c7c8 g8e7 c8d7 g4f4 b6e3 f4g4 f3d4 f6e5 h2h3 g4h5 d7e6 f7e8 d3b5 e8d8" );
        TestOne( "startpos moves e2e4 d7d5 e4d5 d8d5 c2c4 d5e4 f1e2 e4g2 e2f3 g2g6 d1b3 g6a6 c4c5 e7e5 f3e2 a6a5 b3d5 b8c6 b2b4 a5b4 e2h5 g7g6 h5f3 b4d4 d5d4 c6d4 f3d1 f8c5 c1b2 g8f6 g1f3 d4f3 d1f3 c5d4 b2d4 e5d4 h1g1 e8g8 g1g5 h7h6 g5c5 f8e8 e1d1 c8g4 f3g4 f6g4 d1c2 g4f2 c5c4 d4d3 c2b3 a8c8 c4f4 f2e4 b3c4 c8d8 f4h4 g6g5 h4h3 e8e6 a2a4 e6c6 c4b3 c6a6 b3a3" );
        TestOne( "startpos moves g1f3 e7e6 d2d4 d8f6 a2a3 c7c5 d4c5 b8c6 e2e4 f8c5 f1b5 a7a6 b5c6 b7c6 e4e5 f6g6 f3g5 c5b6 b1c3 b6c7 c1f4 h7h6 d1d3 h6g5 d3g6 f7g6 f4g5 c7e5 f2f4 e5c3 b2c3 g8f6 g5f6 g7f6 c3c4 g6g5 f4g5 f6g5 a3a4 h8h4 c4c5 h4c4 a1a3 c4c5 a3h3 c5c2 h3h8 e8f7 e1d1 c2g2 h2h3 g2g3 a4a5 f7g7 h8e8 g7f7" );
        TestOne( "startpos moves b1c3 e7e5 e2e3 g8f6 g1f3 e5e4 e1e2 e4f3 e2d3 b7b6 c3b5 c8a6" );
        TestOne( "fen 6r1/1p1b4/5k1p/2P1p2K/1P5P/p3R1P1/P4P2/8 b - - 0 45" );
    }

    INTERFACE_PROTOTYPE( UCIGo )
    {
        stringstream ss( sParams );
        string sParam;
        Director director;

        while ( ss >> sParam )
        {
            if ( sParam == "depth" )
            {
                int depth;
                ss >> depth;
                director.m_nDepth = depth;
                continue;
            }
            else if ( sParam == "wtime" )
            {
                int wtime;
                ss >> wtime;
                director.m_WhiteTime = wtime;
                continue;
            }
            else if ( sParam == "btime" )
            {
                int btime;
                ss >> btime;
                director.m_BlackTime = btime;
                continue;
            }
            else if ( sParam == "winc" )
            {
                int winc;
                ss >> winc;
                director.m_WhiteInc = winc;
                continue;
            }
            else if ( sParam == "binc" )
            {
                int binc;
                ss >> binc;
                director.m_BlackInc = binc;
                continue;
            }
            else if ( sParam == "movestogo" )
            {
                unsigned int movestogo;
                ss >> movestogo;
                director.m_nMovesToGo = movestogo;
                continue;
            }
            else if ( sParam == "nodes" )
            {
                unsigned int nodes;
                ss >> nodes;
                director.m_nNodes = nodes;
                continue;
            }
            else if ( sParam == "mate" )
            {
                unsigned int mate;
                ss >> mate;
                director.m_nMateInMoves = mate;
                continue;
            }
            else if ( sParam == "movetime" )
            {
                unsigned int movetime;
                ss >> movetime;
                director.m_nMoveTime = movetime;
                continue;
            }
            else if ( sParam == "infinite" )
            {
                director.m_bInfinite = true;
                continue;
            }
            else
            {
                stringstream ssfail;
                ssfail << "Unknown go parameter: ";
                ssfail << sParam;
                Notify( ssfail.str() );
                break;
            }
        };

        m_pSearcher->SetDirector( director );
        m_pSearcher->Start( * ( m_pGame->GetPosition() ) );
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
                        sFen.append( " " );
                    sFen.append( sArg );
                }

                Position pos;
                pos.SetFEN( sFen );
                m_pGame->SetPosition( pos );

                Notify( "New position: " );
                Notify( sFen );
            }

            if ( sType == "startpos" )
                m_pGame->New();

            if ( sType == "moves" )
            {
                string sMove;

                while ( ss >> sMove )
                {
                    Position *pLast = m_pGame->GetPosition();
                    Move nextMove( sMove, pLast->GetColorToMove() );

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

    int TimeToSeconds( const string &sTime )
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

    void Execute( const string &sCommand )
    {
        string sParams, sVerb;

        stringstream ss;
        ss.str( sCommand );

        ss >> sVerb;

        if ( sVerb.length() < sCommand.length() )
            sParams = sCommand.substr( sVerb.length() + 1, MAX_COMMAND_LENGTH );

        InterfaceFunctionType ic = m_CommandMap[ sVerb ];

        if ( ic )
            ( this->*ic )( sParams );
        else
        {
            stringstream unk;
            unk << "Unknown command: " << sCommand ;
            Notify( unk.str() );
        }
    }

protected:
    istream *m_In;
    ostream *m_Out;

    mutex m_Lock;

    Moves m_PrincipalVariation;

    ProtocolType m_Protocol;
    int m_Protover;
    bool m_bShowThinking;
    bool m_bPonder;
    bool m_bLogInputToFile;
    shared_ptr<Game> m_pGame;
    shared_ptr<Searcher> m_pSearcher;

protected:
    unordered_map<string, InterfaceFunctionType> m_CommandMap;
};

void Die( const string &s )
{
    bool bAbortOnDie = true;

    if ( s_pDefaultInterface )
        s_pDefaultInterface->Notify( s );

    if ( bAbortOnDie )
        abort();
}

void SearcherBase::Notify( const string &s ) const
{
    m_pInterface->Notify( s );
}

void SearcherBase::Instruct( const string &s ) const
{
    m_pInterface->Instruct( s );
}

void SearcherBase::Bestmove( const string &s ) const
{
    m_pInterface->Bestmove( s );
}

int main( int , char ** )
{
    Clock c;
    PieceInitializer pieceInitializer;
    HashInitializer hashInitializer;
    HashTableInitializer hashTableInitializer;
    Interface i;

    i.Run();

    return 0;
}

