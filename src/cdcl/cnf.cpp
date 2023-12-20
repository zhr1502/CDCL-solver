#include "include/cdcl/cnf.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void DIMACS::from_stringstream(std::istream& istream)
{
    this->header.clear();
    this->clauses.clear();

    std::getline(istream, this->header);

    std::istringstream s_header(this->header);
    std::string s;
    std::vector<std::string> header_args;

    for (auto i = 1; std::getline(s_header, s, ' '); i++)
        header_args.push_back(s);

    // this->? = header_args[0];  //todo
    this->form = header_args[1];
    this->literal_num = stoi(header_args[2]);
    this->clause_num = stoi(header_args[3]);

    for (auto i = 1; i <= this->clause_num; i++)
    {
        this->clauses.push_back(std::vector<int>());

        std::string clause_i;
        std::getline(istream, clause_i);

        std::istringstream s_clause(clause_i);

        while (std::getline(s_clause, s, ' '))
        {
            if (s == "0") break;

            (this->clauses.end() - 1)->push_back(stoi(s));
        }
    }
    return;
}

void DIMACS::input()
{
    this->from_stringstream(std::cin);
    return;
}

Literal::Literal(CNF* c, int idx, bool neg) : cnf(c)
{
    index = idx;
    is_neg = neg;
};

int Literal::get_var() { return index; }

bool Literal::neg() { return is_neg; }

Clause::Clause(CNF* cnf, std::string str) : cnf(cnf)
{
    std::istringstream s_clause(str);
    std::string s;

    while (std::getline(s_clause, s, ' '))
    {
        if (s == "0") break;

        int lit = stoi(s);

        literals.push_back(Literal(cnf, abs(lit), lit < 0));
    }
    return;
};

Clause::Clause(CNF* cnf, std::vector<int>& clause) : cnf(cnf)
{
    if (cnf != nullptr)
        this->index = cnf->clause_size();
    else
        this->index = 0;
    this->cnf = cnf;

    for (auto iter = clause.begin(); iter != clause.end(); iter++)
    {
        // auto pos = cnf.insert_literal(abs(*iter), *iter < 0);
        Literal pos(cnf, abs(*iter), *iter < 0);

        this->literals.push_back(pos);
    }

    return;
}

Clause::Clause() : cnf(nullptr) {}

Clause::Clause(const Clause& cls) : cnf(cls.cnf), index(cls.index)
{
    literals = cls.literals;
    return;
}

std::vector<Literal>& Clause::get_literals() { return literals; }

void CNF::from_DIMACS(DIMACS& src)
{
    clauses.resize(0);
    for (auto iter = src.clauses.begin(); iter != src.clauses.end(); iter++)

    {
        Clause clause(this, (*iter));

        this->clauses.push_back(clause);
    }

    this->variable_number = src.literal_num;

    return;
};

int CNF::clause_size() { return clauses.size(); }

CNF::CNF(DIMACS& d) { this->from_DIMACS(d); }

CNF::CNF() {}

int CNF::var_num() { return variable_number; }

void CNF::push_back(Clause&& cls)
{
    clauses.push_back(cls);
    return;
}

int CNF::size_of_clause(int idx)
{
    return clauses.at(idx).get_literals().size();
}

Clause CNF::copy_clause(int idx)
{
    Clause cpy(clauses.at(idx));
    return cpy;
}

Literal CNF::locate(int cls, int lit)
{
    return clauses.at(cls).get_literals().at(lit);
}

void Literal::debug()
{
    std::cout << "Index:" << this->index
              << " Negatived:" << (this->is_neg ? "true" : "false")
              << std::endl;
    return;
}

void Clause::debug()
{
    std::cout << "=============================================" << std::endl;
    std::cout << "Clauses: " << this->index << " With Literals:" << std::endl;
    for (auto& lit : this->literals)
    {
        lit.debug();
    }
    return;
}

void CNF::debug()
{
    std::cout << "CNF debug info:" << std::endl;
    for (auto clause : this->clauses) clause.debug();
    return;
}
