#include "cdcl.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <ostream>
#include <ratio>
#include <utility>
#include <vector>
#include <iostream>
#include <queue>
#include <chrono>

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
    auto up_result = this->unit_propogation();
    if (up_result)
        return; // Unit propagation when no variable is picked returns conflict,
                // the CNF is not satisfiable.
    do
    {
        Literal* pick_lit = this->choose_variable();
        if (pick_lit == nullptr)
        {
            satisfiable = true;
            return;
        }
        Assign assign =
            Assign{pick_lit->index, rand() % 2 ? Value::False : Value::True};
        pick_stack.push(assign);

        this->update(assign);
        this->graph->pick_var(nullptr, pick_lit);

        auto up_result = this->unit_propogation();
        // this->debug();
        while (up_result)
        {
            Assign deassign;

            auto last_conflict_clause = *(this->conflict_clause.end() - 1);
            auto conflict_clause_last_lit =
                *(last_conflict_clause->clause->literals.end() - 1);

            int unpick_to_rank;

            if (last_conflict_clause->clause->literals.size() == 1)
                unpick_to_rank = 1;
            else
                unpick_to_rank = vars_rank.at(conflict_clause_last_lit->index);

            while (pick_stack.size() >= unpick_to_rank)
            {
                deassign = pick_stack.top();
                this->pick_stack.pop();
            }

            this->graph->drop_to(unpick_to_rank);

            up_result = this->unit_propogation();

            if ((*(this->conflict_clause.end() - 1))->clause->literals.size() ==
                0)
            {
                satisfiable = false;
                return;
            }
        }
    } while (true);

    return;
}

bool CDCL::unit_propogation()
{
    std::vector<Assign> picked_lits;
    Assign new_picked_lit;
    bool conflict = false;

    do
    {
        if (this->pickable_clause.empty()) break;

        auto reason = *pickable_clause.begin();
        auto pure_lit = reason->find_pure_lit();
        if (pure_lit == nullptr)
        {
            std::cout << "cdcl.cpp: CDCL::unit_propogation(): Should be able "
                         "to find a pure lit but failed."
                      << std::endl;
            abort();
        }

        new_picked_lit = Assign{pure_lit->index,
                                pure_lit->is_neg ? Value::False : Value::True};

        picked_lits.push_back(new_picked_lit);

        auto update_result = this->update(new_picked_lit);
        this->graph->pick_var(reason, pure_lit);
        // this->debug();

        if (update_result.second) // Conflict encountered
        {
            conflict = true;

            Clause* learned_clause =
                this->graph->conflict_clause_gen(update_result.first);

            ClauseWrapper* learned_clause_wrapped =
                new ClauseWrapper(learned_clause, this);

            this->add_clause(learned_clause_wrapped, this->conflict_clause);
            this->cdcl_cnf->clauses.push_back(learned_clause);
        }

    } while (!conflict);

    return conflict;
}

std::pair<ClauseWrapper*, bool> CDCL::update(Assign assign, bool do_return)
{
    this->assignment[assign.variable_index] = assign;

    for (auto c : this->vars_contained_clause.at(assign.variable_index))
        if (c->update(assign) && do_return) return std::pair(c, true);

    return std::pair(nullptr, false);
}

void CDCL::add_clause(ClauseWrapper* clause,
                      std::vector<ClauseWrapper*>& clause_vec)
{
    clause_vec.push_back(clause);
    auto pos = clause->lits_value.begin();
    for (auto l : clause->clause->literals)
    {
        this->vars_contained_clause.at(l->index).push_back(clause);
        auto value = assignment.at(l->index).value;
        if (value != Value::Free)
        {
            clause->picked_lits_number++;
            clause->satisfied_lits_num +=
                ((value == Value::True && !l->is_neg) ||
                 (value == Value::False && l->is_neg));
        }
        (*pos) = value;
        pos++;
    }
    return;
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

    return this->cdcl_cnf->insert_literal(variable, false);
}

void CDCL::init(CNF* cnf)
{
    this->drop();
    this->origin_cnf = cnf;

    CNF* conflict_cnf = new CNF;
    conflict_cnf->variable_number = cnf->variable_number;
    this->cdcl_cnf = conflict_cnf;

    for (int i = 0; i <= cnf->variable_number; i++)
        this->assignment.push_back(Assign{i, Value::Free});

    this->vars_contained_clause.resize(this->origin_cnf->variable_number + 1);
    this->vars_rank.resize(this->origin_cnf->variable_number + 1);

    for (auto c : origin_cnf->clauses)
        this->add_clause(new ClauseWrapper(c, this), this->clause);

    ImpGraph* graph = new ImpGraph;
    graph->init(this);

    this->graph = graph;

    return;
}

