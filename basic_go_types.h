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

  string col_tab = "ABCDEFGHJKLMNOPQRSTUVWXYZ";

  // TODO to gtp string
  template<uint T> string row_to_string (t row) {
    check<T> (row);
    ostringstream ss;
    ss << T - row;
    return ss.str ();
  }

  template<uint T> string col_to_string (t col) {
    check<T> (col);
    ostringstream ss;
    ss << col_tab [col];
    return ss.str ();
  }

}

template<uint T> class Vertex {


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

 // TODO make this constructor a static function
  Vertex (coord::t r, coord::t c) {
    coord::check2<T> (r, c);
    idx = (r+1) * dNS + (c+1) * dWE;
  }

  uint get_idx () const { return idx; }

  bool operator== (Vertex other) const { return idx == other.idx; }
  bool operator!= (Vertex other) const { return idx != other.idx; }

  bool in_range ()          const { return idx < cnt; }
  void next ()                    { idx++; }

  void check ()             const { assertc (vertex_ac, in_range ()); }

  coord::t row () const { return idx / dNS - 1; }

  coord::t col () const { return idx % dNS - 1; }

  // this usualy can be achieved quicker by color_at lookup
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

  Vertex NW () const { return N ().W (); } // TODO can it be faster?
  Vertex NE () const { return N ().E (); } // only Go
  Vertex SW () const { return S ().W (); } // only Go
  Vertex SE () const { return S ().E (); }

  string to_string () const {
    coord::t r;
    coord::t c;
    
    if (idx == pass_idx) {
      return "pass";
    } else if (idx == any_idx) {
      return "any";
    } else if (idx == resign_idx) {
      return "resign";
    } else {
      r = row ();
      c = col ();
      ostringstream ss;
      ss << coord::col_to_string<T> (c) << coord::row_to_string<T> (r);
      return ss.str ();
    }
  }

  static Vertex pass   () { return Vertex (Vertex::pass_idx); }
  static Vertex any    () { return Vertex (Vertex::any_idx); }
  static Vertex resign () { return Vertex (Vertex::resign_idx); }

  static Vertex of_sgf_coords (string s) {
    if (s == "") return pass ();
    if (s == "tt" && T <= 19) return pass ();
    if (s.size () != 2 ) return any ();
    coord::t col = s[0] - 'a';
    coord::t row = s[1] - 'a';
    
    if (coord::is_on_board<T> (row) &&
        coord::is_on_board<T> (col)) {
      return Vertex (row, col);
    } else {
      return any ();
    }
  }
};
//------------------------------------------------------------------------------
//Player
enum Player { black_player = 0, white_player = 1, wrong_player = 2};
const uint Player_cnt = 2;

inline Player Player_other(Player pl) {
	return Player(pl ^ 1);
}
inline bool Player_in_range(Player pl) {
	return pl < wrong_player;
}
inline Player operator++(Player& pl) {
	return (pl = Player(pl+1));
}
inline void Player_check(Player pl) {
    assertc (player_ac, (pl & (~1)) == 0);
}
inline string Player_to_string(Player pl) {
	if(pl == black_player) {
		return "#";
	} else {
		return "O";
	}
}
istream& operator>> (istream& in, Player& pl) {
  string s;
  in >> s;
  if (s == "b" || s == "B" || s == "Black" || s == "BLACK "|| s == "black" || s == "#") { pl = black_player; return in; }
  if (s == "w" || s == "W" || s == "White" || s == "WHITE "|| s == "white" || s == "O") { pl = white_player; return in; }
  in.setstate (ios_base::badbit);
  return in;
}
ostream& operator<< (ostream& out, Player& pl) { out << Player_to_string(pl); return out; }
// faster than non-loop
#define player_for_each(pl) \
  for (Player pl = black_player; Player_in_range(pl); ++pl)

//------------------------------------------------------------------------------
// class color

enum Color {
	color_black = 0, 
	color_white = 1, 
	color_empty = 2, 
	color_off_board = 3, 
	color_wrong = 40
};
const int Color_cnt = 4;
inline Color Color_from_char(char c) {
     switch (c) {
     case '#': return color_black;
     case 'O': return color_white;
     case '.': return color_empty;
     case '*': return color_off_board;
     default : return color_wrong;
     }
}
inline char Color_to_char(Color cl) {
    switch (cl) {
    case color_black:      return '#';
    case color_white:      return 'O';
    case color_empty:      return '.';
    case color_off_board:  return ' ';
    default : assertc (color_ac, false);
    }
    return '?';
}
inline void Color_check(Color cl) {
    assertc (color_ac, (cl & (~3)) == 0); 
}
inline bool Color_is_player(Color cl) {
	return cl <= color_white;
}
inline bool Color_is_not_player(Color cl) {
	return cl > color_white;
}
inline bool Color_in_range(Color cl) {
	return cl < Color_cnt;
}
inline Color operator++(Color& pl) {
	return (pl = Color(pl+1));
}
// TODO test it for performance
#define color_for_each(col) \
  for (Color col = color_black; Color_in_range(col); ++col)
