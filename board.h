#ifndef _BOARD_H_
#define _BOARD_H_
#include <math.h>
#include "basic_go_types.h"
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
 * ��12��bit����ʾ���ڵ���black,white,empty����
 * ÿ4��bit��ʾһ�����������
 * off_board�������������blackҲ��white
 */
class NbrCounter {
public:
	uint8 bitfield;
	static const uint max = 4;                 // maximal number of neighbours
	static const uint cnt = 256;
	NbrCounter ():bitfield(0x00) { }

	void player_inc (Player player, uint i) { 
		bitfield |= (player << i); 
	}
	void player_dec (Player player, uint i) { 
		bitfield &= ~(color::off_board << i); 
	}
	void off_board_inc (uint i) { 
		bitfield |= (color::off_board << i); 
	}
	static const uint cnt_map[player::cnt][cnt];
	static const uint eye_map[player::cnt][cnt];

	uint empty_cnt  () const { return cnt_map[0][bitfield]; }
	uint player_cnt (Player pl) const { return cnt_map[pl][bitfield]; }
	uint player_cnt_is_max (Player pl) const { 
		return cnt_map[pl][bitfield] == max;
	}
	// diag_nbr less than 2 or 1
	uint is_nbr_lt2(Player pl) const { return eye_map[pl][bitfield]; }

	void check () {
		if (!nbr_cnt_ac) return;
		assert (empty_cnt () <= max);
		assert (player_cnt (player::black) <= max);
		assert (player_cnt (player::white) <= max);
	}
	static void output_cnt_map() {
		for(int i = 0; i < 3; i++) {
			printf("{");
			for(int j = 0; j < 256; j++) {
				if((j % 16) == 0) {
					printf("\n");
				}
				int count = 0;
				for(int n = 0; n < 8; n+= 2) {
					int b = ((j >> n) & 3);
					if(i == 0) {
						if(b == 0) count++;
					} else {
						if((b & i) == i) count++;
					}
				}
				printf("%d,",count);
			}
			printf("\n},\n");
		}
	}
	static void output_eye_map() {
		player_for_each(i) {
			printf("{");
			for(int j = 0; j < 256; j++) {
				if((j % 16) == 0) {
					printf("\n");
				}
				int count = 0;
				int off = 0;
				for(int n = 0; n < 8; n+= 2) {
					int b = ((j >> n) & 3);
					if(b == 0) {
					} else if(b == 3) {
						off = 1;
					} else if(b != i) {
						count++;
					}
				}
				printf("%d,",(count+off<2));
			}
			printf("\n},\n");
		}
	}
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

	FastMap<Vertex<T>, Color>       color_at; // ÿ�������ɫ���������̵Ļ����ṹ
	FastMap<Vertex<T>, NbrCounter>  nbr_cnt; // incremental, for fast eye checking ÿ������ڵ���
	FastMap<Vertex<T>, NbrCounter>  diag_nbr_cnt; // ÿ����ļ�
	FastMap<Vertex<T>, uint>        empty_pos; // ���һ������empty����¼����empty_v�е�����ֵ
	FastMap<Vertex<T>, Vertex<T> >      chain_next_v;

	uint                         chain_lib_cnt [Vertex<T>::cnt]; // indexed by chain_id �崮������
	FastMap<Vertex<T>, uint>        chain_id; // ÿ�����������崮id
  
	Vertex<T>                       empty_v [board_area]; // �����ϵĿյ�
	uint                         empty_v_cnt;
	uint                         last_empty_v_cnt;

	// ��¼Playerռ�ݵ�����
	uint			       player_v_cnt[player::cnt]; // map Player to uint
	Vertex<T>		       player_last_v[player::cnt];// map Player to Vertex

	Hash                         hash;
	int                          komi;

	Vertex<T>                       ko_v;             // vertex forbidden by ko(��)

	//Player                       last_player;      // player who made the last play (other::player is forbidden to retake)
	Player                       a_player;      // player who made the last play (other::player is forbidden to retake)
	uint                         move_no;

	play_ret_t                   last_move_status;
	Move<T>                         move_history [max_game_length];

public:                         // macros

// �����пյ�
  #define empty_v_for_each(board, vv, i) {                              \
      Vertex<T> vv = Vertex<T>::any ();                                       \
      rep (ev_i, (board)->empty_v_cnt) {                                \
        vv = (board)->empty_v [ev_i];                                   \
        i;                                                              \
      }                                                                 \
    }

