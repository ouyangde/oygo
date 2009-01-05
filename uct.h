#ifndef _UCT_H
#define _UCT_H
// class Pool
template <class e, uint size> class Pool {
public:
	e* memory;
	e** index;
	uint   free_count;
	Pool() {
		memory = (e*) new char[size*sizeof(e)];
		index = new e*[size];
		rep (i, size) 
			index[i] = memory + i;
		free_count = size;
	}
	~Pool() {
		delete [] (char*)memory;
		delete [] index;
	}
	e* malloc() { 
		assertc(pool_ac, free_count > 0);
		return index[--free_count]; 
	}
	void free(e* p) { 
		index[free_count++] = p;  
	}
};
const uint  uct_max_nodes = uct_node_memory * 1000000 / 16;
const uint  ucg_max_nodes = uct_node_memory * 1000000 / 16;
const uint  ucg_max_hash = uct_node_memory * 1000000 / sizeof(20);
class Stat {
public:
  float sample_count;
  float sample_sum;

  Stat () {
    reset ();
  }
  void reset () {
    sample_count       = 0; // TODO 
    sample_sum         = 0.0; // TODO
  }
  void update_win() {
    sample_sum         += 1;
    sample_count       += 1.0;
  }
  void update_loss() {
    sample_sum         -= 1;
    sample_count       += 1.0;
  }
  float mean () { 
    return sample_sum / sample_count; 
  }
  float count () { return sample_count; }
  void init() {}
  void init(Player pl, Hash hash) {}
};

class _UCGStat : public Stat {
public:
	uint ref;
	Hash hash;
	Player pl;
	_UCGStat() {
		ref = 0;
	}
};
class UCGStat {
public:
	static _UCGStat* m_statpool;
	_UCGStat * stat;
	float mean() {
		if(stat) return stat->mean();
		return large_float;
	}
	float count() {
		if(stat) return stat->count(); 
		return 0;
	}
	void update_win() {
		assert(stat != NULL);
		stat->update_win();
	}
	void update_loss() {
		assert(stat != NULL);
		stat->update_loss();
	}
	UCGStat() {
		stat = NULL;
	}
	~UCGStat() {
		if(stat) {
			stat->ref--;
		}
	}
	void init() {
		static _UCGStat s_stat;
		if(stat) return;
		stat = &s_stat;
		stat->ref++;
	}
	void init(Player pl, Hash hash) {
		if(stat) return;
		uint idx_start = hash.hash % ucg_max_hash;
		uint pos = idx_start;
		while(m_statpool[pos].ref) {
			//cerr<<"exists"<<endl;
			if(m_statpool[pos].hash == hash && m_statpool[pos].pl == pl) {
				stat = &(m_statpool[pos]);
				m_statpool[pos].ref++;
				return;
			} else {
				pos = (pos + 1) % ucg_max_hash;
			}
			if(pos == idx_start) {
				cerr<<"new stat"<<endl;
				stat = new _UCGStat;
				return;
			}
		}
		m_statpool[pos].ref = 1;
		m_statpool[pos].pl = pl;
		m_statpool[pos].hash = hash;
		stat = &(m_statpool[pos]);
	}
};
#define node_for_each_child(node, act_node, i) do {   \
	assertc (tree_ac, node!= NULL);                         \
	Node<T>* act_node;                                       \
	act_node = node->first_child;                      \
	while (act_node != NULL) {                              \
		i;                                                    \
		act_node = act_node->sibling;                         \
	}                                                       \
} while (false)

