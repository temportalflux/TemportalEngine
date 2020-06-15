#pragma once

#include "TemportalEnginePCH.hpp"

NS_UTILITY

class Guid
{

public:
	static Guid create();
	static Guid fromString(std::string const str);
	static i32 compare(Guid const &a, Guid const &b);

	Guid();
	Guid(Guid const &other);

	bool isValid() const;

	/**
	 * If the Guid is valid, then this will return a string with 40 characters matching the format:
	 * `########-####-####-####-############` (8-4-4-4-12)
	 */
	std::string toString() const;

	bool operator<(Guid const &other) const;
	bool operator>(Guid const &other) const;
	bool operator==(Guid const &other) const;
	bool operator!=(Guid const &other) const;
	void operator=(Guid const &other);

	ui32 hash() const;

private:
	struct Data
	{
		ui32 data1;
		ui16 data2;
		ui16 data3;
		ui8 data4[8];
	};

	Guid(Data const &data);

	Data mData;

};

NS_END