// �����пյ��Լ�pass
  #define empty_v_for_each_and_pass(board, vv, i) {                     \
      Vertex<T> vv = Vertex<T>::pass ();                                      \
      i;                                                                \
      rep (ev_i, (board)->empty_v_cnt) {                                \
        vv = (board)->empty_v [ev_i];                                   \
        i;                                                              \
      }                                                                 \
    }


public:                         // consistency checks


	void check_empty_v () const { //���color_at,empty_pos,empty_v��һ����
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
			if (color::is_player(color_at [v])) exp_player_v_cnt [color::to_player(color_at[v])]++;
		}

		player_for_each (pl) 
			assert (exp_player_v_cnt [pl] == player_v_cnt [pl]);
	}


	void check_hash () const {
		assertc (board_hash_ac, hash == recalc_hash ());
	}


	void check_color_at () const { // �����ϵĵ㲻����off_board
		if (!board_color_at_ac) return;

		vertex_for_each_all (v) {
			color::check(color_at [v]);
			assert ((color_at[v] != color::off_board) == (v.is_on_board ()));
		}
	}


	void check_nbr_cnt () const { // �����ڵ�Ķ�����һ��nbr_cnt�Ƿ���ȷ
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

			vertex_for_each_nbr (v, nbr_v, i, nbr_color_cnt [color_at [nbr_v]]++);

/*
			expected_nbr_cnt =        // definition of nbr_cnt[v]
				+ ((nbr_color_cnt [color::black] + nbr_color_cnt [color::off_board]) 
						<< NbrCounter::f_shift_black)
				+ ((nbr_color_cnt [color::white] + nbr_color_cnt [color::off_board])
						<< NbrCounter::f_shift_white)
				+ ((nbr_color_cnt [color::empty]) 
						<< NbrCounter::f_shift_empty);

*/
			assert (nbr_cnt[v].player_cnt(player::black) == (nbr_color_cnt [color::black] + nbr_color_cnt [color::off_board]));
			assert (nbr_cnt[v].player_cnt(player::white) == (nbr_color_cnt [color::white] + nbr_color_cnt [color::off_board]));
			assert (nbr_cnt[v].empty_cnt() == (nbr_color_cnt [color::empty]));
		}
	}


	void check_chain_at () const {
		if (!chain_at_ac) return;

		vertex_for_each_all (v) { // whether same color neighbours have same root and liberties
			// TODO what about off_board and empty? ���Բ��ù�
			if (color::is_player(color_at [v])) {

				assert (chain_lib_cnt[ chain_id [v]] != 0); // player�����Ĵ�����Ϊ0

				vertex_for_each_nbr (v, nbr_v,i, {
						if (color_at[v] == color_at[nbr_v]) // ͬɫ�ڵ�����ͬһ��
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


	void check () const { // �����������һ����
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

		vertex_for_each_all (v) // ȷ�϶��ڸ�player��˵��ʣ��Ŀյ�Ҫô���ۣ�Ҫô����
			if (color_at[v] == color::empty)
				assert (is_eyelike (player, v) || is_pseudo_legal (player, v) == false);
	}


public:                         // board interface


	Board () { // TODO: ���Կ���Ԥ�Ƚ���ȫ�ֿ��������Ż�����
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
		//last_player  = player::white; // act player is other
		a_player = player::black;
		last_move_status = play_ok;
		ko_v         = Vertex<T>::any ();
		vertex_for_each_all (v) {
			// �ȳ�ʼ��������ĵ�
			color_at      [v] = color::off_board;
			nbr_cnt       [v] = NbrCounter();
			chain_next_v  [v] = v;
			chain_id      [v] = v;    // TODO is it needed, is it usedt?
			chain_lib_cnt [v] = NbrCounter::max; // TODO is it logical? (off_boards)

			if (v.is_on_board ()) {
				// �Ǽǿյ�
				color_at   [v]              = color::empty;
				empty_pos  [v]              = empty_v_cnt;
				empty_v    [empty_v_cnt++]  = v;
/*
				off_board_cnt = 0;// �ж��ٸ��ڵ㲻��board��
				vertex_for_each_nbr (v, nbr_v, {
						if (!nbr_v.is_on_board ()) 
						off_board_cnt++;
						});
				// �Ǽ��ڵ���
				rep (ii, off_board_cnt) 
					nbr_cnt [v].off_board_inc ();
*/
				vertex_for_each_nbr2(v, nbr_v, i, {
					if (!nbr_v.is_on_board ()) 
					nbr_cnt [v].off_board_inc (i);
				});
				// ���¼�
				vertex_for_each_diag_nbr2(v, nbr_v, i, {
					if (!nbr_v.is_on_board ()) 
					diag_nbr_cnt[v].off_board_inc(i);
				});
			}
		}

		hash = recalc_hash ();

		check ();
	}


	Hash recalc_hash () const { // �����е��ƶ�(Move)�������(^=)
		Hash new_hash;

		new_hash.set_zero ();

		vertex_for_each_all (v) {
			if (color::is_player(color_at [v])) {
				new_hash ^= zobrist->of_pl_v (color::to_player(color_at[v]), v);
			}
		}

		return new_hash;
	}


	void load (const Board* save_board) { // ���ٿ������̣�Ҫ������ʵ��Ϊǳ����
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
	bool is_pseudo_legal (Player player, Vertex<T> v) { // ���ϸ�Ľ����̽��
		check ();
		//v.check_is_on_board (); // TODO check v = pass || onboard

		// 1 pass
		// 2 ���ڶԷ���Χ��(�����ۺͼ���)
		// 3 ���ǽٲ��Ҳ�����Է��ĵ���
		return 
			v == Vertex<T>::pass () || 
			!nbr_cnt[v].player_cnt_is_max (player::other(player)) || 
			//(!play_eye_is_ko (player, v) && 
			(v != ko_v && 
			 !play_eye_is_suicide (v));
	}


	// a very slow function
	bool is_strict_legal (Player pl, Vertex<T> v) {            // slow function
		if (try_play (pl, v) == false) return false;
		sure_undo ();
		return true;
	}


	// MCģ��ʱ�����ǲ����Լ�����,�������Լ��ջ�����С��λ֮����򲻹�
	bool is_eyelike (Player player, Vertex<T> v) { // �Ƿ���һ����
		// 1 �ڵ��Ƿ�ȫ�Ǽ�������
		// 2 ���ϵĶԷ�����������2(�߽�������1)
		// ȱ�㣺�޷��жϻ������,
		// TODO:�Ƿ�����������۵�����??
		assertc (board_ac, color_at [v] == color::empty);
		if (! nbr_cnt[v].player_cnt_is_max (player)) return false;
		return diag_nbr_cnt[v].is_nbr_lt2(player);
		/*
		int diag_color_cnt[color::cnt]; // TODO
		color_for_each (col) 
			diag_color_cnt [col] = 0; // memset is slower

		// ��
		vertex_for_each_diag_nbr (v, diag_v, i, {
			diag_color_cnt [color_at [diag_v]]++;
		});

		return diag_color_cnt [color::from_player(player::other(player))] + (diag_color_cnt [color::off_board] > 0) < 2;
		*/
	}


	// this is very very slow function
	bool is_hash_repeated () { // ������ʷ�岽����һ�����ж�ȫ��ͬ��,��Ȼ������Ϊʲô��Ԥ��hashֵ��
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
		//return (v == ko_v) & (player == player::other(last_player));
		return v == ko_v;
	}


	bool play_eye_is_suicide (Vertex<T> v) {
		uint all_nbr_live = true;
		vertex_for_each_nbr (v, nbr_v,i, all_nbr_live &= (--chain_lib_cnt [chain_id [nbr_v]] != 0));
		vertex_for_each_nbr (v, nbr_v,i, chain_lib_cnt [chain_id [nbr_v]]++); //TODO: �ܷ����
		return all_nbr_live;
	}


	all_inline
	void play_not_eye (Player player, Vertex<T> v) {
		check ();
		v.check_is_on_board ();
		assertc (board_ac, color_at[v] == color::empty);

		basic_play (player, v);

		place_stone (player, v);

		vertex_for_each_nbr (v, nbr_v,i, {

			nbr_cnt [nbr_v].player_inc (player,i);
        
			if (color::is_player(color_at [nbr_v])) {
				// This should be before 'if' to have good lib_cnt for empty vertices
				chain_lib_cnt [chain_id [nbr_v]] --; 

				if (color_at [nbr_v] != color::from_player(player)) { // same color of groups
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
		// ���¼����Ϣ
		vertex_for_each_diag_nbr(v, nbr_v,i,diag_nbr_cnt[nbr_v].player_inc(player,i););
    
		if (chain_lib_cnt [chain_id [v]] == 0) {
			assertc (board_ac, last_empty_v_cnt - empty_v_cnt == 1);
			remove_chain(v);
			assertc (board_ac, last_empty_v_cnt - empty_v_cnt > 0);
			last_move_status = play_suicide;//TODO: Ϊ��Ҫ������һ����undo��?
		} else {
			last_move_status = play_ok;
		}
	}


	// �ڶԷ��۵�λ�����ӣ�������Ϊ�ٶ����µĽ����������Ϊ����ǰ�жϹ��ˣ�
	no_inline
	void play_eye_legal (Player player, Vertex<T> v) {
		vertex_for_each_nbr (v, nbr_v,i, chain_lib_cnt [chain_id [nbr_v]]--);

		basic_play (player, v);
		place_stone (player, v);
    
		vertex_for_each_nbr (v, nbr_v,i, { 
			nbr_cnt [nbr_v].player_inc (player,i);
		});
		// ���¼�
		vertex_for_each_diag_nbr(v, nbr_v,i,{
			diag_nbr_cnt[nbr_v].player_inc(player,i);
		});
		

		vertex_for_each_nbr (v, nbr_v,i, {
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




	void merge_chains (Vertex<T> v_base, Vertex<T> v_new) {
		Vertex<T> act_v;

		chain_lib_cnt [chain_id [v_base]] += chain_lib_cnt [chain_id [v_new]];

		act_v = v_new;
		do {
			chain_id [act_v] = chain_id [v_base];
			act_v = chain_next_v [act_v];
		} while (act_v != v_new);

		swap (chain_next_v[v_base], chain_next_v[v_new]);// �������Ͽ����ֱ�ӵ��Է����ϣ�Ť��һ����
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
			vertex_for_each_nbr (act_v, nbr_v,i, {
				nbr_cnt [nbr_v].player_dec (color::to_player(old_color),i);
				chain_lib_cnt [chain_id [nbr_v]]++;
			});
			// ���¼�
			vertex_for_each_diag_nbr(act_v, nbr_v,i,{
				diag_nbr_cnt[nbr_v].player_dec(color::to_player(old_color),i);
			});

			tmp_v = act_v;
			act_v = chain_next_v[act_v];
			chain_next_v [tmp_v] = tmp_v;

		} while (act_v != v);
	}


	void basic_play (Player player, Vertex<T> v) { // Warning: has to be called before place_stone, because of hash storing
		assertc (board_ac, move_no <= max_game_length);
		ko_v                    = Vertex<T>::any ();
		last_empty_v_cnt        = empty_v_cnt;
		//last_player             = player;
		a_player             = player::other(player);
		player_last_v [player]  = v;
		move_history [move_no]  = Move<T> (player, v);
		move_no                += 1;
	}

	void place_stone (Player pl, Vertex<T> v) {
		hash ^= zobrist->of_pl_v (pl, v);
		player_v_cnt[pl]++;
		color_at[v] = color::from_player(pl);

		empty_v_cnt--;
		empty_pos [empty_v [empty_v_cnt]] = empty_pos [v];
		empty_v [empty_pos [v]] = empty_v [empty_v_cnt];

		assertc (chain_next_v_ac, chain_next_v[v] == v);

		chain_id [v] = v;
		chain_lib_cnt [v] = nbr_cnt[v].empty_cnt ();
	}


	void remove_stone (Vertex<T> v) {
		hash ^= zobrist->of_pl_v (color::to_player(color_at[v]), v);
		player_v_cnt [color::to_player(color_at[v])]--;
		color_at [v] = color::empty;

		empty_pos [v] = empty_v_cnt;
		empty_v [empty_v_cnt++] = v;
		chain_id [v] = v;

		assertc (board_ac, empty_v_cnt < Vertex<T>::cnt);
	}


public:                         // utils


	// TODO/FIXME last_player should be preserverd in undo function
	//Player act_player () const { return player::other(last_player); } 
	Player act_player () const { return a_player; } 


	bool both_player_pass () {
		return 
			(player_last_v [player::black] == Vertex<T>::pass ()) & 
			(player_last_v [player::white] == Vertex<T>::pass ());
	}


	int approx_score () const {
		return komi + player_v_cnt[player::black] -  player_v_cnt[player::white];
	}


	Player approx_winner () { return Player ((approx_score() <= 0)+1); }


	int score () const {
		int eye_score = 0;

		empty_v_for_each (this, v, {
			eye_score += nbr_cnt[v].player_cnt_is_max (player::black);
			eye_score -= nbr_cnt[v].player_cnt_is_max (player::white);
		});

		return approx_score () + eye_score;
	}


	Player winner () const { 
		return Player ((score () <= 0) + 1); 
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
