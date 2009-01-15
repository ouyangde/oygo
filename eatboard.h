#ifndef _EAT_BOARD_H_
#define _EAT_BOARD_H_
#include <math.h>
#include "basic_go_types.h"
#include "basic_board.h"
#include "zobrist.h"
#include "nbrcounter.h"

template<uint T> 
class EatBoard:
public BasicBoard<T, EatBoard<T> >,
public NbrCounterBoard<T, EatBoard<T> >,
public ZobristBoard<T, EatBoard<T> >
{
public:
	static const bool use_mercy_rule = true;
	using BasicBoard<T, EatBoard>::board_area;
	using BasicBoard<T, EatBoard>::color_at;
	using BasicBoard<T, EatBoard>::player_v_cnt;
	using BasicBoard<T, EatBoard>::move_history;
	using BasicBoard<T, EatBoard>::move_no;
	using BasicBoard<T, EatBoard>::max_game_length;
	using NbrCounterBoard<T, EatBoard>::nbr_cnt;
	using ZobristBoard<T, EatBoard>::hash;
public:
	FastMap<Vertex<T>, Vertex<T> >  chain_next_v;
	uint                         	chain_lib_cnt[Vertex<T>::cnt]; // indexed by chain_id 棋串的气数
	FastMap<Vertex<T>, uint>        chain_id; // 每个点所属的棋串id
  
	static const uint empty_size = board_area;
	FastMap<Vertex<T>, bool>        good_at; // 是否good 
	FastMap<Vertex<T>, uint>        good_pos; // good vetex在good_v中的索引

	union {
	Vertex<T>                       good_v[empty_size]; 
	Vertex<T>                       empty_v[empty_size]; // 棋盘上的空点
	};
	union {
	uint                         	good_v_cnt;
	uint				empty_v_cnt;
	};
	int                          komi;
	uint last_empty_v_cnt;
public:                         // board interface
	EatBoard() {
		vertex_for_each_all(v) {
			chain_next_v[v] = v;
			chain_id[v] = v;    // TODO is it needed, is it usedt?
			chain_lib_cnt[v] = nbr_cnt[v].empty_cnt();
			good_at[v] = false;
		}
		good_v_cnt  = 0;
		Vertex<T> v = Vertex<T>::of_coords(T/2,T/2);
		good_at[v] = true;
		good_pos[v] = good_v_cnt;
		good_v[good_v_cnt++] = v;
		komi = 0;
		last_empty_v_cnt = BasicBoard<T, EatBoard>::empty_v_cnt;
	}
	int score() const {
		return komi;
	}
	Player winner() const { 
		return Player((score() <= 0) + 1); 
	}
	int approx_score() const {
		return komi;
	}
	Player approx_winner() { return Player((approx_score() <= 0)+1); }
	int vertex_score(Vertex<T> v) {
		return 0;
	}
	void set_komi(float fkomi) { 
	}
	float get_komi() const { 
		return komi;
	}
public: 
	all_inline 
	bool play(Player player, Vertex<T> v) {
		if(v == Vertex<T>::pass()) {
			// don't allow pass
			return false;
			basic_play(player, Vertex<T>::pass());
			return true;
		}
		v.check_is_on_board();
		assertc(board_ac, color_at[v] == color::empty);
		Player other_player = player::other(player);

		if(nbr_cnt[v].player_cnt_is_max(other_player)) {
			return play_eye(player, v);
		} else if(nbr_cnt[v].empty_cnt() == 0) { // 处理可能的自杀
			// complete suicide testing
			vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]--); 
			uint valid = false;
			uint valid2 = false;
			vertex_for_each_nbr(v, nbr_v,i, {
				// 被打吃或者可提子
				valid |= (chain_lib_cnt[chain_id[nbr_v]] == 0);
				// 不自杀
				valid2 |= (color_at[nbr_v] == color::from_player(player) & chain_lib_cnt[chain_id[nbr_v]] != 0);
			});
			if(!valid || !valid2) {
				vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]++); 
				return false;
			}
		} else {
			uint valid = false;
			vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]--); 
			vertex_for_each_nbr(v, nbr_v,i, {
				// 对方棋子 
				valid |= (color_at[nbr_v] == color::from_player(other_player));
				// 被打吃
				valid |= (color_at[nbr_v] == color::from_player(player) & chain_lib_cnt[chain_id[nbr_v]] == 0);
			});
			if(move_no != 0 && !valid) {
				vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]++); 
				return false;
			}
		}
		basic_play(player, v);
		place_stone(player, v);
		vertex_for_each_nbr(v, nbr_v,i, {
			if(color::is_player(color_at[nbr_v])) {
				if(color_at[nbr_v] != color::from_player(player)) { // same color of groups
					if(chain_lib_cnt[chain_id[nbr_v]] == 0) 
						remove_chain(nbr_v);
				} else {
					if(chain_id[nbr_v] != chain_id[v]) {
						if(chain_lib_cnt[chain_id[v]] > chain_lib_cnt[chain_id[nbr_v]]) {
							merge_chains(v, nbr_v);
						} else {
							merge_chains(nbr_v, v);
						}         
					}
				}
			}
		});
		if(komi == 0) {
			if(BasicBoard<T, EatBoard>::empty_v_cnt >= last_empty_v_cnt) {
				komi = (player == player::black) ? 500 : -500;
			}
		}
		last_empty_v_cnt = BasicBoard<T, EatBoard>::empty_v_cnt;
		return true;
	}
