#include "include/cdcl/cdcl.hpp"
#include "cdcl/basetype.hpp"
#include "cdcl/graph.hpp"
#include "include/cdcl/cnf.hpp"
#include <algorithm>
#include <ostream>
#include <utility>
#include <vector>
#include <iostream>
#include <queue>

void CDCL::solve(int seed)
{
    if (this->solved) return;

    srand(seed);
    this->solved = true;
    auto up_result = this->unit_propogation();
    if (up_result)
        return; // Unit propagation when no variable is picked returns conflict,
                // the CNF is not satisfiable.

    std::vector<int> luby(1, restart_r);
    auto luby_r = luby.begin();

    int confl_cnt = 0;
    do
    {
        Lit pick_lit = this->choose_variable();
        if (pick_lit == nullLit)
        {
            satisfiable = true;
            return;
        }
        Assign assign = Assign{pick_lit.get_var(),
                               pick_lit.is_neg() ? Value::False : Value::True};
        pick_stack.push(assign);

        vars_rank.at(pick_lit.get_var()) =
            this->graph.pick_var(nullCRef, pick_lit);

        this->update(assign);

        auto up_result = this->unit_propogation();
        while (up_result)
        {
            while (pick_stack.size() >= unpick_to_rank) this->pick_stack.pop();

            auto deassign = this->graph.drop_to(unpick_to_rank);
            while (!deassign.empty())
            {
                update(deassign.front());
                deassign.pop();
            }

            up_result = this->unit_propogation();

            if ((*clausedb.get_last_cls()).literals.size() == 0)
            {
                satisfiable = false;
                return;
            }

            confl_cnt++;
            if (confl_cnt >= *luby_r)
            {
                confl_cnt = 0;
                auto deassign = this->graph.drop_to(1);
                while (!deassign.empty())
                {
                    update(deassign.front());
                    deassign.pop();
                }
                while (!pick_stack.empty()) pick_stack.pop();
                while (!unchecked_queue.empty()) unchecked_queue.pop();

                luby_r++;
                if (luby_r == luby.end())
                {
                    luby.push_back(luby.back() * 2);
                    luby_r = luby.begin();
                }
                break;
            }
        }
    } while (true);

    return;
}

int CDCL::unit_propogation()
{
    Assign new_assign;
    bool conflict = false;
    int confl_siz = 0;
    unpick_to_rank = variable_number + 1;

    if (!confl.empty()) // Conflict encountered
    {
        while (!confl.empty())
        {
            Clause learned_clause =
                this->graph.conflict_clause_gen(confl.front());
            confl.pop();
            confl_siz++;

            clausedb.add_clause(learned_clause);
            analyze(clausedb.get_last_cls());
        }
        return confl_siz;
    }

    do
    {
        if (this->unchecked_queue.empty()) break;

        auto reason = unchecked_queue.front().second;
        auto unit_lit = unchecked_queue.front().first;
        unchecked_queue.pop();

        if ((*reason).literals.size() == 1)
        {
            if (assignment[unit_lit.get_var()].value != Value::Free &&
                get_lit_value(unit_lit) == Value::False)
                return true;
        }
        if (!(*reason).is_unit(unit_lit, this)) continue;

        new_assign = Assign{unit_lit.get_var(),
                            unit_lit.is_neg() ? Value::False : Value::True};

        vars_rank.at(unit_lit.get_var()) =
            this->graph.pick_var(reason, unit_lit);

        this->update(new_assign);

        if (!confl.empty()) // Conflict encountered
        {
            conflict = true;
            while (!confl.empty())
            {
                Clause learned_clause =
                    this->graph.conflict_clause_gen(confl.front());
                confl.pop();
                confl_siz++;

                clausedb.add_clause(learned_clause);
                analyze(clausedb.get_last_cls());
            }
        }

    } while (!conflict);

    return confl_siz;
}

void CDCL::update(Assign assign)
{
    assignment.at(assign.variable_index) = assign;
    if (assign.value != Value::Free)
        vars.at(assign.variable_index).set_recent_value(assign.value);
    else
        vsids.insert(assign.variable_index);
    auto res = vars.at(assign.variable_index).update_watchlist(assign);
    while (!res.empty()) confl.push(res.front()), res.pop();
    // if (res != nullCRef) confl = res;
    // return res;
}

