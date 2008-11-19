#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace std;

#include "go_conf.h"
#include "utils.h"
#include "basic_go_types.h"
#include "board.h"
#include "playout.h"
namespace simple_playout_benchmark {

  Board               mc_board [1];

  FastMap<Vertex, int>   vertex_score;
  uint win_cnt[Player_cnt];
  uint                playout_ok_cnt;
  int                 playout_ok_score;
  PerformanceTimer    perf_timer;

  template <bool score_per_vertex> 
  void run (Board const * start_board, 
            uint playout_cnt, 
            ostream& out) 
  {
    playout_status_t status;
    Player winner;
    int score;

    playout_ok_cnt   = 0;
    playout_ok_score = 0;

    player_for_each (pl) 
      win_cnt [pl] = 0;

    vertex_for_each_all (v) 
      vertex_score [v] = 0;

    perf_timer.reset ();

    SimplePolicy policy;
    Playout<SimplePolicy> playout(&policy, mc_board);

    perf_timer.start ();
    float seconds_begin = get_seconds ();

    rep (ii, playout_cnt) {
      mc_board->load (start_board);
      status = playout.run ();
      switch (status) {
      case pass_pass:
        playout_ok_cnt += 1;

        score = mc_board -> score ();
        playout_ok_score += score;

        winner = Player (score <= 0);  // mc_board->winner ()
        win_cnt [winner] ++;

        if (score_per_vertex) {
          vertex_for_each_faster (v)
            vertex_score [v] += mc_board->vertex_score (v);
        }
        break;
      case mercy:
        out << "Mercy rule should be off for benchmarking" << endl;
        return;
        //win_cnt [mc_board->approx_winner ()] ++;
        //break;
      case too_long:
        break;
      }
    }
    
    float seconds_end = get_seconds ();
    perf_timer.stop ();
    
    out << "Initial board:" << endl;
    out << "komi " << start_board->get_komi () << " for white" << endl;
    
    out << start_board->to_string ();
    out << endl;
    
    if (score_per_vertex) {
      FastMap<Vertex, float> black_own;
      vertex_for_each_all (v) 
        black_own [v] = float(vertex_score [v]) / float (playout_ok_cnt);

      out << "P(black owns vertex) rescaled to [-1, 1] (p*2 - 1): " << endl 
          << to_string_2d (black_own) 
          << endl;
    }

    out << "Black wins    = " << win_cnt [black_player] << endl
        << "White wins    = " << win_cnt [white_player] << endl
        << "P(black win)  = " 
        << float (win_cnt [black_player]) / 
           float (win_cnt [black_player] + win_cnt [white_player]) 
        << endl;

    float avg_score = float (playout_ok_score) / float (playout_ok_cnt);

    out << "E(score)      = " 
        << avg_score - 0.5 
        << " (without komi = " << avg_score - mc_board->komi << ")" << endl << endl;

    float seconds_total = seconds_end - seconds_begin;
    float cc_per_playout = perf_timer.ticks () / double (playout_cnt);

    out << "Performance: " << endl
        << "  " << playout_cnt << " playouts" << endl
        << "  " << seconds_total << " seconds" << endl
        << "  " << float (playout_cnt) / seconds_total / 1000.0 << " kpps" << endl
        << "  " << cc_per_playout << " ccpp (clock cycles per playout)" << endl
        << "  " << 1000000.0 / cc_per_playout  << " kpps/GHz (clock independent)" << endl
      ;
  }
}
// main
int main (int argc, char** argv) { 
  setvbuf (stdout, (char *)NULL, _IONBF, 0);
  setvbuf (stderr, (char *)NULL, _IONBF, 0);

  Board board;
  ostringstream response;
  uint playout_cnt = 100000;
  simple_playout_benchmark::run<false> (&board, playout_cnt, response);
  cout << response.str();
  return 0;
}
