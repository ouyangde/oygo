#ifndef _BOARD_H_
#define _BOARD_H_

// class Hash
class Hash {
public:

	uint64 hash;

	Hash () { }

	uint index () const { return hash; }
	uint lock  () const { return hash >> 32; }

	void randomize (PmRandom& pm) { 
		hash =
			(uint64 (pm.rand_int ()) << (0*16)) ^
			(uint64 (pm.rand_int ()) << (1*16)) ^ 
			(uint64 (pm.rand_int ()) << (2*16)) ^ 
			(uint64 (pm.rand_int ()) << (3*16));
	}

	void set_zero () { hash = 0; }

	bool operator== (const Hash& other) const { return hash == other.hash; }
	void operator^= (const Hash& other) { hash ^= other.hash; }

};


// class Zobrist
template<uint T> 
class Zobrist {
public:

	FastMap<Move<T>, Hash> hashes;

	Zobrist (PmRandom& pm) {
		player_for_each (pl) vertex_for_each_all (v) {
			Move<T> m = Move<T> (pl, v);
			hashes [m].randomize (pm);
		}
	}

	Hash of_move (Move<T> m) const {
		return hashes [m];
	}

	Hash of_pl_v (Player pl,  Vertex<T> v) const {
		return hashes [Move<T> (pl, v)];
	}

};


// class NbrCounter
/*
 * 用12个bit来表示其邻点中black,white,empty数量
 * 每4个bit表示一种情况的数量
 * off_board的情况则看做既是black也是white
 */


class NbrCounter {
public:

	static const uint max = 4;                 // maximal number of neighbours
	static const uint f_size = 4;              // size in bits of each of 3 counters in nbr_cnt::t
	static const uint f_shift_black = 0 * f_size; // 分别占4个bit
	static const uint f_shift_white = 1 * f_size;
	static const uint f_shift_empty = 2 * f_size;
	static const uint f_mask = (1 << f_size) - 1; // 用于去除empty的位
	static const uint black_inc_val = (1 << f_shift_black) - (1 << f_shift_empty); // black+,empty-
	static const uint white_inc_val = (1 << f_shift_white) - (1 << f_shift_empty);
	static const uint off_board_inc_val = (1 << f_shift_black) + (1 << f_shift_white) - (1 << f_shift_empty); // empty-,b,w+

public:

	uint bitfield;

	NbrCounter () { }

	NbrCounter (uint black_cnt, uint white_cnt, uint empty_cnt) {
		assertc (nbr_cnt_ac, black_cnt <= max);
		assertc (nbr_cnt_ac, white_cnt <= max);
		assertc (nbr_cnt_ac, empty_cnt <= max);
		bitfield = 
			(black_cnt << f_shift_black) +
			(white_cnt << f_shift_white) +
			(empty_cnt << f_shift_empty);
	}

	// 用于对player做map
	static const uint player_inc_tab [player::cnt];
	void player_inc (Player player) { bitfield += player_inc_tab [player]; }
	void player_dec (Player player) { bitfield -= player_inc_tab [player]; }
	void off_board_inc () { bitfield += off_board_inc_val; }

	uint empty_cnt  () const { return bitfield >> f_shift_empty; }

	static const uint f_shift [3];
	uint player_cnt (Player pl) const { return (bitfield >> f_shift [pl]) & f_mask; }

	static const uint player_cnt_is_max_mask [player::cnt];
	uint player_cnt_is_max (Player pl) const { 
		return 
			(player_cnt_is_max_mask [pl] & bitfield) == 
			player_cnt_is_max_mask [pl]; 
	}

	void check () {
		if (!nbr_cnt_ac) return;
		assert (empty_cnt () <= max);
		assert (player_cnt (player::black) <= max);
		assert (player_cnt (player::white) <= max);
	}
};

const uint NbrCounter::f_shift[3] = { 
	NbrCounter::f_shift_black, 
	NbrCounter::f_shift_white, 
	NbrCounter::f_shift_empty, //这个其实不需要
};
// TODO player_Map
const uint NbrCounter::player_cnt_is_max_mask[player::cnt] = {  
	max << f_shift_black, 
	max << f_shift_white 
};

