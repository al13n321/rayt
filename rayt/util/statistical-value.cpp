#include "statistical-value.h"
#include <limits>
#include <sstream>
#include <iomanip>
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

	const int kColumnWidth = 15;
	const int kPrecision = 6;

	string StatisticalValue::GetTableReportHeader() {
		stringstream ss;

		ss << left << setw(kColumnWidth) << "avg" << left << setw(kColumnWidth) << "max";

		return ss.str();
	}

	string StatisticalValue::GetTableReportRow() const {
		stringstream ss;
		if (count_)
			ss << left << setprecision(kPrecision) << setw(kColumnWidth) << (sum_ / count_) << left << setprecision(kPrecision) << setw(kColumnWidth) << maximum_;
		else
			ss << left << setw(kColumnWidth) << '-' << left << setw(kColumnWidth) << '-';

		return ss.str();
	}

}
