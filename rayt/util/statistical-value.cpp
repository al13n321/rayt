#include "statistical-value.h"
#include <limits>
#include <sstream>
using namespace std;

namespace rayt {

StatisticalValue::StatisticalValue() {
	Reset();
}

StatisticalValue::~StatisticalValue() {}

void StatisticalValue::AddValue(double value) {
	sum_ += value;
	minimum_ = min(minimum_, value);
	maximum_ = max(maximum_, value);
	++count_;
}

void StatisticalValue::Reset() {
	count_ = 0;
	sum_ = 0;
	maximum_ = -numeric_limits<double>::infinity();
	minimum_ =  numeric_limits<double>::infinity();
}

std::string StatisticalValue::FormatAvgMax() const {
	stringstream ss;
	if (count_)
		ss << "avg: " << (sum_ / count_) << " max: " << maximum_;
	else
		ss << "(no values)";
	return ss.str();
}

}