const uint NbrCounter::player_inc_tab [player::cnt] = { 
	NbrCounter::black_inc_val, 
	NbrCounter::white_inc_val
};


// class Board


enum play_ret_t { play_ok, play_suicide, play_ss_suicide, play_ko };


template<uint T> 
class Board {

public:

	static const uint board_area = T * T;
	static const uint max_empty_v_cnt     = board_area;
	static const uint max_game_length     = board_area * 4;
	static const Zobrist<T> zobrist[1];

	FastMap<Vertex<T>, Color>       color_at; // 每个点的颜色，这是棋盘的基本结构
	FastMap<Vertex<T>, NbrCounter>  nbr_cnt; // incremental, for fast eye checking 每个点的邻点数
	FastMap<Vertex<T>, uint>        empty_pos; // 如果一个点是empty，记录其在empty_v中的索引值
	FastMap<Vertex<T>, Vertex<T> >      chain_next_v;

	uint                         chain_lib_cnt [Vertex<T>::cnt]; // indexed by chain_id 棋串的气数
	FastMap<Vertex<T>, uint>        chain_id; // 每个点所属的棋串id
  
	Vertex<T>                       empty_v [board_area]; // 棋盘上的空点
	uint                         empty_v_cnt;
	uint                         last_empty_v_cnt;

	// 记录Player占据的数量
	uint			       player_v_cnt[player::cnt]; // map Player to uint
	Vertex<T>		       player_last_v[player::cnt];// map Player to Vertex

	Hash                         hash;
	int                          komi;

	Vertex<T>                       ko_v;             // vertex forbidden by ko(劫)

	Player                       last_player;      // player who made the last play (other::player is forbidden to retake)
	uint                         move_no;

	play_ret_t                   last_move_status;
	Move<T>                         move_history [max_game_length];

public:                         // macros

// 对所有空点
  #define empty_v_for_each(board, vv, i) {                              \
      Vertex<T> vv = Vertex<T>::any ();                                       \
      rep (ev_i, (board)->empty_v_cnt) {                                \
        vv = (board)->empty_v [ev_i];                                   \
        i;                                                              \
      }                                                                 \
    }

// 对所有空点以及pass
  #define empty_v_for_each_and_pass(board, vv, i) {                     \
      Vertex<T> vv = Vertex<T>::pass ();                                      \
      i;                                                                \
      rep (ev_i, (board)->empty_v_cnt) {                                \
        vv = (board)->empty_v [ev_i];                                   \
        i;                                                              \
      }                                                                 \
    }


