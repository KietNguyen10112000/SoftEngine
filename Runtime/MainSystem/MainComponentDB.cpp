#include "MainComponentDB.h"

NAMESPACE_BEGIN

void MainComponentDB::AddRecord(const ComponentRecord& record)
{
	assert(m_compRecords.find(record) == m_compRecords.end());

	m_compRecords.insert(record);
}

void MainComponentDB::RemoveRecord(const char* name)
{
	m_compRecords.erase({ name, nullptr });
}

NAMESPACE_END