//--------------------------------------------------------------------------------

#define coord_for_each(rc) \
  for (coord::t rc = 0; rc < int(T); rc = coord::t (rc+1))


//--------------------------------------------------------------------------------




// TODO of_gtp_string
template<uint T> istream& operator>> (istream& in, Vertex<T>& v) {
  const uint board_size = T;
  char c;
  int n;
  coord::t row, col;

  string str;
  if (!(in >> str)) return in;
  if (str == "pass" || str == "PASS" || str == "Pass") { v = Vertex<T>::pass (); return in; }
  if (str == "resign" || str == "RESIGN" || str == "Resign") { v = Vertex<T>::resign (); return in; }

  istringstream in2 (str);
  if (!(in2 >> c >> n)) return in;

  row = board_size - n;
  
  col = 0;
  while (col < int (coord::col_tab.size ())) {
    if (coord::col_tab[col] == c || coord::col_tab[col] -'A' + 'a' == c ) break;
    col++;
  }
  
  if (col == int (coord::col_tab.size ())) {
    in.setstate (ios_base::badbit);
    return in;
  }

  v = Vertex<T> (row, col);
  return in;
}

template<uint T> ostream& operator<< (ostream& out, Vertex<T>& v) { out << v.to_string (); return out; }


#define vertex_for_each_all(vv) for (Vertex<T> vv = Vertex<T>(0); vv.in_range (); vv.next ()) // TODO 0 works??? // TODO player the same way!

// misses some offboard vertices (for speed) 
#define vertex_for_each_faster(vv)                                  \
  for (Vertex<T> vv = Vertex<T>(Vertex<T>::dNS+Vertex<T>::dWE);                 \
       vv.get_idx () <= T * (Vertex<T>::dNS + Vertex<T>::dWE);   \
       vv.next ())


#define vertex_for_each_nbr(center_v, nbr_v, block) {   \
    center_v.check_is_on_board ();                      \
    Vertex<T> nbr_v;                                       \
    nbr_v = center_v.N (); block;                       \
    nbr_v = center_v.W (); block;                       \
    nbr_v = center_v.E (); block;                       \
    nbr_v = center_v.S (); block;                       \
  }

#define vertex_for_each_diag_nbr(center_v, nbr_v, block) {      \
    center_v.check_is_on_board ();                              \
    Vertex<T> nbr_v;                                               \
    nbr_v = center_v.NW (); block;                              \
    nbr_v = center_v.NE (); block;                              \
    nbr_v = center_v.SW (); block;                              \
    nbr_v = center_v.SE (); block;                              \
  }

#define player_vertex_for_each_9_nbr(center_v, pl, nbr_v, i) {  \
    v::check_is_on_board (center_v);                            \
    Move    nbr_v;                                              \
    player_for_each (pl) {                                      \
      nbr_v = center_v;                                         \
      i;                                                        \
      vertex_for_each_nbr      (center_v, nbr_v, i);            \
      vertex_for_each_diag_nbr (center_v, nbr_v, i);            \
    }                                                           \
  }


//--------------------------------------------------------------------------------

template<uint T> class Move {
public:

  const static uint cnt = white_player << Vertex<T>::bits_used | Vertex<T>::cnt;
 
  const static uint no_move_idx = 1;

  uint idx;

  void check () {
    Player (idx >> Vertex<T>::bits_used);
    Vertex<T> (idx & ((1 << Vertex<T>::bits_used) - 1)).check ();
  }

  explicit Move (Player player, Vertex<T> v) { 
    idx = (player << Vertex<T>::bits_used) | v.get_idx ();
  }

  explicit Move () {
    Move (black_player, Vertex<T>::any ());
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

  uint get_idx () const { return idx; }

  bool operator== (Move other) const { return idx == other.idx; }
  bool operator!= (Move other) const { return idx != other.idx; }

  bool in_range ()          const { return idx < cnt; }
  void next ()                    { idx++; }
};


template<uint T> istream& operator>> (istream& in, Move<T>& m) {
  Player pl;
  Vertex<T> v;
  if (!(in >> pl >> v)) return in;
  m = Move<T> (pl, v);
  return in;
}

  
template <typename T1,uint T>
string to_string_2d (FastMap<Vertex<T>, T1>& map, int precision = 3) {
  ostringstream out;
  out << setiosflags (ios_base::fixed) ;
  
  coord_for_each (row) {
    coord_for_each (col) {
      Vertex<T> v = Vertex<T> (row, col);
      out.precision(precision);
      out.width(precision + 3);
      out << map [v] << " ";
    }
    out << endl;
  }
  return out.str ();
}
    

#define move_for_each_all(m) for (Move m = Move (0); m.in_range (); m.next ())

/********************************************************************************/