// class Node
template<uint T>
class Node {
	static Pool<Node, ucg_max_nodes> m_pool;
public:
	Vertex<T> v;
	// TODO: use Stat or UCGStat ?
	UCGStat stat;
	//Stat stat;
	float value() { 
		return stat.mean();
	}
	float count() { 
		return stat.count(); 
	}
	Node*  first_child;			// head of list of moves of particular player 
	Node*  sibling;                           // NULL if last child
	static void * operator new(size_t t) {
		assertc(pool_ac, t == sizeof(Node));
		return Node::m_pool.malloc();
	}
	static void operator delete(void *p) {
		Node::m_pool.free((Node*)p);
	}
	static uint free_count() {
		return Node::m_pool.free_count;
	}
public:
	void dump() {
		cerr<<v
			<<","<<stat.mean()
			<<","<<stat.count()
			<<endl;
	}
	void rec_print(ostream& out, uint depth, Player pl) {
		rep (d, depth) out << "  ";
		out
			<< to_string(pl) << " "
			<< v << " "
			<< value() << " "
			<< "(" << count() - initial_bias << ")"
			<< endl;

		rec_print_children (out, depth, player::other(pl));
	}
	void rec_print_children (ostream& out, uint depth, Player player) {
		Node*  child_tab [Vertex<T>::cnt]; // rough upper bound for the number of legal move
		uint     child_tab_size;
		uint     best_child_idx;
		float    min_visit_cnt;

		child_tab_size  = 0;
		best_child_idx  = 0;
		min_visit_cnt   = max(print_visit_threshold_base, (count() - initial_bias) * print_visit_threshold_parent);
		// we want to be visited at least initial_bias times + some percentage of parent's visit_cnt

		// prepare for selection sort
		node_for_each_child (this, child, child_tab[child_tab_size++] = child);

#define best_child child_tab [best_child_idx]

		while (child_tab_size > 0) {
			// find best child
			rep(ii, child_tab_size) {
				if (best_child->value() < child_tab[ii]->value())
					best_child_idx = ii;
			}
			// rec call
			if (best_child->count() - initial_bias >= min_visit_cnt)
				child_tab [best_child_idx]->rec_print (out, depth + 1, player);
			else break;

			// remove best
			best_child = child_tab [--child_tab_size];
		}

#undef best_child
	}
	Node(Vertex<T> v) {
		this->v       = v;
		first_child   = NULL;
		sibling       = NULL;
	}

	~Node() {
	}
	//void init(Vertex<T> v) {
	//}

	void initstat() {
		stat.init();
	}

	void initstat(Player pl, Hash hash) {
		stat.init(pl, hash);
	}
	void add_child(Node* new_child) { // TODO sorting?
		assertc(tree_ac, new_child->sibling == NULL); 
		assertc(tree_ac, new_child->first_child == NULL); 

		new_child->sibling = this->first_child;
		this->first_child = new_child;
	}
	void remove_child(Node* del_child) { // TODO inefficient
		Node* act_child;
		assertc(tree_ac, del_child != NULL);

		if (first_child == del_child) {
			first_child = first_child->sibling;
			return;
		}

		act_child = first_child;

		while (true) {
			assertc (tree_ac, act_child != NULL);
			if (act_child->sibling == del_child) {
				act_child->sibling = act_child->sibling->sibling;
				return;
			}
			act_child = act_child->sibling;
		}
	}
	float UCB(float explore_coeff) { 
		if(!count()) return large_float;
		return value() + sqrt (explore_coeff / count());
	}
	void update_win() {
		stat.update_win();
	}
	void update_loss() {
		stat.update_loss();
	}
	bool is_mature () { 
		return count() > mature_bias_threshold; 
		//return count > initial_bias; 
	}
	bool no_children() {
		return first_child == NULL;
	}
	Node* find_uct_child() {
		Node* best_child;
		float   best_urgency;
		float   explore_coeff;

		best_child     = NULL;
		best_urgency   = -large_float;
		explore_coeff  = log(count()) * explore_rate;

		node_for_each_child (this, child, {
			float child_urgency = child->UCB(explore_coeff);
			if (child_urgency > best_urgency) {
				best_urgency  = child_urgency;
				best_child    = child;
			}
		});
		//best_child->dump();

		assertc (tree_ac, best_child != NULL); // at least pass
		return best_child;
	}
	bool remove_bad_child() {
		Node* worst_child = NULL;
		float worst_urgency = large_float;
		float explore_coeff = log(count()) * explore_rate;

		node_for_each_child (this, child, {
			float child_urgency = child->value();
			if (child_urgency < worst_urgency) {
				worst_urgency  = child_urgency;
				worst_child    = child;
			}
		});
		if(worst_child == this->first_child && worst_child->sibling == NULL) {
			// this is the last one
			return false;
		}
		this->remove_child(worst_child);
		free_subtree(worst_child);
		delete worst_child;

		return true;
	}
	void free_subtree(Node<T>* parent) {
		node_for_each_child(parent, child, {
			free_subtree(child);
			delete child;
		});
	}
	Node* find_most_explored_child() {
		Node* best_child = NULL;
		float max_count = 0;
		best_child     = NULL;

		node_for_each_child (this, child, {
			if (child->count > max_count) {
				max_count     = child->count;
				best_child    = child;
			}
		});
		assertc (tree_ac, best_child != NULL);
		return best_child;
	}
	Node* find_max_value_child () {
		Node* best_child = NULL;
		float max_value = -large_float;
		best_child     = NULL;

		node_for_each_child (this, child, {
			float value = child->value();
			if (value > max_value) {
				max_value     = value;
				best_child    = child;
			}
		});
		assertc (tree_ac, best_child != NULL);
		return best_child;
	}
};
// class Tree
template<uint T>
class UCBTree {
public:

