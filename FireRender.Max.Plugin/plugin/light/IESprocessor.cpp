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
	: m_countLamps(0)
	, m_lumens(0)
	, m_multiplier(0)
	, m_countVerticalAngles(0)
	, m_countHorizontalAngles(0)
	, m_photometricType(0)
	, m_unit(0)
	, m_width(0.0)
	, m_length(0.0)
	, m_height(0.0)
	, m_ballast(0)
	, m_version(0)
	, m_wattage(-1)
	, m_verticalAngles()
	, m_horizontalAngles()
	, m_candelaValues()
	, m_extraData()
{}

void IESProcessor::IESLightData::Clear()
{
	m_countLamps = 0;
	m_lumens = 0;
	m_multiplier = 0;
	m_countVerticalAngles = 0;
	m_countHorizontalAngles = 0;
	m_photometricType = 0;
	m_unit = 0;
	m_width = 0.0f;
	m_length = 0.0f;
	m_height = 0.0f;
	m_ballast = 0;
	m_version = 0;
	m_wattage = -1.0f;
	m_verticalAngles.clear();
	m_horizontalAngles.clear();
	m_candelaValues.clear();
	m_extraData.clear();
}

bool IESProcessor::IESLightData::IsValid() const
{
	// check values correctness (some params could have only one or two values, according to specification)
	bool areValuesCorrect =
		(m_countLamps >= 1) &&  // while Autodesk specification says it always should be zero, this is not the case with real files
		((m_lumens == -1) || (m_lumens > 0)) &&
		(m_photometricType == 1) &&
		((m_unit == 1) || (m_unit == 2)) &&
		(m_ballast == 1) &&
		(m_version == 1) &&
		(m_wattage >= 0.0f); // while Autodesk specification says it always should be zero, this is not the case with real files

						   // check table correctness
	bool isSizeCorrect = m_horizontalAngles.size() * m_verticalAngles.size() == m_candelaValues.size();
	bool isArrDataConsistent = (m_horizontalAngles.size() == m_countHorizontalAngles) && (m_verticalAngles.size() == m_countVerticalAngles);

	// check data correctness (both angles arrays should be in ascending order)
	bool areArrsSorted = std::is_sorted(m_horizontalAngles.begin(), m_horizontalAngles.end()) &&
		std::is_sorted(m_verticalAngles.begin(), m_verticalAngles.end());

	// ensure correct value for angles
	const float tolerance = 0.0001f;
	bool correctAngles = 
		(abs(m_horizontalAngles.back()) <= tolerance) || 
		(abs(m_horizontalAngles.back() - 90.0f)  <= tolerance) ||
		(abs(m_horizontalAngles.back() - 180.0f) <= tolerance) ||
		(abs(m_horizontalAngles.back() - 360.0f) <= tolerance);

	return areValuesCorrect && isSizeCorrect && isArrDataConsistent && areArrsSorted && correctAngles;
}

// the distribution is axially symmetric.
bool IESProcessor::IESLightData::IsAxiallySymmetric(void) const
{
	const float tolerance = 0.0001f;
	return (abs(m_horizontalAngles.back()) <= tolerance);
}

// the distribution is symmetric in each quadrant.
bool IESProcessor::IESLightData::IsQuadrantSymmetric(void) const
{
	const float tolerance = 0.0001f;
	return (abs(m_horizontalAngles.back() - 90.0f) <= tolerance);
}

// the distribution is symmetric about a vertical plane.
bool IESProcessor::IESLightData::IsPlaneSymmetric(void) const
{
	const float tolerance = 0.0001f;
	return (abs(m_horizontalAngles.back() - 180.0f) <= tolerance);
}

