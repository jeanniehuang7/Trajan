/*
    Copyright (C) 2018 Mislav Blažević

    This file is part of Hali.

    Hali is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/
#include "Solver.h"
#include "read_csv.h"
#include <iostream>
#include <fstream>

extern std::string costMatrixFileName;
extern double var_eps;

Solver::Solver(Graph& t1, Graph& t2, string d, double k, bool dag) : t1(t1), t2(t2), d(d), k(k), dag(dag)
{
    if (d == "e"){
        std::string fileName = costMatrixFileName;  // use file name from main function. 
        std::ifstream f(fileName.c_str());
        if (f.good())
        {
            CSVReader reader(costMatrixFileName);
            cost_matrix = reader.getDoubleData();
        }
        else
        {
            std::cout << "PLEASE INPUT WEIGHT(COST) MATRIX DIRECTORY:"; 
            std::cin >> fileName;
            CSVReader reader(fileName);
            cost_matrix = reader.getDoubleData();
        }
        // scale distance matrix 
        // std::cout << "Rescale distance matrix\n"; TODO: 
        int n = (int) cost_matrix.size();
        int m = (int) cost_matrix[0].size();
        scaleCoef = 1.0;
        for (int i=0; i < n; ++i)
            scaleCoef = max(scaleCoef, cost_matrix[i][0]);
        scaleCoef = min(scaleCoef, 100.0);
        for (int i=0; i < n; ++i){
            for (int j=0; j < m;++j)
            {
                cost_matrix[i][j] = cost_matrix[i][j]/scaleCoef;
            }
        }
        // std::cout << "scale coef: " << scaleCoef << m << n << endl;
    }
}

void Solver::PrintScore(double weight)
{
    if (d == "e")
    {
        double optVal2 = -weight;
        int n = (int) cost_matrix.size();
        int m = (int) cost_matrix[0].size();
        for (int i=0; i < n-1; ++i)
            optVal2 += cost_matrix[i][m-1];
        for (int j=0; j < m-1; ++j)
            optVal2 += cost_matrix[n-1][j];
        optVal2 = optVal2*scaleCoef;
        cout << ">>>>>>>>>>>>>>>>>>>>>> Optimal value:" << optVal2 << "." << endl;
        moptVal = optVal2;
//         return optVal2;
    }
    else if (dag){
        cout << weight << " ";
//         return weight;
    }
    else{
        cout << ((d == "j") ? JaccardDist(weight) : SymdifDist(weight)) << " ";
//         return ((d == "j") ? JaccardDist(weight) : SymdifDist(weight));
    }
}

int Solver::GetMax(newick_node* node, int& hmax) const
{
    int sum = 0;
    for (newick_child* child = node->child; child; child = child->next)
        sum += GetMax(child->node, hmax);
    hmax += sum;
    return node->child ? sum : 1;
}

double Solver::GetMax(Graph& t1, Graph& t2, newick_node* root, newick_node* rnode) const
{
    double m = root->parent && root->child ? SymdifSim(t1.clade[rnode], t2.clade[root]) : 0;
    for (newick_child* child = root->child; child; child = child->next)
        m = max(m, GetMax(t1, t2, child->node, rnode));
    return m;
}

double Solver::CalcY(Graph& t1, Graph& t2, newick_node* root) const
{
    double w = root->parent && root->child ? GetMax(t1, t2, t2.GetRoot(), root) : 0;
    for (newick_child* child = root->child; child; child = child->next)
        w += CalcY(t1, t2, child->node);
    return w;
}

double Solver::TumorDist(double weight) const
{
    double y = CalcY(t1, t2, t1.GetRoot()) + CalcY(t2, t1, t2.GetRoot());
    return (1 - (weight / (y - weight))) * 100;
}

double Solver::SymdifDist(double weight) const
{
    if (tt) return TumorDist(weight);
    int max1 = 0, max2 = 0;
    int r1 = GetMax(t1.GetRoot(), max1);
    int r2 = GetMax(t2.GetRoot(), max2);
    return max1 + max2 - r1 - r2 - weight;
}

double Solver::JaccardDist(double weight) const
{
    return t1.GetNumNodes() - t1.L.size() - 1 + t2.GetNumNodes() - 1 - t2.L.size() - weight;
}

bool Solver::IsNotInConflict(int i, int j, int x, int y) const
{
    if (i == j || x == y) return false;
    if (cf == 0) return true;
    bool c2 = (t1.D[j][i] || t1.D[i][j]) == (t2.D[x][y] || t2.D[y][x]);
    bool c1 = (t1.D[i][j] == t2.D[x][y]); // assuming c2 is satisfied
    if (cf == 1) return !c2 || c1;
    return c2 && c1;
}