	union {
		Node<T>* root;
		Node<T>* history[uct_max_depth];
	};
	uint history_top;
	uint max_top;

public:
	void dump(Player pl) {
		root->rec_print(cerr, 0, player::other(pl));
	}

	UCBTree () {
		root = new Node<T>(Vertex<T>::any());
		//root->init(Vertex<T>::any());
		Hash hash;
		hash.set_zero();
		root->initstat(player::black, hash);
		history_top = 0;
		max_top = 0;
	}
	void history_reset() {
		history_top = 0;
	} 
	Node<T>* act_node () {
		return history[history_top];
	}  
	void uct_descend() {
		history[history_top + 1] = act_node()->find_uct_child();
		history_top++;
		if(history_top > max_top) max_top = history_top;
		assertc(tree_ac, act_node() != NULL);
	}  
	void alloc_child(Vertex<T> v) {
		Node<T>* new_node = new Node<T>(v);
		//new_node->init(v);
		act_node()->add_child(new_node);
	}  
	void delete_act_node() {
		assertc(tree_ac, history_top > 0);
		history[history_top-1]->remove_child(act_node());
		delete act_node();
		history_top--;
	} 
	// TODO free history (for sync with base board)

	// ��Ϊĳ��ԭ�������act_player�ĺ����ǵߵ���
	void update_history(Player winner, Player act_player) {
		int stop = (history_top + 1) % 2;
		int i;
		if(winner != act_player) {
			for(i = history_top; i >= stop;) {
				history[i--]->update_win();
				history[i--]->update_loss();
			}
			if(stop) history[i--]->update_win();
		} else {
			for(i = history_top; i >= stop;) {
				history[i--]->update_loss();
				history[i--]->update_win();
			}
			if(stop) history[i--]->update_loss();
		}
	}
};
template <uint T, template<uint T> class Policy, template<uint T> class Board> 
class UCT {
public:
  
	Board<T>*     base_board;
	Policy<T>*    policy;
	UCBTree<T>          tree[1];      // TODO tree->root should be in sync with top of base_board
public:
	UCT(Board<T>* board, Policy<T>* policy) {
		this->policy = policy;
		this->base_board = board;
	}
public:
  
	/*
	 * no need
	 */
	void root_ensure_children_legality(Player pl) { 
		// cares about superko in root (only)
		// ����superko, �ο�http://www.weddslist.com/kgs/past/superko.html
		tree->history_reset();

		assertc(uct_ac, tree->history_top == 0);
		assertc(uct_ac, tree->act_node()->first_child == NULL);

		empty_v_for_each_and_pass(base_board, v, {
		//	if (base_board.is_strict_legal (pl, v))
			tree->alloc_child(v);
		});
	}

