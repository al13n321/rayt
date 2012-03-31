#pragma once

#include "common.h"
#include <string>

namespace rayt {

	class StatisticalValue {
	public:
		StatisticalValue();
		~StatisticalValue();

		void AddValue(double value);
		void Reset();

		static std::string GetTableReportHeader();
		std::string GetTableReportRow() const;
	private:
		double sum_;
		double maximum_;
		double minimum_;
		int count_;
	};

}
