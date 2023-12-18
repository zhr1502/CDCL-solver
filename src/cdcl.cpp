#include "cdcl.hpp"
#include <algorithm>
#include <ratio>
#include <vector>
#include <iostream>
#include <queue>
#include <chrono>

double update_time = 0;

std::string to_string(const Value value)
{
    switch (value)
    {
        case Value::Free: return "Value::Free";
        case Value::True: return "Value::True";
        case Value::False: return "Value::False";
        default: return "";
    }
}
void CDCL::solve()
{
    if (this->solved) return;

    srand(758926699);
    this->solved = true;
    update_time = 0;
    auto up_result = this->unit_propogation();
    if (up_result)
        return; // Unit propagation when no variable is picked returns conflict,
                // the CNF is not satisfiable.
                //
    int confl_cnt = 0, pick_cnt = 0;
    do
    {
        Literal* pick_lit = this->choose_variable();
        if (pick_lit == nullptr)
        {
            satisfiable = true;
            std::cout << "Update Time: " << update_time << "ms" << std::endl;
            return;
        }
        Assign assign =
            Assign{pick_lit->index, rand() % 2 ? Value::False : Value::True};
        pick_stack.push(assign);
        pick_cnt++;

        this->graph->pick_var(nullptr, pick_lit);

        auto start = std::chrono::high_resolution_clock::now();
        this->update(assign);
        auto end = std::chrono::high_resolution_clock::now();
        update_time += std::chrono::duration<double, std::milli>(end- start).count();

        auto up_result = this->unit_propogation();
        while (up_result)
        {
            Assign deassign;

            auto last_conflict_clause = *(this->conflict_clause.end() - 1);
            auto conflict_fuip = last_conflict_clause->clause->literals.at(0);

            int unpick_to_rank;

            if (last_conflict_clause->clause->literals.size() == 1)
                unpick_to_rank = 1;
            else
                unpick_to_rank = vars_rank.at(conflict_fuip->index);

            while (pick_stack.size() >= unpick_to_rank)
            {
                deassign = pick_stack.top();
                this->pick_stack.pop();
            }

            this->graph->drop_to(unpick_to_rank);

            unchecked_queue.push(
                std::make_pair(last_conflict_clause->clause->literals[0],
                               last_conflict_clause));

            confl = nullptr;

            up_result = this->unit_propogation();

            if ((*(this->conflict_clause.end() - 1))->clause->literals.size() ==
                0)
            {
                std::cout << "Update Time: " << update_time << "ms" << std::endl;
                satisfiable = false;
                return;
            }
            confl_cnt++;
        }
        if (confl_cnt > 3000 && (double)pick_cnt / confl_cnt > 0.97)
        {
            confl_cnt = 0; pick_cnt = 0;
            this->graph->drop_to(1);
            while (!pick_stack.empty()) pick_stack.pop();
            while (!unchecked_queue.empty()) unchecked_queue.pop();
            for (int i = 1; i <= origin_cnf->variable_number; i++)
                vars_rank[i] = 0;
        }
        // this->debug();
    } while (true);

    return;
}

bool CDCL::unit_propogation()
{
    std::vector<Assign> picked_lits;
    Assign new_assign;
    bool conflict = false;

    if (confl != nullptr) // Conflict encountered
    {
        // std::cout << "Starting to generating conflict clause" << std::endl;

        Clause* learned_clause = this->graph->conflict_clause_gen(confl);

        ClauseWrapper* learned_clause_wrapped =
            new ClauseWrapper(learned_clause, this);

        this->add_clause(learned_clause_wrapped, this->conflict_clause);
        this->cdcl_cnf->clauses.push_back(learned_clause);
        return true;
    }

    do
    {
        if (this->unchecked_queue.empty()) break;

        auto reason = unchecked_queue.front().second;
        auto unit_lit = unchecked_queue.front().first;
        unchecked_queue.pop();

        if (!reason->is_unit(unit_lit)) continue;

        new_assign = Assign{unit_lit->index,
                            unit_lit->is_neg ? Value::False : Value::True};

        picked_lits.push_back(new_assign);

        this->graph->pick_var(reason, unit_lit);

        auto start = std::chrono::high_resolution_clock::now();
        auto confl = this->update(new_assign);
        auto end = std::chrono::high_resolution_clock::now();
        
        update_time += std::chrono::duration<double, std::milli>(end- start).count();
        // this->debug();

        if (confl != nullptr) // Conflict encountered
        {
            // std::cout << "Starting to generating conflict clause" <<
            // std::endl;
            conflict = true;

            Clause* learned_clause = this->graph->conflict_clause_gen(confl);

            ClauseWrapper* learned_clause_wrapped =
                new ClauseWrapper(learned_clause, this);

            this->add_clause(learned_clause_wrapped, this->conflict_clause);
            this->cdcl_cnf->clauses.push_back(learned_clause);
            // this->debug();

            if (learned_clause_wrapped->clause->index == 182)
            {
                // std::cout << "We're here!!!" << std::endl;
            }
        }

    } while (!conflict);

    return conflict;
}

