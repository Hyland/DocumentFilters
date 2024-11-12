#pragma once

#include <DocumentFiltersObjects.h>
#include <string>

namespace DocumentFiltersSamples
{
	/**
	 * @brief Retrieves the license key.
	 *
	 * @param user_provided_key The user-provided license key.
	 * @return The license key.
	 */
	std::string get_license_key(const std::string& user_provided_key = nullptr);

	/**
	 * @brief Handles the password prompt for the document extractor.
	 *
	 * @param doc The document extractor.
	 * @return The modified document extractor.
	 */
	Hyland::DocFilters::Extractor& handle_password_prompt(Hyland::DocFilters::Extractor& doc);

	/**
	 * @brief Converts a file extension to a corresponding CanvasType.
	 *
	 * @param ext The file extension (e.g., ".png", ".pdf").
	 * @return The corresponding Hyland::DocFilters::CanvasType.
	 * @throws std::invalid_argument if the extension is unknown.
	 */
	Hyland::DocFilters::CanvasType extension_to_canvas(std::string ext);

}