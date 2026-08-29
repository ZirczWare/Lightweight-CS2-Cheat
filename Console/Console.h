#pragma once

#include <string_view>
#include <format>

namespace Console 
{
	void Show();

	namespace Detail 
	{
		// internal print implementation - don't use directly
		void PrintInternal(std::string_view fmt);
	}

	template <typename... Args>
	void Print(std::string_view fmt, Args&&... args) 
	{
		// pass formatted string to internal function so binary size stays small
		Detail::PrintInternal(std::vformat(fmt, std::make_format_args(args...)));
	}
}