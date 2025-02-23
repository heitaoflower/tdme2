#pragma once

#include <string>
#include <vector>

#include <tdme/tdme.h>
#include <tdme/utilities/fwd-tdme.h>

using std::string;
using std::vector;

/**
 * SHA256 hash class
 * @author Andreas Drewke
 */
class tdme::utilities::SHA256 {
public:
	/**
	 * @brief Encodes an string to SHA256 string
	 * @param decodedString string to encode
	 * @returns encodedString
	 */
	inline static const string encode(const string& decodedString) {
		string encodedString;
		encode(decodedString, encodedString);
		return encodedString;
	}

	/**
	 * Encodes an string to SHA256 string
	 * @param decodedString string to encode
	 * @param encodedString encoded string
	 */
	static void encode(const string& decodedString, string& encodedString);

	/**
	 * Encodes uint8_t vector to SHA256 string
	 * @param decodedData data to encode
	 * @param encodedString encoded string
	 */
	static void encode(const vector<uint8_t>& decodedData, string& encodedString);

};