std::string IESProcessor::ToString(const IESLightData& lightData) const
{
	FASSERT(lightData.IsValid());

	std::string firstLine = string_format("%d %d %d %d %d %d %d %f %f %f"
		, lightData.m_countLamps
		, lightData.m_lumens
		, lightData.m_multiplier
		, lightData.m_countVerticalAngles
		, lightData.m_countHorizontalAngles
		, lightData.m_photometricType
		, lightData.m_unit
		, lightData.m_width
		, lightData.m_length
		, lightData.m_height
	);

	std::string secondLine = string_format("%d %d %f"
		, lightData.m_ballast
		, lightData.m_version
		, lightData.m_wattage
	);

	std::string thirdLine = "";
	for (double angle : lightData.m_verticalAngles)
	{
		thirdLine.append(std::to_string(angle));
		thirdLine.append(" ");
	}

	std::string forthLine = "";
	for (double angle : lightData.m_horizontalAngles)
	{
		forthLine.append(std::to_string(angle));
		forthLine.append(" ");
	}

	std::string outString;
	outString = lightData.m_extraData + firstLine + "\n" + secondLine + "\n" + thirdLine + "\n" + forthLine + "\n";

	size_t valuesPerLine = lightData.m_verticalAngles.size(); // verticle angles count is number of columns in candela values table
	auto it = lightData.m_candelaValues.begin();
	while (it != lightData.m_candelaValues.end())
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

void IESProcessor::GetTokensFromFile(std::vector<std::string>& tokens, std::string& text, std::ifstream& inputFile) const
{
	text.clear();

	std::string lineToParse;

	// parse file line after line
	while (std::getline(inputFile, lineToParse))
	{
		// skip all data irrelevant for render
		if (!LineHaveNumbers(lineToParse))
		{
			text += lineToParse += "\n";
			continue;
		}

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
		lightData.m_countLamps = ReadInt(value);
		break;
	}
	case ParseOrder::READ_LUMENS: {
		lightData.m_lumens = ReadInt(value);
		break;
	}
	case ParseOrder::READ_MULTIPLIER: {
		lightData.m_multiplier = ReadInt(value);
		break;
	}
	case ParseOrder::READ_COUNT_VANGLES: {
		lightData.m_countVerticalAngles = ReadInt(value);
		break;
	}
	case ParseOrder::READ_COUNT_HANGLES: {
		lightData.m_countHorizontalAngles = ReadInt(value);
		break;
	}
	case ParseOrder::READ_TYPE: {
		lightData.m_photometricType = ReadInt(value);
		break;
	}
	case ParseOrder::READ_UNIT: {
		lightData.m_unit = ReadInt(value);
		break;
	}
	case ParseOrder::READ_WIDTH: {
		lightData.m_width = ReadDouble(value);
		break;
	}
	case ParseOrder::READ_LENGTH: {
		lightData.m_length = ReadDouble(value);
		break;
	}
	case ParseOrder::READ_HEIGHT: {
		lightData.m_height = ReadDouble(value);
		break;
	}
	case ParseOrder::READ_BALLAST: {
		lightData.m_ballast = ReadInt(value);
		break;
	}
	case ParseOrder::READ_VERSION: {
		lightData.m_version = ReadInt(value);
		break;
	}
	case ParseOrder::READ_WATTAGE: {
		lightData.m_wattage = ReadDouble(value);
		break;
	}
	case ParseOrder::READ_VERTICAL_ANGLES: {
		lightData.m_verticalAngles.push_back(ReadDouble(value));
		if (lightData.m_verticalAngles.size() != lightData.m_countVerticalAngles)
			return true; // exit function without switching state because we haven't read all angle values yet
		break;
	}
	case ParseOrder::READ_HORIZONTAL_ANGLES: {
		lightData.m_horizontalAngles.push_back(ReadDouble(value));
		if (lightData.m_horizontalAngles.size() != lightData.m_countHorizontalAngles)
			return true; // exit function without switching state because we haven't read all angle values yet
		break;
	}
	case ParseOrder::READ_CANDELA_VALUES: {
		lightData.m_candelaValues.push_back(ReadDouble(value));
		if (lightData.m_candelaValues.size() != lightData.m_countVerticalAngles*lightData.m_countHorizontalAngles)
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

bool IESProcessor::ParseTokens(IESLightData& lightData, std::vector<std::string>& tokens, TString& errorMessage) const
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
			errorMessage = L"parse failed";
			return false;
		}
	}

	// parse is not complete => failure
	if (parseState != ParseOrder::END_OF_PARSE)
	{
		errorMessage = L"not enough data in .ies file";
		return false;
	}

	// everything seems good
	return true;
}

bool IESProcessor::Parse(IESLightData& lightData, const char* filename, TString& errorMessage) const
{
	// back-off
	if (filename == nullptr)
	{
		errorMessage = L"empty file";
		return false;
	}

	// file is not IES file => return
	if (!IsIESFile(filename))
	{
		errorMessage = L"not .ies file";
		return false;
	}

	// try open file
	std::ifstream inputFile(filename);
	if (!inputFile)
	{
		errorMessage = L"invalid file";
		return false;
	}

	// flush data that might exist in container
	lightData.Clear();

	// read data from file in a way convinient for further parsing
	std::vector<std::string> tokens;
	GetTokensFromFile(tokens, lightData.m_extraData, inputFile);

	// read tokens to lightData
	bool isParseOk = ParseTokens(lightData, tokens, errorMessage);

	// ensure correct parse results
	if (!isParseOk || !lightData.IsValid())
		errorMessage = L"invalid data read from .ies file";
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
	// scale photometric web
	if (std::fabs(req.m_scale - 1.0f) > 0.01f)
	{
		lightData.m_width *= req.m_scale;
		lightData.m_length *= req.m_scale;
		lightData.m_height *= req.m_scale;
	}

	return true;
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

IESProcessor::IESUpdateRequest::IESUpdateRequest()
	: m_scale (1.0f)
{}