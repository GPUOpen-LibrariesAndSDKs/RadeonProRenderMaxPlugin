/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* IES file parser
*********************************************************************************************************************************/

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>
#include <memory>
#include <iomanip>
#include "IESprocessor.h"

IESProcessor::IESLightData::IESLightData()
	: countLamps(0)
	, lumens(0)
	, multiplier(0)
	, countVerticalAngles(0)
	, countHorizontalAngles(0)
	, photometricType(0)
	, unit(0)
	, width(0.0)
	, length(0.0)
	, height(0.0)
	, ballast(0)
	, version(0)
	, wattage(-1)
	, verticalAngles()
	, horizontalAngles()
	, candelaValues()
{}

void IESProcessor::IESLightData::Clear()
{
	countLamps = 0;
	lumens = 0;
	multiplier = 0;
	countVerticalAngles = 0;
	countHorizontalAngles = 0;
	photometricType = 0;
	unit = 0;
	width = 0.0f;
	length = 0.0f;
	height = 0.0f;
	ballast = 0;
	version = 0;
	wattage = -1.0f;
	verticalAngles.clear();
	horizontalAngles.clear();
	candelaValues.clear();
}

DEFINE_GETTERS(int&, countLamps, CountLamps, IESProcessor::IESLightData)
DEFINE_GETTERS(int&, lumens, Lumens, IESProcessor::IESLightData)
DEFINE_GETTERS(int&, multiplier, Multiplier, IESProcessor::IESLightData)
DEFINE_GETTERS(int&, countVerticalAngles, CountVerticalAngles, IESProcessor::IESLightData)
DEFINE_GETTERS(int&, countHorizontalAngles, CountHorizontalAngles, IESProcessor::IESLightData)
DEFINE_GETTERS(int&, photometricType, PhotometricType, IESProcessor::IESLightData)
DEFINE_GETTERS(int&, unit, Unit, IESProcessor::IESLightData)
DEFINE_GETTERS(double&, width, Width, IESProcessor::IESLightData)
DEFINE_GETTERS(double&, length, Length, IESProcessor::IESLightData)
DEFINE_GETTERS(double&, height, Height, IESProcessor::IESLightData)
DEFINE_GETTERS(int&, ballast, Ballast, IESProcessor::IESLightData)
DEFINE_GETTERS(int&, version, Version, IESProcessor::IESLightData)
DEFINE_GETTERS(double&, wattage, Wattage, IESProcessor::IESLightData)
DEFINE_GETTERS(std::vector<double>&, horizontalAngles, HorizontalAngles, IESProcessor::IESLightData)
DEFINE_GETTERS(std::vector<double>&, verticalAngles, VerticalAngles, IESProcessor::IESLightData)
DEFINE_GETTERS(std::vector<double>&, candelaValues, CandelaValues, IESProcessor::IESLightData)

bool IESProcessor::IESLightData::IsValid() const
{
	// check values correctness (some params could have only one or two values, according to specification)
	bool areValuesCorrect =
		(countLamps >= 1) &&  // while Autodesk specification says it always should be zero, this is not the case with real files
		((lumens == -1) || (lumens > 0)) &&
		(photometricType == 1) &&
		((unit == 1) || (unit == 2)) &&
		(ballast == 1) &&
		(version == 1) &&
		(wattage >= 0.0f); // while Autodesk specification says it always should be zero, this is not the case with real files

						   // check table correctness
	bool isSizeCorrect = horizontalAngles.size() * verticalAngles.size() == candelaValues.size();
	bool isArrDataConsistent = (horizontalAngles.size() == countHorizontalAngles) && (verticalAngles.size() == countVerticalAngles);

	// check data correctness (both angles arrays should be in ascending order)
	bool areArrsSorted = std::is_sorted(horizontalAngles.begin(), horizontalAngles.end()) &&
		std::is_sorted(verticalAngles.begin(), verticalAngles.end());

	// ensure correct value for angles
	const float tolerance = 0.0001f;
	bool correctAngles = 
		(abs(horizontalAngles.back()) <= tolerance) || 
		(abs(horizontalAngles.back() - 90.0f)  <= tolerance) ||
		(abs(horizontalAngles.back() - 180.0f) <= tolerance) ||
		(abs(horizontalAngles.back() - 360.0f) <= tolerance);

	return areValuesCorrect && isSizeCorrect && isArrDataConsistent && areArrsSorted && correctAngles;
}

// the distribution is axially symmetric.
bool IESProcessor::IESLightData::IsAxiallySymmetric(void) const
{
	const float tolerance = 0.0001f;
	return (abs(horizontalAngles.back()) <= tolerance);
}

// the distribution is symmetric in each quadrant.
bool IESProcessor::IESLightData::IsQuadrantSymmetric(void) const
{
	const float tolerance = 0.0001f;
	return (abs(horizontalAngles.back() - 90.0f) <= tolerance);
}

