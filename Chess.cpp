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
** \todo Better endgame logic for say KRK (MopUp only pulls kings together)
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
const unsigned int HASH_TABLE_SIZE = 128 * 1024 * 1024;

/** Maximum command length for UCI commands. */
const unsigned int MAX_COMMAND_LENGTH = 64 * 256;

/** Search depth used for a bare "go" with no clock, depth or node limit */
const unsigned int DEFAULT_SEARCH_DEPTH = 6; //-V112

/** Iterative deepening never goes past this depth. */
const unsigned int MAX_SEARCH_DEPTH = 100;

/** Deepest ply for which killer moves are kept. */
const int MAX_PLY = 128;

/** Half-width, in centipawns, of the root aspiration window. */
const int ASPIRATION_WINDOW = 40;

/** Depth and hash size used by the "bench" command.  Fixed, so that the
 ** node count it prints depends on nothing but the engine's decisions. */
const unsigned int BENCH_DEPTH = 8;
const unsigned int BENCH_HASH_BYTES = 16 * 1024 * 1024;

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
const int KNIGHT_VALUE = 325;
const int BISHOP_VALUE = 325;
const int ROOK_VALUE = 500;
const int QUEEN_VALUE = 975;
const int CHECKMATE_VALUE = 990000; // any abs. value greater than this is mate
const int KING_VALUE = 1000000;

const int DRAW_SCORE = 0;

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

PieceSquareRawTableType psrtDefault =
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
PieceSquareRawTableType psrtKnight =
{
    -40, -30, -30, -30, -30, -30, -30, -40,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -30,   0,   5,  10,  10,   5,   0, -30,
    -30,  10,  15,  30,  30,  15,  10, -30,
    -30,  10,  15,  30,  30,  15,  10, -30,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -40, -30, -30, -30, -30, -30, -30, -40
};

PieceSquareRawTableType psrtWhitePawnEarly =
{
    0,   0,   0,   0,   0,   0,   0,   0,
    5,   5,  10, -20, -20,  10,   5,   5,
    5,  -5, -10,   0,   0, -10,  -5,   5,
    0,   0,   0,  20,  20,   0,   0,   0,
    20, 20,  30,  40,  40,  30,  20,  20,
    40, 50,  60,  80,  80,  60,  50,  40,
    60, 70,  80, 100, 100,  80,  70,  60,
    0,   0,   0,   0,   0,   0,   0,   0
};

/* Go for the touchdown!*/
PieceSquareRawTableType psrtWhitePawnLate =
{
    0,    0,   0,   0,   0,   0,   0,   0,
    5,    5,  10, -20, -20,  10,   5,   5,
    5,   -5,  -5,   0,   0,  -5,  -5,   5,
    0,    0,  20,  40,  40,  20,   0,   0,
    30,  30,  30,  50,  50,  50,  50,  50,
    70,  70,  70,  70,  70,  70,  70,  70,
    100,100, 100, 100, 100, 100, 100, 100,
    0,    0,   0,   0,   0,   0,   0,   0
};


PieceSquareRawTableType psrtBishop =
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

PieceSquareRawTableType psrtRook =
{
    0,   0,  0, 20, 20, 20,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,

    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    20, 20, 20, 20, 20, 20, 20, 20,
    -5,  0,  0,  0,  0,  0,  0, -5
};

/* Reward castled squares, mildly penalise sitting on the central files.
 * The earlier +70 castled bonus roughly doubled real engines' valuations
 * of castling and caused Superpawn's eval to drift ~90 cp more optimistic
 * than Stockfish's on opening positions. Values here are deliberately
 * smaller than the material of a pawn so the eval doesn't swing a full
 * tempo over positional king shuffling.
 */
PieceSquareRawTableType psrtWhiteKingEarly =
{
    0, 10, 20, -5,-10, -5, 20, 10,
    0,  0,  0, -5,-10, -5,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0
};

PieceSquareRawTableType psrtWhiteKingLate =
{
    -50, -40, -30, -30, -30, -30, -40, -50,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  40,  60,  60,  40, -10, -30,
    -30, -10,  40,  60,  60,  40, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -50, -30, -30, -30, -30, -30, -30, -50
};

class PieceSquareTableBase : public Object
{
public:
    PieceSquareTableBase()
    {
        for ( unsigned int i = 0; i < MAX_SQUARES; i++ )
            m_SourceTable[ i ] = 0;
    }

    PieceSquareTableBase( const PieceSquareRawTableType &table )
    {
        m_SourceTable = table;
    }

    /* These methods are not virtual: no code dispatches through a base
     * pointer, and the derived class's two-argument Get is an overload,
     * not an override. Marking either side virtual made the compiler warn
     * that the base Get was being hidden by the derived two-arg one.
     */
    void InvertColor()
    {
        PieceSquareRawTableType temp;
        temp = m_SourceTable;
        for ( unsigned int i = 0; i < MAX_FILES; i++ )
            for ( unsigned int j = 0; j < MAX_FILES; j++ )
            {
                m_SourceTable[ i + j * MAX_FILES ] =
                    temp[ i + ( ( MAX_FILES - 1 ) - j ) * MAX_FILES];
            }
    }

    int Get( unsigned int index ) const
    {
        return m_SourceTable[ ( size_t ) index ];
    }

    PieceSquareRawTableType m_SourceTable;
};

const int MAX_PIECE_SQUARE_INTERPOLATIONS = 64;

class PieceSquareTableInterpolating : PieceSquareTableBase
{
public:
    PieceSquareTableInterpolating() :
        m_nCurrentTable( 0 ),
        m_fPhase( 0.0f )
    {

    }

    void Append( const PieceSquareRawTableType &table,
                 const float fInterpolationFactor = 0.0f )
    {
        /* for now, we're order sensitive on this. */
        PieceSquareTableBase baseTable( table );
        m_SourceTables.push_back( baseTable );
        m_Phases.push_back( fInterpolationFactor );
    }

    PieceSquareTableInterpolating( const PieceSquareRawTableType &table,
                                   const float fInterpolationFactor = 0.0f ) :
        m_nCurrentTable( 0 ),
        m_fPhase( 0.0f )
    {
        Append( table, fInterpolationFactor );
    }

    void InvertColor()
    {
        for ( auto &table : m_InterpolatedTables )
            table.InvertColor();
    }

    /* Named distinctly from the base's one-argument Get() so this isn't a
     * hidden overload. Callers that want the phase-interpolated value use
     * GetAtPhase; the vector of base tables still uses Get(index).
     */
    int GetAtPhase( unsigned int index, const float fPhase = 0.0f ) const
    {

        size_t nSize = m_InterpolatedTables.size();

        if ( m_InterpolatedTables.size() == 1 )
            return m_InterpolatedTables[0].Get( index );

        float fTable = fPhase * ( float )m_InterpolatedTables.size();
        int nTable = ( int )fTable;

        if ( nTable >= ( int )nSize )
            nTable = ( int )nSize - 1;

        if ( nTable < 0 )
            nTable = 0;

        return m_InterpolatedTables[nTable].Get( index );
    }

    virtual void CalculateInterpolations()
    {
        /* Only implemented for up to two children.  Could reimplement for more. */
        switch ( m_SourceTables.size() )
        {
        case 1:
            m_nCurrentTable = 0;
            m_InterpolatedTables.push_back( m_SourceTables[0] );
            break;
        case 2:
            for ( unsigned int i = 0; i < MAX_PIECE_SQUARE_INTERPOLATIONS; i++ )
            {
                m_nCurrentTable = 0;
                PieceSquareRawTableType pstInterpolated;
                for ( unsigned int sq = 0; sq < MAX_SQUARES; sq++ )
                {
                    float fScale;
                    fScale = ( float )i / ( float )MAX_PIECE_SQUARE_INTERPOLATIONS;
                    pstInterpolated[sq] = ( int ) (
                                              ( 1.0f - fScale ) * m_SourceTables[0].Get( sq ) +
                                              fScale * m_SourceTables[1].Get( sq ) );
                }
                PieceSquareTableBase tableBase( pstInterpolated );
                m_InterpolatedTables.push_back( tableBase );
            }
            break;
        default:
            Die( "Unsupported number of interpolated table sources!" );
        }
    }

protected:
    typedef vector< PieceSquareTableBase > TablesType;
    typedef vector< float > PhaseType;
    TablesType m_InterpolatedTables;
    TablesType m_SourceTables;
    PhaseType m_Phases;
    unsigned int m_nCurrentTable;
    float m_fPhase;
};

typedef PieceSquareTableInterpolating PieceSquareTable;

class PieceSquareTableInitializer : Object
{
public:
    PieceSquareTableInitializer()
    {
        Initialize();
    }
    void Initialize();
};

/** A centisecond wall clock. */
class Clock : Object
{
public:

    typedef chrono::steady_clock NativeClockType;
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
        m_PieceSquareTable = psrtDefault;
    }

    Piece( Color color )
    {
        m_Color = color;
        m_PieceType = NONE;
        m_pOtherColor = NULL;
        m_PieceSquareTable = psrtDefault;
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

void PieceSquareTableInitializer::Initialize()
{
    PieceSquareTable pstWhitePawn, pstBlackPawn, pstWhiteKnight, pstBlackKnight;
    PieceSquareTable pstWhiteBishop, pstBlackBishop, pstWhiteRook, pstBlackRook;
    PieceSquareTable pstWhiteQueen, pstBlackQueen, pstWhiteKing, pstBlackKing;

    pstWhitePawn = PieceSquareTable();
    pstWhitePawn.Append( psrtWhitePawnEarly , 0.0f );
    pstWhitePawn.Append( psrtWhitePawnLate, 1.0f );
    pstWhitePawn.CalculateInterpolations();
    pstBlackPawn = pstWhitePawn;
    pstBlackPawn.InvertColor();

    pstWhiteBishop = PieceSquareTable( psrtBishop );
    pstWhiteBishop.CalculateInterpolations();
    pstBlackBishop = pstWhiteBishop;
    pstBlackBishop.InvertColor();

    pstWhiteKnight = PieceSquareTable( psrtKnight );
    pstWhiteKnight.CalculateInterpolations();
    pstBlackKnight = pstWhiteKnight;
    pstBlackKnight.InvertColor();

    pstWhiteRook = PieceSquareTable( psrtRook );
    pstWhiteRook.CalculateInterpolations();
    pstBlackRook = pstWhiteRook;
    pstBlackRook.InvertColor();

    pstWhiteQueen = PieceSquareTable( psrtDefault );
    pstWhiteQueen.CalculateInterpolations();
    pstBlackQueen = pstWhiteQueen;
    pstBlackQueen.InvertColor();

    pstWhiteKing = PieceSquareTable();
    pstWhiteKing.Append( psrtWhiteKingEarly, 0.0f );
    pstWhiteKing.Append( psrtWhiteKingLate, 1.0f );
    pstWhiteKing.CalculateInterpolations();
    pstBlackKing = pstWhiteKing;
    pstBlackKing.InvertColor();

    WhitePawn.SetPieceSquareTable( pstWhitePawn );
    BlackPawn.SetPieceSquareTable( pstBlackPawn );

    WhiteBishop.SetPieceSquareTable( pstWhiteBishop );
    BlackBishop.SetPieceSquareTable( pstBlackBishop );

    WhiteKnight.SetPieceSquareTable( pstWhiteKnight );
    BlackKnight.SetPieceSquareTable( pstBlackKnight );

    WhiteRook.SetPieceSquareTable( pstWhiteRook );
    BlackRook.SetPieceSquareTable( pstBlackRook );

    WhiteQueen.SetPieceSquareTable( pstWhiteQueen );
    BlackQueen.SetPieceSquareTable( pstBlackQueen );

    WhiteKing.SetPieceSquareTable( pstWhiteKing );
    BlackKing.SetPieceSquareTable( pstBlackKing );
}

NoPiece None;

const Piece **AllPieces;
const int AllPiecesSize = 12;


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
/* One hash per castling right (WK, WQ, BK, BQ). A right is active iff
 * both the rook and its king are still virgin. */
HashValue s_CastleRightsHash[ 4 ];
/* One hash per en-passant file. Only XORed in when the target square
 * is set (i.e. the previous move was a pawn double-push). */
HashValue s_EnPassantFileHash[ MAX_FILES ];

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
    virtual int GetPieceSquareValue( int index, const float fPhase ) const
    {
        return Get( index )->
               GetPieceSquareTable().GetAtPhase( index, fPhase );
    }

    virtual int GetPieceSquareValue( const Square &s, const float fPhase ) const;
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

        for ( unsigned int i = 0; i < 4; i++ )
            s_CastleRightsHash[ i ] = mt();

        for ( unsigned int i = 0; i < MAX_FILES; i++ )
            s_EnPassantFileHash[ i ] = mt();
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

    unsigned int ToIndex() const
    {
        return ( i + j * 8 );
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
        Square s1( i + s.I(), j + s.J() );
        return s1;
    }

    bool operator== ( const Square &right ) const
    {
        return ( ( i == right.i ) && ( j == right.j ) );
    }

    int ManhattanDistanceTo( const Square &other ) const
    {
        return abs( i - other.i ) + abs( j - other.j );
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

int BoardPieceSquare::GetPieceSquareValue( const Square &s,
        const float fPhase ) const
{
    return GetPieceSquareValue( s.ToIndex(), fPhase );
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

        m_Piece = &None;
        m_Score = 0;

        /* A malformed string (or the GUI null move "0000") becomes a move
         * from off the board, which will never match a generated move.
         */
        if ( moveLength != 4 && moveLength != 5 ) //-V112
        {
            m_Source = Square( -99, -99 );
            m_Dest = Square( -99, -99 );
            return;
        }

        m_Source.I( sMove[0] - 'a' );
        m_Source.J( sMove[1] - '1' );
        m_Dest.I( sMove[2] - 'a' );
        m_Dest.J( sMove[3] - '1' );

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

        int p = 0;
        m_AllPieces[p++] = &WhitePawn;
        m_AllPieces[p++] = &BlackPawn;
        m_AllPieces[p++] = &WhiteKnight;
        m_AllPieces[p++] = &BlackKnight;
        m_AllPieces[p++] = &WhiteBishop;
        m_AllPieces[p++] = &BlackBishop;
        m_AllPieces[p++] = &WhiteRook;
        m_AllPieces[p++] = &BlackRook;
        m_AllPieces[p++] = &WhiteQueen;
        m_AllPieces[p++] = &BlackQueen;
        m_AllPieces[p++] = &WhiteKing;
        m_AllPieces[p++] = &BlackKing;
        AllPieces = m_AllPieces;
    }
    const Piece *m_AllPieces[AllPiecesSize];
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
        delete[] m_pEntries;
    }

    /** Forget everything: a new game is starting. */
    virtual void Purge()
    {
        SetSize( m_SizeBytes );
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
            if ( pHE->m_Depth > entry.m_Depth )
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

        delete[] m_pEntries;
        m_pEntries = nullptr;
        m_nEntriesInUse = 0;
        m_CacheLookups = m_CacheMisses = m_CacheHits = 0;

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

/* This array must correspond with the AllPieces array exactly. */
const float fPhaseMaterial[AllPiecesSize] =
{
    0.0f, /*pawns*/
    0.0f,
    1.0f, /*knights*/
    1.0f,
    1.0f, /*bishops*/
    1.0f,
    2.0f, /* rooks */
    2.0f,
    4.0f, /*queens*/
    4.0f,
    0.0f, /*kings*/
    0.0f
};

static float s_fMaximumMaterial;

class Material : Object
{
    friend class Position;
protected:
    void Initialize()
    {
        m_fPhase = -1.0f;
        for ( int i = 0; i < AllPiecesSize; i++ )
            m_nCount[i] = 0;
    }

    void UpdateFrom( const Position &pos );
    void CalculateMaximumMaterial()
    {
        s_fMaximumMaterial = GetMaterial();
    }

    void CaptureMaterial( const Piece *pPiece )
    {
        for ( int i = 0; i < AllPiecesSize; i++ )
        {
            if ( AllPieces[i] == pPiece )
            {
                m_nCount[i]--;
                m_fPhase = -1.0f;
                return;
            }
        }
        Die( "Could not find piece to capture!" );
    }

    void AddPiece( const Piece *pPiece )
    {
        for ( int i = 0; i < AllPiecesSize; i++ )
        {
            if ( AllPieces[i] == pPiece )
            {
                m_nCount[i]++;
                m_fPhase = -1.0f;
                return;
            }
        }
        Die( "Could not find piece to add!" );
    }

    float GetMaterial()
    {
        float fMaterial = 0.0f;
        for ( int i = 0; i < AllPiecesSize; i++ )
            fMaterial += m_nCount[i] * fPhaseMaterial[i];
        return fMaterial;
    }

    float GetPhase()
    {
        if ( m_fPhase < 0.0f )
        {
            m_fPhase = 1.0f - ( GetMaterial() / s_fMaximumMaterial );

            if ( m_fPhase > 1.0f )
                m_fPhase = 1.0f;
            if ( m_fPhase < 0.0f )
                m_fPhase = 0.0f;

        }

        return m_fPhase;
    }

    /** True when neither side can possibly deliver mate: bare kings, a
     ** lone minor piece, or only knights (at most two) on the board.
     **/
    bool IsInsufficient() const
    {
        /* Indices follow the AllPieces order: WP BP WN BN WB BB WR BR WQ BQ */
        if ( m_nCount[0] || m_nCount[1] || m_nCount[6] || m_nCount[7] ||
                m_nCount[8] || m_nCount[9] )
            return false;

        unsigned int knights = m_nCount[2] + m_nCount[3];
        unsigned int bishops = m_nCount[4] + m_nCount[5];

        if ( knights + bishops <= 1 )
            return true;

        return ( bishops == 0 && knights <= 2 );
    }

    /** Does this color own anything besides pawns and the king? */
    bool HasNonPawnMaterial( Color color ) const
    {
        /* Iterate rather than slicing AllPieces by index: anything that
         * reorders PieceInitializer's assignment of indices (e.g. adding
         * a new piece type) would otherwise silently break null-move
         * pruning, which uses this method as its zugzwang guard.
         */
        for ( int i = 0; i < AllPiecesSize; i++ )
        {
            if ( m_nCount[i] == 0 ) continue;
            const Piece *p = AllPieces[i];
            if ( p->GetColor() != color ) continue;
            PieceType t = p->Type();
            if ( t != PAWN && t != KING ) return true;
        }
        return false;
    }

protected:
    unsigned int m_nCount[ AllPiecesSize ];
    float m_fPhase;
};

class Position : Object
{
    friend class PositionHasher;
    /* King moves need to see somewhat deeply inside the Position structure for
     * castling at the moment; this could be refactored I suppose
     */
    friend class King;
    friend class Material;

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

        if ( &move == &NullMove )
        {
            m_bNullMove = true;
            SetColorToMove( !GetColorToMove() );
            ComputeHash();
            return;
        }

        Square source = move.Source();
        DevirginizeRooks( source );

        const Piece *pPiece = GetBoard().Get( source );
        DevirginizeKing( pPiece );

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

        ComputeHash();
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
        m_nPlySinceCaptureOrPawnMove = 0;
        m_sEnPassant.Set( -1, -1 );
        m_Moves.Clear();
        m_Board.Initialize();
        m_Material.Initialize();
        m_bIsCheckDetermined = false;
        m_bIsCheck = false;
        m_PreviousPositions.clear();
        m_pParent = nullptr;
        m_Hash = 0;
        m_bNullMove = false;
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
        return GetHashTable()->LookUp( GetHash() );
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


    void ComputeHash()
    {
        PositionHasher ph( *this );
        m_Hash = ph.GetHash();
    }

    /** How many times has this position occurred before, counting the
     ** search path (the chain of parents) and then the game history
     ** stored on the detached position at the top of that chain?
     **/
    unsigned int CountRepetitions() const
    {
        unsigned int count = 0;
        const Position *p = m_pParent;

        while ( p != nullptr )
        {
            if ( p->m_Hash == m_Hash )
                count++;

            if ( p->m_pParent == nullptr )
            {
                for ( auto previous : p->m_PreviousPositions )
                    if ( previous == m_Hash )
                        count++;
            }

            p = p->m_pParent;
        }

        return count;
    }

    /** Positions created by making a move only point at their parent
     ** rather than copying the whole game history.  Before a position
     ** outlives its parent (it is stored in the Game, or becomes a
     ** search root) that chain must be flattened into a list it owns.
     **/
    void DetachHistory()
    {
        if ( m_pParent == nullptr )
            return;

        PreviousPositionType history;
        for ( const Position *p = m_pParent; p != nullptr; p = p->m_pParent )
        {
            history.push_back( p->m_Hash );
            if ( p->m_pParent == nullptr )
                history.insert( history.end(),
                                p->m_PreviousPositions.begin(),
                                p->m_PreviousPositions.end() );
        }

        m_PreviousPositions = history;
        m_pParent = nullptr;
    }

    void CopyFrom( const Position &position )
    {
        m_Board = position.m_Board;
        m_ColorToMove = position.m_ColorToMove;
        m_Material = position.m_Material;
        m_nMaterialScore = position.m_nMaterialScore;
        /* The history is reached through the parent, not copied. */
        m_PreviousPositions.clear();
        m_pParent = &position;
        m_Hash = 0;
        m_bNullMove = false;

        m_nPly = position.m_nPly + 1;
        m_sEnPassant = Square( -1, -1 );

        m_nPlySinceCaptureOrPawnMove = position.m_nPlySinceCaptureOrPawnMove + 1;
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
        {
            /* The fifty move rule resets whenever a pawn is moved. */
            m_nPlySinceCaptureOrPawnMove = 0;
            HandleEnPassant( move, position, captureSquare );
        }
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
        const Piece *pCaptured = m_Board.Get( move.Dest() );
        const Piece *pPromotingPawn = m_Board.Get( move.Source() );

        m_nMaterialScore = position.GetScore() +
                           ( move.GetPromoteTo()->PieceValue() +
                             pCaptured->PieceValue() ) *
                           GetColorBias();

        /* Promotion is both a pawn move and (sometimes) a capture, so the
         * fifty-move clock resets regardless. MovePiece normally handles
         * the pawn-move reset, but PromotePiece bypasses it entirely.
         */
        m_nPlySinceCaptureOrPawnMove = 0;

        /* Update the material counts: pawn is gone, promoted piece appears,
         * and anything we captured on the back rank is also gone.
         */
        if ( pCaptured != &None )
            m_Material.CaptureMaterial( pCaptured );
        m_Material.CaptureMaterial( pPromotingPawn );
        m_Material.AddPiece( move.GetPromoteTo() );

        m_Board.Set( move.Dest().I(), move.Dest().J(),
                     move.GetPromoteTo() );
    }

    /** This is the sexiest function in the entire program. */
    void DevirginizeKing( const Piece *pPiece )
    {
        if ( pPiece == &WhiteKing )
            m_bVirginWhiteKing = false;

        if ( pPiece == &BlackKing )
            m_bVirginBlackKing = false;
    }

    /* This is the second sexiest function in the entire program. */
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
        const Piece *pCaptured = m_Board.Get( captureSquare );
        m_nMaterialScore = position.GetScore() +
                           pCaptured->PieceValue() * GetColorBias();

        if ( pCaptured != &None )
        {
            m_Material.CaptureMaterial( pCaptured );
            m_nPlySinceCaptureOrPawnMove = 0;
        }

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

    unsigned int GetPlySinceCaptureOrPawnMove() const
    {
        return m_nPlySinceCaptureOrPawnMove;
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

    /** Where is this color's king?  Off the board if there isn't one. */
    Square FindKing( Color color ) const
    {
        const Piece *pKing = ( color == WHITE ) ?
                             ( const Piece * )&WhiteKing :
                             ( const Piece * )&BlackKing;

        for ( unsigned int index = 0; index < MAX_SQUARES; index++ )
            if ( m_Board.Get( index ) == pKing )
                return Square( index & 7, index >> 3 );

        return Square( -1, -1 );
    }

    /** Does any piece of 'byColor' attack 'target'?  Looks outward from
     ** the target, so it costs a few dozen board probes rather than a
     ** full move generation.
     **/
    bool IsSquareAttacked( const Square &target, Color byColor ) const
    {
        if ( !target.IsOnBoard() )
            return false;

        const int ti = target.I();
        const int tj = target.J();

        const Piece *pPawn, *pKnight, *pBishop, *pRook, *pQueen, *pKing;
        if ( byColor == WHITE )
        {
            pPawn = &WhitePawn;
            pKnight = &WhiteKnight;
            pBishop = &WhiteBishop;
            pRook = &WhiteRook;
            pQueen = &WhiteQueen;
            pKing = &WhiteKing;
        }
        else
        {
            pPawn = &BlackPawn;
            pKnight = &BlackKnight;
            pBishop = &BlackBishop;
            pRook = &BlackRook;
            pQueen = &BlackQueen;
            pKing = &BlackKing;
        }

        /* Pawns attack diagonally forward, so look diagonally backward. */
        const int pj = ( byColor == WHITE ) ? tj - 1 : tj + 1;
        if ( pj >= 0 && pj < ( int )MAX_FILES )
        {
            if ( ti > 0 && m_Board.Get( ti - 1, pj ) == pPawn )
                return true;
            if ( ti < ( int )HIGHEST_FILE && m_Board.Get( ti + 1, pj ) == pPawn )
                return true;
        }

        static const int knightSteps[8][2] =
        {
            { 1, 2 }, { -1, 2 }, { 1, -2 }, { -1, -2 },
            { 2, 1 }, { -2, 1 }, { 2, -1 }, { -2, -1 }
        };
        static const int kingSteps[8][2] =
        {
            { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
            { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 }
        };

        for ( int n = 0; n < 8; n++ )
        {
            Square sq( ti + knightSteps[n][0], tj + knightSteps[n][1] );
            if ( sq.IsOnBoard() && m_Board.Get( sq ) == pKnight )
                return true;

            Square ks( ti + kingSteps[n][0], tj + kingSteps[n][1] );
            if ( ks.IsOnBoard() && m_Board.Get( ks ) == pKing )
                return true;
        }

        /* Sliders: walk each ray until something is hit. */
        for ( int n = 0; n < 8; n++ )
        {
            const int di = kingSteps[n][0];
            const int dj = kingSteps[n][1];
            const bool bDiagonal = ( di != 0 && dj != 0 );
            const Piece *pSlider = bDiagonal ? pBishop : pRook;

            Square sq( ti + di, tj + dj );
            while ( sq.IsOnBoard() )
            {
                const Piece *p = m_Board.Get( sq );
                if ( p != &None )
                {
                    if ( p == pSlider || p == pQueen )
                        return true;
                    break;
                }
                sq.Change( di, dj );
            }
        }

        return false;
    }

    /** True if the side to move could capture the enemy king, i.e. the
     ** previous move was illegal.
     **/
    bool CanKingBeCapturedNow() const
    {
        return IsSquareAttacked( FindKing( !m_ColorToMove ), m_ColorToMove );
    }

    bool IsCheck()
    {
        if ( m_bIsCheckDetermined )
            return m_bIsCheck;

        m_bIsCheck = IsSquareAttacked( FindKing( m_ColorToMove ), !m_ColorToMove );
        m_bIsCheckDetermined = true;
        return m_bIsCheck;
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
        m_Material.UpdateFrom( *this );
        m_Material.CalculateMaximumMaterial();
        ComputeHash();
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
        unsigned int nHalfmoveClock;
        int nMoves;

        ss >> sBoard >> sToMove;

        /* The last four fields are optional (EPD-style FENs omit them). */
        if ( !( ss >> sVirgins ) )
            sVirgins = "-";
        if ( !( ss >> sEnPassant ) )
            sEnPassant = "-";
        if ( !( ss >> nHalfmoveClock ) )
            nHalfmoveClock = 0;
        if ( !( ss >> nMoves ) || nMoves < 1 )
            nMoves = 1;

        m_nPlySinceCaptureOrPawnMove = nHalfmoveClock;

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

        SetColorToMove( sToMove == "b" ? BLACK : WHITE );

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
                m_bVirginWhiteKing = true;
                break;

            case 'Q':
                m_bVirginA1 = true;
                m_bVirginWhiteKing = true;
                break;

            case 'k':
                m_bVirginH8 = true;
                m_bVirginBlackKing = true;
                break;

            case 'q':
                m_bVirginA8 = true;
                m_bVirginBlackKing = true;
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
        ComputeHash();

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

        bool canWK = m_bVirginH1 && m_bVirginWhiteKing;
        bool canWQ = m_bVirginA1 && m_bVirginWhiteKing;
        bool canBK = m_bVirginH8 && m_bVirginBlackKing;
        bool canBQ = m_bVirginA8 && m_bVirginBlackKing;

        if ( !( canWK || canWQ || canBK || canBQ ) )
            s += "-";
        else
        {
            if ( canWK ) s += "K";
            if ( canWQ ) s += "Q";
            if ( canBK ) s += "k";
            if ( canBQ ) s += "q";
        }

        stringstream ss;

        ss << " ";
        ss << ( string ) m_sEnPassant;
        ss << " ";
        ss << m_nPlySinceCaptureOrPawnMove;
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

    HashValue GetHash() const
    {
        return m_Hash;
    }

    float GetPhase()
    {
        return m_Material.GetPhase();
    }

    bool IsInsufficientMaterial() const
    {
        return m_Material.IsInsufficient();
    }

    /** Was this position produced by passing rather than moving? */
    bool WasNullMove() const
    {
        return m_bNullMove;
    }

    bool HasNonPawnMaterial( Color color ) const
    {
        return m_Material.HasNonPawnMaterial( color );
    }


protected:
    Board   m_Board;
    Color   m_ColorToMove;
    Material m_Material;
    unsigned int    m_nPly;
    int m_nMaterialScore;
    /** For implementing the fifty move rule. */
    unsigned int m_nPlySinceCaptureOrPawnMove;
    /** Virgin rooks; can tell whether any of the four rooks has been moved */
    bool m_bVirginH1, m_bVirginA1, m_bVirginH8, m_bVirginA8;
    bool m_bVirginWhiteKing, m_bVirginBlackKing;
    Square m_sEnPassant;
    /** Cached generated moves. */
    Moves m_Moves;
    Moves m_Captures;
    bool m_bIsCheckDetermined;
    bool m_bIsCheck;
    /** Zobrist hash of this position, fixed once it is constructed. */
    HashValue m_Hash;
    bool m_bNullMove;
    /** The position this one was reached from, or null if detached. */
    const Position *m_pParent;
    /** Hashes of earlier positions; only used on a detached position. */
    typedef std::vector< HashValue > PreviousPositionType;
    PreviousPositionType m_PreviousPositions;
};

void Material::UpdateFrom( const Position &pos )
{
    Initialize();
    Board board = pos.GetBoard();
    for ( unsigned int square = 0; square < MAX_SQUARES; square++ )
    {
        const Piece *pPiece = board.Get( square );
        for ( int p = 0; p < AllPiecesSize; p++ )
        {
            if ( pPiece == &None )
                continue;

            if ( pPiece == AllPieces[p] )
            {
                m_nCount[p]++;
                break;
            }
        }
    }
}

HashValue PositionHasher::GetHash() const
{
    HashValue h = m_pPosition->m_Board.GetHash() ^
                  s_PieceColorHash[( int ) m_pPosition->m_ColorToMove ];

    if ( m_pPosition->m_bVirginH1 && m_pPosition->m_bVirginWhiteKing )
        h ^= s_CastleRightsHash[ 0 ];
    if ( m_pPosition->m_bVirginA1 && m_pPosition->m_bVirginWhiteKing )
        h ^= s_CastleRightsHash[ 1 ];
    if ( m_pPosition->m_bVirginH8 && m_pPosition->m_bVirginBlackKing )
        h ^= s_CastleRightsHash[ 2 ];
    if ( m_pPosition->m_bVirginA8 && m_pPosition->m_bVirginBlackKing )
        h ^= s_CastleRightsHash[ 3 ];

    if ( m_pPosition->m_sEnPassant.IsOnBoard() )
        h ^= s_EnPassantFileHash[ m_pPosition->m_sEnPassant.I() ];

    return h;
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
    /** Material balance from white's point of view, by scanning the board. */
    static int WhiteRelative( const Position &pos )
    {
        const Board &board = pos.GetBoard();
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

        return nScore;
    }

    virtual int Evaluate( Position &pos ) const
    {
        return Bias( pos, WhiteRelative( pos ) );
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
                nScore += ( board.GetPieceSquareValue( i, pos.GetPhase() ) *
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

/** Endgame mop-up: once the board is nearly empty and one side is
 ** clearly ahead, reward driving the losing king toward the edge and
 ** bringing the winning king up to it.  Signed by who is ahead, so the
 ** losing side is pushed away rather than drawn in.
 **/
class EvaluatorMopUp : public EvaluatorBase
{
    virtual int Evaluate( Position &pos ) const
    {
        const float fTurnOnAt = 0.8f;
        const int nMinimumAdvantage = 200;

        if ( pos.GetPhase() < fTurnOnAt )
            return 0;

        /* Material balance from white's point of view. */
        int advantage = pos.GetScore();
        if ( abs( advantage ) < nMinimumAdvantage )
            return 0;

        Square whiteKing, blackKing;
        bool bFoundWhite = false, bFoundBlack = false;

        for ( unsigned int i = 0; i < MAX_FILES; i++ )
            for ( unsigned int j = 0; j < MAX_FILES; j++ )
            {
                Square cur( i, j );
                const Piece *pPiece = pos.GetBoard().Get( cur );

                if ( pPiece == &WhiteKing )
                {
                    whiteKing = cur;
                    bFoundWhite = true;
                }

                if ( pPiece == &BlackKing )
                {
                    blackKing = cur;
                    bFoundBlack = true;
                }
            }

        if ( !bFoundWhite || !bFoundBlack )
            return 0;

        const Square &loser = ( advantage > 0 ) ? blackKing : whiteKing;

        /* Manhattan distance from the centre, doubled to stay in integers:
         * 2 for a central square up to 14 in a corner.
         */
        int centre = abs( 2 * loser.I() - 7 ) + abs( 2 * loser.J() - 7 );
        int kingDistance = whiteKing.ManhattanDistanceTo( blackKing );

        int nScore = centre * 5 + ( 14 - kingDistance ) * 4;

        if ( advantage < 0 )
            nScore = -nScore;

        return Bias( pos, nScore );
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

/** Pawn structure, rook on open file, bishop pair, and king pawn-shield,
 ** all computed from a single board scan.
 **/
class EvaluatorPositional : public EvaluatorBase
{
public:
    virtual int Evaluate( Position &pos ) const
    {
        const Board &board = pos.GetBoard();

        /* Per-file pawn stats, kept from white's perspective (positive is
         * good for white). "MaxRank" is the highest j a pawn of that color
         * sits on; "MinRank" is the lowest. -1 / 8 are sentinels meaning
         * "no pawn of this color on this file."
         */
        int wPawnsOnFile[ MAX_FILES ] = { 0 };
        int bPawnsOnFile[ MAX_FILES ] = { 0 };
        int wMaxRank[ MAX_FILES ];
        int wMinRank[ MAX_FILES ];
        int bMaxRank[ MAX_FILES ];
        int bMinRank[ MAX_FILES ];
        for ( unsigned int i = 0; i < MAX_FILES; i++ )
        {
            wMaxRank[ i ] = -1;
            wMinRank[ i ] = 8;
            bMaxRank[ i ] = -1;
            bMinRank[ i ] = 8;
        }

        int wBishops = 0, bBishops = 0;
        int wRookFiles[ 10 ], bRookFiles[ 10 ];
        int wRookCount = 0, bRookCount = 0;
        Square wKing, bKing;
        bool wKingFound = false, bKingFound = false;

        for ( unsigned int i = 0; i < MAX_FILES; i++ )
            for ( unsigned int j = 0; j < MAX_FILES; j++ )
            {
                const Piece *p = board.Get( i, j );
                if ( p == &None ) continue;

                if ( p == &WhitePawn )
                {
                    wPawnsOnFile[ i ]++;
                    if ( ( int )j > wMaxRank[ i ] ) wMaxRank[ i ] = ( int )j;
                    if ( ( int )j < wMinRank[ i ] ) wMinRank[ i ] = ( int )j;
                }
                else if ( p == &BlackPawn )
                {
                    bPawnsOnFile[ i ]++;
                    if ( ( int )j > bMaxRank[ i ] ) bMaxRank[ i ] = ( int )j;
                    if ( ( int )j < bMinRank[ i ] ) bMinRank[ i ] = ( int )j;
                }
                else if ( p == &WhiteBishop ) wBishops++;
                else if ( p == &BlackBishop ) bBishops++;
                else if ( p == &WhiteRook )
                {
                    if ( wRookCount < 10 ) wRookFiles[ wRookCount++ ] = ( int )i;
                }
                else if ( p == &BlackRook )
                {
                    if ( bRookCount < 10 ) bRookFiles[ bRookCount++ ] = ( int )i;
                }
                else if ( p == &WhiteKing )
                {
                    wKing = Square( i, j );
                    wKingFound = true;
                }
                else if ( p == &BlackKing )
                {
                    bKing = Square( i, j );
                    bKingFound = true;
                }
            }

        const int DOUBLED_PENALTY = 15;
        const int ISOLATED_PENALTY = 15;
        const int ROOK_OPEN = 20;
        const int ROOK_SEMI_OPEN = 10;
        const int BISHOP_PAIR = 30;
        /* Passed-pawn bonus indexed by the pawn's rank (j coordinate).
         * White pawn on rank 6 (j=6) is one square from promotion and so
         * worth the most; black's table is just the mirror.
         */
        static const int PASSED_WHITE[ MAX_FILES ] =
            { 0, 5, 15, 25, 40, 60, 100, 0 };
        static const int PASSED_BLACK[ MAX_FILES ] =
            { 0, 100, 60, 40, 25, 15, 5, 0 };

        int score = 0;

        for ( unsigned int f = 0; f < MAX_FILES; f++ )
        {
            /* Doubled pawns */
            if ( wPawnsOnFile[ f ] > 1 )
                score -= DOUBLED_PENALTY * ( wPawnsOnFile[ f ] - 1 );
            if ( bPawnsOnFile[ f ] > 1 )
                score += DOUBLED_PENALTY * ( bPawnsOnFile[ f ] - 1 );

            /* Isolated pawns: no friendly pawn on either adjacent file */
            if ( wPawnsOnFile[ f ] > 0 )
            {
                bool hasAdj = ( f > 0 && wPawnsOnFile[ f - 1 ] > 0 ) ||
                              ( f < MAX_FILES - 1 && wPawnsOnFile[ f + 1 ] > 0 );
                if ( !hasAdj )
                    score -= ISOLATED_PENALTY * wPawnsOnFile[ f ];
            }
            if ( bPawnsOnFile[ f ] > 0 )
            {
                bool hasAdj = ( f > 0 && bPawnsOnFile[ f - 1 ] > 0 ) ||
                              ( f < MAX_FILES - 1 && bPawnsOnFile[ f + 1 ] > 0 );
                if ( !hasAdj )
                    score += ISOLATED_PENALTY * bPawnsOnFile[ f ];
            }

            /* Passed pawns: look only at the most-advanced pawn on this
             * file. For white that's the pawn with the highest rank; it's
             * passed iff no enemy pawn on this or an adjacent file has
             * a higher rank. Mirror for black.
             */
            if ( wMaxRank[ f ] >= 0 )
            {
                int wr = wMaxRank[ f ];
                bool opposed = false;
                for ( int df = -1; df <= 1 && !opposed; df++ )
                {
                    int ff = ( int )f + df;
                    if ( ff < 0 || ff >= ( int )MAX_FILES ) continue;
                    if ( bMaxRank[ ff ] > wr ) opposed = true;
                }
                if ( !opposed ) score += PASSED_WHITE[ wr ];
            }
            if ( bMinRank[ f ] < ( int )MAX_FILES )
            {
                int br = bMinRank[ f ];
                bool opposed = false;
                for ( int df = -1; df <= 1 && !opposed; df++ )
                {
                    int ff = ( int )f + df;
                    if ( ff < 0 || ff >= ( int )MAX_FILES ) continue;
                    if ( wMinRank[ ff ] < br && wMinRank[ ff ] >= 0 )
                        opposed = true;
                }
                if ( !opposed ) score -= PASSED_BLACK[ br ];
            }
        }

        /* Rook on open or semi-open file */
        for ( int r = 0; r < wRookCount; r++ )
        {
            int f = wRookFiles[ r ];
            if ( wPawnsOnFile[ f ] == 0 )
                score += ( bPawnsOnFile[ f ] == 0 ) ? ROOK_OPEN : ROOK_SEMI_OPEN;
        }
        for ( int r = 0; r < bRookCount; r++ )
        {
            int f = bRookFiles[ r ];
            if ( bPawnsOnFile[ f ] == 0 )
                score -= ( wPawnsOnFile[ f ] == 0 ) ? ROOK_OPEN : ROOK_SEMI_OPEN;
        }

        /* Bishop pair */
        if ( wBishops >= 2 ) score += BISHOP_PAIR;
        if ( bBishops >= 2 ) score -= BISHOP_PAIR;

        /* King safety: shield + open files near king + enemy pawn storm +
         * king-stuck-in-centre, all only material in the opening/early
         * middlegame and scaled linearly down to zero by late middlegame.
         */
        float phase = pos.GetPhase();
        if ( phase < 0.7f )
        {
            float middlegameFactor = 1.0f - phase / 0.7f;
            int wSafety = 0, bSafety = 0;
            if ( wKingFound )
                wSafety = KingSafety( board, wKing, WHITE,
                                      wPawnsOnFile, bPawnsOnFile, bMinRank );
            if ( bKingFound )
                bSafety = KingSafety( board, bKing, BLACK,
                                      bPawnsOnFile, wPawnsOnFile, wMaxRank );
            score += ( int )( ( wSafety - bSafety ) * middlegameFactor );
        }

        return Bias( pos, score );
    }

private:
    /** Positive = this king is safe. Negative = exposed.
     ** 'myPawnsOnFile' / 'enemyPawnsOnFile' are the per-file pawn counts
     ** from the caller's single scan; 'enemyAdvancedRank' is the enemy
     ** pawn closest to this king on each file (bMinRank for white's
     ** king, wMaxRank for black's).
     **/
    static int KingSafety( const Board &board, const Square &king, Color c,
                           const int *myPawnsOnFile,
                           const int *enemyPawnsOnFile,
                           const int *enemyAdvancedRank )
    {
        int kf = king.I();
        int kr = king.J();
        int fwd = ( c == WHITE ) ? 1 : -1;
        int ownBackRank = ( c == WHITE ) ? 0 : ( int )( MAX_FILES - 1 );
        const Piece *myPawn = ( c == WHITE )
                              ? ( const Piece * )&WhitePawn
                              : ( const Piece * )&BlackPawn;

        int score = 0;

        /* Pawn shield: friendly pawns on the three files near the king,
         * on the two ranks in front of it.
         */
        for ( int df = -1; df <= 1; df++ )
        {
            int f = kf + df;
            if ( f < 0 || f >= ( int )MAX_FILES ) continue;
            for ( int dr = 1; dr <= 2; dr++ )
            {
                int r = kr + fwd * dr;
                if ( r < 0 || r >= ( int )MAX_FILES ) continue;
                if ( board.Get( f, r ) == myPawn )
                    score += ( dr == 1 ) ? 6 : 3;
            }
        }

        /* Open / semi-open file near king: file in front of the king with
         * no friendly pawn is a highway for enemy heavy pieces. Fully open
         * (neither side has a pawn) is worse than semi-open.
         */
        for ( int df = -1; df <= 1; df++ )
        {
            int f = kf + df;
            if ( f < 0 || f >= ( int )MAX_FILES ) continue;
            if ( myPawnsOnFile[ f ] == 0 )
                score -= ( enemyPawnsOnFile[ f ] == 0 ) ? 25 : 15;
        }

        /* Enemy pawn storm: enemy pawns within three ranks of the king on
         * the three nearby files. 'enemyAdvancedRank' is already the rank
         * of the enemy pawn closest to us -- a sentinel outside [0,7] if
         * there's no such pawn.
         */
        static const int STORM[ 4 ] = { 0, 25, 15, 5 };
        for ( int df = -1; df <= 1; df++ )
        {
            int f = kf + df;
            if ( f < 0 || f >= ( int )MAX_FILES ) continue;
            int er = enemyAdvancedRank[ f ];
            if ( er < 0 || er >= ( int )MAX_FILES ) continue;
            int dist = fwd * ( er - kr );
            if ( dist > 0 && dist <= 3 )
                score -= STORM[ dist ];
        }

        /* King still on d/e file on its own back rank in the opening/early
         * middlegame: hasn't castled, probably about to get hit. The PST
         * already punishes this square directly, so keep the extra
         * penalty modest to avoid double-counting castling.
         */
        if ( ( kf == 3 || kf == 4 ) && kr == ownBackRank )
            score -= 5;

        return score;
    }
};

class EvaluatorStandard : public EvaluatorWeighted
{
public:
    EvaluatorStandard()
    {
        /* Mobility is deliberately absent: counting only the side to
         * move's moves made the score depend on whose turn it was.
         */
        m_Weighted.Add( m_Material );
        /* The PSTs here were designed with aggressive corner penalties
         * (knights/bishops -30 at edges) that over-reward basic
         * development; at weight 0.8 the engine was ~70 cp optimistic
         * vs Stockfish on quiet openings. Halving brings the eval into
         * a sane range without redesigning each table.
         */
        m_Weighted.Add( m_PieceSquareEvaluator, 0.4f );
        m_Weighted.Add( m_Positional, 1.0f );
        m_Weighted.Add( m_MopUp, 1.0f );
    }

    virtual int Evaluate( Position &pos ) const
    {
        if ( pos.IsInsufficientMaterial() )
            return DRAW_SCORE;

        return m_Weighted.Evaluate( pos );
    }

    EvaluatorMaterial m_Material;
    EvaluatorPieceSquare m_PieceSquareEvaluator;
    EvaluatorMopUp m_MopUp;
    EvaluatorPositional m_Positional;
    EvaluatorWeighted m_Weighted;
};

typedef EvaluatorStandard Evaluator;

void Position::UpdateScore()
{
    /* m_nMaterialScore is kept from white's point of view and updated
     * incrementally from here on, so it must not be biased toward the
     * side to move: that flipped the sign of every FEN position with
     * black to move, for the rest of the game.
     */
    SetScore( EvaluatorSlowMaterial::WhiteRelative( *this ) );
    m_Material.UpdateFrom( *this );
}

/** Keeps track of the search and decides whether it should continue. */
class DirectorBase : Object
{
protected:
    Interface *m_pInterface;
    friend class Interface;
public:
    DirectorBase()
    {
        Initialize();
    }

    void SetInterface( Interface &interface )
    {
        m_pInterface = &interface;
    }

    virtual unsigned int GetDepth() const
    {
        return m_nDepth;
    }

    virtual unsigned int GetNodes() const
    {
        return m_nNodes;
    }

    const Moves &GetSearchMoves() const
    {
        return m_SearchMoves;
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
        m_SearchEmergencyStopTime = 0;
        m_bStopTimeCalculated = false;
    }

    virtual void Action()
    {
        m_SearchStopTime = 0;
        m_SearchEmergencyStopTime = 0;
        m_bStopTimeCalculated = false;
    }

    virtual void Cut()
    {
        m_SearchStopTime = 0;
        m_SearchEmergencyStopTime = 0;
        m_bStopTimeCalculated = false;
    }

    /** Shave a little off a deadline so the bestmove reaches the GUI
     ** before the clock actually runs out.
     **/
    static Clock::ChessTickType SubtractLatency( Clock::ChessTickType t )
    {
        const Clock::ChessTickType margin = 20;
        if ( t > 2 * margin )
            return t - margin;
        return ( t / 2 > 1 ) ? t / 2 : 1;
    }

    /** Decides, once per search, how long to think.  Sets a soft budget
     ** (m_SearchStopTime): once it is half spent no new iteration is
     ** started, because each iteration costs several times its
     ** predecessor.  Also sets a hard budget (m_SearchEmergencyStopTime)
     ** at which the search aborts wherever it happens to be.
     **/
    virtual void CalculateSearchStopTime(
        const Clock::ChessTickType /* currentTime */,
        const int /* nScore */,
        const int /* nDepthSearched */,
        const Position &rootPosition,
        const Moves & /* mPrincipalVariation */
    )
    {
        m_bStopTimeCalculated = true;
        m_SearchStopTime = 0;
        m_SearchEmergencyStopTime = 0;

        if ( m_nMoveTime != 0 )
        {
            m_SearchStopTime = m_nMoveTime;
            m_SearchEmergencyStopTime = SubtractLatency( m_nMoveTime );
            return;
        }

        Color sideToMove = rootPosition.GetColorToMove();
        Clock::ChessTickType timeLeft = ( sideToMove == WHITE ) ?
                                        m_WhiteTime : m_BlackTime;
        Clock::ChessTickType inc = ( sideToMove == WHITE ) ?
                                   m_WhiteInc : m_BlackInc;

        /* No clock at all: leave both limits at zero and ShouldCut falls
         * back to a fixed depth.
         */
        if ( timeLeft <= 0 )
            return;

        Clock::ChessTickType movesToGo = ( m_nMovesToGo != 0 ) ?
                                         m_nMovesToGo : 30;

        Clock::ChessTickType soft = timeLeft / movesToGo + ( inc * 3 ) / 4;

        /* Good chess players tend to fall into a deep think around ply 17 or so.
         * Let's pretend we know what we're doing and do the same.
         */
        int ply = rootPosition.GetPly();
        float factor = 2.0f - fabs( ( float )ply - 17.0f ) / 5.0f ;

        if ( factor > 2.0f )
            factor = 2.0f;
        if ( factor < 1.0f )
            factor = 1.0f;

        soft = ( Clock::ChessTickType )( soft * factor );

        /* Never plan to spend more than a quarter of the clock on one move. */
        if ( soft > timeLeft / 4 )
            soft = timeLeft / 4;
        if ( soft < 1 )
            soft = 1;

        /* Allow an iteration that is already under way to overrun the soft
         * budget, but never by so much that we flag.
         */
        Clock::ChessTickType hard = soft * 3;
        if ( hard > timeLeft / 3 )
            hard = timeLeft / 3;
        hard = SubtractLatency( hard );
        if ( soft > hard )
            soft = hard;

        m_SearchStopTime = soft;
        m_SearchEmergencyStopTime = hard;
    }

    void Notify( const string &s );

    /** Called just before the iterative deepener starts an iteration at
     ** nDepthToSearch.  Returns true if the search should finish instead.
     **/
    virtual bool ShouldCut(
        const Clock::ChessTickType currentTime,
        const int nScore,
        const unsigned int nDepthToSearch,
        const Position &rootPosition,
        const Moves &mPrincipalVariation
    )
    {
        /* This can happen in late end game. */
        if ( nDepthToSearch > MAX_SEARCH_DEPTH )
            return true;

        /* 'go mate N' asks us to stop as soon as we've proved a mate
         * within N moves. We encode mate as |score| > CHECKMATE_VALUE,
         * with KING_VALUE - 2k meaning "mate in k from this ply."
         */
        if ( m_nMateInMoves != 0 )
        {
            int absScore = abs( nScore );
            if ( absScore > CHECKMATE_VALUE )
            {
                int mate = ( 1 + KING_VALUE - absScore ) / 2;
                if ( mate > 0 && mate <= ( int ) m_nMateInMoves )
                    return true;
            }
        }

        if ( m_nDepth != 0 )
            return ( nDepthToSearch > m_nDepth );

        if ( m_bInfinite )
            return false;

        if ( !m_bStopTimeCalculated )
            CalculateSearchStopTime( currentTime, nScore, nDepthToSearch,
                                     rootPosition, mPrincipalVariation );

        if ( m_SearchStopTime == 0 )
        {
            /* No clock and no movetime.  A node limit is enforced by
             * ShouldCutEmergency; otherwise search to a fixed depth.
             */
            if ( m_nNodes != 0 )
                return false;
            return ( nDepthToSearch > DEFAULT_SEARCH_DEPTH );
        }

        /* Don't start an iteration that will probably not finish: each one
         * costs several times its predecessor, so once half the budget is
         * gone the next iteration would overrun it.
         */
        return ( currentTime * 2 >= m_SearchStopTime );
    }

    virtual bool ShouldCutEmergency( const Clock::ChessTickType currentTime,
                                     const uint64_t nNodesSearched = 0 )
    {
        if ( m_nNodes != 0 && nNodesSearched >= m_nNodes )
            return true;

        if ( m_SearchEmergencyStopTime == 0 )
            return false;

        return ( currentTime > m_SearchEmergencyStopTime );
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
    Clock::ChessTickType m_SearchEmergencyStopTime;
    bool m_bStopTimeCalculated;

};


typedef DirectorBase Director;


class SearcherBase : Object
{
public:
    SearcherBase( Interface &interface ) :
        m_nNodesSearched( 0 ),
        m_bQuiet( false )
    {
        m_bTerminated = true;
        m_pInterface = &interface;
        m_Director.SetInterface( interface );
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
        m_Root.DetachHistory();
        m_nNodesSearched = 0;
        m_Score = 0;
        m_Clock.Reset();
        m_Clock.Start();
    }

    virtual void Stop()
    {

    }

    /** Block until any running search finishes of its own accord. */
    virtual void Wait()
    {

    }

    virtual void SetDirector( const Director &director )
    {
        m_Director = director;
        m_Director.SetInterface( *m_pInterface );
    }

    virtual int Evaluate( Position &pos )
    {
        return m_Evaluator.Evaluate( pos );
    }

    uint64_t GetNodesSearched() const
    {
        return m_nNodesSearched;
    }

    /** Quiet searches print no info lines and no bestmove (used by bench). */
    void SetQuiet( bool bQuiet )
    {
        m_bQuiet = bQuiet;
    }

protected:
    void Notify( const string &s ) const;
    void Instruct( const string &s ) const;
    void Bestmove( const string &s ) const;

    void SearchComplete()
    {
        if ( m_Result.Count() == 0 )
        {
            /* Not even the first iteration finished (an immediate "stop",
             * say).  UCI still requires a bestmove, so fall back to the
             * first legal move.
             */
            Moves rootMoves = m_Root.GetMoves();
            for ( const auto &move : rootMoves )
            {
                Position next( m_Root, move );
                if ( !next.CanKingBeCapturedNow() )
                {
                    m_Result.Add( move );
                    break;
                }
            }
        }

        if ( m_Result.Count() > 0 )
            Bestmove( ( string )m_Result.GetFirst() );
        else
            Bestmove( "0000" );

        m_bTerminated = true;
    }

    /** Conducts a search starting at m_Root. */
    virtual int Search() = 0;

    uint64_t m_nNodesSearched;
    bool m_bQuiet;
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

    virtual void Wait()
    {
        SearchLockType guard( m_Lock );

        if ( m_Thread.joinable() )
            m_Thread.join();
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
                break;

            /* Aspiration window: search a narrow window around the last
             * score, widening on the side that fails.
             */
            int alpha = -BIG_NUMBER, beta = BIG_NUMBER;
            if ( nCurrentDepth >= 3 && abs( m_Score ) < CHECKMATE_VALUE )
            {
                alpha = m_Score - ASPIRATION_WINDOW;
                beta = m_Score + ASPIRATION_WINDOW;
            }

            int nScore;
            for ( ;; )
            {
                PV.Clear();
                nScore = InternalSearch( alpha, beta, nCurrentDepth, m_Root, PV );
                if ( m_bTerminated )
                    break;
                /* Widen only a side that is not already wide open, so a
                 * score at or beyond the open window (an illegal root
                 * position whose king can be captured) cannot spin here.
                 */
                if ( nScore <= alpha && alpha > -BIG_NUMBER )
                    alpha = -BIG_NUMBER;
                else if ( nScore >= beta && beta < BIG_NUMBER )
                    beta = BIG_NUMBER;
                else
                    break;
            }

            /* An iteration cut short by the clock, the node limit or a
             * "stop" command has scored a tree that was only partly
             * searched, so neither its score nor its PV can be trusted.
             * Keep the previous completed iteration's result instead.
             */
            if ( m_bTerminated )
                break;

            m_Score = nScore;
            m_Result = PV;
            /* The length of the principal variation may be zero if the position
             * is some sort of terminal condition such as a stalemate or draw.
             */
            ReportCurrentPrincipalVariation( nCurrentDepth, PV );
        }

        SearchComplete();
        return m_Score;
    }

    void ReportCurrentPrincipalVariation( unsigned int nCurrentDepth,
                                          const Moves &PV )
    {
        stringstream ss;
        ss << "info depth " << nCurrentDepth;
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

        /* pv is a variable-length field whose contents extend to end-of-line,
         * so it must come last so GUIs don't try to parse later fields as moves.
         */
        ss << " pv " << ( string )PV;

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
    virtual int InternalSearch( int alpha, int beta, int depth,
                                Position &pos, Moves &pv )
    {
        return SearchPrincipalVariation( alpha, beta, depth, pos, pv );
    }

public:
    virtual void Start( const Position &pos )
    {
        ClearHeuristics();
        super::Start( pos );
    }

protected:
    int PlyFromRoot( const Position &pos ) const
    {
        return ( int ) pos.GetPly() - ( int ) m_Root.GetPly();
    }

    /* ---- move ordering: killer moves and history heuristic ---- */

    Move m_Killers[ MAX_PLY ][ 2 ];
    int m_History[ 2 ][ MAX_SQUARES ][ MAX_SQUARES ];

    void ClearHeuristics()
    {
        for ( int p = 0; p < MAX_PLY; p++ )
            m_Killers[p][0] = m_Killers[p][1] = NullMove;
        for ( int c = 0; c < 2; c++ )
            for ( unsigned int i = 0; i < MAX_SQUARES; i++ )
                for ( unsigned int j = 0; j < MAX_SQUARES; j++ )
                    m_History[c][i][j] = 0;
    }

    bool IsKiller( const Move &move, int ply ) const
    {
        return ( ply < MAX_PLY ) &&
               ( m_Killers[ply][0] == move || m_Killers[ply][1] == move );
    }

    /** A quiet move just caused a beta cutoff: remember it. */
    void RecordCutoff( const Move &move, int ply, int depth, Color color )
    {
        if ( ply < MAX_PLY && !( m_Killers[ply][0] == move ) )
        {
            m_Killers[ply][1] = m_Killers[ply][0];
            m_Killers[ply][0] = move;
        }

        int &h = m_History[color][move.Source().ToIndex()][move.Dest().ToIndex()];
        h += depth * depth;
        if ( h > ( 1 << 20 ) )
            h = ( 1 << 20 );
    }

    /** Sort the list so that the most promising moves come first: the
     ** transposition-table move, then captures by MVV/LVA, then killer
     ** moves, then quiet moves by history.  The Score field is reused
     ** as the sort key; this works on a copy, not the Position's cache.
     **/
    void OrderMoves( Moves &moves, const Position &pos, const Move &ttMove,
                     int ply, int depth ) const
    {
        /* Quiescence lists are already sorted by captured value. */
        if ( depth <= 0 )
            return;

        Color color = pos.GetColorToMove();

        for ( auto &m : moves )
        {
            int key;
            if ( m == ttMove )
                key = 1 << 30;
            else if ( m.Score() > 0 )
            {
                /* Captures and promotions: Score holds the material won. */
                key = ( 1 << 24 ) + m.Score() * 16 - m.GetPiece()->PieceValue() / 10;
            }
            else if ( IsKiller( m, ply ) )
                key = ( 1 << 23 ) + ( ( m_Killers[ply][0] == m ) ? 1 : 0 );
            else
                key = m_History[color][m.Source().ToIndex()][m.Dest().ToIndex()];

            m.Score( key );
        }

        moves.Sort();
    }

    virtual void GetMoves( Moves &myMoves, Position &pos, const int /*depth*/ )
    {
        /* An empty list (only possible from a broken FEN) simply falls
         * through to the no-legal-moves handling in the move loop.
         */
        myMoves = pos.GetMoves();
    }

    int m_nSearchExtension;

    virtual void ExtendSearchDepth( )
    {
        m_nSearchExtension = 1;
    }

    virtual void ReduceSearchDepth( )
    {
        m_nSearchExtension = -1;
    }

    virtual void ResetSearchDepth()
    {
        m_nSearchExtension = 0;
    }

    /** Draws that can be decided without looking at any move.  Mate and
     ** stalemate are discovered by the move loop, which finds no legal
     ** move to play.
     **/
    bool IsEndOfGame( int &score, Position &pos )
    {
        /* Repetition has already been tested by SearchPrincipalVariation. */
        if ( pos.GetPlySinceCaptureOrPawnMove() >= 100 )
        {
            score = DRAW_SCORE;
            return true;
        }

        if ( pos.IsInsufficientMaterial() )
        {
            score = DRAW_SCORE;
            return true;
        }

        if ( pos.IsCheck() )
            ExtendSearchDepth();

        return false;
    }

    /** A position that has occurred before on the path or in the game is
     ** treated as a draw: if it was repeated once, nothing stops it from
     ** being repeated again.
     **/
    bool IsDrawByRepetition( Position &pos, int &score )
    {
        if ( pos.CountRepetitions() >= 1 )
        {
            score = DRAW_SCORE;
            return true;
        }
        return false;
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
            Move &/*bestMove*/, Moves &/*pv*/, int &/*alpha*/, int &/*beta*/,
            const int /*depth*/ )
    {
        return false;
    }

    virtual void CacheNodeType( const HashEntryType &/*het*/, Position &/*pos*/,
                                const int /*score*/, const int /* depth */,
                                const Move &/*move*/ )
    {}

    /* Make sure our time has not gotten away from us. */
    virtual void CheckWhetherToEmergencyStop()
    {
        if ( m_Director.ShouldCutEmergency( m_Clock.Get(), m_nNodesSearched ) )
            m_bTerminated = true;
    }

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

        m_nNodesSearched++;
        ResetSearchDepth();

        const int ply = PlyFromRoot( pos );
        const bool bIsRoot = ( ply == 0 );
        const bool bIsPVNode = ( beta - alpha > 1 );

        /* Hard recursion cap. Quiescence + check extensions can otherwise
         * push the search arbitrarily deep in positions with long forcing
         * sequences, so belt-and-braces this at MAX_PLY from the root.
         */
        if ( ply >= MAX_PLY - 1 )
            return Evaluate( pos );

        /* We have to check draw by repetition first, because the transposition table
         * can't really keep track of them and they could occur at any time.
         * The root itself is never a draw by repetition: the game is
         * still going, and a GUI adjudicates real threefolds.
         */
        if ( !bIsRoot && IsDrawByRepetition( pos, score ) )
            return score;

        /* Now we can see if any previous search has been useful */
        if ( CheckPreviousSearchResults( score, pos, bestMove, pv, alpha, beta,
                                         depth ) )
            return score;

        /* Is this a leaf node?  If so, evaluate now. */
        if ( IsFrontier( score, pos, alpha, beta, depth ) )
            return score;

        if ( IsEndOfGame( score, pos ) )
        {
            /* Mates are worth remembering at any draft.  Draws by the
             * fifty-move rule depend on the path taken, so they are not
             * cached; neither are repetitions, which never reach here.
             */
            if ( abs( score ) > CHECKMATE_VALUE )
                CacheNodeType( HET_PRINCIPAL_VARIATION, pos, score,
                               MAX_SEARCH_DEPTH, NullMove );
            return score;
        }

        /* IsEndOfGame may have asked for a check extension.  The flag is a
         * member that every recursive call resets, so it must be read once
         * here rather than inside the loop, where it would hold whatever
         * the last node of the previous subtree left behind.
         */
        const int nExtension = m_nSearchExtension;
        const bool bInCheck = ( nExtension != 0 );

        /* Null-move pruning: if passing still leaves us at or above beta
         * after a reduced search, a real move surely will too.  Not when
         * in check, not twice in a row, and not with only pawns left,
         * where zugzwang makes passing genuinely attractive.
         */
        if ( !bIsPVNode && !bInCheck && depth >= 2 && !pos.WasNullMove() &&
                pos.HasNonPawnMaterial( pos.GetColorToMove() ) )
        {
            Position nullPos( pos, NullMove );
            Moves nullPV;
            const int R = ( depth > 6 ) ? 3 : 2;
            int nullScore = -SearchPrincipalVariation( -beta, -beta + 1,
                            depth - 1 - R, nullPos, nullPV );
            if ( m_bTerminated )
                return alpha;
            if ( nullScore >= beta && abs( nullScore ) < CHECKMATE_VALUE )
                return beta;
        }

        GetMoves( myMoves, pos, depth );

        /* 'go searchmoves ...' restricts the root to a user-supplied subset. */
        if ( bIsRoot )
        {
            const Moves &allowed = m_Director.GetSearchMoves();
            if ( !allowed.IsEmpty() )
            {
                Moves filtered;
                for ( const auto &m : myMoves )
                    for ( const auto &sm : allowed )
                        if ( m.Source() == sm.Source() &&
                                m.Dest() == sm.Dest() &&
                                m.GetPromoteTo() == sm.GetPromoteTo() )
                        {
                            filtered.Add( m );
                            break;
                        }
                myMoves = filtered;
            }
        }

        OrderMoves( myMoves, pos, bestMove, ply, depth );
        bestMove = NullMove;

        bool bAlphaExceeded = false;
        unsigned int nLegalMoves = 0;
        const Board &board = pos.GetBoard();

        for ( auto &move : myMoves )
        {
            Position nextPos( pos, move );

            /* Move generation is pseudo-legal: skip anything that leaves
             * our own king capturable.
             */
            if ( nextPos.CanKingBeCapturedNow() )
                continue;

            nLegalMoves++;

            const bool bQuiet = ( board.Get( move.Dest() ) == &None &&
                                  move.GetPromoteTo() == &None );
            /* SearchNode subtracts one ply itself. */
            const int nextDepth = depth + nExtension;

            currentPV = pv;
            currentPV.Make( move );

            if ( nLegalMoves == 1 )
            {
                /* The first move gets the full window. */
                score = SearchNode( beta, alpha, nextDepth, nextPos, currentPV );
            }
            else
            {
                /* Later moves are expected to fail low, so prove that with
                 * a zero-width window, and search late quiet moves a ply
                 * shallower still.  Anything that surprises us by beating
                 * alpha is re-searched properly.
                 */
                int reduction = 0;
                if ( bQuiet && depth >= 3 && nLegalMoves > 4 && !bInCheck &&
                        !IsKiller( move, ply ) )
                    reduction = 1;

                score = SearchNode( alpha + 1, alpha, nextDepth - reduction,
                                    nextPos, currentPV );

                if ( reduction > 0 && score > alpha && !m_bTerminated )
                {
                    currentPV = pv;
                    currentPV.Make( move );
                    score = SearchNode( alpha + 1, alpha, nextDepth, nextPos,
                                        currentPV );
                }

                if ( score > alpha && score < beta && !m_bTerminated )
                {
                    currentPV = pv;
                    currentPV.Make( move );
                    score = SearchNode( beta, alpha, nextDepth, nextPos,
                                        currentPV );
                }
            }

            /* Mate scores already encode ply-from-root at the mating leaf
             * (-KING_VALUE + ply), so returning children's scores unchanged
             * gives shorter mates a strictly larger magnitude than longer
             * mates without any per-level attenuation.
             */

            if ( nLegalMoves == 1 )
            {
                bestPV = currentPV;
                bestMove = move;
            }

            if ( score >= beta )
            {
                /* Hard beta cutoff: a CUT node.  The score is a lower bound
                 * on the exact score of the node.
                 */
                if ( bQuiet )
                    RecordCutoff( move, ply, depth, pos.GetColorToMove() );
                pv = currentPV;
                CacheNodeType( HET_CUT_NODE, pos, score, depth, move );
                return beta;   // fail-high beta-cutoff
            }

            if ( score > alpha )
            {
                /* The score is between alpha and beta.  We have a new best move. */
                bAlphaExceeded = true;
                alpha = score; // alpha acts like max in MiniMax
                bestPV = currentPV;
                bestMove = move;
            }

            CheckWhetherToEmergencyStop();

            if ( m_bTerminated )
                break;
        }

        if ( nLegalMoves == 0 )
        {
            /* Below the horizon only captures were tried; none being
             * legal just means we stand pat on the score IsFrontier
             * already folded into alpha.
             */
            if ( depth <= 0 && !bInCheck )
                return alpha;

            /* No legal move at all: checkmate if in check, else stalemate.
             * Encode checkmate as -(KING_VALUE - ply) so mates found deeper
             * from the root have smaller magnitude and shallower mates win.
             */
            score = bInCheck ? -( KING_VALUE - ply ) : DRAW_SCORE;
            if ( score != DRAW_SCORE )
                CacheNodeType( HET_PRINCIPAL_VARIATION, pos, score,
                               MAX_SEARCH_DEPTH, NullMove );
            pv = bestPV;
            return score;
        }

        if ( bAlphaExceeded )
            CacheNodeType( HET_PRINCIPAL_VARIATION, pos, alpha, depth, bestMove );
        else
            CacheNodeType( HET_ALL_NODE, pos, alpha, depth, bestMove );

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
            /* Standing pat while in check is unsafe: the eval doesn't know
             * the king can be captured next move. Fall through to a full
             * search so IsEndOfGame can filter to legal escapes (and apply
             * the normal check extension).
             */
            if ( pos.IsCheck() )
            {
                Report( pos );
                return false;
            }

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
            {
                /* we will stand pat at this depth */
                return true;
            }

            return false;
        }

        Report( pos );
        return false;
    }

    virtual void GetMoves( Moves &myMoves, Position &pos, const int depth )
    {
        /* Below the horizon we normally restrict to captures. When in
         * check we must consider all legal moves so a non-capture escape
         * (king move, interposition) can be found.
         */
        if ( depth > 0 || pos.IsCheck() )
            myMoves = pos.GetMoves();
        else
        {
            /* Rook and bishop under-promotions are practically never
             * better than a queen or a knight; leave them to the
             * full-width search rather than quadrupling promotion nodes.
             */
            myMoves.Clear();
            for ( const auto &m : pos.GetCaptures() )
            {
                const Piece *pPromote = m.GetPromoteTo();
                if ( pPromote != &None &&
                        ( pPromote->Type() == ROOK || pPromote->Type() == BISHOP ) )
                    continue;
                myMoves.Add( m );
            }
        }
    }
};

class SearcherCaching : public SearcherQuiescence
{
public:
    typedef SearcherQuiescence super;
    SearcherCaching( Interface &interface ) :
        super( interface )
    { }
protected:
    /* Mate scores at search nodes encode ply-from-root (-KING_VALUE+ply),
     * so the same position reached from a different root would have a
     * different stored score. Normalize to position-local on store
     * (subtract ply so the stored value is distance-to-mate), and reapply
     * ply on retrieve.
     */
    int ToTT( int score, int ply ) const
    {
        if ( score > CHECKMATE_VALUE )  return score + ply;
        if ( score < -CHECKMATE_VALUE ) return score - ply;
        return score;
    }
    int FromTT( int score, int ply ) const
    {
        if ( score > CHECKMATE_VALUE )  return score - ply;
        if ( score < -CHECKMATE_VALUE ) return score + ply;
        return score;
    }

    virtual void CacheNodeType( const HashEntryType &het, Position &pos,
                                const int score, const int depth,
                                const Move &move )
    {
        PositionHashEntry phe;
        phe.m_Depth = depth;
        phe.m_Ply = pos.GetPly();
        phe.m_BestMove = move;
        phe.m_TypeBits = het;
        phe.m_Score = ToTT( score, PlyFromRoot( pos ) );
        phe.m_Hash = pos.GetHash();
        s_pPositionHashTable->Insert( phe );
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
        const int ply = PlyFromRoot( pos );
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
                    nSearchResult = FromTT( pEntry->m_Score, ply );
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
                {
                    int adj = FromTT( pEntry->m_Score, ply );
                    if ( adj <= alpha )
                    {
                        bestMove = pEntry->m_BestMove;
                        nSearchResult = adj;
                        return true;
                    }
                    break;
                }

                /*
                c. LOWER.  If the value from the table, which is a "lower
                bound" ( but is actually
                beta at the time the entry was stored ) is >= beta, return a "fail high"
                indication to search which says "just return beta, no need to do a search." */
                case HET_CUT_NODE:
                {
                    int adj = FromTT( pEntry->m_Score, ply );
                    if ( adj >= beta )
                    {
                        bestMove = pEntry->m_BestMove;
                        nSearchResult = beta;
                        return true;
                    }
                    break;
                }

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

    virtual bool CheckPreviousSearchResults( int &score, Position &pos,
            Move &bestMove, Moves &pv, int &alpha, int &beta, const int depth )
    {
        int nSearchResult;
        bool bFound = CheckTranspositionTable( bestMove, nSearchResult, pos, alpha,
                                               beta, depth );
        if ( bFound )
        {
            /* Terminal positions are stored with no best move. */
            if ( bestMove != NullMove )
                pv.Add( bestMove );
            score = nSearchResult;
            return true;
        }

        return false;
    }

};

typedef SearcherCaching Searcher;

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
    pPawn = GetColor() ? &WhitePawn :&BlackPawn;
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
    const Board &board = pos.GetBoard();

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
                if ( !pos.IsSquareAttacked( C1, BLACK ) &&
                        !pos.IsSquareAttacked( D1, BLACK ) &&
                        !pos.IsSquareAttacked( E1, BLACK ) )
                {
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
                if ( !pos.IsSquareAttacked( E1, BLACK ) &&
                        !pos.IsSquareAttacked( F1, BLACK ) &&
                        !pos.IsSquareAttacked( G1, BLACK ) )
                {
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
                if ( !pos.IsSquareAttacked( C8, WHITE ) &&
                        !pos.IsSquareAttacked( D8, WHITE ) &&
                        !pos.IsSquareAttacked( E8, WHITE ) )
                {
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
                if ( !pos.IsSquareAttacked( E8, WHITE ) &&
                        !pos.IsSquareAttacked( F8, WHITE ) &&
                        !pos.IsSquareAttacked( G8, WHITE ) )
                {
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

uint64_t Perft( Position &pos, int depth )
{
    if ( depth == 0 )
        return 1;

    uint64_t nodes = 0;
    const Moves &moves = pos.GetMoves();

    for ( const auto &move : moves )
    {
        Position nextPos( pos, move );
        /* GetMoves returns pseudo-legal moves; drop those that leave
         * our own king capturable on the opponent's reply.
         */
        if ( nextPos.CanKingBeCapturedNow() )
            continue;

        nodes += Perft( nextPos, depth - 1 );
    }

    return nodes;
}

class Game : Object
{
public:
    Game()
    {
        /* Setup() also records the starting material total the phase
         * calculation divides by, so it must run before any search or
         * eval, whether or not a "uci" or "ucinewgame" ever arrives.
         */
        New();
    }

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
        pos.DetachHistory();
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
        m_Protocol( PROTOCOL_UCI ),
        m_Protover( 1 ),
        m_bShowThinking( false ),
        m_bPonder( false ),
        m_bLogInputToFile( false ),
        m_pGame( new Game ),
        m_bIsRunning( true )
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
            ss << "Superpawn";
            if ( string( BUILD_BRANCH ).size() )
                ss << " " << BUILD_BRANCH;
            if ( string( BUILD_NUMBER ).size() )
                ss << " build " << BUILD_NUMBER;
            if ( string( BUILD_ID ).size() )
                ss << " (" << BUILD_ID << ")";
            Notify( ss.str() );
        }
        {
            stringstream ss;
            ss << "Compiled on " << __DATE__ << " " << __TIME__;
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

        while ( m_bIsRunning )
        {
            /* End of input must end the engine too, or this loop spins
             * forever on an empty line.  Let a search that is already
             * running finish first, so "printf 'go depth 6' | superpawn"
             * still gets its bestmove.
             */
            if ( !getline( *m_In, sInputLine ) )
                break;

            if ( m_bLogInputToFile )
                LogLineToFile( sInputLine );

            LockGuardType guard( m_Lock );
            Execute( sInputLine );
        }

        m_pSearcher->Wait();
    }

    typedef lock_guard<mutex> LockGuardType;

    mutex &GetLock()
    {
        return m_Lock;
    }

    INTERFACE_PROTOTYPE( Notify )
    {
        LockGuardType guard( m_OutLock );
        switch ( m_Protocol )
        {
        case PROTOCOL_XBOARD:
            ( *m_Out ) << "# " << sParams << endl;
            break;

        default:
        case PROTOCOL_UCI:
            ( *m_Out ) << "info string " << sParams << endl;
            break;
        }
    }

    INTERFACE_PROTOTYPE( Instruct )
    {
        LockGuardType guard( m_OutLock );
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
        RegisterCommand( "perft", &Interface::PerftCmd );
        RegisterCommand( "divide", &Interface::PerftDivideCmd );
        RegisterCommand( "eval", &Interface::EvalCmd );
        RegisterCommand( "bench", &Interface::BenchCmd );
    }

    /** "bench [depth]": search a fixed set of positions to a fixed depth
     ** with a fixed hash size and print the total node count.  The count
     ** is a fingerprint of the search: a change that leaves it unchanged
     ** made no functional difference.  See TESTING.md.
     **/
    INTERFACE_PROTOTYPE( BenchCmd )
    {
        /* The first thirty are Stockfish's classic benchmark set; the
         * last three exercise our endgame terms.
         */
        static const char *benchPositions[] =
        {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 11",
            "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
            "rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14",
            "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14",
            "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
            "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
            "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
            "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
            "2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
            "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
            "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
            "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
            "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
            "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
            "6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b - - 0 1",
            "3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w - - 0 1",
            "2K5/p7/7P/5pR1/8/5k2/r7/8 w - - 0 1",
            "8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 1",
            "7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w - - 0 1",
            "8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w - - 0 1",
            "8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w - - 0 1",
            "8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - - 0 1",
            "8/3p4/p1bk3p/Pp6/1Kp1PpPp/2P2P1P/2P5/5B2 b - - 0 1",
            "5k2/7R/4P2p/5K2/p1r2P1p/8/8/8 b - - 0 1",
            "6k1/6p1/P6p/r1N5/5p2/7P/1b3PP1/4R1K1 w - - 0 1",
            "1r3k2/4q3/2Pp3b/3Bp3/2Q2p2/1p1P2P1/1P2KP2/3N4 w - - 0 1",
            "6k1/4pp1p/3p2p1/P1pPb3/R7/1r2P1PP/3B1P2/6K1 w - - 0 1",
            "8/3p3B/5p2/5P2/p7/PP5b/k7/6K1 w - - 0 1",
            "k7/8/8/8/8/8/8/K6R w - - 0 1",
            "8/8/8/3k4/8/8/4Q3/K7 w - - 0 1",
            "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1",
        };
        const size_t nPositions = sizeof( benchPositions ) / sizeof( benchPositions[0] );

        stringstream ss( sParams );
        unsigned int depth = BENCH_DEPTH;
        ss >> depth;
        if ( depth < 1 )
            depth = 1;

        /* A search still running would be reading the table we are about
         * to replace.
         */
        m_pSearcher->Stop();

        /* Fix the hash size for the run: the count depends on which
         * entries collide, so it must not depend on the configured size.
         */
        size_t savedHashBytes = s_pPositionHashTable->GetSize();
        s_pPositionHashTable->SetSize( BENCH_HASH_BYTES );
        m_pSearcher->SetQuiet( true );

        uint64_t totalNodes = 0;
        Clock clock;
        clock.Start();

        for ( size_t i = 0; i < nPositions; i++ )
        {
            s_pPositionHashTable->Purge();

            /* Explicit string: a bare const char* would prefer the
             * Position( bool colorToMove ) constructor.
             */
            Position pos( string( benchPositions[i] ) );
            Director director;
            director.m_nDepth = depth;
            m_pSearcher->SetDirector( director );
            m_pSearcher->Start( pos );
            m_pSearcher->Wait();
            totalNodes += m_pSearcher->GetNodesSearched();
        }

        Clock::ChessTickType elapsed = clock.Get();

        m_pSearcher->SetQuiet( false );
        s_pPositionHashTable->SetSize( savedHashBytes );

        uint64_t nps = ( elapsed > 0 ) ? totalNodes * 1000 / elapsed : 0;

        stringstream result;
        result << "bench: " << totalNodes << " nodes " << elapsed
               << " ms " << nps << " nps (depth " << depth
               << ", " << nPositions << " positions)";
        Instruct( result.str() );
    }

    INTERFACE_PROTOTYPE_NO_PARAMS( EvalCmd )
    {
        Evaluator ev;
        Position *pPos = m_pGame->GetPosition();
        int s = ev.Evaluate( *pPos );
        stringstream r;
        r << "eval: " << s << " cp (from "
          << ( pPos->GetColorToMove() == WHITE ? "white" : "black" )
          << "'s perspective)";
        Instruct( r.str() );
    }

    INTERFACE_PROTOTYPE( PerftCmd )
    {
        stringstream ss( sParams );
        int depth = 1;
        ss >> depth;

        Position *pPos = m_pGame->GetPosition();
        Clock c;
        c.Start();
        uint64_t nodes = ::Perft( *pPos, depth );
        Clock::ChessTickType elapsed = c.Get();

        stringstream result;
        result << "perft " << depth << ": " << nodes
               << " nodes in " << elapsed << " ms";
        Instruct( result.str() );
    }

    INTERFACE_PROTOTYPE( PerftDivideCmd )
    {
        stringstream ss( sParams );
        int depth = 1;
        ss >> depth;
        if ( depth < 1 ) depth = 1;

        Position *pPos = m_pGame->GetPosition();
        const Moves &moves = pPos->GetMoves();
        uint64_t total = 0;

        for ( const auto &move : moves )
        {
            Position nextPos( *pPos, move );
            if ( nextPos.CanKingBeCapturedNow() )
                continue;
            uint64_t n = ::Perft( nextPos, depth - 1 );
            total += n;
            stringstream line;
            line << "  " << ( string )move << ": " << n;
            Instruct( line.str() );
        }
        stringstream result;
        result << "divide " << depth << ": " << total;
        Instruct( result.str() );
    }

    INTERFACE_PROTOTYPE( UCI )
    {
        RegisterUCI( sParams );

        Instruct( "id name Superpawn" );
        Instruct( "id author John Byrd" );

        Instruct( "option name Hash type spin default 128 min 1 max 2048" );
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
            {
                /* Clamp to the range we advertise so an overflowing
                 * SetSize() can't be reached from UCI input.
                 */
                if ( nValue < 1 ) nValue = 1;
                if ( nValue > 2048 ) nValue = 2048;
                /* GUIs shouldn't resize mid-search, but if one does, a
                 * stopped search beats a search reading freed memory.
                 */
                m_pSearcher->Stop();
                s_pPositionHashTable->SetSize( nValue * 1024 * 1024 );
            }
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
        TestOne( "startpos moves d2d4 c7c6 e2e4 f7f6 b1c3 e7e5 d4e5 f6e5 d1h5 e8e7 h5e5 e7f7 f1c4 "
                 "d7d5 e4d5 f8d6 d5c6 f7g6 c4d3 g6f7 d3c4 f7g6 c4d3 g6f7" );
        TestOne( "startpos moves c2c4 g8f6 g1f3 e7e6 b1c3 f8b4 a2a3 b4c3 d2c3 e8g8 c1g5 "
                 "d7d5 g5f6 g7f6 c4d5 e6d5 d1b3 b7b6 e1c1 c8e6 b3c2 b8d7 e2e3 d7e5 e3e4 d8d6 f1e2 "
                 "a8d8 f3e1 e5c6 e4d5 e6d5 e2d3 d6f4 d1d2 f4h6 f2f4 b6b5 d3b5 h6f4 h1f1 f4g5 b5c6 "
                 "d5c6 c2d1 c6b5 f1f2 c7c5 e1f3 g5e3 d1e1 e3e1 f3e1 d8d2 c1d2 f8d8 d2c2 c5c4 f2d2 "
                 "d8e8 e1f3 b5c6 f3d4 c6a4 c2b1 a4b3 d4b3 c4b3 b1c1 e8e1 d2d1 e1e2 d1d8 g8g7 d8d2 "
                 "e2e1 d2d1 e1e2 d1g1 f6f5 c1b1 g7f6 b1c1 f6e5 h2h3 a7a5 c1b1 f5f4 a3a4 e2e4 b1c1 "
                 "e4a4 g1e1 a4e4 c1d2 e4e1 d2e1 a5a4 e1d2 e5e4 h3h4 h7h5 c3c4 e4d4 c4c5 d4c5 d2c3 "
                 "c5b5 c3d2 b5b4 d2d3 a4a3 b2a3 b4a3 d3e4 b3b2 e4f4 b2b1q f4f3 b1f1 f3g3 f1f5 g3h2 "
                 "f5f2 h2h3 f2e3 h3h2 e3f2 h2h3 f2f4 g2g3 f4f2 g3g4 f2f3 h3h2 f3g4 h2h1 a3a4 h1h2 "
                 "a4a3 h2h1 a3a2 h1h2 a2b2 h2h1 b2a2 h1h2 a2a1 h2h1 a1b2 h1h2 b2b3 h2h1" );
        TestOne( "startpos moves e2e4 d7d5 e4d5 d8d5 c2c4 d5e4 f1e2 e4g2 e2f3 g2g6 d1b3 "
                 "g6a6 c4c5 e7e5 f3e2 a6a5 b3d5 b8c6 b2b4 a5b4 e2h5 g7g6 h5f3 b4d4 d5d4 c6d4 f3d1 "
                 "f8c5 c1b2 g8f6 g1f3 d4f3 d1f3 c5d4 b2d4 e5d4 h1g1 e8g8 g1g5 h7h6 g5c5 f8e8 e1d1 "
                 "c8g4 f3g4 f6g4 d1c2 g4f2 c5c4 d4d3 c2b3 a8c8 c4f4 f2e4 b3c4 c8d8 f4h4 g6g5 h4h3 "
                 "e8e6 a2a4 e6c6 c4b3 c6a6 b3a3" );
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
            else if ( sParam == "searchmoves" )
            {
                /* Consume the rest of the line as moves. UCI spec puts
                 * searchmoves last on the line, so this is safe.
                 */
                Color colorToMove = m_pGame->GetPosition()->GetColorToMove();
                string sMove;
                while ( ss >> sMove )
                {
                    if ( sMove.length() < 4 ) break;
                    Move m( sMove, colorToMove );
                    director.m_SearchMoves.Add( m );
                }
                continue;
            }
            else if ( sParam == "ponder" )
            {
                /* Pondering not implemented; ignore the flag quietly
                 * so GUIs that always send it don't trip the warning.
                 */
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
                /* A full FEN has six fields, but four-field EPD-style
                 * FENs are common, so stop at "moves" or end of line.
                 */
                string sArg, sFen;
                const int fenArgs = 6;

                for ( int t = 0; t < fenArgs; t++ )
                {
                    if ( !( ss >> sArg ) )
                        break;
                    if ( sArg == "moves" )
                    {
                        sType = "moves";
                        break;
                    }
                    if ( t != 0 )
                        sFen.append( " " );
                    sFen.append( sArg );
                }

                Position pos;
                pos.SetFEN( sFen );
                m_pGame->SetPosition( pos );
            }

            if ( sType == "startpos" )
                m_pGame->New();

            if ( sType == "moves" )
            {
                string sMove;

                while ( ss >> sMove )
                {
                    Position *pLast = m_pGame->GetPosition();
                    Move wanted( sMove, pLast->GetColorToMove() );

                    /* Only play moves we can generate ourselves; anything
                     * else (a typo, "0000") is reported and ends the list
                     * rather than aborting the engine.
                     */
                    bool bFound = false;
                    Move matched;
                    for ( const auto &m : pLast->GetMoves() )
                    {
                        if ( m.Source() == wanted.Source() &&
                                m.Dest() == wanted.Dest() &&
                                m.GetPromoteTo() == wanted.GetPromoteTo() )
                        {
                            matched = m;
                            bFound = true;
                            break;
                        }
                    }

                    if ( !bFound )
                    {
                        Notify( "Ignoring unplayable move: " + sMove );
                        break;
                    }

                    Position nextPos( *pLast, matched );
                    m_pGame->SetPosition( nextPos );
                }
            }
        }
    }

    INTERFACE_PROTOTYPE_NO_PARAMS( New )
    {
        /* A search still running would be reading the table we purge. */
        m_pSearcher->Stop();
        m_pGame->New();
        s_pPositionHashTable->Purge();
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
        m_pSearcher->Stop();
        m_bIsRunning = false;
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
    /** The search thread and the command thread both write to m_Out. */
    mutex m_OutLock;

    Moves m_PrincipalVariation;

    ProtocolType m_Protocol;
    int m_Protover;
    bool m_bShowThinking;
    bool m_bPonder;
    bool m_bLogInputToFile;
    shared_ptr<Game> m_pGame;
    shared_ptr<Searcher> m_pSearcher;
    bool m_bIsRunning;

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

void DirectorBase::Notify( const string &s )
{
    m_pInterface->Notify( s );
}

void SearcherBase::Notify( const string &s ) const
{
    m_pInterface->Notify( s );
}

void SearcherBase::Instruct( const string &s ) const
{
    if ( !m_bQuiet )
        m_pInterface->Instruct( s );
}

void SearcherBase::Bestmove( const string &s ) const
{
    if ( !m_bQuiet )
        m_pInterface->Bestmove( s );
}

int main( int , char ** )
{
    Clock c;
    PieceInitializer pieceInitializer;
    PieceSquareTableInitializer pieceSquareTableInitializer;
    HashInitializer hashInitializer;
    HashTableInitializer hashTableInitializer;
    Interface i;

    i.Run();

    return 0;
}

