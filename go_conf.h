#ifndef _GO_CONF_
#define _GO_CONF_

typedef unsigned int uint;
typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
// constants

// board parameters
const float default_komi       = 5.5;
const bool playout_print       = false;

// mercy rule

//const bool use_mercy_rule      = false;
const uint mercy_threshold     = 25;

// uct parameters

const uint initial_value                 = 0;
const uint initial_bias                  = 0;
extern float mature_bias_threshold;
extern float explore_rate;
extern float aaf_fraction;
// how many memory would be use
const uint  uct_node_memory               = 40;
const uint  uct_max_level       	  = 49;
extern float time_per_move;
const uint  uct_max_depth                 = 1000;
const uint  uct_genmove_playout_cnt       = 100000;
const float resign_value                  = 0.45;
const float print_visit_threshold_base    = 5000.0;
const float print_visit_threshold_parent  = 0.03;


// consistency checking / debugging control

//#define DEBUG
#ifdef NDEBUG
//#error "NDEBUG not allowed."
#endif

#ifndef DEBUG
//#error "DEBUG should be defined."
#endif

const bool pm_ac              = false;
const bool stack_ac           = false;

const bool paranoic           = false;
const bool board_ac           = false;

const bool player_ac          = paranoic;
const bool color_ac           = paranoic;
const bool coord_ac           = paranoic;
const bool vertex_ac          = paranoic;



const bool nbr_cnt_ac         = paranoic;
const bool chain_ac           = paranoic;
const bool board_empty_v_ac   = paranoic;
const bool board_hash_ac      = paranoic;
const bool board_color_at_ac  = paranoic;
const bool board_nbr_cnt_ac   = paranoic;
const bool chain_at_ac        = paranoic;
const bool chain_next_v_ac    = paranoic;
const bool chains_ac          = paranoic;

const bool playout_ac         = false;
const bool aaf_ac             = false;
const bool uct_ac             = false;
const bool tree_ac            = false;
const bool pool_ac            = false;
const bool gtp_ac             = false;
#endif // _GO_CONF_