	//flatten 
	void do_playout(Player first_player) {
		Board<T>    play_board[1]; // TODO test for perfomance + memcpy
		//bool was_pass[player::cnt];
		Player   act_player = first_player;
		Vertex<T>   v;


		play_board->load(base_board);
		tree->history_reset();

		//player_for_each (pl) 
			//was_pass [pl] = false; // TODO maybe there was one pass ?

		do {
			// Ҷ�ӽڵ�
			if (tree->act_node()->no_children()) {
				// If the leaf is ready expand the tree -- add children - all potential legal v (i.e.empty)
				if (tree->act_node()->is_mature()) {
					// ��ģ������������ӽڵ�
					empty_v_for_each_and_pass(play_board, v, {
						tree->alloc_child(v); 
						// TODO simple ko should be handled here
						// (suicides and ko recaptures, needs to be dealt with later)
					});
					continue;
				} else {
					tree->act_node()->initstat(act_player, play_board->hash);
					// �������ģ��
					// TODO assert act_plauer == board->Act_player ()
					Playout<T, Policy, Board>(policy, play_board).run();
					//cerr<<to_string(*play_board)<<endl;
					// ���½��
					tree->update_history(play_board->winner(), act_player);
					// һ��ģ��Ծֽ���
					//int winner_idx = play_board->winner().get_idx ();
					// result values are 1 for black, -1 for white
					//tree->update_history(1 - winner_idx - winner_idx); 

					break;
				}

			}
			// ��Ҷ�ӽڵ㣬ѡ��UCT�ӽڵ�
			tree->uct_descend();
			v = tree->act_node()->v;

			if(!play_board->play(act_player, v)) {
				tree->delete_act_node();
				continue; //TODO: return?
			}

			//was_pass [act_player]  = (v == Vertex<T>::pass());
			if(play_board->both_player_pass()) {
			//if (was_pass [player::black] & was_pass [player::white]) {
				tree->act_node()->initstat();
				tree->update_history(play_board->winner(), act_player);
				break;
			}
			act_player = play_board->act_player();
		} while (true);
	}

	void play(Player pl, Vertex<T> v) {
		if(tree->root->no_children()) return;
		Node<T>* new_root = NULL;
		node_for_each_child(tree->root, child, {
			if(v != child->v) {
				tree->root->free_subtree(child);
				delete child;
			} else {
				new_root = child;
			}
		});
		//assert(new_root != NULL);
		if(new_root == NULL) {
			new_root = new Node<T>(v);
			//new_root->init(v);
			new_root->initstat(player::other(pl), base_board->hash);
		} else {
			new_root->sibling = NULL;
		}
		delete tree->root;
		tree->root = new_root;
	}

	Vertex<T> gen_move(Player player) {
		Node<T>* best;
		tree->max_top = 0;
		//root_ensure_children_legality(player);
		//rep(ii, uct_genmove_playout_cnt) do_playout(player);
		float seconds_begin = get_seconds();
		float seconds_end = 0;
		const uint split_cnt = 1000;
		while(true) {
			if(Node<T>::free_count() < split_cnt * base_board->empty_v_cnt) {
				if(!tree->root->remove_bad_child()) {
					break;
				}
				continue;
			}
			rep(ii, split_cnt) do_playout(player);
			seconds_end = get_seconds();
			if(seconds_end - seconds_begin > time_per_move) break;
			if(tree->max_top > uct_max_level) break;
		}

		//rep(ii, 300) do_playout(player);
		//best = tree->root->find_most_explored_child();
		best = tree->root->find_max_value_child();
		float seconds_total = seconds_end - seconds_begin;
		assertc(uct_ac, best != NULL);

		tree->dump(player);
		cerr<<"best:";
		best->dump();
		cerr<<"level:"<<tree->max_top<<endl;
		cerr<<"memory free:"<<Node<T>::free_count()<<endl;
		cerr<<"time:"<<seconds_total<<endl;
		float bestvalue = best->value();
		if(bestvalue < -resign_value) return Vertex<T>::resign();
		return best->v;
	}
};
template<uint T>
// TODO:UCG���ڵ�״̬�����洢����hash����
Pool<Node<T>, ucg_max_nodes> Node<T>::m_pool;
_UCGStat* UCGStat::m_statpool = new _UCGStat[ucg_max_hash];
#endif