ClauseWrapper* CDCL::update(Assign assign)
{
    assignment.at(assign.variable_index) = assign;
    auto res = vars.at(assign.variable_index).update_watchlist(assign);
    if (res != nullptr) confl = res;
    return res;
}

void CDCL::add_clause(ClauseWrapper* clause,
                      std::vector<ClauseWrapper*>& clausedb)
{
    clausedb.push_back(clause);
    int siz = clause->clause->literals.size();
    if (!siz) return;
    if (siz == 1)
    {
        unchecked_queue.push(
            std::make_pair(clause->clause->literals.back(), clause));
    }
    else
    {
        Literal *f = clause->clause->literals.at(0),
                *s = clause->clause->literals.at(1);

        vars.at(f->index).watchlist_pushback(clause, s, f);
        vars.at(s->index).watchlist_pushback(clause, f, s);
        // add_clause have to ensure the first two literals in clause is also
        // the first to be backtraced. This should be done when confl cls is
        // generated, e.g. in CDCL::conflict_clause_gen()
        // Is cdcl_cnf and origin_cnf should share some literal to avoid
        // difference here?
    }
}

Literal* CDCL::choose_variable()
{
    int variable = 0;
    for (int i = 1; i <= this->origin_cnf->variable_number; i++)
        if (this->assignment[i].value == Value::Free)
        {
            variable = i;
            break;
        }
    if (!variable) return nullptr;
    for (auto lit : origin_cnf->literals)
        if (lit->index == variable) return lit;

    return this->insert_literal(variable, false);
}

Literal* CDCL::insert_literal(Variable var, bool is_neg)
{
    for (auto lit : origin_cnf->literals)
    {
        if (lit->is_neg == is_neg && lit->index == var) return lit;
    }
    return cdcl_cnf->insert_literal(var, is_neg);
}

void CDCL::init(CNF* cnf)
{
    this->drop();
    this->origin_cnf = cnf;

    CNF* conflict_cnf = new CNF;
    conflict_cnf->variable_number = cnf->variable_number;
    this->cdcl_cnf = conflict_cnf;

    this->vars.reserve(cnf->variable_number + 1);

    for (int i = 0; i <= cnf->variable_number; i++)
        assignment.push_back(Assign{i, Value::Free}),
            vars.push_back(VariableWrapper(i, this));

    this->vars_rank.resize(this->origin_cnf->variable_number + 1);

    for (auto c : origin_cnf->clauses)
        this->add_clause(new ClauseWrapper(c, this), this->clause);

    ImpGraph* graph = new ImpGraph;
    graph->init(this);

    this->graph = graph;

    return;
}

ClauseWrapper::ClauseWrapper(Clause* raw, CDCL* owner)
{
    cdcl = owner;
    clause = raw;
    // Randomize watch literals
    return;
}

Literal* ClauseWrapper::get_blocker(Variable watcher)
{
    if (clause->literals.size() == 1) return clause->literals[0];

    return clause->literals.at(clause->literals[0]->index == watcher);
}

void ClauseWrapper::drop()
{
    // lits_value should be deprecated here!!!.
    // lits_pos.clear();
    delete this;
}