public:                         // consistency checks


	void check_empty_v () const { //检查color_at,empty_pos,empty_v的一致性
		if (!board_empty_v_ac) return;

		FastMap<Vertex<T>, bool> noticed;
		uint exp_player_v_cnt[player::cnt];

		vertex_for_each_all (v) noticed[v] = false;

		assert (empty_v_cnt <= board_area);

		empty_v_for_each (this, v, {
				assert (noticed [v] == false);
				noticed [v] = true;
				});

		player_for_each (pl) exp_player_v_cnt [pl] = 0;

		vertex_for_each_all (v) {
			assert ((color_at[v] == color::empty) == noticed[v]);
			if (color_at[v] == color::empty) {
				assert (empty_pos[v] < empty_v_cnt);
				assert (empty_v [empty_pos[v]] == v);
			}
			if (color::is_player(color_at [v])) exp_player_v_cnt [color_at[v]]++;
		}

		player_for_each (pl) 
			assert (exp_player_v_cnt [pl] == player_v_cnt [pl]);
	}


	void check_hash () const {
		assertc (board_hash_ac, hash == recalc_hash ());
	}


	void check_color_at () const { // 棋盘上的点不能是off_board
		if (!board_color_at_ac) return;

		vertex_for_each_all (v) {
			color::check(color_at [v]);
			assert ((color_at[v] != color::off_board) == (v.is_on_board ()));
		}
	}


	void check_nbr_cnt () const { // 按照邻点的定义检查一遍nbr_cnt是否正确
		if (!board_nbr_cnt_ac) return;

		vertex_for_each_all (v) {
			coord::t r;
			coord::t c;
			uint nbr_color_cnt[color::cnt];
			uint expected_nbr_cnt;

			if (color_at[v] == color::off_board) continue; // TODO is that right?

			r = v.row ();
			c = v.col ();

			assert (coord::is_on_board<T> (r)); // checking the macro
			assert (coord::is_on_board<T> (c));

			color_for_each (col) nbr_color_cnt [col] = 0;

			vertex_for_each_nbr (v, nbr_v, nbr_color_cnt [color_at [nbr_v]]++);

			expected_nbr_cnt =        // definition of nbr_cnt[v]
				+ ((nbr_color_cnt [color::black] + nbr_color_cnt [color::off_board]) 
						<< NbrCounter::f_shift_black)
				+ ((nbr_color_cnt [color::white] + nbr_color_cnt [color::off_board])
						<< NbrCounter::f_shift_white)
				+ ((nbr_color_cnt [color::empty]) 
						<< NbrCounter::f_shift_empty);

			assert (nbr_cnt[v].bitfield == expected_nbr_cnt);
		}
	}


	void check_chain_at () const {
		if (!chain_at_ac) return;

		vertex_for_each_all (v) { // whether same color neighbours have same root and liberties
			// TODO what about off_board and empty? 可以不用管
			if (color::is_player(color_at [v])) {

				assert (chain_lib_cnt[ chain_id [v]] != 0); // player所属的串气不为0

				vertex_for_each_nbr (v, nbr_v, {
						if (color_at[v] == color_at[nbr_v]) // 同色邻点属于同一串
						assert (chain_id [v] == chain_id [nbr_v]);
						});
			}
		}
	}


	void check_chain_next_v () const {
		if (!chain_next_v_ac) return;
		vertex_for_each_all (v) {
			chain_next_v[v].check ();
			if (!color::is_player(color_at [v])) 
				assert (chain_next_v [v] == v);
		}
	}


	void check () const { // 检查整个棋盘一致性
		if (!board_ac) return;

		check_empty_v       ();
		check_hash          ();
		check_color_at      ();
		check_nbr_cnt       ();
		check_chain_at      ();
		check_chain_next_v  ();
	}


	void check_no_more_legal (Player player) { // at the end of the playout
		unused (player);

		if (!board_ac) return;

		vertex_for_each_all (v) // 确认对于该player来说，剩余的空点要么是眼，要么禁入
			if (color_at[v] == color::empty)
				assert (is_eyelike (player, v) || is_pseudo_legal (player, v) == false);
	}


public:                         // board interface


	Board () { // TODO: 可以考虑预先建立全局空棋盘来优化性能
		clear (); 
		cout << ""; // TODO remove this stupid statement
	}

  
	void clear () {
		uint off_board_cnt;

		set_komi (default_komi);            // standard for chinese rules
		empty_v_cnt  = 0;
		player_for_each (pl) {
			player_v_cnt  [pl] = 0;
			player_last_v [pl] = Vertex<T>::any ();
		}
		move_no      = 0;
		last_player  = player::white; // act player is other
		last_move_status = play_ok;
		ko_v         = Vertex<T>::any ();
		vertex_for_each_all (v) {
			// 先初始化棋盘外的点
			color_at      [v] = color::off_board;
			nbr_cnt       [v] = NbrCounter (0, 0, NbrCounter::max);
			chain_next_v  [v] = v;
			chain_id      [v] = v;    // TODO is it needed, is it usedt?
			chain_lib_cnt [v] = NbrCounter::max; // TODO is it logical? (off_boards)

			if (v.is_on_board ()) {
				// 登记空点
				color_at   [v]              = color::empty;
				empty_pos  [v]              = empty_v_cnt;
				empty_v    [empty_v_cnt++]  = v;

				off_board_cnt = 0;// 有多少个邻点不在board上
				vertex_for_each_nbr (v, nbr_v, {
						if (!nbr_v.is_on_board ()) 
						off_board_cnt++;
						});
				// 登记邻点数
				rep (ii, off_board_cnt) 
					nbr_cnt [v].off_board_inc ();

			}
		}

		hash = recalc_hash ();

		check ();
	}


	Hash recalc_hash () const { // 将所有的移动(Move)异或起来(^=)
		Hash new_hash;

		new_hash.set_zero ();

		vertex_for_each_all (v) {
			if (color::is_player(color_at [v])) {
				new_hash ^= zobrist->of_pl_v ((Player)color_at [v], v);
			}
		}

		return new_hash;
	}


	void load (const Board* save_board) { // 快速拷贝棋盘，要求棋盘实现为浅拷贝
		memcpy(this, save_board, sizeof(Board)); 
		check ();
	}


	void set_komi (float fkomi) { 
		komi = int (ceil (-fkomi));
	}


	float get_komi () const { 
		return float(-komi) + 0.5;
	}