void CDCL::analyze(CRef cls)
{
    auto& clause = *cls;

    int siz = clause.literals.size();

    if (!siz) return;
    if (siz == 1)
    {
        unchecked_queue.push(std::make_pair(clause.literals.back(), cls));
        if (clause.index >= clause_size) unpick_to_rank = 1;
        return;
    }

    auto f = clause.literals.at(0);
    auto s = clause.literals.at(1);

    vars.at(f.get_var()).watchlist_pushback(cls, f);
    vars.at(s.get_var()).watchlist_pushback(cls, s);

    if (clause.index < clause_size) return;

    for (auto lit : clause.literals)
    {
        vsids.bump_var(lit.get_var());
    }
    vsids.decay();
    unpick_to_rank = std::min(unpick_to_rank, vars_rank.at(f.get_var()));
    unchecked_queue.push(std::make_pair(f, cls));

    return;
}

Lit CDCL::choose_variable()
{
    while (!vsids.is_empty())
    {
        int v = vsids.top();
        vsids.pop();
        if (assignment[v].value != Value::Free) continue;
        if (vars.at(v).get_recent_value() == Value::Free)
            return Lit(v, rand() % 2);
        return Lit(v, vars.at(v).get_recent_value() == Value::True);
    }
    return nullLit;
}

bool CDCL::parse_clause(Clause& cls)
{
    std::vector<int> vis(variable_number + 1, 0);
    std::vector<int> parsed;
    for (auto lit : cls.get_literals())
    {
        if (vis[lit.get_var()])
        {
            if (vis[lit.get_var()] < 0 && !lit.neg() ||
                vis[lit.get_var()] > 0 && lit.neg())
                return false;
        }
        else
        {
            vis[lit.get_var()] = lit.neg() ? -1 : 1;
            parsed.push_back(vis[lit.get_var()] * lit.get_var());
        }
    }
    cls = Clause(nullptr, parsed);
    return true;
}

CDCL::CDCL(CNF& cnf)
    : variable_number(cnf.var_num()),
      clause_size(cnf.clause_size()),
      nullCRef(CRef(-1, clausedb)),
      graph(*this),
      vsids(cnf.var_num())
{
    variable_number = cnf.var_num();
    clause_size = cnf.clause_size();

    vars.reserve(variable_number + 1);

    for (int i = 0; i <= variable_number; i++)
        assignment.push_back(Assign{i, Value::Free}),
            vars.push_back(VariableWrapper(i, *this));

    this->vars_rank.resize(variable_number + 1);

    for (int c = 0; c < cnf.clause_size(); c++)
    {
        Clause cls = cnf.copy_clause(c);
        if (parse_clause(cls))
        {
            clausedb.add_clause(cls);
            analyze(clausedb.get_last_cls());
        }
    }
    clause_size = clausedb.size();

    return;
}

Value CDCL::get_lit_value(Lit l)
{
    Value v = assignment.at(l.get_var()).value;
    if (v == Value::Free) return v;
    return ((v == Value::False) == l.is_neg()) ? Value::True : Value::False;
}

int CDCL::get_variable_num() { return variable_number; }

int CDCL::get_clause_size() { return clause_size; }

void CDCL::debug()
{
    std::cout << "============CDCL Object Debug Info=============" << std::endl;
    for (int i = clause_size; i < clausedb.size(); i++)
        (*clausedb.get_clause(i)).debug();
    // std::cout << std::endl << "Learned Conflict Clauses:" << std::endl;
    // for (auto c : conflict_clause) c->debug();
    // std::cout << std::endl;

    std::cout << std::endl << "Implication Graph:" << std::endl;
    graph.debug();

    std::cout << "Satisfiable:" << (satisfiable ? "Yes" : "No") << std::endl
              << std::endl;
}

bool CDCL::is_solved() { return solved; }

bool CDCL::is_sat() { return satisfiable; }

std::vector<int> CDCL::sol()
{
    std::vector<int> sol;
    if (!is_sat()) return sol;
    for (auto a : assignment)
        sol.push_back(a.value == Value::False ? -a.variable_index
                                              : a.variable_index);
    return std::move(sol);
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