Literal* ClauseWrapper::update_watcher(Variable watcher)
{
    if (clause->literals.size() == 1) return nullptr;

    Literal*& origin =
        clause->literals[clause->literals.at(0)->index != watcher];

    auto get_lit_value = [this](Literal* x) {
        auto var_value = cdcl->assignment.at(x->index).value;
        if (var_value == Value::Free) return Value::Free;
        if (x->is_neg)
            return var_value == Value::False ? Value::True : Value::False;
        return var_value;
    };

    Literal* new_watcher = origin;
    bool found = 0;

    int watcher_rank = cdcl->vars_rank.at(watcher);
    for (int i = 2; i < clause->literals.size(); i++)
    {
        auto lit = clause->literals[i];
        if (get_lit_value(lit) != Value::False)
        {
            new_watcher = clause->literals[i];
            std::swap(clause->literals[i], origin);

            found = 1;
            break;
        }
    }

    if (!found)
    {
        int max_rank = watcher_rank, pos = -1;

        for (int i = 2; i < clause->literals.size(); i++)
        {
            auto var = clause->literals[i]->index;
            if (cdcl->vars_rank.at(var) > max_rank)
            {
                pos = i;
                max_rank = cdcl->vars_rank.at(var);
            }
        }

        if (pos != -1)
        {
            new_watcher = clause->literals.at(pos);
            std::swap(clause->literals[pos], origin);
        }
    }

    return new_watcher;
}

bool ClauseWrapper::is_unit(Literal* watcher)
{
    auto get_lit_value = [this](Literal* x) {
        auto var_value = cdcl->assignment.at(x->index).value;
        if (var_value == Value::Free) return Value::Free;
        if (x->is_neg)
            return var_value == Value::False ? Value::True : Value::False;
        return var_value;
    };

    if (clause->literals.size() == 1 && get_lit_value(watcher) == Value::Free)
        return true;

    if (clause->literals[0] != watcher && clause->literals[1] != watcher)
        return false;

    auto blocker = clause->literals[clause->literals[0] == watcher];

    return get_lit_value(watcher) == Value::Free &&
           get_lit_value(blocker) == Value::False;
}

VariableWrapper::VariableWrapper(Variable index, CDCL* owner)
    : var(index), cdcl(owner)
{
}

std::list<ClauseWrapper*>& VariableWrapper::get_watchlist(Assign assign)
{
    if (assign.value == Value::False) return pos_watcher;
    if (assign.value == Value::True) return neg_watcher;
    std::cout << "VariableWrapper::get_watchlist(): Assign of Free found."
              << std::endl;
    abort();
}

void VariableWrapper::watchlist_pushback(ClauseWrapper* clause,
                                         Literal* blocker, Literal* self)
{
    if (self->is_neg)
    {
        neg_watcher.push_back(clause);
    }
    else
    {
        pos_watcher.push_back(clause);
    }

    return;
}

Value VariableWrapper::get_value() { return cdcl->assignment.at(var).value; }

ClauseWrapper* VariableWrapper::update_watchlist(Assign assign)
{
    ClauseWrapper* confl = nullptr;
    if (assign.value == Value::Free) return confl;

    auto& updlist = get_watchlist(assign);
    auto it = updlist.begin();

    auto get_lit_value = [this](Literal* x) {
        auto var_value = cdcl->assignment.at(x->index).value;
        if (var_value == Value::Free) return Value::Free;
        if (x->is_neg)
            return var_value == Value::False ? Value::True : Value::False;
        return var_value;
    };

    while (it != updlist.end())
    {
        // watcher& w = *it;
        ClauseWrapper* clause = *it;
        Literal* blocker = clause->get_blocker(var);
        Literal* watcher = clause->get_blocker(blocker->index);

        Literal* new_watch_var = clause->update_watcher(var);

        if (get_lit_value(new_watch_var) == Value::False)
        {
            if (get_lit_value(blocker) == Value::False)
                confl = clause;
            else
                cdcl->unchecked_queue.push(std::make_pair(blocker, clause));
        }

        if (new_watch_var == watcher)
            it++;
        else
        {
            it = updlist.erase(it);

            cdcl->vars.at(new_watch_var->index)
                .watchlist_pushback(clause, blocker, new_watch_var);
        }
    }

    return confl;
}

