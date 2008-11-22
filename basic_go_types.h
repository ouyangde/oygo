#ifndef _BASIC_GO_TYPES_H_
#define _BASIC_GO_TYPES_H_

/*
 * 坐标类，坐标由两个int值表示，-1表示棋盘外的坐标
 */
namespace coord { // TODO class

	typedef int t;

	template<uint T> bool is_ok (t coord) { return (coord < int (T)) & (coord >= -1); }
	template<uint T> bool is_on_board (t coord) { return uint (coord) < T; }

	template<uint T> void check (t coo) { 
		unused (coo);
		assertc (coord_ac, is_ok<T> (coo)); 
	}

	template<uint T> void check2 (t row, t col) { 
		if (!coord_ac) return;
		if (row == -1 && col == -1) return;
		assertc (coord_ac, is_on_board<T> (row)); 
		assertc (coord_ac, is_on_board<T> (col)); 
	}

}
#define coord_for_each(rc) \
	for (coord::t rc = 0; rc < int(T); rc++)
//for (coord::t rc = 0; rc < int(T); rc = coord::t (rc+1))

//--------------------------------------------------------------------------------

/*
 * 棋盘上的交叉点，参数T是棋盘大小，用一个uint存储
 */
template<uint T> 
class Vertex {

	//static_assert (cnt <= (1 << bits_used));
	//static_assert (cnt > (1 << (bits_used-1)));

	uint idx;

public:

	const static uint dNS = (T + 2);
	const static uint dWE = 1;

	const static uint bits_used = 9;     // on 19x19 cnt == 441 < 512 == 1 << 9;
	const static uint pass_idx = 0;
	const static uint any_idx  = 1; // TODO any
	const static uint resign_idx = 2;

	const static uint cnt = (T + 2) * (T + 2);

	explicit Vertex () { } // TODO is it needed
	explicit Vertex (uint _idx) { idx = _idx; }

	operator uint() const { return idx; }

	bool operator== (Vertex other) const { return idx == other.idx; }
	bool operator!= (Vertex other) const { return idx != other.idx; }

	bool in_range ()          const { return idx < cnt; }
	Vertex& operator++() { ++idx; return *this;}

	void check ()             const { assertc (vertex_ac, in_range ()); }

	coord::t row () const { return idx / dNS - 1; }

	coord::t col () const { return idx % dNS - 1; }

	//TODO:this usualy can be achieved quicker by color_at lookup
	bool is_on_board () const {
		return coord::is_on_board<T> (row ()) & coord::is_on_board<T> (col ());
	}

	void check_is_on_board () const { 
		assertc (vertex_ac, is_on_board ()); 
	}

	Vertex N () const { return Vertex (idx - dNS); }
	Vertex W () const { return Vertex (idx - dWE); }
	Vertex E () const { return Vertex (idx + dWE); }
	Vertex S () const { return Vertex (idx + dNS); }

	/*
	   Vertex NW () const { return N ().W (); } // TODO can it be faster?
	   Vertex NE () const { return N ().E (); } // only Go
	   Vertex SW () const { return S ().W (); } // only Go
	   Vertex SE () const { return S ().E (); }
	   */
	Vertex NW () const { return Vertex (idx - dNS - dWE); }
	Vertex NE () const { return Vertex (idx - dNS + dWE); }
	Vertex SW () const { return Vertex (idx - dWE + dNS); }
	Vertex SE () const { return Vertex (idx + dWE + dNS); }


	static Vertex pass   () { return Vertex (Vertex::pass_idx); }
	static Vertex any    () { return Vertex (Vertex::any_idx); }
	static Vertex resign () { return Vertex (Vertex::resign_idx); }
	// TODO make this constructor a static function
	/*
	   Vertex (coord::t r, coord::t c) {
	   coord::check2<T> (r, c);
	   idx = (r+1) * dNS + (c+1) * dWE;
	   }
	   */
	static Vertex of_coords(coord::t r, coord::t c) {
		coord::check2<T> (r, c);
		return Vertex((r+1) * dNS + (c+1) * dWE);
	}

};



#define vertex_for_each_all(vv) \
	for (Vertex<T> vv = Vertex<T>(0); vv.in_range (); ++vv) // TODO 0 works??? // TODO player the same way!

// misses some offboard vertices (for speed) 
#define vertex_for_each_faster(vv)                                  \
	for (Vertex<T> vv = Vertex<T>(Vertex<T>::dNS+Vertex<T>::dWE);                 \
		vv <= T * (Vertex<T>::dNS + Vertex<T>::dWE);   \
	++vv)

/*
 * 邻点
 */