Literal* ClauseWrapper::find_pure_lit()
{
    if (satisfied_lits_num ||
        picked_lits_number != this->clause->literals.size() - 1)
        return nullptr;
    int index = 0;
    for (auto lits = this->lits_value.begin(); lits != this->lits_value.end();
         lits++, index++)
        if (*lits == Value::Free)
        {
            return this->clause->literals[index];
        }
    std::cout
        << "ClauseWrapper::find_pure_lit(): should be able to find a pure "
           "literal but failed"
        << std::endl;
    abort();
}

ClauseWrapper::ClauseWrapper(Clause* clause, CDCL* cdcl)
{
    this->clause = clause;
    this->cdcl = cdcl;
    this->lits_value.resize(this->clause->literals.size());

    int index = 0;
    for (auto c : this->clause->literals) lits_pos[c->index] = index++;

    return;
}

bool ClauseWrapper::update(Assign assign)
{
    auto f = this->lits_pos.find(assign.variable_index);
    if (f != this->lits_pos.end())
    {
        bool pickable_before =
            (!satisfied_lits_num &&
             picked_lits_number == clause->literals.size() - 1);
        auto id = f->second;
        auto origin_value = this->lits_value.at(id);
        if (origin_value != assign.value)
        {
            if (origin_value == Value::Free && assign.value != Value::Free)
                picked_lits_number++;
            else if (origin_value != Value::Free && assign.value == Value::Free)
                picked_lits_number--;

            bool is_neg = this->clause->literals.at(id)->is_neg;

            int is_satisfied_before =
                ((origin_value == Value::False && is_neg) ||
                 (origin_value == Value::True && !is_neg));
            int is_satisfied_after =
                ((assign.value == Value::False && is_neg) ||
                 (assign.value == Value::True && !is_neg));

            satisfied_lits_num += is_satisfied_after - is_satisfied_before;
        }
        this->lits_value.at(id) = assign.value;
        bool pickable_after =
            (!satisfied_lits_num &&
             picked_lits_number == clause->literals.size() - 1);

        if (pickable_after && !pickable_before)
            this->cdcl->pickable_clause.insert(this);
        if (!pickable_after && pickable_before)
            this->cdcl->pickable_clause.erase(this);
    }

    return (!satisfied_lits_num &&
            picked_lits_number == clause->literals.size());
}

void ClauseWrapper::drop()
{
    lits_value.reserve(0);
    lits_pos.clear();
    delete this;
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
    {
        if (lit != picked_lit)
        {
            rank = std::max(rank, this->cdcl->vars_rank.at(lit->index));
        }
    }
    conclusion = this->add_node(assign, rank);

    for (auto lit : reason->clause->literals)
        if (lit != picked_lit)
        {
            ImpNode* premise = this->vars_to_nodes.at(lit->index);
            this->add_reason(premise, conclusion, reason);
        }

    this->cdcl->vars_rank.at(picked_lit->index) = rank;
    if (this->trail.size() > rank)
        this->trail.at(rank).push_back(conclusion);
    else
    {
        {
            std::cout << "ImpGraph::pick_var: Skipped rank found" << std::endl;
            abort();
        }
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

    for (auto lit : conflict_clause->clause->literals)
    {
        auto node = vars_to_nodes.at(lit->index);
        if (node->get_rank() == decision_rank)
        {
            queue.push(node);
        }
        else
            vars_appearance[node->get_var_index()] = 1;
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
                vars_appearance[reason->get_var_index()] = 1;
        }
    }

    for (auto pair : vars_appearance)
    {
        auto var = pair.first;
        Literal* lit = this->cdcl->cdcl_cnf->insert_literal(
            var, this->cdcl->assignment[var].value == Value::True);

        learned_clause->literals.push_back(lit);
    }

    int fuip = queue.top()->get_var_index();
    Literal* lit = this->cdcl->cdcl_cnf->insert_literal(
        fuip, this->cdcl->assignment[fuip].value == Value::True);
    learned_clause->literals.push_back(lit);

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
    auto literal_value = this->lits_value.begin();

    for (; literal != this->clause->literals.end() &&
           literal_value != this->lits_value.end();
         literal_value++, literal++)
    {
        std::cout << "Variable " << (*literal)->index
                  << ((*literal)->is_neg ? " (Negatived) : "
                                         : "             : ");
        std::cout << to_string(*literal_value) << std::endl;
    }

    std::cout << "Satisfied: " << (satisfied_lits_num ? "True" : "False")
              << std::endl;
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
    std::cout << "Assignment for variables:" << std::endl;
    for (auto assign : assignment)
        std::cout << "Variable " << assign.variable_index << " -> "
                  << to_string(assign.value) << std::endl;

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

    this->vars_contained_clause.clear();
    this->pickable_clause.clear();

    this->conflict_clause.clear();
    this->clause.clear();

    this->assignment.clear();
    while (!pick_stack.empty()) pick_stack.pop();

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