void ImpGraph::init(CDCL* cdcl)
{
    this->cdcl = cdcl;

    this->vars_to_nodes.resize(cdcl->origin_cnf->variable_number + 1);
    this->assigned_order.resize(cdcl->origin_cnf->variable_number + 1);
    this->trail.resize(1);
    return;
}

void ImpGraph::pick_var(ClauseWrapper* reason, Literal* picked_lit)
{
    int rank = 0;
    Assign* assign = &(this->cdcl->assignment.at(picked_lit->index));
    ImpNode* conclusion; //= this->add_node(assign, rank);

    if (reason == nullptr) // The variable is picked because UP can't be
                           // furthered, no cluase can derive it.
    {
        rank = this->cdcl->pick_stack.size();
        conclusion = this->add_node(assign, rank);
        this->cdcl->vars_rank.at(picked_lit->index) = rank;
        this->assigned_order.at(picked_lit->index) = 1;
        this->trail.push_back(std::vector<ImpNode*>(1, conclusion));
        return;
    }

    for (auto lit : reason->clause->literals)
        if (lit != picked_lit)
        {
            rank = std::max(rank, this->cdcl->vars_rank.at(lit->index));
        }

    conclusion = this->add_node(assign, rank);

    for (auto lit : reason->clause->literals)
        if (lit != picked_lit)
        {
            ImpNode* premise = this->vars_to_nodes.at(lit->index);
            if (premise == nullptr)
            {
                std::cout << "Nullptr Encountered !!!! "
                          << reason->clause->index << std::endl;
            }
            this->add_reason(premise, conclusion, reason);
        }

    this->cdcl->vars_rank.at(picked_lit->index) = rank;
    if (this->trail.size() > rank)
        this->trail.at(rank).push_back(conclusion);
    else
    {
        std::cout << "ImpGraph::pick_var: Skipped rank found" << std::endl;
        abort();
    }

    this->assigned_order.at(picked_lit->index) = this->trail.at(rank).size();

    return;
}

ImpNode* ImpGraph::add_node(Assign* assign, int rank)
{
    this->vars_to_nodes.at(assign->variable_index) =
        new ImpNode(assign, rank, rank == 0);
    return this->vars_to_nodes.at(assign->variable_index);
}