public: // legality functions


	// checks for move legality
	// it has to point to empty vertexand empty
	// can't recognize play_suicide
	all_inline 
	bool is_pseudo_legal (Player player, Vertex<T> v) { // 不严格的禁入点探测
		check ();
		//v.check_is_on_board (); // TODO check v = pass || onboard

		// 1 pass
		// 2 不在对方包围中(含真眼和假眼)
		// 3 不是劫并且不是填对方的单眼
		return 
			v == Vertex<T>::pass () || 
			!nbr_cnt[v].player_cnt_is_max (player::other(player)) || 
			(!play_eye_is_ko (player, v) && 
			 !play_eye_is_suicide (v));
	}


	// a very slow function
	bool is_strict_legal (Player pl, Vertex<T> v) {            // slow function
		if (try_play (pl, v) == false) return false;
		sure_undo ();
		return true;
	}


	// MC模拟时仅考虑不填自己的眼,对于填自己空或者缩小眼位之类的则不管
	bool is_eyelike (Player player, Vertex<T> v) { // 是否像一个眼
		// 1 邻点是否全是己方棋子
		// 2 肩上的对方棋子数少于2(边角上少于1)
		// 缺点：无法判断环形棋块,
		// TODO:是否会错过故意填眼的妙招??
		assertc (board_ac, color_at [v] == color::empty);
		if (! nbr_cnt[v].player_cnt_is_max (player)) return false;

		int diag_color_cnt[color::cnt]; // TODO
		color_for_each (col) 
			diag_color_cnt [col] = 0; // memset is slower

		// 肩
		vertex_for_each_diag_nbr (v, diag_v, {
				diag_color_cnt [color_at [diag_v]]++;
				});

		return diag_color_cnt [Color (player::other(player))] + (diag_color_cnt [color::off_board] > 0) < 2;
	}


	// this is very very slow function
	bool is_hash_repeated () { // 根据历史棋步重下一遍来判断全局同形,果然很慢。为什么不预存hash值呢
		Board tmp_board;
		rep (mn, move_no-1) {
			tmp_board.play_legal (move_history [mn].get_player (), move_history [mn].get_vertex ());
			if (hash == tmp_board.hash) 
				return true;
		}
		return false;
	}


public: // play move functions


	// very slow function
	bool try_play (Player player, Vertex<T> v) {
		if (v == Vertex<T>::pass ()) {
			play_legal (player, v);      
			return true;
		}

		v.check_is_on_board ();

		if (color_at [v] != color::empty) 
			return false; 

		if (is_pseudo_legal (player,v) == false)
			return false;

		play_legal (player, v);

		if (last_move_status != play_ok) {
			sure_undo ();
			return false;
		}

		if (is_hash_repeated ()) {
			sure_undo ();
			return false;
		}

		return true;
	}


	// accept pass
	// will ignore simple-ko ban
	// will play single stone suicide
	all_inline void play_legal (Player player, Vertex<T> v) {
		check ();

		if (v == Vertex<T>::pass ()) {
			basic_play (player, Vertex<T>::pass ());
			return;
		}

		v.check_is_on_board ();
		assertc (board_ac, color_at[v] == color::empty);

		if (nbr_cnt[v].player_cnt_is_max (player::other(player))) {
			play_eye_legal (player, v);
		} else {
			play_not_eye (player, v);
			// TODO invent complete suicide testing
			assertc (board_ac, last_move_status == play_ok || last_move_status == play_suicide); 
		}

	}


	// very slow function
	bool undo () {
		Move<T> replay [max_game_length];

		uint   game_length  = move_no;
		float  old_komi     = get_komi ();

		if (game_length == 0) 
			return false;

		rep (mn, game_length-1)
			replay [mn] = move_history [mn];

		clear ();
		set_komi (old_komi); // TODO maybe last_player should be preserverd as well

		rep (mn, game_length-1)
			play_legal (replay [mn].get_player (), replay [mn].get_vertex ());

		return true;
	}


	// very slow function 
	void sure_undo () {
		if (undo () == false) {
			cerr << "sure_undo failed\n";
			exit (1);
		}
	}

