#include "include/cdcl/cdcl.hpp"
#include "include/cdcl/cnf.hpp"
#include <algorithm>
#include <ostream>
#include <vector>
#include <iostream>
#include <queue>

void CDCL::solve()
{
    if (this->solved) return;

    srand(758926699);
    this->solved = true;
    auto up_result = this->unit_propogation();
    if (up_result)
        return; // Unit propagation when no variable is picked returns conflict,
                // the CNF is not satisfiable.
                //
    int confl_cnt = 0, pick_cnt = 0;
    do
    {
        Lit pick_lit = this->choose_variable();
        if (pick_lit == nullLit)
        {
            satisfiable = true;
            return;
        }
        Assign assign =
            Assign{pick_lit.get_var(), rand() % 2 ? Value::False : Value::True};
        pick_stack.push(assign);
        pick_cnt++;

        this->graph->pick_var(nullCRef, pick_lit);

        this->update(assign);

        auto up_result = this->unit_propogation();
        while (up_result)
        {
            Assign deassign;

            auto last_conflict_clause_ref = clausedb.get_last_cls();
            auto last_conflict_clause = *last_conflict_clause_ref;

            Lit conflict_fuip = last_conflict_clause.literals.at(0);

            int unpick_to_rank;

            if (last_conflict_clause.literals.size() == 1)
                unpick_to_rank = 1;
            else
                unpick_to_rank = vars_rank.at(conflict_fuip.get_var());

            while (pick_stack.size() >= unpick_to_rank)
            {
                deassign = pick_stack.top();
                this->pick_stack.pop();
            }

            this->graph->drop_to(unpick_to_rank);

            unchecked_queue.push(
                std::make_pair(conflict_fuip, last_conflict_clause_ref));

            confl = nullCRef;

            up_result = this->unit_propogation();

            if ((*clausedb.get_last_cls()).literals.size() == 0)
            {
                satisfiable = false;
                return;
            }
            confl_cnt++;
        }
        if (confl_cnt > 300 && (double)pick_cnt / confl_cnt > 0.97)
        {
            confl_cnt = 0;
            pick_cnt = 0;
            this->graph->drop_to(1);
            while (!pick_stack.empty()) pick_stack.pop();
            while (!unchecked_queue.empty()) unchecked_queue.pop();
            for (int i = 1; i <= variable_number; i++) vars_rank[i] = 0;
        }
    } while (true);

    return;
}

bool CDCL::unit_propogation()
{
    Assign new_assign;
    bool conflict = false;

    if (confl != nullCRef) // Conflict encountered
    {
        Clause learned_clause = this->graph->conflict_clause_gen(confl);

        clausedb.add_clause(learned_clause);
        analyze(clausedb.get_last_cls());
        return true;
    }

    do
    {
        if (this->unchecked_queue.empty()) break;

        auto reason = unchecked_queue.front().second;
        auto unit_lit = unchecked_queue.front().first;
        unchecked_queue.pop();

        if (!(*reason).is_unit(unit_lit, this)) continue;

        new_assign = Assign{unit_lit.get_var(),
                            unit_lit.is_neg() ? Value::False : Value::True};

        this->graph->pick_var(reason, unit_lit);

        auto confl = this->update(new_assign);

        if (confl != nullCRef) // Conflict encountered
        {
            conflict = true;

            Clause learned_clause = this->graph->conflict_clause_gen(confl);

            clausedb.add_clause(learned_clause);
            analyze(clausedb.get_last_cls());
        }

    } while (!conflict);

    return conflict;
}

CRef CDCL::update(Assign assign)
{
    assignment.at(assign.variable_index) = assign;
    auto res = vars.at(assign.variable_index).update_watchlist(assign);
    if (res != nullCRef) confl = res;
    return res;
}

void CDCL::analyze(CRef cls)
{
    auto& clause = *cls;

    int siz = clause.literals.size();

    if (!siz) return;
    if (siz == 1)
    {
        unchecked_queue.push(std::make_pair(clause.literals.back(), cls));
        return;
    }

    auto f = clause.literals.at(0);
    auto s = clause.literals.at(1);

    vars.at(f.get_var()).watchlist_pushback(cls, f);
    vars.at(s.get_var()).watchlist_pushback(cls, s);
    return;
}

Lit CDCL::choose_variable()
{
    int variable = 0;
    for (int i = 1; i <= variable_number; i++)
        if (this->assignment[i].value == Value::Free)
        {
            variable = i;
            break;
        }
    if (!variable) return nullLit;

    return Lit(variable, rand() % 2);
}

CDCL::CDCL(CNF& cnf)
    : clausedb(cnf), confl(CRef(-1, clausedb)), nullCRef(CRef(-1, clausedb))
{
    variable_number = cnf.var_num();
    clause_size = cnf.clause_size();

    vars.reserve(variable_number + 1);

    for (int i = 0; i <= variable_number; i++)
        assignment.push_back(Assign{i, Value::Free}),
            vars.push_back(VariableWrapper(i, *this));

    this->vars_rank.resize(variable_number + 1);

    for (int c = 0; c < cnf.clause_size(); c++)
        clausedb.add_clause(cnf.copy_clause(c)),
            analyze(clausedb.get_last_cls());

    ImpGraph* graph = new ImpGraph;
    graph->init(this);

    this->graph = graph;

    return;
}

CDCL::~CDCL() { graph->drop(); }

Value CDCL::get_lit_value(Lit l)
{
    Value v = assignment.at(l.get_var()).value;
    if (v == Value::Free) return v;
    return ((v == Value::False) == l.is_neg()) ? Value::True : Value::False;
}

void CDCL::debug()
{
    std::cout << "============CDCL Object Debug Info=============" << std::endl;
    // std::cout << std::endl << "Learned Conflict Clauses:" << std::endl;
    // for (auto c : conflict_clause) c->debug();
    // std::cout << std::endl;

    std::cout << std::endl << "Implication Graph:" << std::endl;
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
    if (satisfiable)
    {
        std::cout << "Possible Truth Assignment:" << std::endl;
        for (auto assign = assignment.begin() + 1; assign != assignment.end();
             assign++)
        {
            std::cout << "Variable " << (*assign).variable_index << " \t-> "
                      << (*assign).value << std::endl;
        }
    }
}

void CDCL::print_dimacs() { stream_dimacs(std::cout, std::cout); }

void CDCL::stream_dimacs(std::ostream& origin, std::ostream& conflict)
{
    if (!this->solved)
    {
        std::cout << "CDCL::stream_dimacs(): Error: Not solved yet."
                  << std::endl;
        return;
    }
    origin << "o cnf ";
    origin << variable_number << " " << clause_size << std::endl;

    for (int i = 0; i < clause_size; i++)
    {
        for (auto l : (*clausedb.get_clause(i)).literals)
        {
            if (l.is_neg()) origin << "-";
            origin << l.get_var() << " ";
        }
        origin << "0" << std::endl;
    }

    conflict << "l cnf ";
    conflict << variable_number << " " << clausedb.size() - clause_size
             << std::endl;

    for (int i = clause_size; i < clausedb.size(); i++)
    {
        for (auto l : (*clausedb.get_clause(i)).literals)
        {
            if (l.is_neg()) conflict << "-";
            conflict << l.get_var() << " ";
        }
        conflict << "0" << std::endl;
    }

    return;
}
