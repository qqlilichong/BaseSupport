
#pragma once

#ifdef W2STL_ALL
#define W2STL_STRING
#define W2STL_VECTOR
#define W2STL_LIST
#define W2STL_MAP
#define W2STL_SSTREAM
#define W2STL_ALGORITHM
#endif

#ifdef W2STL_STRING
#include <string>
#endif

#ifdef W2STL_VECTOR
#include <vector>
#endif

#ifdef W2STL_LIST
#include <list>
#endif

#ifdef W2STL_MAP
#include <map>
#endif

#ifdef W2STL_ALGORITHM
#include <algorithm>
#endif

#ifdef W2STL_SSTREAM
#include <sstream>
#endif

#include <memory>

using namespace std ;