private:
	no_inline
	bool play_eye(Player player, Vertex<T> v) {
		uint all_nbr_live = true;
		vertex_for_each_nbr(v, nbr_v,i, all_nbr_live &= (--chain_lib_cnt[chain_id[nbr_v]] != 0));
		if(all_nbr_live) {
			vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]++); 
			return false;
		}
		basic_play(player, v);
		place_stone(player, v);
		vertex_for_each_nbr(v, nbr_v,i, {
			//NCB::place_stone_side(player, nbr_v, i);
			if((chain_lib_cnt[chain_id[nbr_v]] == 0)) 
				remove_chain(nbr_v);
		});
		assertc(board_ac, chain_lib_cnt[chain_id[v]] != 0);
		return true;
	}

	// accept pass
	// will ignore simple-ko ban
	// will play single stone suicide
	all_inline 
	void play_legal(Player player, Vertex<T> v) {

		if(v == Vertex<T>::pass()) {
			basic_play(player, Vertex<T>::pass());
			return;
		}

		v.check_is_on_board();
		assertc(board_ac, color_at[v] == color::empty);

		if(nbr_cnt[v].player_cnt_is_max(player::other(player))) {
			play_eye_legal(player, v);
		} else {
			play_not_eye(player, v);
			// TODO invent complete suicide testing
		}

	}

	void merge_chains(Vertex<T> v_base, Vertex<T> v_new) {
		Vertex<T> act_v;

		chain_lib_cnt[chain_id[v_base]] += chain_lib_cnt[chain_id[v_new]];

		act_v = v_new;
		do {
			chain_id[act_v] = chain_id[v_base];
			act_v = chain_next_v[act_v];
		} while(act_v != v_new);

		swap(chain_next_v[v_base], chain_next_v[v_new]);// 两个环断开，分别接到对方环上，扭成一个大环
	}


	no_inline 
	void remove_chain(Vertex<T> v){
		Vertex<T> act_v;
		Vertex<T> tmp_v;
		Color old_color;

		old_color = color_at[v];
		act_v = v;

		assertc(board_ac, color::is_player(old_color));

		do {
			remove_stone(color::to_player(old_color), act_v);
			chain_id[act_v] = act_v;
			chain_lib_cnt[act_v] = 0;// TODO: is it fixed?
			act_v = chain_next_v[act_v];
		} while(act_v != v);

		assertc(board_ac, act_v == v);

		do {
			vertex_for_each_nbr(act_v, nbr_v,i, {
				chain_lib_cnt[chain_id[nbr_v]]++;
				//NCB::remove_stone_side(color::to_player(old_color), nbr_v, i);
			});

			tmp_v = act_v;
			act_v = chain_next_v[act_v];
			chain_next_v[tmp_v] = tmp_v;
		} while(act_v != v);
	}
	void basic_play(Player player, Vertex<T> v) { // Warning: has to be called before place_stone, because of hash storing
		BasicBoard<T, EatBoard>::play(player, v);
	}
	void place_stone(Player player, Vertex<T> v) {
		BasicBoard<T, EatBoard>::place_stone(player, v);
		NbrCounterBoard<T, EatBoard>::place_stone(player, v);
		ZobristBoard<T, EatBoard>::place_stone(player, v);

		if(good_at[v]) {
			good_v_cnt--;
			good_at[v] = false;
			good_pos[good_v[good_v_cnt]] = good_pos[v];
			good_v[good_pos[v]] = good_v[good_v_cnt];
		}
		vertex_for_each_nbr(v, nbr_v, i, {
			if(!good_at[nbr_v] && color_at[nbr_v] == color::empty) {
				good_at[nbr_v] = true;
				good_pos[nbr_v] = good_v_cnt;
				good_v[good_v_cnt++] = nbr_v;
			}
		});
		assertc(chain_next_v_ac, chain_next_v[v] == v);
		assertc(chain_next_v_ac, chain_id[v] == v);
		assertc(chain_next_v_ac, chain_lib_cnt[v] == nbr_cnt[v].empty_cnt());
		//chain_id[v] = v;
		//chain_lib_cnt[v] = nbr_cnt[v].empty_cnt();
	}
	void remove_stone(Player pl, Vertex<T> v) {
		// 按照place_stone相反的顺序
		ZobristBoard<T, EatBoard>::remove_stone(pl, v);
		NbrCounterBoard<T, EatBoard>::remove_stone(pl, v);
		BasicBoard<T, EatBoard>::remove_stone(pl, v);
		
		//chain_id[v] = v;
	}

public:
};
#endif //_EAT_BOARD_H_