#define vertex_for_each_nbr(center_v, nbr_v, block) {       \
	center_v.check_is_on_board ();                      \
	Vertex<T> nbr_v;                                    \
	nbr_v = center_v.N (); block;                       \
	nbr_v = center_v.W (); block;                       \
	nbr_v = center_v.E (); block;                       \
	nbr_v = center_v.S (); block;                       \
}

/*
 * 对角邻点
 */
#define vertex_for_each_diag_nbr(center_v, nbr_v, block) {      \
	center_v.check_is_on_board ();                          \
	Vertex<T> nbr_v;                                        \
	nbr_v = center_v.NW (); block;                          \
	nbr_v = center_v.NE (); block;                          \
	nbr_v = center_v.SW (); block;                          \
	nbr_v = center_v.SE (); block;                          \
}

#define player_vertex_for_each_9_nbr(center_v, pl, nbr_v, i) {      \
	v::check_is_on_board (center_v);                            \
	Move    nbr_v;                                              \
	player_for_each (pl) {                                      \
		nbr_v = center_v;                                   \
		i;                                                  \
		vertex_for_each_nbr      (center_v, nbr_v, i);      \
		vertex_for_each_diag_nbr (center_v, nbr_v, i);      \
	}                                                           \
}

//------------------------------------------------------------------------------
/*
 * Player,使用位运算加速计算另一方:other
 */
//Player
namespace player {

	//typedef uint t;
	enum t{ black = 0, white = 1, wrong = 2};
	const uint cnt = 2;

	inline t other(t pl) {
		return t(pl ^ 1);
	}
	inline bool in_range(t pl) {
		return pl < wrong;
	}
	inline t operator++(t& pl) { return (pl = t(pl+1)); }
	inline void check(t pl) {
		assertc (player_ac, (pl & (~1)) == 0);
	}
};
typedef player::t Player;
// faster than non-loop
#define player_for_each(pl) \
	for (Player pl = player::black; player::in_range(pl); ++pl)

//------------------------------------------------------------------------------
/*
 * Color与Player对应，但是多了几种状态
 *
 */
// class color
namespace color {
	typedef uint t;
	enum {
		black = 0, 
		white = 1, 
		empty = 2, 
		off_board = 3, 
		wrong = 40
	};
	const int cnt = 4;
	inline t from_char(char c) {
		switch (c) {
			case '#': return black;
			case 'O': return white;
			case '.': return empty;
			case '*': return off_board;
			default : return wrong;
		}
	}
	inline char to_char(t cl) {
		switch (cl) {
			case black:      return '#';
			case white:      return 'O';
			case empty:      return '.';
			case off_board:  return ' ';
			default : assertc (color_ac, false);
		}
		return '?';
	}
	inline void check(t cl) {
		assertc (color_ac, (cl & (~3)) == 0); 
	}
	inline bool is_player(t cl) {
		return cl <= white;
	}
	inline bool is_not_player(t cl) {
		return cl > white;
	}
	inline bool in_range(t cl) {
		return cl < cnt;
	}
	//inline t operator++(t& pl) { return (pl = t(pl+1));}
}
typedef color::t Color;
// TODO test it for performance
#define color_for_each(col) \
	for (Color col = color::black; color::in_range(col); ++col)

//--------------------------------------------------------------------------------

/*
 * 用一个uint表示一次Move,即在Vertex前面再加上一个表示Player的bit
 */
template<uint T> class Move {
public:

	const static uint cnt = player::white << Vertex<T>::bits_used | Vertex<T>::cnt;

	const static uint no_move_idx = 1;

	uint idx;

	void check () {
		Player (idx >> Vertex<T>::bits_used);
		Vertex<T> (idx & ((1 << Vertex<T>::bits_used) - 1)).check ();
	}

	explicit Move (Player player, Vertex<T> v) { 
		idx = (player << Vertex<T>::bits_used) | v;
	}

	explicit Move () {
		Move (player::black, Vertex<T>::any ());
	}

	explicit Move (int idx_) {
		idx = idx_;
	}

	Player get_player () { 
		return Player (idx >> Vertex<T>::bits_used);
	}

	Vertex<T> get_vertex () { 
		return Vertex<T> (idx & ((1 << ::Vertex<T>::bits_used) - 1)) ; 
	}

	string to_string () {
		return Player_to_string(get_player ()) + " " + get_vertex ().to_string ();
	}

	operator uint() const { return idx; }

	bool operator== (Move other) const { return idx == other.idx; }
	bool operator!= (Move other) const { return idx != other.idx; }

	bool in_range ()          const { return idx < cnt; }
	Move& operator++() { ++idx; return *this;}
};

    

#define move_for_each_all(m) for (Move m = Move (0); m.in_range (); ++m)

/********************************************************************************/
#endif // _BASIC_GO_TYPES_H_
