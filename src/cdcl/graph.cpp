#include "cdcl/graph.hpp"
#include "cdcl/cdcl.hpp"
#include "cdcl/basetype.hpp"
#include <iostream>
#include <vector>

void ImpGraph::init(CDCL* cdcl)
{
    this->cdcl = cdcl;

    this->vars_to_nodes.resize(cdcl->variable_number + 1);
    this->assigned_order.resize(cdcl->variable_number + 1);
    this->trail.resize(1);
    return;
}

void ImpGraph::pick_var(CRef reason, Lit picked_lit)
{
    int rank = 0;
    Assign assign = this->cdcl->assignment.at(picked_lit.get_var());
    ImpNode* conclusion; //= this->add_node(assign, rank);

    if (reason.isnull()) // The variable is picked because UP can't be
                         // furthered, no cluase can derive it.
    {
        rank = this->cdcl->pick_stack.size();
        conclusion = this->add_node(assign, rank);
        this->cdcl->vars_rank.at(picked_lit.get_var()) = rank;
        this->assigned_order.at(picked_lit.get_var()) = 1;
        this->trail.push_back(std::vector<ImpNode*>(1, conclusion));
        return;
    }

    for (auto lit : (*reason).literals)
        if (lit != picked_lit)
        {
            rank = std::max(rank, cdcl->vars_rank.at(lit.get_var()));
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

    this->cdcl->vars_rank.at(picked_lit.get_var()) = rank;
    if (this->trail.size() > rank)
        this->trail.at(rank).push_back(conclusion);
    else
    {
        std::cout << "ImpGraph::pick_var: Skipped rank found" << std::endl;
        abort();
    }

    this->assigned_order.at(picked_lit.get_var()) = this->trail.at(rank).size();

    return;
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
        int lit = (cdcl->assignment.at(var).value == Value::True) ? -var : var;

        if (var == blocker)
        {
            sec_watcher = lit;
            continue;
        }

        clause_raw.push_back(lit);
    }

    int fuip = queue.top()->get_var_index();
    int lit = (cdcl->assignment.at(fuip).value == Value::True) ? -fuip : fuip;
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
            deassign.variable_index = (*node)->get_assign().variable_index;
            this->cdcl->update(deassign);

            this->vars_to_nodes[(*node)->get_var_index()] = nullptr;
            (*node)->drop();
        }
        trail_back--;
    }
    trail.resize(rank);
    return;
}

void ImpGraph::drop() {
    for(auto node: this->vars_to_nodes)
        if(node != nullptr) node->drop();
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

void ImpRelation::drop()
{
    delete this;
    return;
}

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