Clause* ImpGraph::conflict_clause_gen(ClauseWrapper* conflict_clause)
{
    // std::cout << "Generating new clause ... Reason: "
    //           << (conflict_clause->clause->cnf == cdcl->origin_cnf
    //                   ? ""
    //                   : "(Conflict CDCL) ")
    //           << conflict_clause->clause->index << std::endl;

    if (conflict_clause->clause->index == 60)
    {
        // std::cout << "We are here!" << std::endl;
    }

    Clause* learned_clause = new Clause; // Create a new raw clause
    learned_clause->cnf = this->cdcl->cdcl_cnf;
    learned_clause->index = this->cdcl->conflict_clause.size();

    auto cmp = [this](ImpNode* lhs, ImpNode* rhs) -> bool {
        return assigned_order.at(lhs->get_var_index()) <
               assigned_order.at(rhs->get_var_index());
    };

    std::priority_queue<ImpNode*, std::vector<ImpNode*>, decltype(cmp)> queue(
        cmp);

    std::map<int, bool> vars_appearance;

    int decision_rank = 0;

    for (auto lit : conflict_clause->clause->literals)
    {
        auto node = vars_to_nodes.at(lit->index);
        if (node == nullptr)
        {
            std::cout
                << "ImpGraph::conflict_clause_gen: Unassigned variable found."
                << std::endl;
            abort();
        }
        decision_rank = std::max(decision_rank, node->get_rank());
    }
    if (!decision_rank) return learned_clause;

    int blocker = -1;
    std::pair<int, int> lastest(0, 0);

    for (auto lit : conflict_clause->clause->literals)
    {
        auto node = vars_to_nodes.at(lit->index);
        if (node->get_rank() == decision_rank)
        {
            queue.push(node);
        }
        else
        {
            auto order = std::make_pair(
                node->get_rank(), assigned_order.at(node->get_var_index()));
            if (order > lastest)
            {
                lastest = order;
                blocker = node->get_var_index();
            }
            vars_appearance[node->get_var_index()] = 1;
        }
    }
    while (!queue.empty())
    {
        if (queue.size() == 1) break;
        auto node = queue.top();
        queue.pop();
        for (auto reason : node->get_in_node())
        {
            if (reason == nullptr)
            {
                std::cout
                    << "ImpGraph::conflict_clause_gen: Nullptr encountered."
                    << std::endl;
                abort();
            }
            if (reason->get_rank() == decision_rank)
                queue.push(reason);
            else
            {
                vars_appearance[reason->get_var_index()] = 1;
                auto order =
                    std::make_pair(reason->get_rank(),
                                   assigned_order.at(reason->get_var_index()));
                if (order > lastest)
                {
                    lastest = order;
                    blocker = reason->get_var_index();
                }
            }
        }
    }

    Literal* sec_watcher = nullptr;

    for (auto pair : vars_appearance)
    {
        auto var = pair.first;
        Literal* lit = this->cdcl->insert_literal(
            var, this->cdcl->assignment[var].value == Value::True);

        if (lit->index == blocker)
        {
            sec_watcher = lit;
            continue;
        }

        learned_clause->literals.push_back(lit);
    }

    int fuip = queue.top()->get_var_index();
    Literal* lit = this->cdcl->insert_literal(
        fuip, this->cdcl->assignment[fuip].value == Value::True);
    learned_clause->literals.push_back(lit);

    std::swap(learned_clause->literals.back(),
              learned_clause->literals.front());

    if (sec_watcher != nullptr)
    {
        learned_clause->literals.push_back(sec_watcher);
        std::swap(learned_clause->literals.at(1),
                  learned_clause->literals.back());
    }

    return learned_clause;
}

void ImpGraph::drop_to(int rank)
{
    if (this->trail.end() - this->trail.begin() <= rank) return;
    auto trail_back = this->trail.end() - 1;
    // Memory Leak Probability Here.
    while (trail_back - this->trail.begin() >= rank)
    {
        for (auto node = (*trail_back).rbegin(); node != (*trail_back).rend();
             node++)
        {
            Assign deassign;
            deassign.value = Value::Free;
            deassign.variable_index = (*node)->get_assign()->variable_index;
            this->cdcl->update(deassign);

            this->vars_to_nodes[(*node)->get_var_index()] = nullptr;
            (*node)->drop();
        }
        trail_back--;
    }
    trail.resize(rank);
    return;
}

void ImpGraph::add_reason(ImpNode* premise, ImpNode* conclusion,
                          ClauseWrapper* reason)
{
    if (premise != nullptr)
        premise->out_node.push_back(conclusion),
            premise->out_reason.push_back(reason);
    conclusion->in_node.push_back(premise);
    conclusion->in_reason.push_back(reason);
    return;
}

void ImpGraph::drop()
{
    for (auto node : this->vars_to_nodes)
        if (node != nullptr) node->drop();
    delete this;
    return;
}

void ImpNode::drop()
{
    for (auto node : in_node)
        if (node != nullptr)
        {
            node->out_node.pop_back();
            node->out_reason.pop_back();
        }
    delete this;
    return;
}

ImpNode::ImpNode(Assign* assign, int rank, bool fixed)
{
    this->assign = assign;
    this->rank = rank;
    this->fixed = fixed;
    return;
}

int ImpNode::get_rank() { return this->rank; }

int ImpNode::get_var_index() { return this->assign->variable_index; };

Assign* ImpNode::get_assign() { return this->assign; }

const std::vector<ImpNode*>& ImpNode::get_in_node() { return this->in_node; }

void ImpRelation::drop()
{
    delete this;
    return;
}

void ClauseWrapper::debug()
{
    std::cout << "============================" << std::endl;
    std::cout << "Clause " << clause->index << ":\n";

    auto literal = this->clause->literals.begin();
    for (; literal != this->clause->literals.end(); literal++)
    {
        std::cout << "Variable " << (*literal)->index
                  << ((*literal)->is_neg ? " (Negatived) : "
                                         : "             : ");
        std::cout << to_string(cdcl->assignment[(*literal)->index].value)
                  << std::endl;
    }

    // std::cout << "Satisfied: " << (satisfied_lits_num ? "True" : "False")
    //           << std::endl;
    return;
}