// the distribution is symmetric about a vertical plane.
bool IESProcessor::IESLightData::IsPlaneSymmetric(void) const
{
	const float tolerance = 0.0001f;
	return (abs(horizontalAngles.back() - 180.0f) <= tolerance);
}

std::string IESProcessor::ToString(const IESLightData& lightData) const
{
	FASSERT(lightData.IsValid());

	std::string firstLine = string_format("%d %d %d %d %d %d %d %f %f %f"
		, lightData.CountLamps()
		, lightData.Lumens()
		, lightData.Multiplier()
		, lightData.CountVerticalAngles()
		, lightData.CountHorizontalAngles()
		, lightData.PhotometricType()
		, lightData.Unit()
		, lightData.Width()
		, lightData.Length()
		, lightData.Height()
	);

	std::string secondLine = string_format("%d %d %f"
		, lightData.Ballast()
		, lightData.Version()
		, lightData.Wattage()
	);

	std::string thirdLine = "";
	for (double angle : lightData.VerticalAngles())
	{
		thirdLine.append(std::to_string(angle));
		thirdLine.append(" ");
	}

	std::string forthLine = "";
	for (double angle : lightData.HorizontalAngles())
	{
		forthLine.append(std::to_string(angle));
		forthLine.append(" ");
	}

	std::string outString;
	outString = firstLine + "\n" + secondLine + "\n" + thirdLine + "\n" + forthLine + "\n";

	size_t valuesPerLine = lightData.VerticalAngles().size(); // verticle angles count is number of columns in candela values table
	auto it = lightData.CandelaValues().begin();
	while (it != lightData.CandelaValues().end())
	{
		auto endl = it + valuesPerLine;
		std::string candelaValuesLine = "";
		for (; it != endl; ++it)
		{
			candelaValuesLine.append(std::to_string(*it));
			candelaValuesLine.append(" ");
		}
		candelaValuesLine.append("\n");
		outString.append(candelaValuesLine);
	}

	return outString;
}

bool IESProcessor::IsIESFile(const char* filename) const
{
	int length = strlen(filename);
	if (length == 0)
		return false;

	if ((tolower(filename[length - 1]) != 's') ||
		(tolower(filename[length - 2]) != 'e') ||
		(tolower(filename[length - 3]) != 'i'))
		return false;

	return true;
}

void IESProcessor::SplitLine(std::vector<std::string>& tokens, const std::string& lineToParse) const
{
	// split string
	std::istringstream iss(lineToParse);
	std::vector<std::string> line_tokens
	(
		std::istream_iterator<std::string>(iss), {}
	);

	// add splited strings 
	tokens.insert(tokens.end(), line_tokens.begin(), line_tokens.end());
}

bool LineHaveNumbers(const std::string& lineToParse)
{
	const char* c = lineToParse.c_str();
	while (c && isspace((unsigned char)*c))
	{
		++c;
	}

	return (isdigit(*c) || (*c == '-'));
}

void IESProcessor::GetTokensFromFile(std::vector<std::string>& tokens, std::ifstream& inputFile) const
{
	std::string lineToParse;

	// parse file line after line
	while (std::getline(inputFile, lineToParse))
	{
		// skip all data irrelevant for render
		if (!LineHaveNumbers(lineToParse))
			continue;

		// split line
		SplitLine(tokens, lineToParse);
	}
}

double ReadDouble(const std::string& input)
{
	return atof(input.c_str());
}

int ReadInt(const std::string& input)
{
	return std::stoi(input.c_str());
}

enum class IESProcessor::ParseOrder
{
	READ_COUNT_LAMPS = 0,
	READ_LUMENS,
	READ_MULTIPLIER,
	READ_COUNT_VANGLES,
	READ_COUNT_HANGLES,
	READ_TYPE,
	READ_UNIT,
	READ_WIDTH,
	READ_LENGTH,
	READ_HEIGHT,
	READ_BALLAST,
	READ_VERSION,
	READ_WATTAGE,
	READ_VERTICAL_ANGLES,
	READ_HORIZONTAL_ANGLES,
	READ_CANDELA_VALUES,
	END_OF_PARSE
};

IESProcessor::ParseOrder& operator++(IESProcessor::ParseOrder &value)
{
	if (value == IESProcessor::ParseOrder::END_OF_PARSE)
	{
		value = static_cast<IESProcessor::ParseOrder>(0);

		return value;
	}

	value = static_cast<IESProcessor::ParseOrder>(static_cast<std::underlying_type<IESProcessor::ParseOrder>::type>(value) + 1);

	return value;
}

IESProcessor::ParseOrder IESProcessor::FirstParseState(void) const
{
	return ParseOrder::READ_COUNT_LAMPS;
}

