#pragma once

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#define PI 3.1415926535897932384626433832795

using namespace std;

typedef float Scalar;

bool eq(const Scalar &a, Scalar const &b);
Scalar min(const Scalar &a, const Scalar &b);
int wrap(const int &a, const int &b);
Scalar srand(const Scalar &min, const Scalar &max);

template <class T> T &at(vector<T> v, int i) { return v[wrap(i, v.size())]; };
