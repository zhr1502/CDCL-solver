#include "cnf.hpp"
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

void DIMACS::Input() {
    this->from_stringstream(std::cin);
    return;
}

Literal::Literal(CNF* cnf, int index, bool is_neg)
{
    this->cnf = cnf;
    this->index = index;
    this->is_neg = is_neg;
};

void Clause::from_str(CNF* cnf, std::string str)
{ /* todo */
    return;
};

void Clause::from_vec(CNF* cnf, std::vector<int>* clause)
{
    this->index = cnf->clauses.size();
    this->cnf = cnf;

    for (auto iter = clause->begin(); iter != clause->end(); iter++)
    {
        auto pos = cnf->insert_literal(abs(*iter), *iter < 0);

        this->literals.push_back(pos);
    }

    return;
}

void CNF::from_DIMACS(DIMACS* src)
{
    this->drop();
    for (auto iter = src->clauses.begin(); iter != src->clauses.end(); iter++)

    {
        Clause* clause = new Clause;

        clause->from_vec(this, &(*iter));
        this->clauses.push_back(clause);
    }

    this->variable_number = src->literal_num;

    return;
};

Literal* CNF::insert_literal(int index, bool is_neg)
{
    auto pos = find_if(this->literals.begin(), this->literals.end(),
                       [index, is_neg](Literal* lit) {
                           return lit->index == index && lit->is_neg == is_neg;
                       });

    if (pos == this->literals.end()) // the given literal doesn't exist in CNF
    {
        auto lit = new Literal(this, index, is_neg);
        this->literals.push_back(lit);

        return *(this->literals.end() - 1);
    }
    else // the given literal already exists in CNF
    {
        return *pos;
    }
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
    for (auto lit : this->literals)
    {
        lit->debug();
    }
    return;
}

void CNF::debug()
{
    std::cout << "CNF debug info:" << std::endl;
    for (auto clause : this->clauses) clause->debug();
    return;
}

void CNF::drop() {
    for(auto c : this->clauses) c->drop();
    for(auto l : this->literals) l->drop();
    this->clauses.clear();
    this->literals.clear();
    this->clauses.reserve(0);
    this->literals.reserve(0);
    variable_number = 0;
    return;
}

void Literal::drop() {
    delete this;
    return;
}

void Clause::drop() {
    this->literals.reserve(0);
    delete this;
    return;
}
