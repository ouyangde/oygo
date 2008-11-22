#ifndef _GO_IO_H_
#define _GO_IO_H_

#include <string>
#include <sstream>
#include <iostream>
#include <basic_go_types.h>
using namespace std;

extern const char col_tab[]; // "ABCDEFGHJKLMNOPQRSTUVWXYZ";
//----------------------------
//coord
// TODO to gtp string
template<uint T> string row_to_string (coord::t row) {
	coord::check<T> (row);
	ostringstream ss;
	ss << T - row;
	return ss.str ();
}
template<uint T> string col_to_string (coord::t col) {
	coord::check<T> (col);
	ostringstream ss;
	ss << col_tab [col];
	return ss.str ();
}
//----------------------------
//Player
inline string to_string(Player pl) {
	if(pl == player::black) {
		return "#";
	} else {
		return "O";
	}
}
istream& operator>> (istream& in, Player& pl) {
	string s;
	in >> s;
	if (s == "b" || s == "B" || s == "Black" || s == "BLACK "|| s == "black" || s == "#") { 
		pl = player::black; return in; 
	}
	if (s == "w" || s == "W" || s == "White" || s == "WHITE "|| s == "white" || s == "O") { 
		pl = player::white; return in; 
	}
	in.setstate (ios_base::badbit);
	return in;
}
ostream& operator<< (ostream& out, Player& pl) { 
	out << to_string(pl); 
	return out; 
}

//----------------------------
//Move
template<uint T> 
istream& operator>> (istream& in, Move<T>& m) {
	Player pl;
	Vertex<T> v;
	if (!(in >> pl >> v)) return in;
	m = Move<T> (pl, v);
	return in;
}
//----------------------------
//Vertex
template<uint T> 
string to_string (Vertex<T>& v) {
	coord::t r;
	coord::t c;

	if (v == Vertex<T>::pass_idx) {
		return "pass";
	} else if (v == Vertex<T>::any_idx) {
		return "any";
	} else if (v == Vertex<T>::resign_idx) {
		return "resign";
	} else {
		r = v.row ();
		c = v.col ();
		ostringstream ss;
		ss << ::col_to_string<T> (c) << ::row_to_string<T> (r);
		return ss.str ();
	}
}
template <typename T1,uint T>
string to_string_2d (FastMap<Vertex<T>, T1>& map, int precision = 3) {
	ostringstream out;
	out << setiosflags (ios_base::fixed) ;

	coord_for_each (row) {
		coord_for_each (col) {
			Vertex<T> v = Vertex<T>::of_coords (row, col);
			out.precision(precision);
			out.width(precision + 3);
			out << map [v] << " ";
		}
		out << endl;
	}
	return out.str ();
}
template<uint T>
Vertex<T> of_sgf_string (string s) {
	if (s == "") return Vertex<T>::pass ();
	if (s == "tt" && T <= 19) return Vertex<T>::pass ();
	if (s.size () != 2 ) return Vertex<T>::any ();
	coord::t col = s[0] - 'a';
	coord::t row = s[1] - 'a';

	if (coord::is_on_board<T> (row) &&
			coord::is_on_board<T> (col)) {
		return Vertex<T>(row, col);
	} else {
		return Vertex<T>::any ();
	}
}
// TODO of_gtp_string
template<uint T> 
Vertex<T> of_gtp_string (string s) {
	const uint board_size = T;
	char c;
	int n;
	coord::t row, col;

	if (s == "pass" || s == "PASS" || s == "Pass") { return Vertex<T>::pass (); }
	if (s == "resign" || s == "RESIGN" || s == "Resign") { return Vertex<T>::resign (); }

	istringstream in2 (s);
	if (!(in2 >> c >> n)) return Vertex<T>::any();

	row = board_size - n;

	col = 0;
	while (col < int (sizeof(::col_tab) - 1)) {
		if (::col_tab[col] == c || ::col_tab[col] -'A' + 'a' == c ) break;
		col++;
	}

	if (col == int (sizeof(::col_tab) - 1)) {
		return Vertex<T>::any();
	}

	return Vertex<T> (row, col);
}
template<uint T> 
ostream& operator<< (ostream& out, Vertex<T>& v) { 
	out << v.to_string (); 
	return out; 
}

//----------------------------
//Board
template<uint T>
string to_string (const Board<T>& idx, Vertex<T> mark_v = Vertex<T>::any ()) {
	ostringstream out;

#define os(n)      out << " " << n
#define o_left(n)  out << "(" << n
#define o_right(n) out << ")" << n

	out << " ";
	const uint board_size = T;
	if (board_size < 10) out << " "; else out << "  ";
	coord_for_each (col) os (::col_to_string<T> (col));
	out << endl;

	coord_for_each (row) {
		if (board_size >= 10 && board_size - row < 10) out << " ";
		os (::row_to_string<T> (row));
		coord_for_each (col) {
			Vertex<T> v = Vertex<T>::of_coords(row, col);
			char ch = color::to_char(idx.color_at [v]);
			if      (v == mark_v)        o_left  (ch);
			else if (v == mark_v.E ())   o_right (ch);
			else                         os (ch);
		}
		if (board_size >= 10 && board_size - row < 10) out << " ";
		os (::row_to_string<T> (row));
		out << endl;
	}

	if (board_size < 10) out << "  "; else out << "   ";
	coord_for_each (col) os (::col_to_string<T> (col));
	out << endl;

#undef os
#undef o_left
#undef o_right

	return out.str ();
}
#endif // _GO_IO_H_
