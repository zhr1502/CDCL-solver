#include <cdcl/basetype.hpp>
#include <cdcl/cdcl.hpp>
#include <iostream>

void ClauseDatabase::add_clause(Clause cls)
{
    ClauseWrapper cls_w(cls, size());
    clause.push_back(cls_w);
    return;
}

CRef ClauseDatabase::get_clause(int id) { return CRef(id, *this); }

CRef ClauseDatabase::get_last_cls() { return get_clause(size() - 1); }

int ClauseDatabase::size() { return clause.size(); }

ClauseWrapper::ClauseWrapper(Clause& raw, int idx)
{
    index = idx;
    for (auto lit : raw.get_literals())
    {
        literals.push_back(Lit(lit.get_var(), lit.neg()));
    }
    // Randomize watch literals
    return;
}

Lit ClauseWrapper::get_blocker(Variable watcher)
{
    if (literals.size() == 1) return literals[0];

    return literals.at(literals[0].get_var() == watcher);
}

Lit ClauseWrapper::update_watcher(Variable watcher, CDCL& cdcl)
{
    if (literals.size() == 1) return literals[0];

    Lit& origin = literals[literals.at(0).get_var() != watcher];

    Lit new_watcher = origin;
    bool found = 0;

    int watcher_rank = cdcl.vars_rank.at(watcher);
    for (int i = 2; i < literals.size(); i++)
    {
        auto lit = literals[i];
        if (cdcl.get_lit_value(lit) != Value::False)
        {
            new_watcher = literals[i];
            std::swap(literals[i], origin);

            found = 1;
            break;
        }
    }

    if (!found)
    {
        int max_rank = watcher_rank, pos = -1;

        for (int i = 2; i < literals.size(); i++)
        {
            auto var = literals[i].get_var();
            if (cdcl.vars_rank.at(var) > max_rank)
            {
                pos = i;
                max_rank = cdcl.vars_rank.at(var);
            }
        }

        if (pos != -1)
        {
            new_watcher = literals.at(pos);
            std::swap(literals[pos], origin);
        }
    }

    return new_watcher;
}

bool ClauseWrapper::is_unit(Lit watcher, CDCL* cdcl)
{
    if (literals.size() == 1 && cdcl->get_lit_value(watcher) == Value::Free)
        return true;

    if (literals[0] != watcher && literals[1] != watcher) return false;

    auto blocker = literals[literals[0] == watcher];

    return cdcl->get_lit_value(watcher) == Value::Free &&
           cdcl->get_lit_value(blocker) == Value::False;
}

void ClauseWrapper::debug()
{
    std::cout << "============================" << std::endl;
    std::cout << "Clause " << index << ":\n";

    auto literal = literals.begin();
    for (; literal != literals.end(); literal++)
    {
        std::cout << "Variable " << (*literal).get_var()
                  << ((*literal).is_neg() ? " (Negatived)" : "") << std::endl;
    }

    return;
}

CRef::CRef(int cid, ClauseDatabase& clausedb) : cid(cid), db(clausedb) {}

bool CRef::isnull() { return cid == -1; }

Lit::Lit(int index, bool is_neg) : lit(is_neg ? -index : index) {}

int Lit::get_var() { return lit < 0 ? -lit : lit; }

bool Lit::is_neg() { return lit < 0; }

VariableWrapper::VariableWrapper(Variable index, CDCL& owner)
    : var(index), cdcl(owner)
{
}

std::list<CRef>& VariableWrapper::get_watchlist(Assign assign)
{
    if (assign.value == Value::False) return pos_watcher;
    if (assign.value == Value::True) return neg_watcher;

    std::cout << "VariableWrapper::get_watchlist(): Assign of Free found."
              << std::endl;
    abort();
}

void VariableWrapper::watchlist_pushback(CRef clause, Lit self)
{
    if (self.is_neg())
        neg_watcher.push_back(clause);
    else
        pos_watcher.push_back(clause);

    return;
}

void VariableWrapper::set_recent_value(Value v) { recent_value = v; }

Value VariableWrapper::get_value() { return cdcl.assignment.at(var).value; }

Value VariableWrapper::get_recent_value() { return recent_value; }

CRef VariableWrapper::update_watchlist(Assign assign)
{
    CRef confl = cdcl.nullCRef;
    if (assign.value == Value::Free) return confl;

    auto& updlist = get_watchlist(assign);
    auto it = updlist.begin();

    while (it != updlist.end())
    {
        // watcher& w = *it;
        CRef clause = *it;
        Lit blocker = (*clause).get_blocker(var);
        Lit watcher = (*clause).get_blocker(blocker.get_var());

        Lit new_watch_var = (*clause).update_watcher(var, cdcl);

        if (cdcl.get_lit_value(new_watch_var) == Value::False)
        {
            if (cdcl.get_lit_value(blocker) == Value::False)
                confl = clause;
            else
                cdcl.unchecked_queue.push(std::make_pair(blocker, clause));
        }

        if (new_watch_var == watcher)
            it++;
        else
        {
            it = updlist.erase(it);

            cdcl.vars.at(new_watch_var.get_var())
                .watchlist_pushback(clause, new_watch_var);
        }
    }

    return confl;
}

std::ostream& operator<<(std::ostream& stream, const Value& v)
{
    std::string str;
    if (v == Value::False)
        str = "False";
    else if (v == Value::True)
        str = "True";
    else
        str = "Free";
    stream << str;
    return stream;
}
