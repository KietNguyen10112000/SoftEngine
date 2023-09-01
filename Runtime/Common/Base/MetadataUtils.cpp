#include "MetadataUtils.h"

NAMESPACE_BEGIN

void MetadataParser::Initialize()
{
	s_instance.reset(new MetadataParser());
}

void MetadataParser::Finalize()
{
	s_instance.reset();
}

std::vector<std::string_view>& MetadataParser::GetValueChain(const std::string_view& editFormat, const String& _input)
{
	m_values.clear();

	auto len = editFormat.length();
	auto inputLen = _input.length();
	bool isInString = false;

	size_t editFormatIndex = 0;
	size_t inputIndex = 0;

	std::string_view input = _input.c_str();

	for (editFormatIndex = 0; editFormatIndex < len; editFormatIndex++)
	{
		auto c = editFormat[editFormatIndex];

		if (c == '%')
		{
			auto index = editFormat.find_first_of(' ', ++editFormatIndex);
			/*if (index == std::string_view::npos)
			{
				index = editFormat.find_first_of(']', ++editFormatIndex);
				goto Return;
			}*/

			std::string_view valueFormat = editFormat.substr(editFormatIndex, index - editFormatIndex);
			
			if (valueFormat.find("number") != std::string_view::npos)
			{
				editFormatIndex = index;

				auto iChar = input[inputIndex];
				auto count = inputIndex;
				while (std::isdigit(iChar) || iChar == '.')
				{
					iChar = input[++count];
				}

				m_values.push_back(input.substr(inputIndex, count - inputIndex));

				inputIndex = count + 1;

				if (inputIndex >= inputLen)
				{
					goto Return;
				}
			}
			else if (valueFormat.find("string") != std::string_view::npos)
			{
				assert(0 && "It's 2 A.M, just go to sleep!!!");
			}
			else 
			{
				goto Return;
			}
		}

		if (input[inputIndex] == c)
		{
			inputIndex++;
		}
	}

Return:
	return m_values;
}

NAMESPACE_END