void ImpNode::debug()
{
    std::cout << "Node Variable Index: " << assign->variable_index
              << "   Truth Value:" << to_string(assign->value).substr(7)
              << std::endl;
    for (int index = 0; index < out_node.size(); index++)
    {
        std::cout << " -- Clause " << out_reason.at(index)->clause->index
                  << " -> "
                  << "Variable " << out_node.at(index)->assign->variable_index
                  << std::endl;
    }
    std::cout << "Rank :" << rank << std::endl;
    return;
}

void ImpGraph::debug()
{
    for (auto node : this->vars_to_nodes)
    {
        if (node != nullptr) node->debug(), std::cout << std::endl;
    }
    std::cout << std::endl;
    return;
}

void CDCL::debug()
{
    std::cout << "============CDCL Object Debug Info=============" << std::endl;
    // std::cout << "Assignment for variables:" << std::endl;
    // for (auto assign : assignment)
    //     std::cout << "Variable " << assign.variable_index << " -> "
    //               << to_string(assign.value) << std::endl;

    std::cout << std::endl << "Learned Conflict Clauses:" << std::endl;
    for (auto c : conflict_clause) c->debug();
    std::cout << std::endl;

    std::cout << std::endl << "Implication Graph:" << std::endl;
    graph->debug();

    std::cout << "Satisfiable:" << (satisfiable ? "Yes" : "No") << std::endl
              << std::endl;
}

void CDCL::drop()
{
    if (this->graph != nullptr)
    {
        this->graph->drop();
        this->graph = nullptr;
    }

    this->satisfiable = this->solved = false;
    for (auto c : this->conflict_clause) c->drop();
    for (auto c : this->clause) c->drop();

    this->conflict_clause.clear();
    this->clause.clear();

    this->assignment.clear();
    while (!pick_stack.empty()) pick_stack.pop();
    while (!unchecked_queue.empty()) unchecked_queue.pop();
    vars.clear();
    confl = nullptr;

    if (this->cdcl_cnf != nullptr) this->cdcl_cnf->drop();
    delete this->cdcl_cnf;
    this->cdcl_cnf = nullptr;
    this->origin_cnf = nullptr;
    return;
}

void CDCL::print()
{
    if (!this->solved)
    {
        std::cout << "CDCL::solve() hasn't been run yet." << std::endl;
        return;
    }
    std::cout << std::endl;
    std::cout << "Satisfiable: " << (satisfiable ? "Yes" : "No") << std::endl;
    if (satisfiable)
    {
        std::cout << "Possible Truth Assignment:" << std::endl;
        for (auto assign = assignment.begin() + 1; assign != assignment.end();
             assign++)
        {
            std::cout << "Variable " << (*assign).variable_index << " \t-> "
                      << to_string((*assign).value)
                             .substr(((std::string) "Value::").length())
                      << std::endl;
        }
    }
}

void CDCL::print_dimacs()
{
    if (!this->solved)
    {
        std::cout << "CDCL::print_dimacs(): Error: Not solved yet."
                  << std::endl;
        return;
    }
    std::cout << "o cnf ";
    std::cout << this->origin_cnf->variable_number << " "
              << this->origin_cnf->clauses.size() << std::endl;

    for (auto c : this->origin_cnf->clauses)
    {
        for (auto l : c->literals)
        {
            if (l->is_neg) std::cout << "-";
            std::cout << l->index << " ";
        }
        std::cout << "0" << std::endl;
    }

    std::cout << "l cnf ";
    std::cout << this->origin_cnf->variable_number << " "
              << this->conflict_clause.size() << std::endl;

    for (auto c : this->conflict_clause)
    {
        for (auto l : c->clause->literals)
        {
            if (l->is_neg) std::cout << "-";
            std::cout << l->index << " ";
        }
        std::cout << "0" << std::endl;
    }

    return;
}
