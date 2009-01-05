#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
using namespace std;
#include "go_conf.h"
#include "utils.h"
#include "goboard.h"
#include "renjuboard.h"
#include "playout.h"
#include "go_io.h"
#include "uct.h"
namespace simple_playout_benchmark {


	uint win_cnt[player::cnt];
	uint                playout_ok_cnt;
	int                 playout_ok_score;
	PerformanceTimer    perf_timer;

	template <bool score_per_vertex,uint T, template<uint T> class Policy, template<uint T> class Board > 
	void run(Board<T> const * start_board, 
		Policy<T> * policy,
			uint playout_cnt, 
			ostream& out) 
	{
		Board<T>  mc_board [1];
		FastMap<Vertex<T>, int>   vertex_score;
		playout_status_t status;
		Player winner;
		int score;

		playout_ok_cnt   = 0;
		playout_ok_score = 0;

		player_for_each(pl) 
			win_cnt [pl] = 0;

		vertex_for_each_all(v) 
			vertex_score [v] = 0;

		perf_timer.reset();

		Playout<T, Policy, Board> playout(policy, mc_board);

		perf_timer.start();
		float seconds_begin = get_seconds();
		uint move_no = 0;
		uint pl_cnt = 0;

		rep(ii, playout_cnt) {
			mc_board->load(start_board);
			status = playout.run();
			//cerr<<to_string(*mc_board)<<endl;
			switch(status) {
				case pass_pass:
					playout_ok_cnt += 1;

					score = mc_board -> score();
					playout_ok_score += score;

					winner = Player((score <= 0) + 1);  // mc_board->winner()
					win_cnt [winner] ++;
					move_no += mc_board->move_no;
					pl_cnt += 1;

					if(score_per_vertex) {
						vertex_for_each_faster(v)
							vertex_score [v] += mc_board->vertex_score(v);
					}
					break;
				case mercy:
					//out << "Mercy rule should be off for benchmarking" << endl;
					//return;
					win_cnt [mc_board->approx_winner()] ++;
					move_no += mc_board->move_no;
					pl_cnt += 1;
					break;
				case too_long:
					break;
			}
		}

		float seconds_end = get_seconds();
		perf_timer.stop();

		out << "Initial board:" << endl;
		out << "komi " << start_board->get_komi() << " for white" << endl;
		out << "av step " << (move_no/pl_cnt) << endl;

		//out << start_board->to_string();
		//out << to_string(*start_board);
		out << endl;

		if(score_per_vertex) {
			FastMap<Vertex<T>, float> black_own;
			vertex_for_each_all(v) 
				black_own [v] = float(vertex_score [v]) / float(playout_ok_cnt);

			out << "P(black owns vertex) rescaled to [-1, 1] (p*2 - 1): " << endl 
			<< to_string_2d(black_own) << endl;
		}

		out << "Black wins    = " << win_cnt [player::black] << endl
			<< "White wins    = " << win_cnt [player::white] << endl
			<< "P(black win)  = " 
			<< float(win_cnt [player::black]) / 
			float(win_cnt [player::black] + win_cnt [player::white]) 
			<< endl;

		float avg_score = float(playout_ok_score) / float(playout_ok_cnt);

		out << "E(score)      = " 
			<< avg_score - 0.5 
			<< " (without komi = " << avg_score - mc_board->komi << ")" << endl << endl;

		float seconds_total = seconds_end - seconds_begin;
		float cc_per_playout = perf_timer.ticks() / double(playout_cnt);

		out << "Performance: " << endl
			<< "  " << playout_cnt << " playouts" << endl
			<< "  " << seconds_total << " seconds" << endl
			<< "  " << float(playout_cnt) / seconds_total / 1000.0 << " kpps" << endl
			<< "  " << cc_per_playout << " ccpp(clock cycles per playout)" << endl
			<< "  " << 1000000.0 / cc_per_playout  << " kpps/GHz(clock independent)" << endl;
	}
}
template<uint T, class Board>
void pre(Board & board, int a[][2], int size) {
	for(int i = 0; i < size; i++)
	board.play(board.act_player(), Vertex<T>::of_coords(a[i][0], a[i][1]));
}
template <uint T, template<uint T> class Policy, template<uint T> class Board> 
void match_human(Board<T>* board, Policy<T>* policy, bool aifirst = true) {

	UCT<T, Policy, Board>* engine = new UCT<T, Policy, Board>(board, policy);

	if(aifirst) {
		Player pl = board->act_player();
		Vertex<T> v = engine->gen_move(pl);
		board->play(pl, v);
		engine->play(pl,v);
		//cerr<<to_string(*board);
		cout<<"="<<v<<endl;
	}
	while(true) {
		string line;
		if (!getline(cin, line)) break;
		Vertex<T> v = of_gtp_string<T>(line);
		Player pl = board->act_player();
		if(v == Vertex<T>::any() || board->color_at[v] != color::empty || !board->play(pl,v)) {
			cout<<"?"<<endl;
			continue;
		}
		engine->play(pl,v);
		pl = board->act_player();
		v = engine->gen_move(pl);
		if(v != Vertex<T>::resign()) {
			board->play(pl, v);
			engine->play(pl,v);
		}
		cerr<<to_string(*board);
		cout<<"="<<v<<endl;
	}
}
bool parse_arg(int argc, char** argv) {
	bool ret = true;
	while(--argc > 0) {
		switch(argv[argc][0]) {
			case '1': ret = false;
				  break;
			case '-': argv[argc]+=2;
				  {
					  int time = atoi(argv[argc]);
					  if(time != 0) {
						  time_per_move = time;
					  }
				  }
				  break;
			default:
				  break;
		}
	}
	return ret;
}
float time_per_move		  = 1.0;
// main
int main(int argc, char** argv) { 
	setvbuf(stdout, (char *)NULL, _IONBF, 0);
	setvbuf(stderr, (char *)NULL, _IONBF, 0);

	//NbrCounter::output_eye_map();
	//return 0;
	GoBoard<9> board;
	GoPolicy<9> policy;
	//RenjuBoard<15> board;
	//RenjuPolicy<15> policy;

	//ostringstream response;
	//uint playout_cnt = 100000;
	//playout_cnt = 1;
	//simple_playout_benchmark::run<false> (&board, &policy, playout_cnt, response);
	//cout << response.str();
	
	//ofstream fout("log.txt"); 
	if(argc > 1) {
		//cerr.rdbuf(fout.rdbuf());
	}
	match_human(&board, &policy, parse_arg(argc, argv));
	return 0;
}
