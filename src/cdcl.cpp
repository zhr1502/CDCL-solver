#include "cdcl.hpp"
#include <iterator>
#include <ostream>
#include <utility>
#include <vector>
#include <iostream>
#include <queue>

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

    srand(75892669);
    this->solved = true;
    auto up_result = this->unit_propogation();
    if (up_result.second)
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
        this->graph->construct(std::pair(nullptr, pick_lit));

        auto up_result = this->unit_propogation();
        while (up_result.second)
        {
            Assign deassign;
            deassign.value = Value::Free;

            auto last_conflict_clause = *(this->conflict_clause.end() - 1);
            auto unpick_to_variable =
                (*(last_conflict_clause->clause->literals.end() - 1))->index;

            do
            {
                deassign = pick_stack.top();
                deassign.value = Value::Free;

                this->update(deassign);
                this->graph->drop_to(this->pick_stack.size());
                this->pick_stack.pop();

            } while (deassign.variable_index != unpick_to_variable);

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

std::pair<std::vector<Assign>, bool> CDCL::unit_propogation()
{
    std::vector<Assign> picked_lits;
    Assign new_picked_lit;
    bool conflict = false;

    do
    {
        bool pure_lit_exist = false;
        std::pair<ClauseWrapper*, Literal*> pure_literal_with_clause;

        for (auto c : this->conflict_clause)
        {
            Literal* pure_lit = c->find_pure_lit();
            if (pure_lit != nullptr)
            {
                new_picked_lit =
                    Assign{pure_lit->index,
                           pure_lit->is_neg ? Value::False : Value::True};
                pure_literal_with_clause = std::pair(c, pure_lit);
                pure_lit_exist = true;
                break;
            }
        }
        if (!pure_lit_exist)
            for (auto c : this->clause)
            {
                Literal* pure_lit = c->find_pure_lit();
                if (pure_lit != nullptr)
                {
                    new_picked_lit =
                        Assign{pure_lit->index,
                               pure_lit->is_neg ? Value::False : Value::True};
                    pure_literal_with_clause = std::pair(c, pure_lit);
                    pure_lit_exist = true;
                    break;
                }
            }

        if (!pure_lit_exist) break;

        picked_lits.push_back(new_picked_lit);

        auto update_result = this->update(new_picked_lit);
        this->graph->construct(pure_literal_with_clause);

        if (update_result.second) // Conflict encountered
        {
            conflict = true;

            Clause* learned_clause =
                this->graph->conflict_clause_gen(update_result.first);

            ClauseWrapper* learned_clause_wrapped =
                new ClauseWrapper(learned_clause);

            for (auto assign : this->assignment)
                learned_clause_wrapped->update(assign);

            this->conflict_clause.push_back(learned_clause_wrapped);
            this->cdcl_cnf->clauses.push_back(learned_clause);
        }

    } while (!conflict);

    return std::pair(picked_lits, conflict);
}

std::pair<ClauseWrapper*, bool> CDCL::update(Assign assign, bool do_return)
{
    this->assignment[assign.variable_index] = assign;

    for (auto c : conflict_clause)
        if (c->update(assign) && do_return)
        {
            return std::pair(c, true);
        }

    for (auto c : clause)
        if (c->update(assign) && do_return)
        {
            return std::pair(c, true);
        }

    return std::pair(nullptr, false);
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

    for (auto c : origin_cnf->clauses)
        this->clause.push_back(new ClauseWrapper(c));

    for (int i = 0; i <= cnf->variable_number; i++)
        this->assignment.push_back(Assign{i, Value::Free});

    ImpGraph* graph = new ImpGraph;
    graph->init(this);

    this->graph = graph;

    return;
}

Literal* ClauseWrapper::find_pure_lit()
{
    if (satisfied || picked_lits_number != this->clause->literals.size() - 1)
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

ClauseWrapper::ClauseWrapper(Clause* clause)
{
    this->clause = clause;
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
        this->lits_value[f->second] = assign.value;
        auto literal = this->clause->literals.begin();
        auto literal_value = this->lits_value.begin();

        satisfied = false;
        picked_lits_number = 0;

        for (; literal != this->clause->literals.end() &&
               literal_value != this->lits_value.end();
             literal_value++, literal++)
        {
            if ((*literal_value == Value::False && (*literal)->is_neg) ||
                (*literal_value == Value::True && !((*literal)->is_neg)))
                satisfied = true;

            if (*literal_value != Value::Free) picked_lits_number++;
        }
    }

    return (!satisfied && picked_lits_number == clause->literals.size());
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

    this->map_from_vars_to_nodes.resize(cdcl->origin_cnf->variable_number + 1,
                                        nullptr);
    return;
}

void ImpGraph::construct(std::pair<ClauseWrapper*, Literal*> propagated_literal)
{
    int rank = 0;
    auto& [clause, lit] = propagated_literal;
    Assign* assign = &(this->cdcl->assignment.at(lit->index));
    ImpNode* conclusion; //= this->add_node(assign, rank);

    if (clause ==
        nullptr) // The variable is randomly picked, no cluase can derive to it.
    {
        rank = this->cdcl->pick_stack.size();
        conclusion = this->add_node(assign, rank);
        return;
    }

    conclusion = new ImpNode;
    conclusion->assign = assign;
    conclusion->rank = 0;

    if (clause->clause->literals.size() == 1)
        this->add_rel(nullptr, conclusion, clause);
    else
        for (auto clause_lit : clause->clause->literals)
            if (clause_lit != lit)
            {
                auto premise =
                    this->map_from_vars_to_nodes.at(clause_lit->index);
                this->add_rel(premise, conclusion, clause);
                conclusion->rank = std::max(conclusion->rank, premise->rank);
            }

    if (conclusion->rank)
        conclusion->rank = this->cdcl->pick_stack.size(),
        this->nodes.push_back(conclusion);
    else
        this->fixed_var_nodes.push_back(conclusion);
    this->map_from_vars_to_nodes.at(conclusion->assign->variable_index) =
        conclusion;
    return;
}

ImpNode* ImpGraph::add_node(Assign* assign, int rank)
{
    ImpNode* node = new ImpNode;
    node->assign = assign;
    node->rank = rank;
    this->map_from_vars_to_nodes[assign->variable_index] = node;
    if (rank > 0)
        this->nodes.push_back(node);
    else
        this->fixed_var_nodes.push_back(node);
    return node;
}

ImpRelation* ImpGraph::add_rel(ImpNode* premise, ImpNode* conclusion,
                               ClauseWrapper* clause)
{
    ImpRelation* rel = new ImpRelation;
    rel->premise = premise;
    rel->conclusion = conclusion;
    rel->relation_clause = clause->clause;

    this->relations.push_back(rel);
    if (premise != nullptr) premise->relation.push_back(rel);
    if (conclusion != nullptr) conclusion->relation.push_back(rel);

    return rel;
}

Clause* ImpGraph::conflict_clause_gen(ClauseWrapper* conflict_clause)
{
    Clause* learned_clause = new Clause; // Create a new raw clause
    learned_clause->cnf = this->cdcl->cdcl_cnf;
    learned_clause->index = this->cdcl->conflict_clause.size();

    std::queue<ImpNode*>
        queue; // queue stores all current nodes while toposorting
    std::map<int, bool>
        variable_in_learned_clause; // It is possible to pass the same node
                                    // during the toposorting, thus it is
                                    // nesscary to use map to reduct the
                                    // them.
    for (auto lit : conflict_clause->clause->literals)
    // Go through the clause that cause the
    // conflict and push them to the queue
    {
        auto node = map_from_vars_to_nodes.at(lit->index);
        if (node == nullptr)
        {
            std::cout
                << "ImpGraph::conflict_clause_gen: Unassigned variable found."
                << std::endl;
            abort();
        }
        queue.push(node);
    }

    while (!queue.empty())
    {
        auto node = queue.front();
        queue.pop();
        bool node_has_premise = false;

        for (auto rel : node->relation)
            if (rel->conclusion == node)
            {
                if (rel->premise != nullptr) queue.push(rel->premise);
                node_has_premise = true;
            }

        if (!node_has_premise)
            variable_in_learned_clause[node->assign->variable_index] = true;
    }

    for (auto pair : variable_in_learned_clause)
    {
        auto var = pair.first;
        Literal* lit = this->cdcl->cdcl_cnf->insert_literal(
            var, this->cdcl->assignment[var].value == Value::True);

        learned_clause->literals.push_back(lit);
    }

    return learned_clause;
}

void ImpGraph::drop_to(int rank)
{
    auto rel = this->relations.end() - 1;
    for (; rel >= this->relations.begin(); rel--)
    {
        if ((*rel)->conclusion->rank < rank) break;
        if ((*rel)->premise != nullptr) (*rel)->premise->relation.pop_back();
        delete (*rel);
    }
    relations.resize(rel - relations.begin() + 1);

    auto node = this->nodes.end() - 1;
    Assign deassign;
    deassign.value = Value::Free;
    for (; node >= this->nodes.begin(); node--)
    {
        if ((*node)->rank < rank) break;
        map_from_vars_to_nodes.at((*node)->assign->variable_index) = nullptr;
        deassign.variable_index = (*node)->assign->variable_index;

        this->cdcl->update(deassign, false);

        delete (*node);
    }
    nodes.resize(node - nodes.begin() + 1);
    return;
}

void ImpGraph::drop()
{
    for (auto n : this->nodes) n->drop();
    for (auto n : this->fixed_var_nodes) n->drop();
    for (auto r : this->relations) r->drop();
    this->nodes.reserve(0);
    this->fixed_var_nodes.reserve(0);
    this->relations.reserve(0);
    this->map_from_vars_to_nodes.reserve(0);
    delete this;
    return;
}

void ImpNode::drop()
{
    this->relation.reserve(0);
    delete this;
    return;
}

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

    std::cout << "Satisfied: " << (satisfied ? "True" : "False") << std::endl;
    return;
}

void ImpGraph::debug()
{
    for (auto node : this->nodes)
    {
        std::cout << "Node Variable Index: " << node->assign->variable_index
                  << "   Truth Value:"
                  << to_string(node->assign->value).substr(7) << std::endl;
        for (auto rel : node->relation)
            if (rel->premise == node)
            {
                std::cout << " -- Caause " << rel->relation_clause->index
                          << " -> "
                          << "Variable "
                          << rel->conclusion->assign->variable_index
                          << std::endl;
            }
        std::cout << "Rank :" << node->rank << std::endl << std::endl;
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
    this->satisfiable = this->solved = false;
    for (auto c : this->conflict_clause) c->drop();
    for (auto c : this->clause) c->drop();

    this->conflict_clause.clear();
    this->clause.clear();

    if (this->graph != nullptr)
    {
        this->graph->drop();
        this->graph = nullptr;
    }

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