public: // auxiliary functions


	bool play_eye_is_ko (Player player, Vertex<T> v) {
		return (v == ko_v) & (player == player::other(last_player));
	}


	bool play_eye_is_suicide (Vertex<T> v) {
		uint all_nbr_live = true;
		vertex_for_each_nbr (v, nbr_v, all_nbr_live &= (--chain_lib_cnt [chain_id [nbr_v]] != 0));
		vertex_for_each_nbr (v, nbr_v, chain_lib_cnt [chain_id [nbr_v]]++); //TODO: 能否更快
		return all_nbr_live;
	}


	all_inline
	void play_not_eye (Player player, Vertex<T> v) {
		check ();
		v.check_is_on_board ();
		assertc (board_ac, color_at[v] == color::empty);

		basic_play (player, v);

		place_stone (player, v);

		vertex_for_each_nbr (v, nbr_v, {

			nbr_cnt [nbr_v].player_inc (player);
        
			if (color::is_player(color_at [nbr_v])) {
				// This should be before 'if' to have good lib_cnt for empty vertices
				chain_lib_cnt [chain_id [nbr_v]] --; 

				if (color_at [nbr_v] != Color (player)) { // same color of groups
					if (chain_lib_cnt [chain_id [nbr_v]] == 0) 
						remove_chain (nbr_v);
				} else {
					if (chain_id [nbr_v] != chain_id [v]) {
						if (chain_lib_cnt [chain_id [v]] > chain_lib_cnt [chain_id [nbr_v]]) {
							merge_chains (v, nbr_v);
						} else {
							merge_chains (nbr_v, v);
						}         
					}
				}
			}
		});
    
		if (chain_lib_cnt [chain_id [v]] == 0) {
			assertc (board_ac, last_empty_v_cnt - empty_v_cnt == 1);
			remove_chain(v);
			assertc (board_ac, last_empty_v_cnt - empty_v_cnt > 0);
			last_move_status = play_suicide;//TODO: 为何要先下这一手再undo呢?
		} else {
			last_move_status = play_ok;
		}
	}


	// 在对方眼的位置下子（忽略因为劫而导致的禁入情况，因为调用前判断过了）
	no_inline
	void play_eye_legal (Player player, Vertex<T> v) {
		vertex_for_each_nbr (v, nbr_v, chain_lib_cnt [chain_id [nbr_v]]--);

		basic_play (player, v);
		place_stone (player, v);
    
		vertex_for_each_nbr (v, nbr_v, { 
			nbr_cnt [nbr_v].player_inc (player);
		});

		vertex_for_each_nbr (v, nbr_v, {
			if ((chain_lib_cnt [chain_id [nbr_v]] == 0)) 
			remove_chain (nbr_v);
		});

		assertc (board_ac, chain_lib_cnt [chain_id [v]] != 0);

		if (last_empty_v_cnt == empty_v_cnt) { // if captured exactly one stone, end this was eye
			ko_v = empty_v [empty_v_cnt - 1]; // then ko formed
		} else {
			ko_v = Vertex<T>::any ();
		}
	}


	void basic_play (Player player, Vertex<T> v) { // Warning: has to be called before place_stone, because of hash storing
		assertc (board_ac, move_no <= max_game_length);
		ko_v                    = Vertex<T>::any ();
		last_empty_v_cnt        = empty_v_cnt;
		last_player             = player;
		player_last_v [player]  = v;
		move_history [move_no]  = Move<T> (player, v);
		move_no                += 1;
	}


	void merge_chains (Vertex<T> v_base, Vertex<T> v_new) {
		Vertex<T> act_v;

		chain_lib_cnt [chain_id [v_base]] += chain_lib_cnt [chain_id [v_new]];

		act_v = v_new;
		do {
			chain_id [act_v] = chain_id [v_base];
			act_v = chain_next_v [act_v];
		} while (act_v != v_new);

		swap (chain_next_v[v_base], chain_next_v[v_new]);// TODO:没看懂
	}


	no_inline 
	void remove_chain (Vertex<T> v){
		Vertex<T> act_v;
		Vertex<T> tmp_v;
		Color old_color;

		old_color = color_at[v];
		act_v = v;

		assertc (board_ac, color::is_player(old_color));

		do {
			remove_stone (act_v);
			act_v = chain_next_v[act_v];
		} while (act_v != v);

		assertc (board_ac, act_v == v);

		do {
			vertex_for_each_nbr (act_v, nbr_v, {
				nbr_cnt [nbr_v].player_dec ((Player)old_color);
				chain_lib_cnt [chain_id [nbr_v]]++;
			});

			tmp_v = act_v;
			act_v = chain_next_v[act_v];
			chain_next_v [tmp_v] = tmp_v;

		} while (act_v != v);
	}


	void place_stone (Player pl, Vertex<T> v) {
		hash ^= zobrist->of_pl_v (pl, v);
		player_v_cnt[pl]++;
		color_at[v] = Color (pl);

		empty_v_cnt--;
		empty_pos [empty_v [empty_v_cnt]] = empty_pos [v];
		empty_v [empty_pos [v]] = empty_v [empty_v_cnt];

		assertc (chain_next_v_ac, chain_next_v[v] == v);

		chain_id [v] = v;
		chain_lib_cnt [v] = nbr_cnt[v].empty_cnt ();
	}


	void remove_stone (Vertex<T> v) {
		hash ^= zobrist->of_pl_v ((Player)color_at [v], v);
		player_v_cnt [color_at[v]]--;
		color_at [v] = color::empty;

		empty_pos [v] = empty_v_cnt;
		empty_v [empty_v_cnt++] = v;
		chain_id [v] = v;

		assertc (board_ac, empty_v_cnt < Vertex<T>::cnt);
	}


