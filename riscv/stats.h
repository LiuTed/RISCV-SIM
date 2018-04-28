#ifndef STATS_H
#define STATS_H
#include <map>
#include <string>
#include <cstdio>
class Statistics
{
	std::map<std::string, double> cnt;
public:
	Statistics();
	void summary(std::string key, double val = 1);
	void print(FILE* = stdout);
	double get(std::string key);
	void propagate();
};
#endif
