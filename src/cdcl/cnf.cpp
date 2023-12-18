#include "include/cdcl/cnf.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

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

void DIMACS::Input()
{
    this->from_stringstream(std::cin);
    return;
}

Literal::Literal(CNF* c, int idx, bool neg) : cnf(c)
{
    index = idx;
    is_neg = neg;
};

Clause::Clause(CNF* cnf, std::string str) : cnf(cnf)
{ /* todo */
    return;
};

Clause::Clause(CNF* cnf, std::vector<int>& clause) : cnf(cnf)
{
    this->index = cnf->clauses.size();
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

CNF::CNF(DIMACS& d) { this->from_DIMACS(d); }

CNF::CNF() {}

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