bool IESProcessor::ReadValue(IESLightData& lightData, IESProcessor::ParseOrder& state, const std::string& value) const
{
	// back-off
	if (state == ParseOrder::END_OF_PARSE)
		return false;

	// read values from input
	switch (state) {
	case ParseOrder::READ_COUNT_LAMPS: {
		lightData.CountLamps() = ReadInt(value);
		break;
	}
	case ParseOrder::READ_LUMENS: {
		lightData.Lumens() = ReadInt(value);
		break;
	}
	case ParseOrder::READ_MULTIPLIER: {
		lightData.Multiplier() = ReadInt(value);
		break;
	}
	case ParseOrder::READ_COUNT_VANGLES: {
		lightData.CountVerticalAngles() = ReadInt(value);
		break;
	}
	case ParseOrder::READ_COUNT_HANGLES: {
		lightData.CountHorizontalAngles() = ReadInt(value);
		break;
	}
	case ParseOrder::READ_TYPE: {
		lightData.PhotometricType() = ReadInt(value);
		break;
	}
	case ParseOrder::READ_UNIT: {
		lightData.Unit() = ReadInt(value);
		break;
	}
	case ParseOrder::READ_WIDTH: {
		lightData.Width() = ReadDouble(value);
		break;
	}
	case ParseOrder::READ_LENGTH: {
		lightData.Length() = ReadDouble(value);
		break;
	}
	case ParseOrder::READ_HEIGHT: {
		lightData.Height() = ReadDouble(value);
		break;
	}
	case ParseOrder::READ_BALLAST: {
		lightData.Ballast() = ReadInt(value);
		break;
	}
	case ParseOrder::READ_VERSION: {
		lightData.Version() = ReadInt(value);
		break;
	}
	case ParseOrder::READ_WATTAGE: {
		lightData.Wattage() = ReadDouble(value);
		break;
	}
	case ParseOrder::READ_VERTICAL_ANGLES: {
		lightData.VerticalAngles().push_back(ReadDouble(value));
		if (lightData.VerticalAngles().size() != lightData.CountVerticalAngles())
			return true; // exit function without switching state because we haven't read all angle values yet
		break;
	}
	case ParseOrder::READ_HORIZONTAL_ANGLES: {
		lightData.HorizontalAngles().push_back(ReadDouble(value));
		if (lightData.HorizontalAngles().size() != lightData.CountHorizontalAngles())
			return true; // exit function without switching state because we haven't read all angle values yet
		break;
	}
	case ParseOrder::READ_CANDELA_VALUES: {
		lightData.CandelaValues().push_back(ReadDouble(value));
		if (lightData.CandelaValues().size() != lightData.CountVerticalAngles()*lightData.CountHorizontalAngles())
			return true; // exit function without switching state because we haven't read all angle values yet
		break;
	}
	default:
		return false;
	}

	// update state (to read next value)
	++state;

	return true;
}

bool IESProcessor::ParseTokens(IESLightData& lightData, std::vector<std::string>& tokens, std::string& errorMessage) const
{
	// initial state to read data
	IESProcessor::ParseOrder parseState = FirstParseState();

	// iterate over tokens
	for (const std::string& value : tokens)
	{
		// try parse token
		bool parseOk = ReadValue(lightData, parseState, value);

		if (!parseOk)
		{
			// parse failed
			errorMessage = "parse failed";
			return false;
		}
	}

	// parse is not complete => failure
	if (parseState != ParseOrder::END_OF_PARSE)
	{
		errorMessage = "not enough data in .ies file";
		return false;
	}

	// everything seems good
	return true;
}

bool IESProcessor::Parse(IESLightData& lightData, const char* filename, std::string& errorMessage) const
{
	// back-off
	if (filename == nullptr)
	{
		errorMessage = "empty file";
		return false;
	}

	// file is not IES file => return
	if (!IsIESFile(filename))
	{
		errorMessage = "not .ies file";
		return false;
	}

	// try open file
	std::ifstream inputFile(filename);
	if (!inputFile)
	{
		errorMessage = "invalid file";
		return false;
	}

	// flush data that might exist in container
	lightData.Clear();

	// read data from file in a way convinient for further parsing
	std::vector<std::string> tokens;
	GetTokensFromFile(tokens, inputFile);

	// read tokens to lightData
	bool isParseOk = ParseTokens(lightData, tokens, errorMessage);

	// ensure correct parse results
	if (isParseOk || !lightData.IsValid())
		errorMessage = "invalid data read from .ies file";
	if (!isParseOk || !lightData.IsValid())
	{
		// report failure
		lightData.Clear(); // function shouldn't return garbage
		return false;
	}

	// parse successfull!
	return true;
}

bool IESProcessor::Update(IESLightData& lightData, const IESUpdateRequest& req) const
{
	// NIY
	return false;
}

bool IESProcessor::Save(const IESLightData& lightData, const char* outfilename) const
{
	// NIY
	return false;
}

bool IESProcessor::Load(IESLightData& lightData, const char* filename) const
{
	// NIY
	return false;
}
