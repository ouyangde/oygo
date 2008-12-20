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
const uint default_board_size  = 9;
const bool playout_print       = false;

// mercy rule

//const bool use_mercy_rule      = false;
const uint mercy_threshold     = 25;

// uct parameters

const float initial_value                 = 0.0;
const float initial_bias                  = 1.0;
const float mature_bias_threshold         = initial_bias + 100.0;
const float explore_rate                  = 1.0;
const uint  uct_max_depth                 = 1000;
const uint  uct_max_nodes                 = 1000000;
const float resign_value                  = 0.95;
const uint  uct_genmove_playout_cnt       = 50000;
const float print_visit_threshold_base    = 500.0;
const float print_visit_threshold_parent  = 0.02;


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
const bool gtp_ac             = true;
#endif // _GO_CONF_
