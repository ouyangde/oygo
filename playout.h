
enum playout_status_t { pass_pass, mercy, too_long };


// ----------------------------------------------------------------------
template <typename Policy,uint T> class Playout {
public:
  static const uint max_playout_length = T * T * 2;
  Policy*  policy;
  Board<T>*   board;

  Playout (Policy* policy_, Board<T>*  board_) : policy (policy_), board (board_) {}

  all_inline
  void play_move () {
    policy->prepare_vertex ();
    
    while (true) {
      Player   act_player = board->act_player ();
      Vertex<T>   v          = policy->next_vertex ();

      if (board->is_pseudo_legal (act_player, v) == false) {
        policy->bad_vertex (v);
        continue;
      } else {
        board->play_legal (act_player, v);
        policy->played_vertex (v);
        break;
      }
    }
  }

  all_inline
  playout_status_t run () {
    
    policy->begin_playout (board);
    while (true) {
      play_move ();
      
      if (board->both_player_pass ()) {
        policy->end_playout (pass_pass);
        return pass_pass;
      }
      
      if (board->move_no >= max_playout_length) {
        policy->end_playout (too_long);
        return too_long;
      }
      
      if (use_mercy_rule && uint (abs (board->approx_score ())) > mercy_threshold) {
        policy->end_playout (mercy);
        return mercy;
      }
    }
  }
  
};

// ----------------------------------------------------------------------
template<uint T> class SimplePolicy {
protected:

  static PmRandom pm; // TODO make it non-static

  uint to_check_cnt;
  uint act_evi;

  Board<T>* board;
  Player act_player;

public:

  SimplePolicy () : board (NULL) { }

  void begin_playout (Board<T>* board_) { 
    board = board_;
  }

  void prepare_vertex () {
    act_player     = board->act_player ();
    to_check_cnt   = board->empty_v_cnt;
    act_evi        = pm.rand_int (board->empty_v_cnt); 
  }

  Vertex<T> next_vertex () {
    Vertex<T> v;
    while (true) {
      if (to_check_cnt == 0) return Vertex<T>::pass ();
      to_check_cnt--;
      v = board->empty_v [act_evi];
      act_evi++;
      if (act_evi == board->empty_v_cnt) act_evi = 0;
      if (!board->is_eyelike (act_player, v)) return v;
    }
  }

  void bad_vertex (Vertex<T>) {
  }

  void played_vertex (Vertex<T>) { 
  }

  void end_playout (playout_status_t) { 
  }

};

template<uint T> PmRandom SimplePolicy<T>::pm(123);
