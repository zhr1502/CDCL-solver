#include "cdcl/graph.hpp"
#include "cdcl/cdcl.hpp"
#include "cdcl/basetype.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>

ImpGraph::ImpGraph(CDCL& cdcl)
{
    auto v = cdcl.get_variable_num();
    vars_to_nodes.resize(v + 1);
    assigned_order.resize(v + 1);
    trail.reserve(v + 1);
    trail.resize(1);
    return;
}

int ImpGraph::pick_var(CRef reason, Lit picked_lit)
{
    int rank = 0;
    Assign assign = Assign{picked_lit.get_var(),
                           picked_lit.is_neg() ? Value::False : Value::True};
    ImpNode* conclusion;

    if (reason.isnull()) // The variable is picked because UP can't be
                         // furthered, no cluase can derive it.
    {
        rank = trail.size();
        conclusion = this->add_node(assign, rank);
        this->assigned_order.at(picked_lit.get_var()) = 1;
        this->trail.push_back(std::vector<ImpNode*>(1, conclusion));
        return rank;
    }

    for (auto lit : (*reason).literals)
        if (lit != picked_lit)
        {
            rank = std::max(rank, vars_to_nodes.at(lit.get_var())->get_rank());
        }

    conclusion = this->add_node(assign, rank);

    for (auto lit : (*reason).literals)
        if (lit != picked_lit)
        {
            ImpNode* premise = this->vars_to_nodes.at(lit.get_var());
            if (premise == nullptr)
            {
                std::cout << "Nullptr Encountered !!!! " << (*reason).index
                          << std::endl;
            }
            this->add_reason(premise, conclusion, reason);
        }

    if (this->trail.size() > rank)
        this->trail.at(rank).push_back(conclusion);
    else
    {
        std::cout << "ImpGraph::pick_var: Skipped rank found" << std::endl;
        abort();
    }

    this->assigned_order.at(picked_lit.get_var()) = this->trail.at(rank).size();

    return rank;
}

ImpNode* ImpGraph::add_node(Assign assign, int rank)
{
    this->vars_to_nodes.at(assign.variable_index) =
        new ImpNode(assign, rank, rank == 0);
    return this->vars_to_nodes.at(assign.variable_index);
}

Clause ImpGraph::conflict_clause_gen(CRef conflict_clause)
{
    auto cmp = [this](ImpNode* lhs, ImpNode* rhs) -> bool {
        return assigned_order.at(lhs->get_var_index()) <
               assigned_order.at(rhs->get_var_index());
    };

    std::priority_queue<ImpNode*, std::vector<ImpNode*>, decltype(cmp)> queue(
        cmp);

    std::map<int, bool> vars_appearance;

    int decision_rank = 0;

    for (auto lit : (*conflict_clause).literals)
    {
        auto node = vars_to_nodes.at(lit.get_var());
        if (node == nullptr)
        {
            std::cout
                << "ImpGraph::conflict_clause_gen: Unassigned variable found."
                << std::endl;
            abort();
        }
        decision_rank = std::max(decision_rank, node->get_rank());
    }
    if (!decision_rank) return Clause();

    int blocker = -1;
    std::pair<int, int> lastest(0, 0);

    for (auto lit : (*conflict_clause).literals)
    {
        auto node = vars_to_nodes.at(lit.get_var());
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

    int sec_watcher = 0;

    std::vector<int> clause_raw;

    for (auto pair : vars_appearance)
    {
        auto var = pair.first;
        int lit = (vars_to_nodes.at(var)->get_assign().value == Value::True)
                      ? -var
                      : var;

        if (var == blocker)
        {
            sec_watcher = lit;
            continue;
        }

        clause_raw.push_back(lit);
    }

    int fuip = queue.top()->get_var_index();
    int lit = (vars_to_nodes.at(fuip)->get_assign().value == Value::True)
                  ? -fuip
                  : fuip;
    clause_raw.push_back(lit);

    std::swap(clause_raw.back(), clause_raw.front());

    if (sec_watcher != 0)
    {
        clause_raw.push_back(sec_watcher);
        std::swap(clause_raw.at(1), clause_raw.back());
    }

    Clause learned_clause(nullptr, clause_raw); // Create a new raw clause
    return learned_clause;
}

std::queue<Assign> ImpGraph::drop_to(int rank)
{
    std::queue<Assign> dropped;
    if (this->trail.end() - this->trail.begin() <= rank) return dropped;
    auto trail_back = this->trail.end() - 1;
    // Memory Leak Probability Here.
    while (trail_back - this->trail.begin() >= rank)
    {
        for (auto node = (*trail_back).rbegin(); node != (*trail_back).rend();
             node++)
        {
            Assign deassign;
            deassign.value = Value::Free;
            deassign.variable_index = (*node)->get_assign().variable_index;
            dropped.push(deassign);

            this->vars_to_nodes[(*node)->get_var_index()] = nullptr;

            delete (*node);
        }
        trail_back--;
    }
    trail.resize(rank);
    return std::move(dropped);
}

ImpGraph::~ImpGraph()
{
    for (auto node : this->vars_to_nodes)
        if (node != nullptr) delete node;
    return;
}

void ImpGraph::add_reason(ImpNode* premise, ImpNode* conclusion, CRef reason)
{
    if (premise != nullptr)
        premise->out_node.push_back(conclusion),
            premise->out_reason.push_back(reason);
    conclusion->in_node.push_back(premise);
    conclusion->in_reason.push_back(reason);
    return;
}

ImpNode::~ImpNode()
{
    for (auto node : in_node)
        if (node != nullptr)
        {
            node->out_node.pop_back();
            node->out_reason.pop_back();
        }
    return;
}

ImpNode::ImpNode(Assign assign, int rank, bool fixed)
{
    this->assign = assign;
    this->rank = rank;
    this->fixed = fixed;
    return;
}

int ImpNode::get_rank() { return this->rank; }

int ImpNode::get_var_index() { return this->assign.variable_index; };

Assign ImpNode::get_assign() { return this->assign; }

const std::vector<ImpNode*>& ImpNode::get_in_node() { return this->in_node; }

void ImpNode::debug()
{
    std::cout << "Node Variable Index: " << assign.variable_index
              << "   Truth Value:" << assign.value << std::endl;
    for (int index = 0; index < out_node.size(); index++)
    {
        std::cout << " -- Clause " << (*(out_reason.at(index))).index << " -> "
                  << "Variable " << out_node.at(index)->assign.variable_index
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