public:                         // utils


	// TODO/FIXME last_player should be preserverd in undo function
	Player act_player () const { return player::other(last_player); } 


	bool both_player_pass () {
		return 
			(player_last_v [player::black] == Vertex<T>::pass ()) & 
			(player_last_v [player::white] == Vertex<T>::pass ());
	}


	int approx_score () const {
		return komi + player_v_cnt[player::black] -  player_v_cnt[player::white];
	}


	Player approx_winner () { return Player (approx_score () <= 0); }


	int score () const {
		int eye_score = 0;

		empty_v_for_each (this, v, {
			eye_score += nbr_cnt[v].player_cnt_is_max (player::black);
			eye_score -= nbr_cnt[v].player_cnt_is_max (player::white);
		});

		return approx_score () + eye_score;
	}


	Player winner () const { 
		return Player (score () <= 0); 
	}


	int vertex_score (Vertex<T> v) {
		//     FastMap<Color, int> Coloro_score;
		//     Coloro_score [Color::black ()] = 1;
		//     Coloro_score [Color::white ()] = -1;
		//     Coloro_score [Color::empty ()] =
		//       (nbr_cnt[v].player_cnt_is_max (Player::black ())) -
		//       (nbr_cnt[v].player_cnt_is_max (Player::white ()));
		//     Coloro_score [Color::off_board ()] = 0;
		//     return Coloro_score [color_at [v]];
		switch (color_at [v]) {
			case color::black: return 1;
			case color::white: return -1;
			case color::empty: 
				return 
					(nbr_cnt[v].player_cnt_is_max (player::black)) -
					(nbr_cnt[v].player_cnt_is_max (player::white));
			case color::off_board: return 0;
			default: assert (false);
		}
	}

	void print_cerr (Vertex<T> v = Vertex<T>::pass ()) const {
		cerr << to_string (v);
	}

};

extern PmRandom zobrist_pm;
template<uint T> 
const Zobrist<T> Board<T>::zobrist[1] = { Zobrist<T> (zobrist_pm) }; // TODO move it to board

#endif // _BOARD_H_
