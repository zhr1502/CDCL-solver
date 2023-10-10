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
    srand(758926699);

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
            break;
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
            auto last_conflict_clause = *(this->conflict_clause.end() - 1);
            auto unpick_to_variable = 0;
            if (last_conflict_clause->clause->literals.size() >= 2)
                unpick_to_variable =
                    (*(last_conflict_clause->clause->literals.end() - 2))
                        ->index;
            do
            {
                deassign = pick_stack.top();
                deassign.value = Value::Free;
                this->update(deassign);

                this->graph->drop_to(this->pick_stack.size());

                this->pick_stack.pop();
            } while (deassign.variable_index != unpick_to_variable);

            // std::cout << "===A Conflict Clause has been generated==="
            //           << std::endl;

            // this->debug();
            // last_conflict_clause->debug();

            up_result = this->unit_propogation();

            if ((*(this->conflict_clause.end() - 1))->clause->literals.size() ==
                0)
                break;
        }
        // this->debug();

    } while (!pick_stack.empty());

    satisfiable = true;
    for (auto c : clause) satisfiable &= c->satisfied;

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
        }

    } while (!conflict);

    return std::pair(picked_lits, conflict);
}

std::pair<ClauseWrapper*, bool> CDCL::update(Assign assign)
{
    this->assignment[assign.variable_index] = assign;
    // std::cout << "CDCL::update(): The value of assign is "
    //           << assign.variable_index << " and " << to_string(assign.value)
    //           << std::endl;

    for (auto c : conflict_clause)
        if (c->update(assign))
        {
            return std::pair(c, true);
        }

    for (auto c : clause)
        if (c->update(assign))
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
    this->origin_cnf = cnf;

    CNF* conflict_cnf = new CNF;
    conflict_cnf->variable_number = cnf->variable_number;
    this->cdcl_cnf = conflict_cnf;

    for (auto c : origin_cnf->clauses)
        this->clause.push_back(new ClauseWrapper(c));

    for (int i = 0; i <= cnf->variable_number; i++)
        this->assignment.push_back(Assign{i, Value::Free});

    ImplGraph* graph = new ImplGraph;
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

void ImplGraph::init(CDCL* cdcl)
{
    this->cdcl = cdcl;
    ImplNode* null_node = new ImplNode;
    null_node->assign = &cdcl->assignment[0];
    null_node->rank = -1;

    this->map_from_vars_to_nodes.resize(cdcl->origin_cnf->variable_number + 1,
                                        nullptr);
    // for (int i = 0; i <= this->cdcl->origin_cnf->variable_number; i++)
    //     this->nodes.push_back(null_node);
    // this->nodes.push_back(null_node);
    return;
}

void ImplGraph::construct(
    std::pair<ClauseWrapper*, Literal*> propagated_literal)
{
    int rank = this->cdcl->pick_stack.size();

    auto& [clause, lit] = propagated_literal;
    Assign* assign = &(this->cdcl->assignment.at(lit->index));
    ImplNode* conclusion = this->add_node(assign, rank);

    if (clause != nullptr)
    {
        if (clause->clause->literals.size() == 1)
        {
            ImplRelation* relation = this->add_rel(nullptr, conclusion, clause);
        }
        else
            for (auto clause_lit : clause->clause->literals)
                if (clause_lit != lit)
                {
                    ImplRelation* relation = this->add_rel(
                        this->map_from_vars_to_nodes.at(clause_lit->index),
                        conclusion, clause);
                }
    }
    return;
}

ImplNode* ImplGraph::add_node(Assign* assign, int rank)
{
    ImplNode* node = new ImplNode;
    node->assign = assign;
    node->rank = rank;
    this->map_from_vars_to_nodes[assign->variable_index] = node;
    this->nodes.push_back(node);
    return node;
}

ImplRelation* ImplGraph::add_rel(ImplNode* premise, ImplNode* conclusion,
                                 ClauseWrapper* clause)
{
    ImplRelation* rel = new ImplRelation;
    rel->premise = premise;
    rel->conclusion = conclusion;
    rel->relation_clause = clause->clause;

    this->relations.push_back(rel);
    if (premise != nullptr) premise->relation.push_back(rel);
    if (conclusion != nullptr) conclusion->relation.push_back(rel);

    return rel;
}

Clause* ImplGraph::conflict_clause_gen(ClauseWrapper* conflict_clause)
{
    Clause* learned_clause = new Clause;
    learned_clause->cnf = this->cdcl->cdcl_cnf;
    learned_clause->index = this->cdcl->conflict_clause.size();

    std::queue<ImplNode*> queue;
    std::map<int, bool> variable_in_learned_clause;
    for (auto lit : conflict_clause->clause->literals)
    {
        auto node = map_from_vars_to_nodes.at(lit->index);
        if (node == nullptr)
        {
            std::cout
                << "ImplGraph::conflict_clause_gen: Unassigned variable found."
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

void ImplGraph::drop_to(int rank)
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
    Assign deassign = Assign{0, Value::Free};
    for (; node >= this->nodes.begin(); node--)
    {
        if ((*node)->rank < rank) break;
        map_from_vars_to_nodes.at((*node)->assign->variable_index) = nullptr;
        deassign.variable_index = (*node)->assign->variable_index;

        this->cdcl->update(deassign);

        delete (*node);
    }
    nodes.resize(node - nodes.begin() + 1);
    return;
}

void ImplGraph::drop()
{
    this->drop_to(-1);
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

void ImplGraph::debug()
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

    // std::cout << std::endl << "Clauses:" << std::endl;
    // for (auto c : clause) c->debug();
    // std::cout << std::endl;

    std::cout << std::endl << "Learned Conflict Clauses:" << std::endl;
    for (auto c : conflict_clause) c->debug();
    std::cout << std::endl;

    std::cout << std::endl << "Imply Graph:" << std::endl;
    graph->debug();

    std::cout << "Satisfiable:" << (satisfiable ? "Yes" : "No") << std::endl
              << std::endl;
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
    std::cout << "Possible Truth Assignment:" << std::endl;
    if (satisfiable)
        for (auto assign = assignment.begin() + 1; assign != assignment.end();
             assign++)
        {
            std::cout << "Variable " << (*assign).variable_index << " \t-> "
                      << to_string((*assign).value)
                             .substr(((std::string) "Value::").length())
                      << std::endl;
        }
    // std::cout << (satisfiable ? "Yes" : "No") << std::endl;
    // if (satisfiable)
    //     for (auto assign = assignment.begin() + 1; assign !=
    //     assignment.end();
    //          assign++)
    //     {
    //         std::cout << ((*assign).value == Value::True) << " ";
    //     }
}
