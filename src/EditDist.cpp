/*
    Copyright (C) 2018 Mislav Blažević, Luka Borozan

    This file is part of Trajan.

    Trajan is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/
#include <iostream>
#include <numeric>
#include <limits>
#include <cassert>
#include <algorithm>
#include "Graph.h"
#include "read_csv.h"

typedef vector<int> vi;
typedef vector<vi> vvi;
typedef vector<vvi> vvvi;
typedef vector<vvvi> vvvvi;
typedef vector<string> vs;
typedef vector<vs> vvs;
typedef tuple<int, int, int, int> i4;

enum
{
    INS_TREE = -2,
    DEL_TREE = -1,
    MATCH = 0,
    DEL_NODE = 1,
    INS_NODE = 2,
};

int GetNumTrees(newick_node* root, vn& N)
{
    int k = 0, s = 1, f = 1;
    N.push_back(root);
    for (newick_child* child = root->child; child; child = child->next)
    {
        f *= ++k;
        s *= GetNumTrees(child->node, N);
    }
    return s * f;
}

void GenPerms(vn& v, int k, vvi& S, vvvi& P)
{
    if (k == v.size())
    {
        P.push_back(S);
        return;
    }
    int c = 0;
    for (newick_child* child = v[k]->child; child; child = child->next)
        c++;

    vector<int> I(c);
    iota(I.begin(), I.end(), 0);
    do
    {
        S.push_back(I);
        GenPerms(v, k + 1, S, P);
        S.pop_back();
    } while (next_permutation(I.begin(), I.end()));
}

int MakeTree(newick_node* root, newick_node* nroot, int k, vvi& I)
{
    vn c;
    for (newick_child* child = root->child; child; child = child->next)
        c.push_back(child->node);

    vn nc;
    newick_child** childptr = &nroot->child;
    vi J(I[k].size());
    for (int i = 0; i < I[k].size(); ++i)
    {
        newick_node* croot = new newick_node(c[I[k][i]]->taxon);
        *childptr = new newick_child(croot);
        childptr = &(*childptr)->next;
        nc.push_back(croot);
        J[I[k][i]] = i;
    }
    int s = 1;
    for (int i = 0; i < c.size(); ++i)
        s += MakeTree(c[i], nc[J[i]], k + s, I);
    return s;
}

int CalcSubtreeSizes(newick_node* root, map<newick_node*, int>& S)
{
    int s = 1;
    for (newick_child* child = root->child; child; child = child->next)
        s += CalcSubtreeSizes(child->node, S);
    return S[root] = s;
}

int Distance(vvvvi& DP, vvvvi& P, vvi& T1, vvi& T2, vvi& L1, vvi& L2, vvi& D1, vvi& D2, int n, int m, int i, int j, int k, int l, vector<vector<int>> & cost_matrix)
{
    if (i + j == n)
    {
        P[i][j][k][l] = INS_TREE;
        int cost = 0;
        for (int iter = l+1; iter <= m-k; ++iter)
            cost += cost_matrix[n][L2[k][iter]];
        return cost;
    }
    if (k + l == m)
    {
        P[i][j][k][l] = DEL_TREE;
        int cost = 0;
        for (int iter = j+1; iter <= n-i; ++iter)
            cost += cost_matrix[L1[i][iter]][m];
        return cost;
    }
    int del_i = L1[i][j+1];
    int insert_j = L2[k][l+1];
    int p = D1[i][j + 1], q = D2[k][l + 1];
    int x = n - p - T1[i][j + 1], y = m - q - T2[k][l + 1];
    int o = DP[i][j + 1][k][l] + cost_matrix[del_i][m];
    int a = DP[i][j][k][l + 1] + cost_matrix[n][insert_j];
    int b = DP[i][j + T1[i][j + 1]][k][l + T2[k][l + 1]];
    int c = DP[p + 1][x][q + 1][y];
    int d = cost_matrix[del_i][insert_j];
    int mm = min(min(o, a), b + c + d);
    if (mm == b + c + d)
        P[i][j][k][l] = MATCH;
    else if (mm == o)
        P[i][j][k][l] = DEL_NODE;
    else
        P[i][j][k][l] = INS_NODE;
    return mm;
}

newick_node* GetLastRemoved(newick_node* root, map<newick_node*, int>& R, int i, int j)
{
    list<newick_node*> D(1, root);
    int ii = 0;
    while (i--)
    {
        root = D.front();
        R[root] = ii++;
        D.pop_front();
        auto it = D.begin();
        for (newick_child* child = root->child; child; child = child->next)
            D.insert(it, child->node);
    }
    while (j--)
    {
        root = D.back();
        D.pop_back();
        for (newick_child* child = root->child; child; child = child->next)
            D.push_back(child->node);
    }
    return root;
}

void GenTables(newick_node* root, int n, vvi& L, vvi& T, vvi& D)
{
    map<newick_node*, int> S, R;
    CalcSubtreeSizes(root, S);
    for (int i = n; i >= 0; --i)
        for (int j = 0; j <= n; ++j)
            if (newick_node* node = (i + j && i + j <= n ? GetLastRemoved(root, R, i, j) : nullptr))
                L[i][j] = stoi(node->taxon), T[i][j] = S[node], D[i][j] = R[node];
}

int OrderedEditDist(vvvvi& P, vvi& L1, vvi& L2, vvi& T1, vvi& T2, vvi& D1, vvi& D2, int n, int m, vector<vector<int>> & cost_matrix)
{
    vvvvi DP(n + 1, vvvi(n + 1, vvi(m + 1, vi(m + 1, -1))));
    for (int i = n; i >= 0; --i)
        for (int j = n; j >= 0; --j)
            for (int k = m; k >= 0; --k)
                for (int l = m; l >= 0; --l)
                    if (i + j <= n && k + l <= m)
                        DP[i][j][k][l] = Distance(DP, P, T1, T2, L1, L2, D1, D2, n, m, i, j, k, l, cost_matrix);

    return DP[0][0][0][0];
}

void DeallocTree(newick_node* root)
{
    for (newick_child* child = root->child; child; child = child->next)
        DeallocTree(child->node);
    delete root;
}

void PrintMatching(vvvvi& BP, vvi& L1, vvi& L2, vvi& T1, vvi& T2, vvi& D1, vvi& D2, i4 r, bool swp, 
                   ofstream& myfile, std::map<string, string> & t1Label2Node, std::map<string, string> & t2Label2Node)
{
    int i, j, k, l;
    tie(i, j, k, l) = r;
    int n = L1.size() - 1, m = L2.size() - 1;
    switch (BP[i][j][k][l])
    {
        case INS_TREE:
            for (int nl = l + 1; nl <= m - k; ++nl)
                cout << (!swp ? "INS " : "DEL ") << L2[k][nl] << '\n';
            break;
        case DEL_TREE:
            for (int nj = j + 1; nj <= n - i; ++nj)
                cout << (!swp ? "DEL " : "INS ") << L1[i][nj] << '\n';
            break;
        case MATCH:
        {
            int xs = L1[i][j + 1], ys = L2[k][l + 1];
            int p = D1[i][j + 1], q = D2[k][l + 1];
            int x = n - p - T1[i][j + 1], y = m - q - T2[k][l + 1];
            cout << "MATCH " << (swp ? ys : xs) << ' ' << (swp ? xs : ys) << '\n';
            myfile << (swp ? t2Label2Node[std::to_string(ys)] : t1Label2Node[std::to_string(xs)]) << ' ' << (swp ? t1Label2Node[std::to_string(xs)] : t2Label2Node[std::to_string(ys)]) << '\n';
            PrintMatching(BP, L1, L2, T1, T2, D1, D2, {i, j + T1[i][j + 1], k, l + T2[k][l + 1]}, swp, myfile, t1Label2Node, t2Label2Node);
            PrintMatching(BP, L1, L2, T1, T2, D1, D2, {p + 1, x, q + 1, y}, swp, myfile, t1Label2Node, t2Label2Node);
            break;
        }
        case DEL_NODE:
            cout << (!swp ? "DEL " : "INS ") << L1[i][j + 1] << '\n';
            PrintMatching(BP, L1, L2, T1, T2, D1, D2, {i, j + 1, k, l}, swp, myfile, t1Label2Node, t2Label2Node);
            break;
        case INS_NODE:
            cout << (!swp ? "INS " : "DEL ") << L2[k][l + 1] << '\n';
            PrintMatching(BP, L1, L2, T1, T2, D1, D2, {i, j, k, l + 1}, swp, myfile, t1Label2Node, t2Label2Node);
            break;
    }
}

void EditDist(Graph& g1, Graph& g2, string & fileName, string & outSolution, std::map<string, string> & t1Label2Node, std::map<string, string> & t2Label2Node, string & outScore)
{
    vn n1, n2;
    int k1 = GetNumTrees(g1.GetRoot(), n1), k2 = GetNumTrees(g2.GetRoot(), n2);
    Tree& t1 = (Tree&)(k1 >= k2 ? g1 : g2), &t2 = (Tree&)(k1 >= k2 ? g2 : g1);
    bool swp = false;
    if (k2 > k1)
    {
        cout << "Swap two tree\n";
        swap(n1, n2), swp = true;
        std::map<string, string> temp;
        temp = t1Label2Node;
        t1Label2Node = t2Label2Node;
        t2Label2Node = temp;
    }

    // read data.
    CSVReader reader(fileName);
    vector<vector<double>> cost_matrix = reader.getDoubleData();
    vector<vector<int>> scale_cost_matrix;
    const double scale = 1000.0;
    scale_cost_matrix.resize(swp ? cost_matrix[0].size() : cost_matrix.size());
    for (int i = 0; i < (int) cost_matrix.size(); ++i)
        for (int j = 0; j < (int) cost_matrix[0].size(); ++j)
            scale_cost_matrix[swp ? j : i].push_back((int) (scale*cost_matrix[i][j]));

    vvvi P;
    vvi S;
    GenPerms(n2, 0, S, P);
    vvvvi BP;
    vvi BL1, BL2;
    vvi BT1, BT2, BD1, BD2;
    int mm = numeric_limits<int>::max();
    for (int i = 0; i < P.size(); ++i)
    {
        newick_node* nroot = new newick_node(n2[0]->taxon);
        MakeTree(n2[0], nroot, 0, P[i]);
        int n = n1.size(), m = n2.size();
        vvi L1(n + 1, vi(n + 1)), L2(m + 1, vi(m + 1));
        vvi T1(n + 1, vi(n + 1, 1)), T2(m + 1, vi(m + 1, 1));
        vvi D1(n + 1, vi(n + 1, -1)), D2(m + 1, vi(m + 1, -1));
        GenTables(t1.GetRoot(), n, L1, T1, D1);
        GenTables(nroot, m, L2, T2, D2);
        vvvvi NP(n + 1, vvvi(n + 1, vvi(m + 1, vi(m + 1, -1))));
        int nm = OrderedEditDist(NP, L1, L2, T1, T2, D1, D2, n, m, scale_cost_matrix);
        if (nm < mm)
        {
            mm = nm, BP = move(NP);
            BL1 = move(L1), BL2 = move(L2);
            BT1 = move(T1), BT2 = move(T2);
            BD1 = move(D1), BD2 = move(D2);
        }
        DeallocTree(nroot);
    }
    ofstream myfile(outSolution);
    PrintMatching(BP, BL1, BL2, BT1, BT2, BD1, BD2, {0, 0, 0, 0}, swp, myfile, t1Label2Node, t2Label2Node);
    myfile.close();
    clog << "DIST: " << (double)mm/scale << endl;
    ofstream score(outScore);
    score << (double)mm/scale << endl;
    score.close();
}
